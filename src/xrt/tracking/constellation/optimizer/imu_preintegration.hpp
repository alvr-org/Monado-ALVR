// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Generic math helpers for IMU preintegration.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup tracking
 */

#pragma once

#include "os/os_time.h"

#include "math/m_eigen_interop.hpp"
#include "math/m_quatexpmap.hpp"
#include "math/m_quatexpmap_bigceres.hpp"

#include <Eigen/Core>
#include <Eigen/Geometry>

#include "ceres/jet.h"

#include "internal_math.hpp"

#include <cstring>
#include <cmath>
#include <mutex>
#include <string>
#include <algorithm>
#include <span>
#include <cassert>


namespace xrt::tracking::constellation::optimizer {

using namespace xrt::auxiliary::math;

using Eigen::Map;
using Eigen::Matrix;
using Eigen::Matrix3d;
using Eigen::MatrixBase;
using Eigen::Quaternion;
using Eigen::Quaterniond;
using Eigen::Vector;
using Eigen::Vector2;
using Eigen::Vector3;
using Eigen::Vector3d;
using Eigen::Vector4;

using ceres::Jet;

// @todo Tune this per device. Right now these are CV1 HMD numbers.
//! BMI055 gyroscope noise density, 0.014 deg/s/sqrt(Hz) from the datasheet, in rad/s/sqrt(Hz).
constexpr double kGyroNoiseDensity = 2.44e-4;
//! BMI055 accelerometer noise density, 150 ug/sqrt(Hz) from the datasheet, in (m/s^2)/sqrt(Hz).
constexpr double kAccelNoiseDensity = 1.47e-3;

// @todo These need to be computed properly, not guessed.
constexpr double kAccelBiasRandomWalkSigma = 1e-3;  // (m/s^2) / sqrt(s)
constexpr double kGyroBiasRandomWalkSigma = 1e-4;   // (rad/s) / sqrt(s)
constexpr double kAccelScaleRandomWalkSigma = 1e-5; // 1 / sqrt(s)

// @todo This needs to be computed properly, not hardcoded for CV1.
constexpr double kAccelBiasAnchorSigma = 0.687; // m/s^2
constexpr double kGyroBiasAnchorSigma = 0.0175; // rad/s
constexpr double kAccelScaleAnchorSigma = 0.05; // dimensionless

//! Largest accelerometer bias to allow, in m/s^2. The BMI055's zero-g offset is +-70mg, so this is ~1.4x that.
constexpr double kMaxAccelBias = 1.0;
//! Largest gyroscope bias to allow, in rad/s. The BMI055's zero-rate offset is +-1 deg/s, this is ~3 deg/s.
constexpr double kMaxGyroBias = 0.05;

// Some reasonable defaults. This should be pretty close to 1.0 already in most cases.
constexpr double kMinAccelScale = 0.8;
constexpr double kMaxAccelScale = 1.0 / kMinAccelScale;

/*!
 * Camera samples from the same mosaic closer together than this are treated as one keyframe. Sometimes frame timestamps
 * are a couple microseconds off due to guesswork/interpolation, this groups them together.
 *
 * @todo We should by default group based on sequence_id! However, to make external users more reliable, let's not rely
 *       on that just yet until the infrasturcture for matching sequence ID across multiple cameras is available.
 */
constexpr time_duration_ns kKeyframeGroupingToleranceNs = U_TIME_1MS_IN_NS;

enum class ImuBiasStateIndex : int
{
	AccelBiasX,
	AccelBiasY,
	AccelBiasZ,

	GyroBiasX,
	GyroBiasY,
	GyroBiasZ,

	AccelScaleX,
	AccelScaleY,
	AccelScaleZ,

