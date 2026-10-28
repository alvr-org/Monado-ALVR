// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Generic math helpers for constellation optimizer code.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup tracking
 */

#pragma once

#include "tracking/t_camera_models.hpp"

#include "math/m_eigen_interop.hpp"
#include "math/m_quatexpmap.hpp"
#include "math/m_quatexpmap_bigceres.hpp"

#include <Eigen/Core>
#include <Eigen/Geometry>

#include "ceres/jet.h"
#include "ceres/product_manifold.h"
#include "ceres/autodiff_manifold.h"
#include "ceres/ceres.h"

#include "pose_optimize.hpp"
#include "math.hpp"

#include <ceres/crs_matrix.h>
#include <cstring>
#include <cmath>


namespace xrt::tracking::constellation::optimizer {

/*!
 * Optimize directly on the pre-undistorted points, don't compute residuals in distorted pixel space.
 *
 * We don't have a "project" routine for the CV1 distortion algorithm, so we work in undistorted space, instead.
 */
constexpr bool kOptimizeUndistortedPoints = true;
//! Print debug info about the residuals as we're computing them.
constexpr bool kResidualDebugPrint = false;

constexpr double kMaxGravity = MATH_GRAVITY_M_S2 * 1.01;
constexpr double kMinGravity = MATH_GRAVITY_M_S2 * 0.99;

/*!
 * Fewer correspondences than this and the observation is dropped. Any less and this is likely a degenerate solve.
 */
constexpr size_t kMinCorrespondencesPerObservation = 4;

/*!
 * Where a camera observation stops being treated as data and starts being treated as a mistake, in standard
 * deviations. Used to size the Huber loss function.
 *
 * The loss is applied to a whole observation rather than to each LED of it, so this is measured against the spread of
 * the observation's chi-square rather than against a single reprojection error, and the threshold it produces grows
 * with the number of correspondences.
 */
constexpr double kCameraHuberDeltaSigmas = 3.0;

using namespace xrt::auxiliary::math;
using namespace xrt::auxiliary::tracking;

using Eigen::Map;
using Eigen::Matrix;
using Eigen::Matrix3d;
using Eigen::MatrixBase;
using Eigen::MatrixXd;
using Eigen::Quaternion;
using Eigen::Quaterniond;
using Eigen::Vector;
using Eigen::Vector2;
using Eigen::Vector3;
using Eigen::Vector3d;
using Eigen::Vector4;
using Eigen::VectorXd;

using ceres::Jet;


/*
 *
 * Types
 *
 */

template <typename T, int N> struct SeedableVector
{
	using VectorType = Vector<T, N>;

public: // Fields
	VectorType vec;
	bool seeded{false};

	/*!
	 * The value this state is held linearized around, once it has been anchored.
	 *
	 * A marginalization prior is a statement about one particular linearization of the states it covers, and it
	 * goes on making that statement long after the solve has moved them. Everything that touches an anchored
	 * state has to keep linearizing it here, or the frozen factor and the live residuals stop describing the
	 * same point.
	 */
	VectorType first_estimate;

	//! Whether @ref first_estimate has been taken.
	bool anchored{false};

public: // Methods
	SeedableVector() = default;

	/*!
	 * Returns the data to use for a parameter, asserts you have seeded the data before this.
	 */
	T *
	data()
	{
		assert(this->seeded);
		return this->vec.data();
	}

	/*!
	 * Returns the vector for you to write into and seed. Assumes you have seeded it after this function is called.
	 */
	VectorType &
	seedVec()
	{
		this->seeded = true;
		return this->vec;
	}

	//! Freezes the current value as the point this state is linearized at, unless it already has one.
	void
	anchor()
	{
		assert(this->seeded);

		if (this->anchored) {
			return;
		}

		this->first_estimate = this->vec;
		this->anchored = true;
	}

	//! The frozen linearization point, or null for a state that has never been anchored.
	const T *
	anchorData() const
	{
		return this->anchored ? this->first_estimate.data() : nullptr;
	}

	//! Forgets the frozen linearization point, so the state linearizes wherever it currently is again.
	void
	releaseAnchor()
	{
		this->anchored = false;
	}

	//! Drops everything held here, for a slot about to be handed to a state of its own.
	void
	unseed()
	{
		this->seeded = false;
		this->anchored = false;
	}
};

// Covariance lives in the manifold's tangent space, which is one dimension smaller than the ambient
// parameter block: the quaternion contributes 3 tangent dimensions, not 4.
typedef Eigen::Matrix<double, kPoseCovarianceSize, kPoseCovarianceSize> PoseStateCovarianceMatrix;

/*
 *
 * Parameter block types
 *
 */

/*!
 * The strength of gravity in the world, as a parameter block of its own.
 *
 * This is a property of where the capture was taken rather than of any one sensor, so every IMU in a solve shares this
 * single block while each keeps its own @ref ImuBias.
 */
template <typename T> struct WorldGravity
{
	static_assert(!std::is_const_v<T>,
	              "block scalar must not be const — did a `T*` overload deduce T = const double?");

	static constexpr int kNumParameters = 1;

	static constexpr int kGravityMagIndex = 0;

	//! Gravity in meters per second squared.
	T gravity_mag;

	WorldGravity() : gravity_mag(T(MATH_GRAVITY_M_S2)) {}

	explicit WorldGravity(T gravity_mag) : gravity_mag(gravity_mag) {}

	WorldGravity(Eigen::Ref<const Vector<T, kNumParameters>> parameters) : gravity_mag(parameters[kGravityMagIndex])
	{}

	void
	pack(Eigen::Ref<Vector<T, kNumParameters>> parameters) const
	{
		parameters[kGravityMagIndex] = this->gravity_mag;
	}

	//! Gravity as a vector, which points along +Y since world up is -Y in the OpenCV space the optimizer works in.
	Vector3<T>
	toVector() const
	{
		return Vector3<T>(T(0), this->gravity_mag, T(0));
	}
};

//! A parameter block for a camera with 2 degrees of freedom, pitch and roll. Yaw is fixed to 0, and it has no position.
template <typename T> struct PitchRoll
{
	static_assert(!std::is_const_v<T>,
	              "block scalar must not be const — did a `T*` overload deduce T = const double?");

	static constexpr int kNumParameters = 2;

	Vector2<T> pitch_roll;

	void
	toQuaternion(Quaternion<T> &q) const
	{
		// @todo don't optimize on euler angles, use something without singularities.
		//       what are the chances someone has a camera pointed 90 degrees down?

		q = Quaternion<T>(Eigen::AngleAxis<T>(pitch_roll.y(), Vector3<T>::UnitZ()));
		q *= Quaternion<T>(Eigen::AngleAxis<T>(pitch_roll.x(), Vector3<T>::UnitX()));
	}

	Eigen::Transform<T, 3, Eigen::Isometry>
	toTransform() const
	{
		Quaternion<T> q;
		this->toQuaternion(q);

		return Eigen::Transform<T, 3, Eigen::Isometry>(q);
	}

	PitchRoll(Eigen::Ref<const Vector<T, kNumParameters>> parameters) : pitch_roll(parameters) {}
};

/*!
 * Pose with 6 degrees of freedom, translation and rotation.
 *
 * Stored as a translation and a unit quaternion, so the ambient block is one larger than the 6 degrees of freedom
 * it stands for. @ref PoseManifold supplies the retraction that takes the tangent space back to 6.
 */
template <typename T> struct Pose
{
	static_assert(!std::is_const_v<T>,
	              "block scalar must not be const — did a `T*` overload deduce T = const double?");

	//! Size of the parameter block handed to Ceres: (tX, tY, tZ), (qX, qY, qZ, qW).
	static constexpr int kNumParameters = 7;
	//! Degrees of freedom the manifold exposes to the solver.
	static constexpr int kNumTangentParameters = 6;
	static constexpr int kNumResiduals = 6;

	Vector3<T> translation;
	Quaternion<T> rotation;

	Pose(Quaternion<T> quat) : translation(Eigen::Vector3<T>::Zero()), rotation(quat) {}

	Pose(xrt_pose pose) : Pose(map_quat(pose.orientation).cast<T>().normalized())
	{
		this->translation = Vector3<T>(pose.position.x, pose.position.y, pose.position.z);
	}

	Pose(Eigen::Ref<const Vector<T, kNumParameters>> parameters)
	    : translation(parameters.template segment<3>(kPoseStatePosStart)),
	      rotation(Quaternion<T>(parameters.template segment<4>(kPoseStateRotStart)))
	{}

	template <typename TransformType> Pose(const Eigen::Transform<TransformType, 3, Eigen::Isometry> &transform)
	{
		this->translation = transform.translation().template cast<T>();
		Quaternion<TransformType> q(transform.rotation());
		this->rotation = q.template cast<T>();
	}

	void
	pack(Eigen::Ref<Vector<T, kNumParameters>> parameters) const
	{
		parameters.template segment<3>(kPoseStatePosStart) = this->translation;
		parameters.template segment<4>(kPoseStateRotStart) = this->rotation.coeffs();
	}

	Eigen::Transform<T, 3, Eigen::Isometry>
	toTransform() const
	{
		return Eigen::Translation<T, 3>(translation.x(), translation.y(), translation.z()) *
		       Eigen::Transform<T, 3, Eigen::Isometry>(this->rotation);
	}

	void
	getQuaternion(Quaternion<T> &q) const
	{
		q = this->rotation;
	}

	xrt_pose
	toXrtPose() const
	{
		xrt_pose pose;

		map_vec3(pose.position) = this->translation.template cast<float>();
		map_quat(pose.orientation) = this->rotation.coeffs().template cast<float>();

		return pose;
	}

private:
	Pose() = default;
};

/*!
 * Pose with 9 degrees of freedom, translation, rotation, and linear velocity.
 *
 * Stored as a `Pose` with an extra 3-parameter vector for the velocity.
 *
 * See @ref PoseWithVelocityManifold for the correct way to optimize on this,
 * which takes tangent space from 10 -> 9 parameters (quat exponential map).
 */
template <typename T> struct PoseWithVelocity : Pose<T>
{
	static_assert(!std::is_const_v<T>,
	              "block scalar must not be const — did a `T*` overload deduce T = const double?");

	static constexpr int kNumParameters = Pose<T>::kNumParameters + 3;
	static constexpr int kNumTangentParameters = Pose<T>::kNumTangentParameters + 3;

	Vector3<T> velocity;

	PoseWithVelocity(xrt_pose p) : Pose<T>(p), velocity(Vector3<T>::Zero()) {}

	template <typename Derived>
	PoseWithVelocity(xrt_pose p, const Vector3<Derived> &velocity)
	    : Pose<T>(p), velocity(velocity.template cast<T>())
	{}

	PoseWithVelocity(Eigen::Ref<const Vector<T, kNumParameters>> parameters)
	    : Pose<T>(parameters.template segment<Pose<T>::kNumParameters>(0)),
	      velocity(Vector3<T>(parameters.template segment<3>(Pose<T>::kNumParameters)))
	{}

	void
	pack(Eigen::Ref<Vector<T, kNumParameters>> parameters) const
	{
		this->Pose<T>::pack(parameters.template segment<Pose<T>::kNumParameters>(0));
		parameters.template segment<3>(Pose<T>::kNumParameters) = this->velocity;
	}
};

template <typename T> struct ImuExtrinsics
{
	static_assert(!std::is_const_v<T>,
	              "block scalar must not be const — did a `T*` overload deduce T = const double?");

	//! (qX, qY, qZ, qW)
	static constexpr int kNumParameters = 4;
	//! Degrees of freedom
	static constexpr int kNumTangentParameters = 3;

	Quaternion<T> Q_imu_model;

	template <typename Derived> ImuExtrinsics(Quaternion<Derived> quat) : Q_imu_model(quat.template cast<T>()) {}

	ImuExtrinsics(Eigen::Ref<const Vector<T, kNumParameters>> parameters)
	    : Q_imu_model(Quaternion<T>(parameters.template segment<4>(0)))
	{}

	void
	pack(Eigen::Ref<Vector<T, kNumParameters>> parameters) const
	{
		parameters.template segment<4>(0) = this->Q_imu_model.coeffs();
	}
};

/*
 *
 * Manifolds
 *
 */

/*!
 * Right-multiplying quaternion manifold, matches our local convention.
 */
struct QuaternionManifoldFunctor
{
	template <typename T>
	bool
	Plus(const T *x, const T *delta, T *x_plus_delta) const
	{
		const Eigen::Map<const Eigen::Quaternion<T>> q(x);
		const Eigen::Map<const Eigen::Vector3<T>> w(delta);
		Eigen::Map<Eigen::Quaternion<T>> result(x_plus_delta);

		result = q * quat_exp_so3(w);
		return true;
	}

