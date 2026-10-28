// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Constellation tracker rig calibration.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup tracking
 */

#include "xrt/xrt_config_build.h"

#include "util/u_debug.h"
#include "util/u_var.h"

#include "math/m_eigen_interop.hpp"

#include "constellation/t_constellation_tracker_internal.hpp"

#include "ceres/autodiff_cost_function.h"
#include "ceres/loss_function.h"
#include "ceres/problem.h"
#include "ceres/solver.h"

#ifdef XRT_FEATURE_RERUN
#include "rig_calibration_rerun.hpp"
#endif

#include "rig_calibration.hpp"
#include "internal_math.hpp"
#include "imu_preintegration.hpp"

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <cstring>
#include <cmath>
#include <mutex>
#include <string>
#include <algorithm>


namespace {

DEBUG_GET_ONCE_LOG_OPTION(rig_calibration_log, "T_OFFLINE_SENSOR_CALIBRATION_LOG", U_LOGGING_WARN)
DEBUG_GET_ONCE_BOOL_OPTION(rig_calibration_check_jacobian, "T_OFFLINE_SENSOR_CALIBRATION_CHECK_JACOBIAN", false)
DEBUG_GET_ONCE_BOOL_OPTION(rig_calibration_enable_rerun, "T_OFFLINE_SENSOR_CALIBRATION_RERUN_ENABLE", false)
#ifdef XRT_FEATURE_RERUN
DEBUG_GET_ONCE_BOOL_OPTION(rig_calibration_rerun_spawn, "T_OFFLINE_SENSOR_CALIBRATION_RERUN_SPAWN", true)
#endif // XRT_FEATURE_RERUN

/*
 * The camera timestamp offset sweep.
 *
 * Frame timestamps come with some unknown latency offset from the IMU timestamps, Sweeping a constant offset over the
 * camera timestamps and re-solving turns that into something measurable: the offset which minimizes the camera chi^2
 * per residual is the likely offset. Set the range to a non-zero value to run the sweep, which costs one full
 * solve per step.
 *
 * This doesn't actually correct the problem long-term, but it's a *very* useful debugging tool to have in our toolbelt.
 */
DEBUG_GET_ONCE_NUM_OPTION(rig_calibration_time_offset_ns, "T_OFFLINE_SENSOR_CALIBRATION_TIME_OFFSET_NS", 0)
DEBUG_GET_ONCE_NUM_OPTION(rig_calibration_sweep_range_ns, "T_OFFLINE_SENSOR_CALIBRATION_SWEEP_RANGE_NS", 0)
DEBUG_GET_ONCE_NUM_OPTION(rig_calibration_sweep_step_ns, "T_OFFLINE_SENSOR_CALIBRATION_SWEEP_STEP_NS", 1000 * 1000)

#define RS_TRACE(rig_calibration, ...) U_LOG_IFL_T(rig_calibration->log_level, __VA_ARGS__)
#define RS_DEBUG(rig_calibration, ...) U_LOG_IFL_D(rig_calibration->log_level, __VA_ARGS__)
#define RS_INFO(rig_calibration, ...) U_LOG_IFL_I(rig_calibration->log_level, __VA_ARGS__)
#define RS_WARN(rig_calibration, ...) U_LOG_IFL_W(rig_calibration->log_level, __VA_ARGS__)
#define RS_ERROR(rig_calibration, ...) U_LOG_IFL_E(rig_calibration->log_level, __VA_ARGS__)

using namespace xrt::auxiliary::math;
using namespace xrt::tracking::constellation::optimizer;
using namespace xrt::tracking::constellation::optimizer::rig_calibration;

/*!
 * Whether the strength of gravity is optimized for. Only weakly observable, we probably don't want this enabled for
 * rig calibration given we often don't have particularly great observations of gravity magnitude in most motions.
 */
constexpr bool kOptimizeWorldGravity = false;

struct OptimizationCameraSample
{
	DeviceCameraSample camera_sample;
	//! The tracker's own pose for this observation, used to seed the solve. See @ref DeviceCameraSample.
	Pose<double> Tcv_cam_device_seed;
};

struct DeviceKeyframe
{
	timepoint_ns timestamp_ns;
	PreintegratedImuSamples preintegration;
	std::vector<OptimizationCameraSample> camera_samples;
};

struct OptimizerCamera
{
	//! The index of this camera in the mosaic.
	size_t mosaic_idx;
	//! The index of this camera in the device.
	size_t camera_idx;
	//! The index of the parameter block for this camera in the optimizer parameter vector, if applicable.
	size_t dynamic_parameter_idx;

	//! What the reprojection residuals project through.
	camera_model params;
	//! Where the camera is held for the whole solve, only meaningful when @ref dynamic is false.
	xrt_pose Tcv_world_cam;

	bool is_2dof_camera;
	bool dynamic;
};

/*!
 * Owns the parameter blocks the solver optimizes.
 */
struct ParameterBlocks
{
public: // Fields
	//! Shared by every IMU factor in the solve, since gravity does not change between devices.
	std::array<double, WorldGravity<double>::kNumParameters> world_gravity{};
	std::array<double, ImuBias<double>::kNumParameters> imu_bias{};
	std::array<double, ImuExtrinsics<double>::kNumParameters> imu_extrinsics{};
	std::array<double, PitchRoll<double>::kNumParameters> static_camera{};
	std::vector<std::array<double, Pose<double>::kNumParameters>> dynamic_cameras;
	std::vector<std::array<double, PoseWithVelocity<double>::kNumParameters>> keyframes;

private: // Fields
	bool has_static_camera{false};

public: // Methods
	ParameterBlocks() = default;

	ParameterBlocks(const std::map<std::pair<size_t, size_t>, OptimizerCamera> &cameras, int num_keyframes_)
	{
		int num_dynamic_cameras = 0;
		for (auto &pair : cameras) {
			const auto &camera = pair.second;

			if (camera.is_2dof_camera) {
				this->has_static_camera = true;
			} else if (camera.dynamic) {
				num_dynamic_cameras += 1;
			}
		}

		this->dynamic_cameras.resize(num_dynamic_cameras);
		this->keyframes.resize(num_keyframes_);
	}

	int
	numDynamicCameras() const
	{
		return static_cast<int>(this->dynamic_cameras.size());
	}

	int
	numDeviceKeyframes() const
	{
		return static_cast<int>(this->keyframes.size());
	}

	bool
	hasStaticCamera() const
	{
		return this->has_static_camera;
	}

	//! True when every block the solver was allowed to touch came back finite.
	bool
	allFinite() const
	{
		const auto block_finite = [](const double *values, size_t count) {
			for (size_t i = 0; i < count; i++) {
				if (!std::isfinite(values[i])) {
					return false;
				}
			}
			return true;
		};

		if (!block_finite(this->world_gravity.data(), this->world_gravity.size())) {
			return false;
		}

		if (!block_finite(this->imu_bias.data(), this->imu_bias.size())) {
			return false;
		}

		if (!block_finite(this->imu_extrinsics.data(), this->imu_extrinsics.size())) {
			return false;
		}

		if (this->has_static_camera && !block_finite(this->static_camera.data(), this->static_camera.size())) {
			return false;
		}

		for (const auto &camera : this->dynamic_cameras) {
			if (!block_finite(camera.data(), camera.size())) {
				return false;
			}
		}

		for (const auto &keyframe : this->keyframes) {
			if (!block_finite(keyframe.data(), keyframe.size())) {
				return false;
			}
		}

		return true;
	}
};

//! A single LED observation
struct CameraObservationLed
{
	xrt_vec2 point2d;
	xrt_vec3 point3d;
};

/*!
 * Writes the whitened reprojection residuals for every LED of one observation, two per LED.
 *
 * The whole observation is one factor rather than one per LED, so that a pose recovered from a bad constellation
 * match is robustified as the single mistake it is: its LEDs are individually well localized and all wrong in the
 * same direction, which is exactly the case a per-LED loss cannot see.
 */
template <typename T>
void
computeCameraObservationResiduals(const camera_model &params,
                                  const std::span<const CameraObservationLed> &leds,
                                  const ImuExtrinsics<T> &imu_extrinsics,
                                  const Eigen::Transform<T, 3, Eigen::Isometry> &predicted_T_world_camera,
                                  const Eigen::Transform<T, 3, Eigen::Isometry> &predicted_T_world_device,
                                  T *residuals)
{
	const auto predicted_T_camera_world = predicted_T_world_camera.inverse();
	const auto predicted_T_camera_imu = predicted_T_camera_world * predicted_T_world_device;

	const Quaternion<T> Q_cam_model = Quaternion<T>(predicted_T_camera_imu.rotation()) * imu_extrinsics.Q_imu_model;

	for (size_t i = 0; i < leds.size(); i++) {
		const CameraObservationLed &led = leds[i];

		const Vector2<T> blob_position_2d(T(led.point2d.x), T(led.point2d.y));
		const Vector3<T> T_model_led(T(led.point3d.x), T(led.point3d.y), T(led.point3d.z));

		Map<Vector2<T>> residual(residuals + (i * kNumLedResiduals));
		computeLedResidual<T>(                    //
		    params,                               //
		    predicted_T_camera_imu.translation(), //
		    Q_cam_model,                          //
		    blob_position_2d,                     //
		    T_model_led,                          //
		    residual);                            //

		residual /= T(kBlobPositionSigmaPixels);
	}
}

/*
 *
 * Cost functors
 *
 */


//! Camera observation with a fixed-pose camera.
struct FixedCameraObservationCostFunctor
{
	camera_model params;
	std::vector<CameraObservationLed> leds;
	xrt_pose Tcv_world_cam;