	NumIndices,
};

typedef ceres::Jet<double, static_cast<int>(ImuBiasStateIndex::NumIndices)> ImuBiasJet;

/*!
 * A full-state IMU bias, with Accelerometer Bias, Gyroscope Bias, and Accelerometer Scale.
 *
 * Everything here belongs to one physical IMU, so a solve over several devices holds one of these per device. The
 * strength of gravity is shared by all of them and lives in @ref WorldGravity instead.
 */
template <typename T> struct ImuBias
{
	static_assert(!std::is_const_v<T>,
	              "block scalar must not be const — did a `T*` overload deduce T = const double?");

	static constexpr int kNumParameters = static_cast<int>(ImuBiasStateIndex::NumIndices);

	static constexpr int kAccelBiasIndex = static_cast<int>(ImuBiasStateIndex::AccelBiasX);
	static constexpr int kGyroBiasIndex = static_cast<int>(ImuBiasStateIndex::GyroBiasX);
	static constexpr int kAccelScaleIndex = static_cast<int>(ImuBiasStateIndex::AccelScaleX);

	Vector3<T> accel_bias;
	Vector3<T> gyro_bias;

	Vector3<T> accel_scale;

	ImuBias()
	    : accel_bias(Vector3<T>::Zero()), gyro_bias(Vector3<T>::Zero()), accel_scale(Vector3<T>::Constant(T(1)))
	{}

	ImuBias(const Vector3<T> &accel_bias, const Vector3<T> &gyro_bias, const Vector3<T> &accel_scale)
	    : accel_bias(accel_bias), gyro_bias(gyro_bias), accel_scale(accel_scale)
	{}

	ImuBias(Eigen::Ref<const Vector<T, kNumParameters>> parameters)
	    : accel_bias(parameters.template segment<3>(kAccelBiasIndex)),
	      gyro_bias(parameters.template segment<3>(kGyroBiasIndex)),
	      accel_scale(parameters.template segment<3>(kAccelScaleIndex))
	{}

	void
	pack(Eigen::Ref<Vector<T, kNumParameters>> parameters)
	{
		parameters.template segment<3>(kAccelBiasIndex) = this->accel_bias;
		parameters.template segment<3>(kGyroBiasIndex) = this->gyro_bias;
		parameters.template segment<3>(kAccelScaleIndex) = this->accel_scale;
	}

	ImuBias<ImuBiasJet>
	seed() const
	{
		auto seeded = ImuBias<ImuBiasJet>();
		seeded.accel_bias = {{this->accel_bias.x(), kAccelBiasIndex + 0},
		                     {this->accel_bias.y(), kAccelBiasIndex + 1},
		                     {this->accel_bias.z(), kAccelBiasIndex + 2}};
		seeded.gyro_bias = {{this->gyro_bias.x(), kGyroBiasIndex + 0},
		                    {this->gyro_bias.y(), kGyroBiasIndex + 1},
		                    {this->gyro_bias.z(), kGyroBiasIndex + 2}};
		seeded.accel_scale = {{this->accel_scale.x(), kAccelScaleIndex + 0},
		                      {this->accel_scale.y(), kAccelScaleIndex + 1},
		                      {this->accel_scale.z(), kAccelScaleIndex + 2}};
		return seeded;
	}
};

enum class ImuBiasCovarianceIndex : int
{
	RotX,
	RotY,
	RotZ,

	VelocityX,
	VelocityY,
	VelocityZ,

	PositionX,
	PositionY,
	PositionZ,

	NumIndices,
};

static constexpr int kImuBiasRotCovIndex = static_cast<int>(ImuBiasCovarianceIndex::RotX);
static constexpr int kImuBiasVelCovIndex = static_cast<int>(ImuBiasCovarianceIndex::VelocityX);
static constexpr int kImuBiasPosCovIndex = static_cast<int>(ImuBiasCovarianceIndex::PositionX);
static constexpr int kNumImuBiasCovIndices = static_cast<int>(ImuBiasCovarianceIndex::NumIndices);

//! [rotation; velocity; position], the order computeImuResidual writes its residual in.
typedef Matrix<double, kNumImuBiasCovIndices, kNumImuBiasCovIndices> PreintegrationCovarianceMatrix;

struct PreintegratedImuSamples
{
	static constexpr int kNumResiduals = 9;