	template <typename T>
	bool
	Minus(const T *y, const T *x, T *y_minus_x) const
	{
		const Eigen::Map<const Eigen::Quaternion<T>> q_y(y);
		const Eigen::Map<const Eigen::Quaternion<T>> q_x(x);
		Eigen::Map<Eigen::Vector3<T>> result(y_minus_x);

		result = quat_ln_so3(Eigen::Quaternion<T>(q_x.conjugate() * q_y));
		return true;
	}
};

//! Our quaternion manifold functor
typedef ceres::AutoDiffManifold<QuaternionManifoldFunctor, 4, 3> QuaternionManifold;

//! Pose manifold, (tX, tY, tZ), (qX, qY, qZ, qW)
typedef ceres::ProductManifold<ceres::EuclideanManifold<3>, QuaternionManifold> PoseManifold;

//! Pose plus linear velocity, (tX, tY, tZ), (qX, qY, qZ, qW), (vX, vY, vZ). 10 ambient, 9 tangent.
typedef ceres::ProductManifold<ceres::EuclideanManifold<3>, //
                               QuaternionManifold,          //
                               ceres::EuclideanManifold<3>> //
    PoseWithVelocityManifold;


/*
 *
 * Helper functions
 *
 */

//! Converts an @ref xrt_pose into an Eigen Isometry3f
inline Eigen::Isometry3f
isometryFromPose(const xrt_pose &p)
{
	return {Eigen::Translation3f{map_vec3(p.position)} * map_quat(p.orientation)};
}

/*!
 * Finds a LED in the model by its id.
 *
 * A LED's id is an opaque identifier chosen by the driver, not its position in the model: drivers are free to
 * derive it from device-specific numbering (the Rift uses the headset's own LED indices, which include a slot for
 * the IMU). So a blob's matched_device_led_id has to be resolved through here rather than used as an index.
 *
 * @return The index of the LED in the model, or -1 when no LED carries that id.
 */
static inline int32_t
findLedIndexById(const t_constellation_tracker_led_model *leds_model, t_constellation_led_id_it led_id)
{
	for (size_t i = 0; i < leds_model->led_count; i++) {
		if (leds_model->leds[i].id == led_id) {
			return static_cast<int32_t>(i);
		}
	}

	return -1;
}

/*!
 * Measures how well one kind of factor was fit, at whatever the problem's parameters currently hold.
 *
 * Both factor types are whitened, so this is directly comparable between them and against 1.
 *
 * @param problem         The problem, evaluated at its current parameter values.
 * @param residual_blocks The blocks belonging to this kind of factor.
 */
FactorFitness
evaluateFactorFitness(ceres::Problem &problem, const std::vector<ceres::ResidualBlockId> &residual_blocks);

//! Converts a CRS matrix to a dense Eigen matrix
void
matrixCrsToEigen(const ceres::CRSMatrix &mat_crs, Eigen::MatrixXd &mat);

/*!
 * Marginalize the first @p num_base_states states out of the linearized system given by @p jacobian and
 * @p residual, handing back the linear prior that replaces them.
 *
 * Inspired/specialized from MarginalizationError::marginalizeOut() from OKVIS.
 */
void
marginalize(const VectorXd &residual,  //
            const MatrixXd &jacobian,  //
            const int num_base_states, //
            VectorXd &out_e0,          //
            MatrixXd &out_j);          //

/*
 *
 * LED residuals
 *
 */

//! X and Y in the image, for one LED.
constexpr int kNumLedResiduals = 2;

/*!
 * How well a blob centroid locates the LED that lit it, in pixels.
 *
 * The camera residuals are reprojection errors, so they are only comparable to the whitened IMU residuals once they
 * are divided by the noise that produced them. This is the same floor the per-frame PnP puts on its own residual
 * sigma, since a blob has about this much pixels of noise, at least on CV1.
 */
constexpr double kBlobPositionSigmaPixels = 0.16;

template <typename T>
static void
project_led(const camera_model &params,
            const Eigen::Quaternion<T> &Q_cam_model,
            const Eigen::Vector3<T> &T_cam_model,
            const Eigen::Vector3<T> &T_model_led,
            Eigen::Vector2<T> &out_projected_point,
            T *led_depth_m)
{
	// Rotate the point around the pose's local frame.
	Eigen::Vector3<T> T_cam_led = Q_cam_model * T_model_led;
	// Translate the point into the camera frame.
	T_cam_led += T_cam_model;

	if (led_depth_m != nullptr) {
		*led_depth_m = T_cam_led.z();
	}

	Eigen::Vector2<T> projected_point = {T(1e6), T(1e6)};

	if (T_cam_led.z() <= T(0)) {
		// The point is behind the camera, so we can't project it. Return a large value to indicate this.
		out_projected_point = projected_point;
		return;
	}

	const t_camera_model_params &dist = kOptimizeUndistortedPoints ? params.calib_pinhole : params.calib_true;

	(void)camera_models::project(dist,                 //
	                             T_cam_led.x(),        //
	                             T_cam_led.y(),        //
	                             T_cam_led.z(),        //
	                             projected_point.x(),  //
	                             projected_point.y()); //

	out_projected_point = projected_point;
}

template <typename T, typename Derived>
static void
computeLedResidual(const camera_model &params,
                   const Eigen::Vector3<T> &T_cam_model,
                   const Eigen::Quaternion<T> &Q_cam_model,
                   const Eigen::Vector2<T> &blob_position_2d,
                   const Eigen::Vector3<T> &T_model_led,
                   Eigen::MatrixBase<Derived> &residual)
{
	Eigen::Vector2<T> predicted_point;
	T led_depth;
	project_led<T>(params,          //
	               Q_cam_model,     //
	               T_cam_model,     //
	               T_model_led,     //
	               predicted_point, //
	               &led_depth);     //

	if constexpr (kResidualDebugPrint) {
		if constexpr (std::is_same_v<T, double>) {
			U_LOG_W("meas=(%f,%f)", blob_position_2d.x(), blob_position_2d.y());
			U_LOG_W("pred=(%f,%f)", predicted_point.x(), predicted_point.y());

			U_LOG_W("pose=(%f,%f,%f), (%f,%f,%f)", T_cam_model.x(), T_cam_model.y(), T_cam_model.z(),
			        Q_cam_model.x(), Q_cam_model.y(), Q_cam_model.z());

			if (std::isnan(predicted_point.x()) || std::isnan(predicted_point.y())) {
				U_LOG_W("Projected point is NaN, led_depth=%f", led_depth);

				// XRT_DEBUGBREAK();
			}
		} else {
			U_LOG_W("meas=(%f,%f)", blob_position_2d.x().a, blob_position_2d.y().a);
			U_LOG_W("pred=(%f,%f)", predicted_point.x().a, predicted_point.y().a);

			U_LOG_W("pose=(%f,%f,%f), (%f,%f,%f)", T_cam_model.x().a, T_cam_model.y().a, T_cam_model.z().a,
			        Q_cam_model.x().a, Q_cam_model.y().a, Q_cam_model.z().a);

			if (std::isnan(predicted_point.x().a) || std::isnan(predicted_point.y().a)) {
				U_LOG_W("Projected point is NaN, led_depth=%f", led_depth.a);

				// XRT_DEBUGBREAK();
			}
		}
	}

	// Compute the residual
	residual = predicted_point - blob_position_2d;

	/*
	 * Provide a smooth penalty for points behind the camera, since if we just set a large residual,
	 * the solver will have no derivative to try to work back from. This solution penalizes bad
	 * poses, while pushing the optimizer towards the correct solution (no LEDs behind the camera).
	 */
	if (led_depth < T(0.001)) {
		residual += Eigen::Vector2<T>::Constant(T(1000) * (T(0.001) - T(led_depth)));
	}
}

}; // namespace xrt::tracking::constellation::optimizer