	template <typename T>
	bool
	operator()(const T *const keyframe_parameters, const T *const imu_extrinsics_parameters, T *residuals) const
	{
		const Pose<T> predicted_T_world_device{
		    Map<const Vector<T, Pose<T>::kNumParameters>>(keyframe_parameters)};

		const ImuExtrinsics<T> predicted_imu_extrinsics{
		    Map<const Vector<T, ImuExtrinsics<T>::kNumParameters>>(imu_extrinsics_parameters)};

		computeCameraObservationResiduals<T>(                //
		    this->params,                                    //
		    this->leds,                                      //
		    predicted_imu_extrinsics,                        //
		    isometryFromPose(this->Tcv_world_cam).cast<T>(), //
		    predicted_T_world_device.toTransform(),          //
		    residuals);                                      //

		return true;
	}
};

typedef ceres::AutoDiffCostFunction<FixedCameraObservationCostFunctor,        //
                                    ceres::DYNAMIC,                           //
                                    PoseWithVelocity<double>::kNumParameters, //
                                    ImuExtrinsics<double>::kNumParameters>    //
    FixedCameraObservationCostFunction;

//! Camera observation against the camera pinned at the origin with only pitch and roll free.
struct TwoDofCameraObservationCostFunctor
{
	camera_model params;
	std::vector<CameraObservationLed> leds;

	template <typename T>
	bool
	operator()(const T *const keyframe_parameters,
	           const T *const imu_extrinsics_parameters,
	           const T *const pitch_roll_parameters,
	           T *residuals) const
	{
		const Pose<T> predicted_T_world_device{
		    Map<const Vector<T, Pose<T>::kNumParameters>>(keyframe_parameters)};

		const ImuExtrinsics<T> predicted_imu_extrinsics{
		    Map<const Vector<T, ImuExtrinsics<T>::kNumParameters>>(imu_extrinsics_parameters)};

		const PitchRoll<T> predicted_pitch_roll{
		    Map<const Vector<T, PitchRoll<T>::kNumParameters>>(pitch_roll_parameters)};

		computeCameraObservationResiduals<T>(                             //
		    this->params,                                                 //
		    this->leds,                                                   //
		    predicted_imu_extrinsics, predicted_pitch_roll.toTransform(), //
		    predicted_T_world_device.toTransform(),                       //
		    residuals);                                                   //

		return true;
	}
};

typedef ceres::AutoDiffCostFunction<TwoDofCameraObservationCostFunctor,       //
                                    ceres::DYNAMIC,                           //
                                    PoseWithVelocity<double>::kNumParameters, //
                                    ImuExtrinsics<double>::kNumParameters,    //
                                    PitchRoll<double>::kNumParameters>        //
    TwoDofCameraObservationCostFunction;

//! Camera observation against a camera whose full pose is being solved for.
struct DynamicCameraObservationCostFunctor
{
	camera_model params;
	std::vector<CameraObservationLed> leds;

	template <typename T>
	bool
	operator()(const T *const keyframe_parameters,
	           const T *const imu_extrinsics_parameters,
	           const T *const camera_parameters,
	           T *residuals) const
	{
		const Pose<T> predicted_T_world_device{
		    Map<const Vector<T, Pose<T>::kNumParameters>>(keyframe_parameters)};

		const ImuExtrinsics<T> predicted_imu_extrinsics{
		    Map<const Vector<T, ImuExtrinsics<T>::kNumParameters>>(imu_extrinsics_parameters)};

		const Pose<T> predicted_T_world_camera{
		    Map<const Vector<T, Pose<T>::kNumParameters>>(camera_parameters)};

		computeCameraObservationResiduals<T>(       //
		    this->params,                           //
		    this->leds,                             //
		    predicted_imu_extrinsics,               //
		    predicted_T_world_camera.toTransform(), //
		    predicted_T_world_device.toTransform(), //
		    residuals);                             //

		return true;
	}
};

typedef ceres::AutoDiffCostFunction<DynamicCameraObservationCostFunctor,      //
                                    ceres::DYNAMIC,                           //
                                    PoseWithVelocity<double>::kNumParameters, //
                                    ImuExtrinsics<double>::kNumParameters,    //
                                    Pose<double>::kNumParameters>             //
    DynamicCameraObservationCostFunction;

/*!
 * Holds everything the solve needs: the observations, how each camera is parameterized, and the parameter blocks
 * themselves.
 *
 * The camera classification in the constructor is unchanged from when this was a single cost functor; only the way
 * the parameters are stored and handed to the solver differs.
 */

struct RigCalibrationProblem
{
	std::vector<DeviceKeyframe> keyframes;
	std::map<std::pair<size_t, size_t>, OptimizerCamera> cameras;

	//! Whether we're optimizing the pitch and roll of a camera with a fixed yaw.
	bool optimizing_static_camera{};

	ParameterBlocks parameter_blocks;

	//! Filled in by @ref build, kept so the final cost can be attributed to the sensor that could not be fit.
	std::vector<ceres::ResidualBlockId> imu_residual_blocks;
	std::vector<ceres::ResidualBlockId> camera_residual_blocks;

	/*!
	 * The camera losses, keyed by the residual count they were sized for. Borrowed by the problem rather than
	 * owned by it, so they have to outlive it. The problem is built from this and torn down
	 * first. Map nodes are stable, so handing out references to them is safe as more are added.
	 */
	std::map<int, ceres::HuberLoss> camera_losses;

	RigCalibrationProblem(const std::vector<DeviceKeyframe> &keyframes,
	                      const std::vector<CameraDescription> &cameras)
	    : keyframes(keyframes), cameras({})
	{
		/*
		 * If no cameras have a concrete pose, then we need to lock one camera at the origin with fixed yaw and
		 * variable pitch/roll. The optimizer needs some static reference frame to be able to converge.
		 */
		this->optimizing_static_camera =
		    !std::any_of(cameras.begin(), cameras.end(),
		                 [](const CameraDescription &camera) { return camera.has_concrete_pose; });

		/*
		 * Counts the parameter blocks handed out so far, which is not the same as the number of cameras
		 * visited: the 2dof camera has no pose block of its own.
		 */
		uint32_t dynamic_camera_idx = 0;
		bool assigned_2dof_camera = false;
		for (auto &camera_description : cameras) {
			OptimizerCamera optimizer_camera = {
			    .mosaic_idx = camera_description.mosaic_idx,
			    .camera_idx = camera_description.camera_idx,
			    .dynamic_parameter_idx = 0,
			    .params = camera_description.params,
			    .Tcv_world_cam = camera_description.Tcv_world_cam,
			    .is_2dof_camera = false,
			    .dynamic = !camera_description.has_concrete_pose,
			};

			if (this->optimizing_static_camera && !assigned_2dof_camera) {
				// The first camera is the static camera, it has a fixed yaw and variable pitch/roll.
				optimizer_camera.is_2dof_camera = true;
				assigned_2dof_camera = true;
			}

			if (optimizer_camera.dynamic && !optimizer_camera.is_2dof_camera) {
				optimizer_camera.dynamic_parameter_idx = dynamic_camera_idx;
				dynamic_camera_idx += 1;
			}

			this->cameras[{camera_description.mosaic_idx, camera_description.camera_idx}] =
			    optimizer_camera;
		}

		this->parameter_blocks = ParameterBlocks(this->cameras, static_cast<int>(keyframes.size()));
	}

	/*!
	 * The Huber loss for a camera factor carrying `num_residuals` residuals, allocating it on first use.
	 */
	ceres::LossFunction &
	cameraLossForResidualCount(int num_residuals)
	{
		const auto it = this->camera_losses.find(num_residuals);
		if (it != this->camera_losses.end()) {
			return it->second;
		}

		const double dof = static_cast<double>(num_residuals);
		const double threshold = std::sqrt(dof + (kCameraHuberDeltaSigmas * std::sqrt(2.0 * dof)));

		return this->camera_losses.try_emplace(num_residuals, threshold).first->second;
	}