	//! The preintegrated rotation from the IMU samples, as a quaternion.
	Quaterniond Q_start_end;
	//! This is really "delta integrated specific force", but we call it delta_velocity for simplicity.
	Vector3d delta_velocity;
	//! This is really "delta double-integrated specific force", but we call it delta_position for simplicity.
	Vector3d delta_position;

	//! The whitening matrix for this preintegration, the lower triangular.
	PreintegrationCovarianceMatrix whitening;

	double dt;

	Vector3d accel_bias_at_integration;
	Vector3d gyro_bias_at_integration;

	Vector3d accel_scale_at_integration;

	/*
	 * Bias Jacobians
	 */

	Matrix3d J_R_bg; //!< d(Q_start_cur) / d(gyro_bias)

	Matrix3d J_v_ba; //!< d(delta_velocity) / d(accel_bias)
	Matrix3d J_v_sa; //!< d(delta_velocity) / d(accel_scale)
	Matrix3d J_v_bg; //!< d(delta_velocity) / d(gyro_bias)

	Matrix3d J_p_ba; //!< d(delta_position) / d(accel_bias)
	Matrix3d J_p_sa; //!< d(delta_position) / d(accel_scale)
	Matrix3d J_p_bg; //!< d(delta_position) / d(gyro_bias)

	static PreintegratedImuSamples
	Identity()
	{
		return {
		    .Q_start_end = Quaterniond::Identity(),
		    .delta_velocity = Vector3d::Zero(),
		    .delta_position = Vector3d::Zero(),

		    .whitening = PreintegrationCovarianceMatrix::Zero(),

		    .dt = 0,

		    .accel_bias_at_integration = Vector3d::Zero(),
		    .gyro_bias_at_integration = Vector3d::Zero(),
		    .accel_scale_at_integration = Vector3d::Constant(1.0),

		    .J_R_bg = Matrix3d::Zero(),
		    .J_v_ba = Matrix3d::Zero(),
		    .J_v_sa = Matrix3d::Zero(),
		    .J_v_bg = Matrix3d::Zero(),
		    .J_p_ba = Matrix3d::Zero(),
		    .J_p_sa = Matrix3d::Zero(),
		    .J_p_bg = Matrix3d::Zero(),
		};
	}
};

/*!
 * Does IMU pre-integration on a set of IMU samples, interpolates the starting sample to the start time, and returns the
 * pre-integrated samples.
 *
 * @param imu_samples The IMU samples to preintegrate
 * @param first_sample_after_end_time The first sample after the end timestamp
 * @param imu_bias The Jet-seeded IMU biases
 * @param start_time_ns The start time of the preintegration
 * @param end_time_ns The end time of the preintegration
 */
PreintegratedImuSamples
preintegrate(const std::span<const xrt_imu_sample> &imu_samples,
             const xrt_imu_sample &first_sample_after_end_time,
             const ImuBias<ImuBiasJet> &imu_bias,
             timepoint_ns start_time_ns,
             timepoint_ns end_time_ns);


/*
 *
 * IMU residuals
 *
 */

template <typename T>
static void
computeImuResidual(const PreintegratedImuSamples &imu_preintegration,
                   const PoseWithVelocity<T> &start_T_world_device,
                   const PoseWithVelocity<T> &end_T_world_device,
                   const ImuBias<T> &imu_bias,
                   const WorldGravity<T> &world_gravity,
                   Vector<T, PreintegratedImuSamples::kNumResiduals> &residual)
{
	Quaternion<T> Q_world_device_start;
	start_T_world_device.getQuaternion(Q_world_device_start);
	auto start_Q_device_world = Q_world_device_start.conjugate();

	Quaternion<T> end_Q_world_device;
	end_T_world_device.getQuaternion(end_Q_world_device);

	Vector3<T> gravity_vector = world_gravity.toVector();

	// I'm sure we've all seen these exact formulas a million times.
	auto end_predicted_Q_start = start_Q_device_world * end_Q_world_device;
	auto predicted_start_delta_velocity = start_Q_device_world *                      //
	                                      (end_T_world_device.velocity -              //
	                                       start_T_world_device.velocity -            //
	                                       (gravity_vector * imu_preintegration.dt)); //
	auto predicted_start_delta_position =                                             //
	    start_Q_device_world *                                                        //
	    (end_T_world_device.translation -                                             //
	     start_T_world_device.translation -                                           //
	     start_T_world_device.velocity * imu_preintegration.dt -                      //
	     (T(0.5) * gravity_vector * imu_preintegration.dt * imu_preintegration.dt));  //

	// Compute the deltas for how much IMU bias has changed from now to preintegration.
	Vector3<T> delta_accel_bias = imu_bias.accel_bias - imu_preintegration.accel_bias_at_integration.cast<T>();
	Vector3<T> delta_gyro_bias = imu_bias.gyro_bias - imu_preintegration.gyro_bias_at_integration.cast<T>();
	Vector3<T> delta_accel_scale = imu_bias.accel_scale - imu_preintegration.accel_scale_at_integration.cast<T>();

	// Correct the amount of rotation by the change in gyro bias
	Quaternion<T> corrected_Q_start_end =                                    //
	    imu_preintegration.Q_start_end.cast<T>() *                           //
	    quat_exp_so3(imu_preintegration.J_R_bg.cast<T>() * delta_gyro_bias); //

	// Correct the change in velocity and position by the change by accel and gyro bias/scale.
	Vector3<T> corrected_start_delta_velocity =                    //
	    imu_preintegration.delta_velocity.cast<T>()                //
	    + imu_preintegration.J_v_ba.cast<T>() * delta_accel_bias   //
	    + imu_preintegration.J_v_bg.cast<T>() * delta_gyro_bias    //
	    + imu_preintegration.J_v_sa.cast<T>() * delta_accel_scale; //

	Vector3<T> corrected_start_delta_position =                    //
	    imu_preintegration.delta_position.cast<T>()                //
	    + imu_preintegration.J_p_ba.cast<T>() * delta_accel_bias   //
	    + imu_preintegration.J_p_bg.cast<T>() * delta_gyro_bias    //
	    + imu_preintegration.J_p_sa.cast<T>() * delta_accel_scale; //

	// Compute the amount of rotation error for the predicted vs. measured
	auto delta_rotation_error = corrected_Q_start_end.conjugate() * end_predicted_Q_start;
	if (delta_rotation_error.w() < T(0)) {
		delta_rotation_error.coeffs() = -delta_rotation_error.coeffs();
	}

	residual.template segment<3>(0) = quat_ln_so3(delta_rotation_error);
	residual.template segment<3>(3) = predicted_start_delta_velocity - corrected_start_delta_velocity;
	residual.template segment<3>(6) = predicted_start_delta_position - corrected_start_delta_position;
}

/*
 *
 * Cost functors
 *
 */

//! IMU preintegration factor, between each camera keyframe.
struct ImuCostFunctor
{
	PreintegratedImuSamples preintegration;

	template <typename T>
	bool
	operator()(const T *const world_gravity_parameters,
	           const T *const imu_bias_parameters,
	           const T *const start_keyframe_parameters,
	           const T *const end_keyframe_parameters,
	           T *residuals) const
	{
		const WorldGravity<T> world_gravity{
		    Map<const Vector<T, WorldGravity<T>::kNumParameters>>(world_gravity_parameters)};

		const ImuBias<T> imu_bias{Map<const Vector<T, ImuBias<T>::kNumParameters>>(imu_bias_parameters)};

		const PoseWithVelocity<T> start_T_world_device{
		    Map<const Vector<T, PoseWithVelocity<T>::kNumParameters>>(start_keyframe_parameters)};
		const PoseWithVelocity<T> end_T_world_device{
		    Map<const Vector<T, PoseWithVelocity<T>::kNumParameters>>(end_keyframe_parameters)};

		Vector<T, PreintegratedImuSamples::kNumResiduals> computed_residual;
		computeImuResidual<T>(this->preintegration, //
		                      start_T_world_device, //
		                      end_T_world_device,   //
		                      imu_bias,             //
		                      world_gravity,        //
		                      computed_residual);   //

		Map<Vector<T, PreintegratedImuSamples::kNumResiduals>> out_residual(residuals);
		out_residual = this->preintegration.whitening.cast<T>() * computed_residual;

		return true;
	}
};

typedef ceres::AutoDiffCostFunction<ImuCostFunctor,                           //
                                    PreintegratedImuSamples::kNumResiduals,   //
                                    WorldGravity<double>::kNumParameters,     // world_gravity
                                    ImuBias<double>::kNumParameters,          // imu_bias
                                    PoseWithVelocity<double>::kNumParameters, // start_keyframe
                                    PoseWithVelocity<double>::kNumParameters> // end_keyframe
    ImuCostFunction;

//! IMU Bias random walk cost functor, between each keyframe.
struct ImuBiasRandomWalkCostFunctor
{
	static constexpr int kNumResiduals = ImuBias<double>::kNumParameters;