	/*!
	 * Wires every factor up to the blocks it reads.
	 *
	 * The manifolds are borrowed rather than owned, so they must outlive the problem. Cost functions are heap
	 * allocated and left to Ceres to free, since there is one per factor.
	 */
	void
	build(ceres::Problem &problem,
	      PoseManifold &pose_manifold,
	      PoseWithVelocityManifold &pose_velocity_manifold,
	      QuaternionManifold &quaternion_manifold)
	{
		ParameterBlocks &blocks = this->parameter_blocks;

		// World gravity parameter block
		{
			problem.AddParameterBlock(blocks.world_gravity.data(), WorldGravity<double>::kNumParameters);

			if constexpr (kOptimizeWorldGravity) {
				/*
				 * Gravity varies by a few tenths of a percent over latitude and altitude, so a solve
				 * that wants more than a percent is using it to absorb something else. This keeps it a
				 * free parameter without letting it become a fudge factor.
				 */
				problem.SetParameterLowerBound(blocks.world_gravity.data(),
				                               WorldGravity<double>::kGravityMagIndex, kMinGravity);
				problem.SetParameterUpperBound(blocks.world_gravity.data(),
				                               WorldGravity<double>::kGravityMagIndex, kMaxGravity);
			} else {
				// Stays at whatever it was seeded with, and the IMU factors read it as a constant.
				problem.SetParameterBlockConstant(blocks.world_gravity.data());
			}
		}

		// IMU Bias parameter block
		{
			problem.AddParameterBlock(blocks.imu_bias.data(), ImuBias<double>::kNumParameters);

			// Sets the scale bounds to reasonable values
			for (int i = 0; i < 3; i++) {
				problem.SetParameterLowerBound(blocks.imu_bias.data(),
				                               ImuBias<double>::kAccelScaleIndex + i, kMinAccelScale);
				problem.SetParameterUpperBound(blocks.imu_bias.data(),
				                               ImuBias<double>::kAccelScaleIndex + i, kMaxAccelScale);
			}

			/*
			 * Biases are what the solve reaches for when it would rather not move a camera, so they need
			 * to be held to what the sensor can actually do. Both bounds are already generous, so what is
			 * left should be well inside the raw part-to-part spread these come from.
			 */
			for (int i = 0; i < 3; i++) {
				problem.SetParameterLowerBound(blocks.imu_bias.data(),
				                               ImuBias<double>::kAccelBiasIndex + i, -kMaxAccelBias);
				problem.SetParameterUpperBound(blocks.imu_bias.data(),
				                               ImuBias<double>::kAccelBiasIndex + i, kMaxAccelBias);

				problem.SetParameterLowerBound(blocks.imu_bias.data(),
				                               ImuBias<double>::kGyroBiasIndex + i, -kMaxGyroBias);
				problem.SetParameterUpperBound(blocks.imu_bias.data(),
				                               ImuBias<double>::kGyroBiasIndex + i, kMaxGyroBias);
			}
		}

		// IMU extrinsics parameter block
		{
			problem.AddParameterBlock(blocks.imu_extrinsics.data(), ImuExtrinsics<double>::kNumParameters,
			                          &quaternion_manifold);
		}

		if (blocks.hasStaticCamera()) {
			problem.AddParameterBlock(blocks.static_camera.data(), PitchRoll<double>::kNumParameters);
		}

		for (int i = 0; i < blocks.numDynamicCameras(); i++) {
			problem.AddParameterBlock(blocks.dynamic_cameras.at(i).data(), Pose<double>::kNumParameters,
			                          &pose_manifold);
		}

		for (int i = 0; i < blocks.numDeviceKeyframes(); i++) {
			problem.AddParameterBlock(blocks.keyframes.at(i).data(),
			                          PoseWithVelocity<double>::kNumParameters, &pose_velocity_manifold);
		}

		// IMU factors, one between each consecutive pair of keyframes.
		this->imu_residual_blocks.reserve(blocks.numDeviceKeyframes());
		for (int i = 0; i + 1 < blocks.numDeviceKeyframes(); i++) {
			// Don't place IMU preintegrations between keyframes with a dt that is too large.
			if (this->keyframes[i].preintegration.dt > 0.500) {
				continue;
			}

			this->imu_residual_blocks.push_back(problem.AddResidualBlock(
			    new ImuCostFunction(new ImuCostFunctor{this->keyframes[i].preintegration}),
			    nullptr,                             //
			    blocks.world_gravity.data(),         //
			    blocks.imu_bias.data(),              //
			    blocks.keyframes.at(i).data(),       //
			    blocks.keyframes.at(i + 1).data())); //
		}

		/*
		 * Camera factors, one per observation with every LED correspondence of it stacked in, so that the loss
		 * applies to the observation as a whole. RANSAC has already vetted the labels individually, so what is
		 * left to be robust against is a whole pose recovered from a bad constellation match, whose LEDs are
		 * all wrong together and coherently.
		 */
		for (int keyframe = 0; keyframe < blocks.numDeviceKeyframes(); keyframe++) {
			for (const auto &observed_camera_device : this->keyframes[keyframe].camera_samples) {
				const auto &camera_sample = observed_camera_device.camera_sample;

				const auto camera_description_it = this->cameras.find(
				    std::make_pair(camera_sample.mosaic_idx, camera_sample.camera_idx));
				if (camera_description_it == this->cameras.end()) {
					throw std::runtime_error("Camera not found in optimizer camera list");
				}
				const auto &camera_description = (*camera_description_it).second;

				double *keyframe_block = blocks.keyframes.at(keyframe).data();

				std::vector<CameraObservationLed> leds;
				leds.reserve(camera_sample.num_points);
				for (size_t i = 0; i < camera_sample.num_points; i++) {
					leds.push_back({
					    .point2d = camera_sample.points2d[i],
					    .point3d = camera_sample.points3d[i],
					});
				}

				if (leds.empty()) {
					continue;
				}

				const int num_residuals = static_cast<int>(leds.size()) * kNumLedResiduals;
				ceres::LossFunction &camera_loss = this->cameraLossForResidualCount(num_residuals);

				if (camera_description.is_2dof_camera) {
					// 2dof camera
					this->camera_residual_blocks.push_back(problem.AddResidualBlock( //
					    new TwoDofCameraObservationCostFunction(
					        new TwoDofCameraObservationCostFunctor{camera_description.params,
					                                               std::move(leds)},
					        num_residuals),
					    &camera_loss,                  //
					    keyframe_block,                //
					    blocks.imu_extrinsics.data(),  //
					    blocks.static_camera.data())); //
				} else if (camera_description.dynamic) {
					// Dynamic camera
					this->camera_residual_blocks.push_back(problem.AddResidualBlock( //
					    new DynamicCameraObservationCostFunction(
					        new DynamicCameraObservationCostFunctor{camera_description.params,
					                                                std::move(leds)},
					        num_residuals),
					    &camera_loss,                 //
					    keyframe_block,               //
					    blocks.imu_extrinsics.data(), //
					    blocks.dynamic_cameras.at(camera_description.dynamic_parameter_idx)
					        .data())); //
				} else {
					// Fixed camera
					this->camera_residual_blocks.push_back(problem.AddResidualBlock( //
					    new FixedCameraObservationCostFunction(
					        new FixedCameraObservationCostFunctor{camera_description.params,
					                                              std::move(leds),
					                                              camera_description.Tcv_world_cam},
					        num_residuals),
					    &camera_loss,                   //
					    keyframe_block,                 //
					    blocks.imu_extrinsics.data())); //
				}
			}
		}
	}
};

/*
 *
 * Result unpacking
 *
 */

xrt_pose
toXrtPose(const Quaterniond &orientation, const Vector3d &position)
{
	xrt_pose out = XRT_POSE_IDENTITY;
	map_quat(out.orientation) = orientation.normalized().cast<float>();
	map_vec3(out.position) = position.cast<float>();

	return out;
}

xrt_pose
toXrtPose(const Pose<double> &pose)
{
	Quaterniond orientation;
	pose.getQuaternion(orientation);

	return toXrtPose(orientation, pose.translation);
}

/*!
 * Estimates the pitch and roll of the 2dof camera by estimating where "up" is to the IMU.
 *
 * @todo Replace this algorithm by having each `Device` contain a "gravity vector" solve using some 3dof IMU fusion. We
 *       can then pull that gravity vector each camera observation.
 *
 * @param keyframes The keyframes to search, whose preintegration must already be filled in.
 * @param camera    The 2dof camera, whose observations pick out which samples are usable.
 *
 * @return The (pitch, roll) to seed with, or nothing when no usable observation was found.
 */
std::optional<Vector2<double>>
estimateTwoDofCameraTilt(const std::vector<DeviceKeyframe> &keyframes, const OptimizerCamera &camera)
{
	//! How far a keyframe's specific force may sit from one gravity and still count as stationary.
	constexpr double kStationaryToleranceMs2 = 0.5;
	//! Below this many stationary keyframes the estimate is averaged over everything instead.
	constexpr size_t kMinStationaryKeyframes = 16;

	Vector3d stationary_up_sum = Vector3d::Zero();
	Vector3d all_up_sum = Vector3d::Zero();
	size_t num_stationary = 0;
	size_t num_all = 0;

	for (const auto &keyframe : keyframes) {
		if (keyframe.preintegration.dt <= 0.0) {
			continue;
		}

		for (const auto &sample : keyframe.camera_samples) {
			if (sample.camera_sample.mosaic_idx != camera.mosaic_idx ||
			    sample.camera_sample.camera_idx != camera.camera_idx) {
				continue;
			}

			/*
			 * The preintegration integrates specific force in the device frame of its own keyframe, so
			 * over one keyframe interval this is the average the accelerometer saw there.
			 */
			const Vector3d specific_force =
			    keyframe.preintegration.delta_velocity / keyframe.preintegration.dt;
			const double magnitude = specific_force.norm();
			if (magnitude <= 0.0) {
				break;
			}

			// R_cam_device takes the reading into the camera's frame, where it points along world up.
			const Vector3d up_in_camera =
			    (sample.Tcv_cam_device_seed.rotation * specific_force).normalized();

			all_up_sum += up_in_camera;
			num_all++;

			if (std::abs(magnitude - MATH_GRAVITY_M_S2) < kStationaryToleranceMs2) {
				stationary_up_sum += up_in_camera;
				num_stationary++;
			}

			break;
		}
	}

	const Vector3d up_sum = num_stationary >= kMinStationaryKeyframes ? stationary_up_sum : all_up_sum;
	if ((num_stationary >= kMinStationaryKeyframes ? num_stationary : num_all) == 0 ||
	    up_sum.norm() <= std::numeric_limits<double>::epsilon()) {
		return std::nullopt;
	}

	const Vector3d up = up_sum.normalized();

	/*
	 * PitchRoll builds Rz(roll) * Rx(pitch), so pitch is chosen to flatten Z and roll then swings what is left
	 * onto world up, which is -Y in this Y-down world.
	 */
	const double pitch = std::atan2(-up.z(), up.y());
	const double flattened_y = std::sqrt((up.y() * up.y()) + (up.z() * up.z()));
	const double roll = std::atan2(-up.x(), -flattened_y);

	return Vector2<double>(pitch, roll);
}

/*!
 * How trustworthy one observation's seed pose is. Bigger is better.
 *
 * Only used to pick which observation to seed from, so it does not have to be a real uncertainty. The number of
 * correspondences is a decent stand-in.
 */
size_t
observationQuality(const DeviceCameraSample &camera_sample)
{
	return camera_sample.num_points;
}

//! The results of the seeding, so we can log it.
struct SeedSummary
{
	//! How many keyframes no camera could reach, which kept the fallback pose.
	size_t num_unseeded_keyframes;
	//! The (pitch, roll) the 2dof camera was seeded at, absent when there is no 2dof camera or no usable samples.
	std::optional<Vector2<double>> two_dof_tilt;
};

/*!
 * Writes a starting point for every parameter block, by doing a best guess of it. Solving often has local minimums, so
 * we need to try to get as close as possible.
 *
 * @param problem The problem to seed, whose parameter blocks are overwritten.
 *
 * @return What the seeding was able to establish, see @ref SeedSummary.
 */
SeedSummary
seedParameterBlocks(RigCalibrationProblem &problem)
{
	ParameterBlocks &blocks = problem.parameter_blocks;
	const size_t num_keyframes = problem.keyframes.size();

	SeedSummary summary = {
	    .num_unseeded_keyframes = 0,
	    .two_dof_tilt = std::nullopt,
	};

	using Isometry = Eigen::Transform<double, 3, Eigen::Isometry>;

	// Everything below is in OpenCV space, matching the observations and the residuals.

	std::map<std::pair<size_t, size_t>, Isometry> camera_seeds;
	std::vector<std::optional<Isometry>> keyframe_seeds(num_keyframes, std::nullopt);

	/*
	 * The cameras whose world pose is known before any keyframe is: the ones this solve is not free to move, plus
	 * the 2dof camera, which is only free in pitch and roll about a fixed origin.
	 */
	for (const auto &pair : problem.cameras) {
		const OptimizerCamera &camera = pair.second;

		if (camera.is_2dof_camera) {
			Map<Vector<double, PitchRoll<double>::kNumParameters>> pitch_roll_parameters(
			    blocks.static_camera.data());

			const auto tilt = estimateTwoDofCameraTilt(problem.keyframes, camera);
			pitch_roll_parameters = tilt.value_or(Vector2<double>::Zero());

			summary.two_dof_tilt = tilt;
			camera_seeds[pair.first] = PitchRoll<double>(pitch_roll_parameters).toTransform();
			continue;
		}

		if (!camera.dynamic) {
			camera_seeds[pair.first] = isometryFromPose(camera.Tcv_world_cam).cast<double>();
		}
	}

	/*
	 * Alternate between the two directions until nothing new can be reached. A camera is only ever seeded once, so
	 * this settles after at most one pass per camera; the extra iteration is what detects that it has settled.
	 */
	for (size_t round = 0; round <= problem.cameras.size(); round++) {
		bool progressed = false;

		// Cameras to keyframes.
		for (size_t i = 0; i < num_keyframes; i++) {
			if (keyframe_seeds[i].has_value()) {
				continue;
			}

			const OptimizationCameraSample *best = nullptr;
			size_t best_quality = 0;

			for (const auto &sample : problem.keyframes[i].camera_samples) {
				const auto key =
				    std::make_pair(sample.camera_sample.mosaic_idx, sample.camera_sample.camera_idx);
				if (camera_seeds.count(key) == 0) {
					continue;
				}

				const size_t quality = observationQuality(sample.camera_sample);
				if (best == nullptr || quality > best_quality) {
					best = &sample;
					best_quality = quality;
				}
			}

			if (best == nullptr) {
				continue;
			}

			const auto key = std::make_pair(best->camera_sample.mosaic_idx, best->camera_sample.camera_idx);
			keyframe_seeds[i] = camera_seeds.at(key) * best->Tcv_cam_device_seed.toTransform();
			progressed = true;
		}

		// Keyframes to cameras, taking each camera's most certain observation rather than its first.
		for (const auto &pair : problem.cameras) {
			if (camera_seeds.count(pair.first) != 0) {
				continue;
			}

			const OptimizationCameraSample *best = nullptr;
			size_t best_quality = 0;
			size_t best_keyframe = 0;

			for (size_t i = 0; i < num_keyframes; i++) {
				if (!keyframe_seeds[i].has_value()) {
					continue;
				}

				for (const auto &sample : problem.keyframes[i].camera_samples) {
					if (sample.camera_sample.mosaic_idx != pair.second.mosaic_idx ||
					    sample.camera_sample.camera_idx != pair.second.camera_idx) {
						continue;
					}

					const size_t quality = observationQuality(sample.camera_sample);
					if (best == nullptr || quality > best_quality) {
						best = &sample;
						best_quality = quality;
						best_keyframe = i;
					}
				}
			}

			if (best == nullptr) {
				continue;
			}

			// T_world_cam = T_world_device * T_device_cam.
			camera_seeds[pair.first] =
			    keyframe_seeds[best_keyframe].value() * best->Tcv_cam_device_seed.toTransform().inverse();
			progressed = true;
		}

		if (!progressed) {
			break;
		}
	}

	/*
	 * Velocities, from the seeded positions of the neighbouring keyframes. Zero is a poor guess for a device that
	 * was deliberately swung around, and the IMU factors are the ones that read it.
	 */
	std::vector<Vector3d> keyframe_velocities(num_keyframes, Vector3d::Zero());
	for (size_t i = 0; i < num_keyframes; i++) {
		const size_t previous = i > 0 ? i - 1 : i;
		const size_t next = i + 1 < num_keyframes ? i + 1 : i;

		if (previous == next || !keyframe_seeds[previous].has_value() || !keyframe_seeds[next].has_value()) {
			continue;
		}

		const double dt_s =
		    time_ns_to_s(problem.keyframes[next].timestamp_ns - problem.keyframes[previous].timestamp_ns);
		if (dt_s <= 0.0) {
			continue;
		}

		keyframe_velocities[i] =
		    (keyframe_seeds[next]->translation() - keyframe_seeds[previous]->translation()) / dt_s;
	}

	// Kept for the keyframes no camera could reach, which is the only case left that has nothing to go on.
	const xrt_pose fallback_pose = {
	    .orientation = {.x = 0, .y = 1, .z = 0, .w = 0}, // facing the camera
	    .position = {.x = 0, .y = 0, .z = 1},            // one meter away from it
	};

	for (size_t i = 0; i < num_keyframes; i++) {
		Map<Vector<double, PoseWithVelocity<double>::kNumParameters>> parameters(blocks.keyframes.at(i).data());

		if (!keyframe_seeds[i].has_value()) {
			PoseWithVelocity<double>(fallback_pose).pack(parameters);
			summary.num_unseeded_keyframes++;
			continue;
		}

		Pose<double>(keyframe_seeds[i].value()).pack(parameters.segment<Pose<double>::kNumParameters>(0));
		parameters.segment<3>(Pose<double>::kNumParameters) = keyframe_velocities[i];
	}

	for (const auto &pair : problem.cameras) {
		const OptimizerCamera &camera = pair.second;

		if (!camera.dynamic || camera.is_2dof_camera) {
			continue;
		}

		Map<Vector<double, Pose<double>::kNumParameters>> parameters(
		    blocks.dynamic_cameras.at(camera.dynamic_parameter_idx).data());

		if (camera_seeds.count(pair.first) == 0) {
			/*
			 * Never observed alongside a camera that was already placed, so there is nothing tying it to
			 * the others. Anywhere but exactly on top of another camera will do.
			 */
			const xrt_pose fallback_camera_pose = {
			    .orientation = XRT_QUAT_IDENTITY,
			    .position = {.x = 0.1f * (float)(camera.dynamic_parameter_idx + 1), .y = 0, .z = 0},
			};
			Pose<double>(fallback_camera_pose).pack(parameters);
			continue;
		}

		Pose<double>(camera_seeds.at(pair.first)).pack(parameters);
	}

	return summary;
}

//! Unpacks the parameter block into the real types we want.
CalibrationResult
unpackResult(t_constellation_device_id_t device_id,
             const RigCalibrationProblem &problem,
             const ceres::Solver::Summary &summary)
{
	const ParameterBlocks &blocks = problem.parameter_blocks;

	CalibrationResult result = {
	    .device_id = device_id,
	    .imu = {},
	    .gravity_mag = 0.0,
	    .cameras = {},
	    .keyframes = {},
	    .imu_fitness = {},
	    .camera_fitness = {},
	    .initial_cost = summary.initial_cost,
	    .final_cost = summary.final_cost,
	    .iterations = static_cast<int>(summary.iterations.size()),
	};

	{
		const ImuBias<double> bias(
		    Map<const Vector<double, ImuBias<double>::kNumParameters>>(blocks.imu_bias.data()));

		const ImuExtrinsics<double> extrinsics(
		    Map<const Vector<double, ImuExtrinsics<double>::kNumParameters>>(blocks.imu_extrinsics.data()));

		result.imu = {
		    .accel_bias = {},
		    .gyro_bias = {},
		    .accel_scale = {},

		    .Qcv_imu_model = {},
		};
		map_vec3_f64(result.imu.accel_bias) = bias.accel_bias;
		map_vec3_f64(result.imu.gyro_bias) = bias.gyro_bias;
		map_vec3_f64(result.imu.accel_scale) = bias.accel_scale;
		map_quat(result.imu.Qcv_imu_model) = extrinsics.Q_imu_model.cast<float>();
	}

	{
		const WorldGravity<double> gravity(
		    Map<const Vector<double, WorldGravity<double>::kNumParameters>>(blocks.world_gravity.data()));

		result.gravity_mag = gravity.gravity_mag;
	}

	result.cameras.reserve(problem.cameras.size());
	for (const auto &pair : problem.cameras) {
		const OptimizerCamera &camera = pair.second;

		CameraCalibration camera_calibration = {
		    .mosaic_idx = camera.mosaic_idx,
		    .camera_idx = camera.camera_idx,
		    .Tcv_world_cam = XRT_POSE_IDENTITY,
		    .params = camera.params,
		    .optimized = camera.dynamic,
		    .is_2dof = camera.is_2dof_camera,
		};

		if (camera.is_2dof_camera) {
			const PitchRoll<double> pitch_roll(
			    Map<const Vector<double, PitchRoll<double>::kNumParameters>>(blocks.static_camera.data()));

			Quaterniond orientation;
			pitch_roll.toQuaternion(orientation);

			camera_calibration.Tcv_world_cam = XRT_POSE_IDENTITY;
			map_quat(camera_calibration.Tcv_world_cam.orientation) = orientation.cast<float>();
		} else if (camera.dynamic) {
			const Pose<double> pose(Map<const Vector<double, Pose<double>::kNumParameters>>(
			    blocks.dynamic_cameras.at(camera.dynamic_parameter_idx).data()));

			camera_calibration.Tcv_world_cam = toXrtPose(pose);
		} else {
			// Held where the solve was told it sits, so it comes back out unchanged.
			camera_calibration.Tcv_world_cam = camera.Tcv_world_cam;
		}

		result.cameras.push_back(camera_calibration);
	}

	result.keyframes.reserve(problem.keyframes.size());
	for (int i = 0; i < (int)problem.keyframes.size(); i++) {
		const DeviceKeyframe &keyframe = problem.keyframes[i];

		const PoseWithVelocity<double> pose(
		    Map<const Vector<double, PoseWithVelocity<double>::kNumParameters>>(blocks.keyframes.at(i).data()));

		KeyframeCalibration keyframe_calibration = {
		    .timestamp_ns = keyframe.timestamp_ns,
		    .Tcv_world_device = toXrtPose(pose),
		    .velocity_m_s = {},
		    .camera_samples = {},
		};
		map_vec3(keyframe_calibration.velocity_m_s) = pose.velocity.cast<float>();

		keyframe_calibration.camera_samples.reserve(keyframe.camera_samples.size());
		for (const auto &optimization_camera_sample : keyframe.camera_samples) {
			keyframe_calibration.camera_samples.push_back(optimization_camera_sample.camera_sample);
		}

		result.keyframes.push_back(std::move(keyframe_calibration));
	}

	return result;
}

}; // namespace

namespace xrt::tracking::constellation::optimizer::rig_calibration {

using namespace xrt::tracking::constellation;

/*
 *
 * DeviceSamples implementations
 *
 */

DeviceSamples::DeviceSamples() : camera_samples(), imu_samples() {}

/*
 *
 * RigCalibration implementations
 *
 */

RigCalibration::RigCalibration() : log_level(debug_get_log_option_rig_calibration_log()), device_samples(), lock()
{
	this->u_var_button_begin_collecting_data = {
	    .cb =
	        [](void *ptr) {
		        RigCalibration *rig_calibration = (RigCalibration *)ptr;
		        rig_calibration->beginCollectingData();
	        },
	    .ptr = this,
	    .downed = 0,
	    .label = "Begin Collecting Data",
	    .width = 0,
	    .height = 0,
	    .disabled = false,
	};
	this->u_var_button_end_collecting_data = {
	    .cb =
	        [](void *ptr) {
		        RigCalibration *rig_calibration = (RigCalibration *)ptr;
		        rig_calibration->endCollectingData();
	        },
	    .ptr = this,
	    .downed = 0,
	    .label = "End Collecting Data",
	    .width = 0,
	    .height = 0,
	    .disabled = false,
	};
	this->u_var_button_optimize = {
	    .cb =
	        [](void *ptr) {
		        RigCalibration *rig_calibration = (RigCalibration *)ptr;
		        CalibrationResult result;
		        if (!rig_calibration->optimize(result)) {
			        RS_ERROR(rig_calibration, "Room setup optimization failed.");
		        }
	        },
	    .ptr = this,
	    .downed = 0,
	    .label = "Optimize",
	    .width = 0,
	    .height = 0,
	    .disabled = false,
	};

	if (debug_get_bool_option_rig_calibration_enable_rerun()) {
#ifdef XRT_FEATURE_RERUN
		this->rerun_context = std::make_unique<RerunContext>();
		RS_INFO(this, "Offline sensor calibration Rerun stream enabled");

		if (debug_get_bool_option_rig_calibration_rerun_spawn()) {
			this->rerun_context->spawnViewer();
		} else {
			this->rerun_context->stream->connect_grpc().exit_on_failure();
		}
#else
		RS_ERROR(this, "Rerun stream requested but XRT_FEATURE_RERUN is not enabled");
#endif
	}

	u_var_add_root(this, "Room Setup", true);
	u_var_add_log_level(this, &this->log_level, "Log Level");
	u_var_add_bool(this, &this->collecting_data, "Collecting Data");
	u_var_add_button(this, &this->u_var_button_begin_collecting_data, "Begin Collecting Data");
	u_var_add_button(this, &this->u_var_button_end_collecting_data, "End Collecting Data");
	u_var_add_button(this, &this->u_var_button_optimize, "Optimize");
}

RigCalibration::~RigCalibration()
{
	u_var_remove_root(this);
}

void
RigCalibration::addCamera(const CameraDescription &camera_description)
{
	std::unique_lock<os::Mutex> lock(this->lock);

	this->cameras.push_back(camera_description);
}

void
RigCalibration::beginCollectingData()
{
	std::unique_lock<os::Mutex> lock(this->lock);
	this->collecting_data = true;

	this->device_samples.clear();
}

void
RigCalibration::endCollectingData()
{
	std::unique_lock<os::Mutex> lock(this->lock);
	this->collecting_data = false;
}

bool
RigCalibration::isCollectingData()
{
	std::unique_lock<os::Mutex> lock(this->lock);
	return this->collecting_data;
}

void
RigCalibration::pushCameraSample(ConstellationTracker *tracker, const CameraSample &sample)
{
	std::unique_lock<os::Mutex> lock(this->lock);

	if (!this->collecting_data) {
		return;
	}

	for (uint32_t i = 0; i < sample.device_count; i++) {
		const auto &device_state = sample.device_states[i];

		if (!device_state.found_pose.has_value()) {
			continue;
		}

		const auto device_id = device_state.device_id;
		const auto &found_pose = device_state.found_pose.value();

		// Skip this found pose if it's already been handled.
		if (found_pose.handled) {
			continue;
		}

		const auto device_iter =
		    std::find_if(tracker->devices.begin(), tracker->devices.end(),
		                 [&](const std::unique_ptr<Device> &dev) { return dev->id == device_id; });

		if (device_iter == tracker->devices.end()) {
			continue;
		}

		const auto &device = *device_iter;

		uint32_t num_points = 0;
		std::array<xrt_vec2, XRT_CONSTELLATION_MAX_LEDS_PER_DEVICE> points2d;
		std::array<xrt_vec3, XRT_CONSTELLATION_MAX_LEDS_PER_DEVICE> points3d;

		for (uint32_t j = 0; j < sample.blob_count; j++) {
			const auto &blob = sample.blobs[j];
			if (blob.matched_device_id == device_id) {
				int32_t index =
				    findLedIndexById(device->search_model->led_model, blob.matched_device_led_id);
				if (index == -1) {
					continue;
				}

				if constexpr (kOptimizeUndistortedPoints) {
					points2d[num_points] = blob.center_undistorted;
				} else {
					points2d[num_points] = blob.center_distorted;
				}
				points3d[num_points] = device->search_model->led_model->leds[index].position;
				num_points++;

				assert(num_points <= XRT_CONSTELLATION_MAX_LEDS_PER_DEVICE);
			}
		}

		/*
		 * The pose the tracker found came out of a fit to at least this many correspondences, but blobs are
		 * re-matched to the final pose afterwards, so an observation can still end up short of it here.
		 */
		if (num_points < kMinCorrespondencesPerObservation) {
			continue;
		}

		DeviceCameraSample device_sample = {
		    .mosaic_idx = sample.mosaic_index,
		    .camera_idx = sample.camera_index,

		    .timestamp_ns = sample.timestamp_ns,

		    .Tcv_cam_device_seed = found_pose.Tcv_cam_device,

		    .num_points = num_points,
		    .points2d = points2d,
		    .points3d = points3d,
		};

		auto &device_samples = this->getDeviceSamplesLocked(device_id);
		device_samples.camera_samples.push_back(device_sample);
	}
}

void
RigCalibration::pushImuSample(t_constellation_device_id_t device_id, const xrt_imu_sample &sample)
{
	std::unique_lock<os::Mutex> lock(this->lock);

	if (!this->collecting_data) {
		return;
	}

	auto &device_samples = this->getDeviceSamplesLocked(device_id);

	xrt_imu_sample cv_sample = sample;
	// Convert IMU samples from OpenXR -> OpenCV coordinate space, since the optimizer works in OpenCV space.
	math_vec3_f64_convert_from_opencv(&sample.accel_m_s2, &cv_sample.accel_m_s2);
	math_vec3_f64_convert_from_opencv(&sample.gyro_rad_secs, &cv_sample.gyro_rad_secs);

	device_samples.imu_samples.push_back(cv_sample);
}

bool
RigCalibration::optimize(CalibrationResult &out_result)
{
	/*
	 * Everything the solve reads is taken out of the object here and the lock is dropped before any of the work
	 * starts.
	 *
	 * If we don't copy it out now it will stall the IMU and Camera push threads, which will cause the constellation
	 * tracker to hold a shared lock for a really long time, and freeze up camera threads.
	 */
	std::vector<CameraDescription> cameras;
	DeviceSamples device_samples;
	t_constellation_device_id_t device_to_optimize_for;

	{
		std::unique_lock<os::Mutex> lock(this->lock);

		if (this->cameras.size() == 0) {
			assert(false); // This should never happen
			return false;
		}

		if (this->collecting_data) {
			RS_ERROR(this, "Cannot optimize while collecting data.");
			return false;
		}

		if (!this->bestDeviceLocked(device_to_optimize_for)) {
			return false;
		}

		DeviceSamples &collected = this->getDeviceSamplesLocked(device_to_optimize_for);

		if (collected.camera_samples.empty()) {
			RS_ERROR(this, "No camera samples were collected for device %u.", device_to_optimize_for);
			return false;
		}

		if (collected.imu_samples.size() < 2) {
			RS_ERROR(this, "Only %zu IMU samples were collected for device %u.",
			         collected.imu_samples.size(), device_to_optimize_for);
			return false;
		}

		cameras = this->cameras;
		device_samples = std::move(collected);
	}

	std::stable_sort(
	    device_samples.imu_samples.begin(), device_samples.imu_samples.end(),
	    [](const xrt_imu_sample &a, const xrt_imu_sample &b) { return a.timestamp_ns < b.timestamp_ns; });

	{
		/*
		 * How the observations sit in time, before any grouping gets involved.
		 *
		 * Two cameras watching one device should give two samples on every exposure timestamp. Falling short of
		 * that has two very different causes which this separates: samples sharing a timestamp but landing in
		 * different keyframes would be a grouping problem, whereas timestamps that only ever carry one sample
		 * mean a camera never produced a pose for that exposure at all.
		 */
		std::map<timepoint_ns, size_t> samples_per_timestamp;
		std::map<std::pair<size_t, size_t>, size_t> samples_per_camera;
		std::map<std::pair<size_t, size_t>, size_t> lone_samples_per_camera;

		for (const auto &sample : device_samples.camera_samples) {
			samples_per_timestamp[sample.timestamp_ns]++;
			samples_per_camera[{sample.mosaic_idx, sample.camera_idx}]++;
		}

		std::map<size_t, size_t> timestamp_histogram;
		for (const auto &pair : samples_per_timestamp) {
			timestamp_histogram[pair.second]++;
		}

		for (const auto &sample : device_samples.camera_samples) {
			if (samples_per_timestamp.at(sample.timestamp_ns) == 1) {
				lone_samples_per_camera[{sample.mosaic_idx, sample.camera_idx}]++;
			}
		}

		RS_DEBUG(this, "Collected %zu camera samples across %zu distinct timestamps",
		         device_samples.camera_samples.size(), samples_per_timestamp.size());
		for (const auto &pair : timestamp_histogram) {
			RS_DEBUG(this, "  %zu timestamps carry %zu sample(s)", pair.second, pair.first);
		}
		for (const auto &pair : samples_per_camera) {
			RS_DEBUG(this, "  camera %zu/%zu: %zu samples, %zu of them the only one at their timestamp",
			         pair.first.first, pair.first.second, pair.second,
			         lone_samples_per_camera.count(pair.first) != 0 ? lone_samples_per_camera.at(pair.first)
			                                                        : 0);
		}
	}

	const time_duration_ns base_offset_ns = debug_get_num_option_rig_calibration_time_offset_ns();
	const time_duration_ns sweep_range_ns = debug_get_num_option_rig_calibration_sweep_range_ns();
	const time_duration_ns sweep_step_ns = debug_get_num_option_rig_calibration_sweep_step_ns();

	CalibrationResult result{};
	time_duration_ns solved_offset_ns = base_offset_ns;

	if (sweep_range_ns > 0 && sweep_step_ns > 0) {
		/*
		 * One solve per step, so this is deliberately opt-in. The winner is the offset with the lowest camera
		 * chi^2 per residual: the IMU factors are the fixed reference the cameras are being slid against, so
		 * judging on the total cost would let a solve win by simply fitting the IMU better.
		 */
		const int64_t half_steps = sweep_range_ns / sweep_step_ns;
		bool have_result = false;

		RS_DEBUG(this,
		         "Sweeping camera time offset over %+" PRId64 " ns to %+" PRId64 " ns in %" PRId64 " steps",
		         base_offset_ns - half_steps * sweep_step_ns, base_offset_ns + half_steps * sweep_step_ns,
		         half_steps * 2 + 1);

		for (int64_t step = -half_steps; step <= half_steps; step++) {
			const time_duration_ns offset_ns = base_offset_ns + step * sweep_step_ns;

			CalibrationResult candidate{};
			if (!this->solveWithCameraTimeOffset(device_to_optimize_for, cameras,
			                                     device_samples.camera_samples, device_samples.imu_samples,
			                                     offset_ns, candidate)) {
				RS_WARN(this, "Camera time offset %+" PRId64 " ns did not produce a usable solve",
				        offset_ns);
				continue;
			}

			RS_DEBUG(this,
			         "Camera time offset %+" PRId64
			         " ns: camera chi^2/residual %g (%g sigma rms), IMU "
			         "chi^2/residual %g (%g sigma rms)",        //
			         offset_ns,                                 //
			         candidate.camera_fitness.meanChiSquared(), //
			         candidate.camera_fitness.rmsSigma(),       //
			         candidate.imu_fitness.meanChiSquared(),    //
			         candidate.imu_fitness.rmsSigma());         //

			if (!have_result ||
			    candidate.camera_fitness.meanChiSquared() < result.camera_fitness.meanChiSquared()) {
				result = std::move(candidate);
				solved_offset_ns = offset_ns;
				have_result = true;
			}
		}

		if (!have_result) {
			RS_ERROR(this, "No camera time offset in the sweep produced a usable solve.");
			return false;
		}

		RS_DEBUG(this, "Best camera time offset %+" PRId64 " ns with camera chi^2/residual %g (%g sigma rms)",
		         solved_offset_ns, result.camera_fitness.meanChiSquared(), result.camera_fitness.rmsSigma());

		if (solved_offset_ns == base_offset_ns - half_steps * sweep_step_ns ||
		    solved_offset_ns == base_offset_ns + half_steps * sweep_step_ns) {
			RS_WARN(this,
			        "The best camera time offset is at the edge of the swept range, so the true minimum "
			        "is probably outside it. Widen T_OFFLINE_SENSOR_CALIBRATION_SWEEP_RANGE_NS.");
		}
	} else if (!this->solveWithCameraTimeOffset(device_to_optimize_for, cameras, device_samples.camera_samples,
	                                            device_samples.imu_samples, base_offset_ns, result)) {
		return false;
	}

	/*
	 * Attributed per sensor, since a whitened residual is in standard deviations and the cost on its own
	 * cannot say which sensor the model failed to explain.
	 */
	RS_DEBUG(this,
	         "IMU factors: %zu residuals, chi^2 %g, chi^2/residual %g (%g sigma rms)", //
	         result.imu_fitness.num_residuals,                                         //
	         result.imu_fitness.chi_squared,                                           //
	         result.imu_fitness.meanChiSquared(),                                      //
	         result.imu_fitness.rmsSigma());                                           //
	RS_DEBUG(this,
	         "Camera factors: %zu residuals, chi^2 %g, chi^2/residual %g (%g sigma rms)", //
	         result.camera_fitness.num_residuals,                                         //
	         result.camera_fitness.chi_squared,                                           //
	         result.camera_fitness.meanChiSquared(),                                      //
	         result.camera_fitness.rmsSigma());                                           //

	RS_DEBUG(this,
	         "Solved IMU calibration: accel bias [%f, %f, %f] m/s^2, gyro bias [%f, %f, %f] rad/s, accel scale "
	         "[%f, %f, %f], gravity %f m/s^2", //
	         result.imu.accel_bias.x, result.imu.accel_bias.y,
	         result.imu.accel_bias.z, //
	         result.imu.gyro_bias.x, result.imu.gyro_bias.y,
	         result.imu.gyro_bias.z, //
	         result.imu.accel_scale.x, result.imu.accel_scale.y,
	         result.imu.accel_scale.z, //
	         result.gravity_mag);      //
	for (const auto &camera : result.cameras) {
		RS_DEBUG(this,
		         "Solved camera %zu/%zu (%s): position [%f, %f, %f], orientation [%f, %f, %f, %f]", //
		         camera.mosaic_idx, camera.camera_idx,                                              //
		         camera.is_2dof ? "2dof" : (camera.optimized ? "solved" : "fixed"),                 //
		         camera.Tcv_world_cam.position.x,                                                   //
		         camera.Tcv_world_cam.position.y,                                                   //
		         camera.Tcv_world_cam.position.z,                                                   //
		         camera.Tcv_world_cam.orientation.x,                                                //
		         camera.Tcv_world_cam.orientation.y,                                                //
		         camera.Tcv_world_cam.orientation.z,                                                //
		         camera.Tcv_world_cam.orientation.w);                                               //
	}

#ifdef XRT_FEATURE_RERUN
	if (this->rerun_context) {
		this->rerun_context->logResult(result);
	}
#endif

	out_result = result;

	return true;
}

bool
RigCalibration::numSamplesForBestDevice(size_t &out_num_samples) const
{
	if (this->cameras.size() == 0) {
		assert(false); // This should never happen
		return false;
	}

	std::unique_lock<os::Mutex> lock(this->lock);

	t_constellation_device_id_t best_device;
	if (!this->bestDeviceLocked(best_device)) {
		return false;
	}

	out_num_samples = this->device_samples.find(best_device)->second.camera_samples.size();
	return true;
}

bool
RigCalibration::bestDeviceLocked(t_constellation_device_id_t &out_device) const
{
	if (this->device_samples.size() == 0) {
		return false;
	}

	// Pick a device to optimize using. Let's go with the first one for now.
	// @todo Compute this by picking the device that was seen the most by all cameras.
	out_device = this->device_samples.begin()->first;
	return true;
}

bool
RigCalibration::solveWithCameraTimeOffset(t_constellation_device_id_t device_id,
                                          const std::vector<CameraDescription> &cameras,
                                          std::vector<DeviceCameraSample> camera_samples,
                                          const std::vector<xrt_imu_sample> &imu_samples,
                                          time_duration_ns camera_time_offset_ns,
                                          CalibrationResult &out_result)
{
	// Seed with our identity bias.
	ImuBias<ImuBiasJet> imu_bias = ImuBias<double>().seed();

	/*
	 * Applied before the grouping below, so a shift large enough to move a sample past a neighbour changes which
	 * keyframe it lands in, exactly as a real timing error would have.
	 */
	for (auto &camera_sample : camera_samples) {
		camera_sample.timestamp_ns += camera_time_offset_ns;
	}

	for (auto &camera_sample : camera_samples) {
		const auto camera_it =
		    std::find_if(cameras.begin(), cameras.end(), [&](const CameraDescription &camera) {
			    return camera.mosaic_idx == camera_sample.mosaic_idx &&
			           camera.camera_idx == camera_sample.camera_idx;
		    });
		if (camera_it == cameras.end()) {
			RS_ERROR(this, "Sample from camera %u/%u which is not part of the calibration.",
			         camera_sample.mosaic_idx, camera_sample.camera_idx);
			return false;
		}
	}

	/*
	 * Grouped by mosaic first so that one mosaic's synchronized exposure ends up in one keyframe even when
	 * another mosaic's samples interleave with it in time. The keyframes are put back in time order below.
	 */
	std::stable_sort(camera_samples.begin(), camera_samples.end(),
	                 [](const DeviceCameraSample &a, const DeviceCameraSample &b) {
		                 if (a.mosaic_idx != b.mosaic_idx) {
			                 return a.mosaic_idx < b.mosaic_idx;
		                 }
		                 return a.timestamp_ns < b.timestamp_ns;
	                 });

	auto keyframes = std::vector<DeviceKeyframe>();
	keyframes.reserve(camera_samples.size());

	{ // Construct non-preintegrated keyframes, grouping each mosaic's synchronized exposure together.
		DeviceKeyframe accumulated_keyframe = DeviceKeyframe{
		    .timestamp_ns = camera_samples.front().timestamp_ns,
		    .preintegration = PreintegratedImuSamples::Identity(),
		    .camera_samples = {},
		};
		size_t accumulated_mosaic_idx = camera_samples.front().mosaic_idx;

		for (auto &camera_sample : camera_samples) {
			/*
			 * Only cameras of the same mosaic share an exposure, so a sample from a different mosaic
			 * always starts a new keyframe no matter how close in time it lands.
			 */
			bool same_exposure = camera_sample.mosaic_idx == accumulated_mosaic_idx &&
			                     std::abs(camera_sample.timestamp_ns - accumulated_keyframe.timestamp_ns) <=
			                         kKeyframeGroupingToleranceNs;

			if (!same_exposure) {
				keyframes.push_back(std::move(accumulated_keyframe));

				accumulated_keyframe = DeviceKeyframe{
				    .timestamp_ns = camera_sample.timestamp_ns,
				    .preintegration = PreintegratedImuSamples::Identity(),
				    .camera_samples = {},
				};
				accumulated_mosaic_idx = camera_sample.mosaic_idx;
			}

			accumulated_keyframe.camera_samples.push_back({
			    .camera_sample = camera_sample,
			    .Tcv_cam_device_seed = Pose<double>(camera_sample.Tcv_cam_device_seed),
			});
		}

		// Add the last accumulated keyframe if it has any camera samples.
		if (!accumulated_keyframe.camera_samples.empty()) {
			keyframes.push_back(std::move(accumulated_keyframe));
		}
	}

	// Undo the per-mosaic grouping order, everything below walks the keyframes forwards in time.
	std::stable_sort(keyframes.begin(), keyframes.end(), [](const DeviceKeyframe &a, const DeviceKeyframe &b) {
		return a.timestamp_ns < b.timestamp_ns;
	});

	// Ensure that all keyframes have an IMU sample before and after them.
	{
		while (keyframes.front().timestamp_ns < imu_samples.front().timestamp_ns) {
			keyframes.erase(keyframes.begin());

			if (keyframes.size() == 0) {
				RS_ERROR(this, "No keyframe has an IMU sample before it.");
				return false;
			}
		}

		while (keyframes.back().timestamp_ns > imu_samples.back().timestamp_ns) {
			keyframes.pop_back();

			if (keyframes.size() == 0) {
				RS_ERROR(this, "No keyframe has an IMU sample after it.");
				return false;
			}
		}
	}

	if (keyframes.size() < 2) {
		RS_ERROR(this, "Not enough keyframes to optimize, need at least 2.");
		return false;
	}

	for (size_t i = 0; i < keyframes.size() - 1; i++) {
		auto &kf_start = keyframes[i];
		auto &kf_end = keyframes[i + 1];

		// find the first IMU samples that is at the start or after the start of the keyframe period
		auto imu_start_it = std::lower_bound(imu_samples.begin(), imu_samples.end(), kf_start.timestamp_ns,
		                                     [](const xrt_imu_sample &imu_sample, timepoint_ns timestamp) {
			                                     return imu_sample.timestamp_ns < timestamp;
		                                     });

		// Find the last IMU sample that is at the end or before the end of the keyframe period
		auto imu_end_it = std::upper_bound(imu_samples.begin(), imu_samples.end(), kf_end.timestamp_ns,
		                                   [](timepoint_ns timestamp, const xrt_imu_sample &imu_sample) {
			                                   return timestamp < imu_sample.timestamp_ns;
		                                   });


		if (imu_end_it - imu_start_it >= 2) {
			auto imu_samples = std::span<const xrt_imu_sample>(&*imu_start_it, &*imu_end_it);
			/*
			 * No +1 on `imu_end_it` seems like a bug, but that is very intentional since the iterator
			 * produces the item *after*.
			 */
			kf_start.preintegration = preintegrate(imu_samples, *imu_end_it, imu_bias,
			                                       kf_start.timestamp_ns, kf_end.timestamp_ns);
		} else {
			kf_start.preintegration = PreintegratedImuSamples::Identity();
		}
	}

#if 0
	for (auto &keyframe : keyframes) {
		// Print out delta per keyframe
		RS_DEBUG(this,
		         "Keyframe at %lu ns: Q_start_end = [%f, %f, %f, %f], delta_velocity = [%f, %f, %f], "
		         "delta_position = [%f, %f, %f]",
		         keyframe.timestamp_ns, keyframe.preintegration.Q_start_end.w(),
		         keyframe.preintegration.Q_start_end.x(), keyframe.preintegration.Q_start_end.y(),
		         keyframe.preintegration.Q_start_end.z(), keyframe.preintegration.delta_velocity.x(),
		         keyframe.preintegration.delta_velocity.y(), keyframe.preintegration.delta_velocity.z(),
		         keyframe.preintegration.delta_position.x(), keyframe.preintegration.delta_position.y(),
		         keyframe.preintegration.delta_position.z());
	}
#endif

	RigCalibrationProblem f(keyframes, cameras);
	ParameterBlocks &blocks = f.parameter_blocks;

	// write world gravity guess
	{
		auto gravity = WorldGravity<double>();
		Map<Vector<double, WorldGravity<double>::kNumParameters>> gravity_parameters(
		    blocks.world_gravity.data());
		gravity.pack(gravity_parameters);
	}

	// write IMU bias guess
	{
		auto bias = ImuBias<double>();
		Map<Vector<double, ImuBias<double>::kNumParameters>> bias_parameters(blocks.imu_bias.data());
		bias.pack(bias_parameters);
	}

	// write IMU extrinsics guess
	{
		auto extrinsics = ImuExtrinsics<double>(Quaterniond::Identity());
		Map<Vector<double, ImuExtrinsics<double>::kNumParameters>> extrinsics_parameters(
		    blocks.imu_extrinsics.data());
		extrinsics.pack(extrinsics_parameters);
	}

	/*
	 * Write the camera and keyframe guesses. The static camera keeps its zero pitch and roll, which is what the
	 * rest of the seeding is anchored on.
	 */
	{
		const SeedSummary seed = seedParameterBlocks(f);

		if (seed.two_dof_tilt.has_value()) {
			RS_DEBUG(this, "Seeded the 2dof camera at pitch %f deg, roll %f deg",
			         seed.two_dof_tilt->x() * (180.0 / M_PI), seed.two_dof_tilt->y() * (180.0 / M_PI));
		} else if (f.optimizing_static_camera) {
			RS_WARN(this, "Could not read the 2dof camera's tilt out of the IMU, seeding it level");
		}

		if (seed.num_unseeded_keyframes > 0) {
			RS_WARN(this, "%zu of %zu keyframes could not be seeded from a camera and start at a guess",
			        seed.num_unseeded_keyframes, keyframes.size());
		}
	}

	/*
	 * Borrowed by the problem below, so they have to outlive it. The camera losses are sized per observation and
	 * live on `f`, which does too.
	 */
	PoseManifold pose_manifold;
	PoseWithVelocityManifold pose_velocity_manifold;
	QuaternionManifold quaternion_manifold;

	ceres::Problem::Options problem_options{};
	problem_options.manifold_ownership = ceres::DO_NOT_TAKE_OWNERSHIP;
	problem_options.loss_function_ownership = ceres::DO_NOT_TAKE_OWNERSHIP;

	ceres::Problem problem(problem_options);
	f.build(problem, pose_manifold, pose_velocity_manifold, quaternion_manifold);

	ceres::Solver::Options options{};
	options.max_num_iterations = 100;
	/*
	 * The problem is block sparse: a keyframe only touches its own camera observations and the two IMU factors
	 * either side of it.
	 */
	options.linear_solver_type = ceres::LinearSolverType::SPARSE_NORMAL_CHOLESKY;
	options.sparse_linear_algebra_library_type = ceres::SparseLinearAlgebraLibraryType::EIGEN_SPARSE;
	options.minimizer_progress_to_stdout = false;
	options.logging_type = ceres::LoggingType::PER_MINIMIZER_ITERATION;
	options.num_threads = 16;
	// @todo Tune the tolerances to actual numbers.

	if (debug_get_bool_option_rig_calibration_check_jacobian()) {
		/*
		 * Compares each residual block's autodiff jacobian against finite differences, reporting the offending
		 * block by name. Replaces the hand-rolled whole-problem check the stacked parameter vector needed.
		 */
		options.check_gradients = true;
		/*
		 * Ceres defaults this to 1e-8, which these blocks cannot meet and which has nothing to do with whether
		 * the jacobian is right. A camera block holds a focal length of ~5.7e2 next to derivatives of ~1e-5,
		 * so a central difference over the whole block bottoms out around 1e-13 absolute, which against the
		 * small entries reads as a relative error of a few times 1e-8. A real mistake in a hand-written
		 * jacobian is wrong by far more than this.
		 */
		options.gradient_check_relative_precision = 1e-6;
	}

	ceres::Solver::Summary summary;
	ceres::Solve(options, &problem, &summary);

	switch (summary.termination_type) {
	case ceres::CONVERGENCE:
	case ceres::USER_SUCCESS: RS_DEBUG(this, "Room setup converged (%s)", summary.message.c_str()); break;
	case ceres::NO_CONVERGENCE:
		RS_DEBUG(this, "Room setup hit max iterations (%d)", options.max_num_iterations);
		break;
	default: RS_ERROR(this, "Room setup failed: %s", summary.message.c_str()); return false;
	}
	RS_DEBUG(this, "Initial cost: %f\tFinal cost: %f", summary.initial_cost, summary.final_cost);

	if (!summary.IsSolutionUsable()) {
		RS_ERROR(this, "Room setup optimization failed. Solution is not usable.");
		return false;
	}

	if (!blocks.allFinite()) {
		RS_ERROR(this, "Room setup optimization failed. Got non-finite parameters.");
		return false;
	}

	out_result = unpackResult(device_id, f, summary);

	/*
	 * Attributed per sensor, since a whitened residual is in standard deviations and the cost on its own
	 * cannot say which sensor the model failed to explain. The camera half is also what the offset sweep ranks on.
	 */
	out_result.imu_fitness = evaluateFactorFitness(problem, f.imu_residual_blocks);
	out_result.camera_fitness = evaluateFactorFitness(problem, f.camera_residual_blocks);

	return true;
}

DeviceSamples &
RigCalibration::getDeviceSamplesLocked(t_constellation_device_id_t device_id)
{
	return this->device_samples[device_id];
}

}; // namespace xrt::tracking::constellation::optimizer::rig_calibration