	//! Seconds spanned by this interval
	double dt;

	template <typename T>
	bool
	operator()(const T *const start_bias_parameters, //
	           const T *const end_bias_parameters,   //
	           T *residuals) const                   //
	{
		const ImuBias<T> start{Map<const Vector<T, ImuBias<T>::kNumParameters>>(start_bias_parameters)};
		const ImuBias<T> end{Map<const Vector<T, ImuBias<T>::kNumParameters>>(end_bias_parameters)};

		assert(dt > 0);

		const T sqrt_dt = T(std::sqrt(this->dt));

		Map<Vector3<T>> accel_residual(residuals + 0);
		Map<Vector3<T>> gyro_residual(residuals + 3);
		Map<Vector3<T>> scale_residual(residuals + 6);

		/*
		 * Assume that the difference between the start and ending bias is close to zero (it should be),
		 * whitened by how much we *expect* it to walk.
		 */
		accel_residual = (end.accel_bias - start.accel_bias) / (T(kAccelBiasRandomWalkSigma) * sqrt_dt);
		gyro_residual = (end.gyro_bias - start.gyro_bias) / (T(kGyroBiasRandomWalkSigma) * sqrt_dt);
		scale_residual = (end.accel_scale - start.accel_scale) / (T(kAccelScaleRandomWalkSigma) * sqrt_dt);

		// @todo Manually write the jacobians for these for performance

		return true;
	}
};

typedef ceres::AutoDiffCostFunction<ImuBiasRandomWalkCostFunctor,                //
                                    ImuBiasRandomWalkCostFunctor::kNumResiduals, //
                                    ImuBias<double>::kNumParameters,             // start bias
                                    ImuBias<double>::kNumParameters>             // end bias
    ImuBiasRandomWalkCostFunction;

//! IMU Bias anchor cost functor, to hold it at some value when marginalization has no guess for it.
struct ImuBiasAnchorCostFunctor
{
	static constexpr int kNumResiduals = ImuBias<double>::kNumParameters;

	ImuBias<double> anchor;

	template <typename T>
	bool
	operator()(const T *const bias_parameters, //
	           T *residuals) const             //
	{
		const ImuBias<T> bias{Map<const Vector<T, ImuBias<T>::kNumParameters>>(bias_parameters)};

		Map<Vector3<T>> accel_residual(residuals + 0);
		Map<Vector3<T>> gyro_residual(residuals + 3);
		Map<Vector3<T>> scale_residual(residuals + 6);

		// Assume the IMU bias is close to what we expect it to be.
		accel_residual = (bias.accel_bias - this->anchor.accel_bias) / T(kAccelBiasAnchorSigma);
		gyro_residual = (bias.gyro_bias - this->anchor.gyro_bias) / T(kGyroBiasAnchorSigma);
		scale_residual = (bias.accel_scale - this->anchor.accel_scale) / T(kAccelScaleAnchorSigma);

		// @todo Manually write the jacobians for these for performance

		return true;
	}
};

typedef ceres::AutoDiffCostFunction<ImuBiasAnchorCostFunctor,                //
                                    ImuBiasAnchorCostFunctor::kNumResiduals, //
                                    ImuBias<double>::kNumParameters>         // bias
    ImuBiasAnchorCostFunction;

}; // namespace xrt::tracking::constellation::optimizer
