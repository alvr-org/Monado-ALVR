// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Constellation tracker sensor fusion.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup tracking
 */

#include "constellation/t_constellation_tracker_internal.hpp"

#include "math/m_eigen_interop.hpp"

#include "internal_math.hpp"
#include "sensor_fusion.hpp"

#ifdef XRT_FEATURE_RERUN
#include "sensor_fusion_rerun.hpp"
#endif

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cassert>
#include <cmath>
#include <map>
#include <memory>
#include <set>
#include <span>
#include <thread>
#include <vector>


namespace {

DEBUG_GET_ONCE_LOG_OPTION(sensor_fusion_log, "T_CONSTELLATION_SENSOR_FUSION_LOG", U_LOGGING_WARN)
DEBUG_GET_ONCE_BOOL_OPTION(sensor_fusion_enable_rerun, "T_CONSTELLATION_SENSOR_FUSION_RERUN_ENABLE", false)
#ifdef XRT_FEATURE_RERUN
DEBUG_GET_ONCE_BOOL_OPTION(sensor_fusion_rerun_spawn, "T_CONSTELLATION_SENSOR_FUSION_RERUN_SPAWN", true)
#endif // XRT_FEATURE_RERUN

#define SF_TRACE(sensor_fusion, ...) U_LOG_IFL_T(sensor_fusion->log_level, __VA_ARGS__)
#define SF_DEBUG(sensor_fusion, ...) U_LOG_IFL_D(sensor_fusion->log_level, __VA_ARGS__)
#define SF_INFO(sensor_fusion, ...) U_LOG_IFL_I(sensor_fusion->log_level, __VA_ARGS__)
#define SF_WARN(sensor_fusion, ...) U_LOG_IFL_W(sensor_fusion->log_level, __VA_ARGS__)
#define SF_ERROR(sensor_fusion, ...) U_LOG_IFL_E(sensor_fusion->log_level, __VA_ARGS__)

using namespace xrt::tracking;
using namespace xrt::tracking::constellation::sensor_fusion;

using Eigen::MatrixXd;
using Eigen::Vector;
using Eigen::Vector3;
using Eigen::VectorXd;

//! Ceres hands its jacobian blocks out row-major, which is not Eigen's default for a dynamically sized matrix.
typedef Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> RowMajorMatrixXd;

bool
constructObservation(SensorFusion *fusion,
                     const constellation::CameraSample &sample,
                     const constellation::DeviceState &device,
                     DeviceObservation &out_observation)
{
	const auto led_model = fusion->device.led_model;

	xrt_pose Tcv_world_cam = sample.Txr_world_cam.value();
	math_pose_convert_from_opencv(&Tcv_world_cam, &Tcv_world_cam);

	out_observation = {
	    .camera_idx = sample.camera_index,
	    .Tcv_world_cam = Tcv_world_cam,
	    .Tcv_cam_device_seed = device.found_pose->Tcv_cam_device,
	    .points2d = {},
	    .points3d = {},
	    .num_points = 0,
	};

	for (uint32_t bi = 0; bi < sample.blob_count; bi++) {
		const auto &blob = sample.blobs[bi];

		if (blob.matched_device_id == device.device_id) {
			int32_t led_idx = findLedIndexById(led_model, blob.matched_device_led_id);

			// Ignore blob
			if (led_idx == -1) {
				continue;
			}

			if (kOptimizeUndistortedPoints) {
				out_observation.points2d[out_observation.num_points] = blob.center_undistorted;
			} else {
				out_observation.points2d[out_observation.num_points] = blob.center_distorted;
			}

			out_observation.points3d[out_observation.num_points] = led_model->leds[led_idx].position;
			out_observation.num_points += 1;
		}
	}

	if (out_observation.num_points == 0) {
		SF_WARN(fusion, "Observation from camera %u has zero points?", sample.camera_index);

		assert(!"Unreachable."); // We should only be given samples with poses?
		return false;
	}

	return true;
}

void
writeSampleToKeyframe(SensorFusion *fusion,
                      Keyframe &keyframe,
                      const DeviceObservation &observation,
                      uint32_t camera_index)
{
	/*
	 * A keyframe has room for every camera of a mosaic to observe the device exactly once, which is all
	 * that can legitimately land in one. Should never happen, but overrunning would scribble straight over
	 * the rest of the fusion state.
	 */
	assert(keyframe.num_observations < keyframe.observations.size());
	if (keyframe.num_observations >= keyframe.observations.size()) {
		SF_WARN(fusion, "Keyframe at %" PRIi64 " is full at %zu observations, dropping camera %u's.",
		        keyframe.timestamp_ns, keyframe.observations.size(), camera_index);

		assert(!"Unreachable.");

		return;
	}

	keyframe.observations[keyframe.num_observations++] = observation;
}

//! How many times a collect will retry after being lapped by the IMU thread before it gives up.
constexpr uint32_t kImuCollectAttempts = 4;

/*!
 * Velocity seeds above this are taken as a failed PnP rather than a fast device, and dropped in favour of zero. A
 * tracked controller tops out a long way under this even when thrown (please don't try to prove me wrong).
 */
constexpr double kMaxSeedVelocityMetersPerSecond = 10.0;

//! The maximum gap between keyframes in seconds
constexpr double kMaxTimeBetweenKeyframesSeconds = 5.0;

constexpr bool kOptimizeWorldGravity = false;

/*!
 * Whether to exit early if we hit an RMS floor. Disabled for now since we don't have our noise model tweaked very well.
 */
constexpr bool kExitOnRmsFloor = false;

/*!
 * The residual RMS, in standard deviations, at which the solve is called good enough and stopped.
 *
 * Every residual in the problem is already divided through by its own noise model, so one RMS across all of them is
 * dimensionless and means the same thing whatever the window happens to hold.
 *
 * The camera factors carry a Huber loss, so what the callback compares against is the robustified cost rather than
 * the raw sum of squares.
 *
 * @todo Measure the real floor and set this from it. Too low and the solve never exits early, falling back on
 *       `max_num_iterations`; too high and it stops while the fit is still visibly wrong.
 */
constexpr double kTargetResidualRmsSigma = 0.5;

enum class ImuCollectResult
{
	//! Samples were collected successfully.
	Ok,
	//! The IMU thread overwrote slots while we were reading them, so the collected samples are untrustworthy.
	Lapped,
	//! More samples fall in the range than the caller's staging buffer can hold.
	Overflow,
	//! No sample newer than `end_time_ns` has arrived yet, so the range is not closed off.
	NoSampleAfterEnd,
	//! The ring has already overwritten the start of the range, so the samples collected would leave a gap there.
	Truncated,
};

/*!
 * One attempt at collecting `device`'s buffered IMU samples falling in the range `(start_time_ns, end_time_ns]` into
 * `out_samples`, oldest first, along with the first sample newer than `end_time_ns`.
 *
 * @note Runs concurrently with `pushImuSample()`, so it can come back lapped; use `collectImuSamplesForRange()` rather
 *       than calling this directly.
 */
ImuCollectResult
tryCollectImuSamplesForRange(Device &device,
                             timepoint_ns start_time_ns,
                             timepoint_ns end_time_ns,
                             std::span<xrt_imu_sample> out_samples,
                             uint32_t &out_num_samples,
                             xrt_imu_sample &out_first_after)
{
	/*
	 * Acquire, pairing with the release store in pushImuSample(): every slot below this count has been fully
	 * written and is visible to us.
	 */
	const uint64_t write_count = device.imu_write_count.load(std::memory_order_acquire);

	/*
	 * Absolute index of the oldest sample the ring can still be trusted to hold. The next write, `write_count`, may
	 * already be overwriting absolute index `write_count - kImuBufferSize`, so a full ring holds one less than its
	 * size. Before the ring has filled for the first time this is 0, so we never read a slot that has not been
	 * written yet and the buffer needs no zero-initialization.
	 */
	const uint64_t oldest = write_count - std::min<uint64_t>(write_count, kImuBufferSize - 1);

	// The lowest absolute index we actually dereference, used to bound the lap check below.
	uint64_t lowest_read = write_count;

	const auto slot_at = [&](uint64_t abs) -> const RingImuSample & {
		lowest_read = std::min(lowest_read, abs);
		return device.imu_samples[abs % kImuBufferSize];
	};

	/*
	 * Samples ascend in time with their absolute index, so the window is a contiguous run of indices and its start
	 * can be found by bisection. This keeps us from touching thousands of slots we do not need, which matters less
	 * for speed than for the lap check: the fewer slots we read, the narrower the window in which the IMU thread
	 * can invalidate them.
	 */
	uint64_t lo = oldest;
	uint64_t hi = write_count;
	while (lo < hi) {
		const uint64_t mid = lo + ((hi - lo) / 2);

		/*
		 * Only the timestamp is needed to steer the bisection, and a probe that reads one word instead of the
		 * whole sample is one fewer chance for the writer to tear something underneath us.
		 */
		if (slot_at(mid).loadTimestamp() <= start_time_ns) {
			lo = mid + 1;
		} else {
			hi = mid;
		}
	}

	out_num_samples = 0;

	bool have_first_after = false;
	for (uint64_t abs = lo; abs < write_count; abs++) {
		const xrt_imu_sample sample = slot_at(abs).load();

		if (sample.timestamp_ns > end_time_ns) {
			/*
			 * Ascending timestamps mean the first sample past the end is the one preintegrate() wants to
			 * interpolate the tail of the range with.
			 */
			out_first_after = sample;
			have_first_after = true;
			break;
		}

		if (out_num_samples >= out_samples.size()) {
			out_num_samples = 0;
			return ImuCollectResult::Overflow;
		}

		out_samples[out_num_samples++] = sample;
	}

	/*
	 * The IMU thread writes slots without waiting for us, so anything read above may have been overwritten
	 * underneath us, possibly even a torn read, where some field are from a newer sample, since a sample is wider
	 * than a single store. Write `n` overwrites absolute index `n - kImuBufferSize`, and it starts once count `n`
	 * has been published, so a count of `lowest_read + kImuBufferSize` already means the oldest index we touched
	 * may have been overwritten. Then what we collected is garbage and has to be thrown away rather than fed to the
	 * optimizer.
	 */
	std::atomic_thread_fence(std::memory_order_acquire);
	const uint64_t write_count_after = device.imu_write_count.load(std::memory_order_relaxed);
	if (write_count_after >= lowest_read + kImuBufferSize) {
		return ImuCollectResult::Lapped;
	}

	/*
	 * The bisection only settles on `oldest` when even that sample is newer than the start of the range. Once the
	 * ring has wrapped, whatever came before it has been overwritten, so the head of the range is missing rather
	 * than empty, and integrating what is left would stretch the first retained sample over the whole gap.
	 */
	if (lo == oldest && oldest > 0) {
		out_num_samples = 0;
		return ImuCollectResult::Truncated;
	}

	if (!have_first_after) {
		return ImuCollectResult::NoSampleAfterEnd;
	}

	return ImuCollectResult::Ok;
}

/*!
 * Collects `device`'s buffered IMU samples falling in the range `(start_time_ns, end_time_ns]` into `out_samples`,
 * oldest first so that preintegration reads linearly forward, and returns the first sample newer than `end_time_ns` in
 * `out_first_after`.
 */
ImuCollectResult
collectImuSamplesForRange(Device &device,
                          timepoint_ns start_time_ns,
                          timepoint_ns end_time_ns,
                          std::span<xrt_imu_sample> out_samples,
                          uint32_t &out_num_samples,
                          xrt_imu_sample &out_first_after)
{
	/*
	 * Being lapped is caused by fresh samples landing mid-collect, which is transient: a retry reads a window that
	 * has just moved on slightly and succeeds. Only a stalled fusion thread would see this repeat.
	 */
	for (uint32_t attempt = 0; attempt < kImuCollectAttempts; attempt++) {
		const ImuCollectResult result = tryCollectImuSamplesForRange( //
		    device,                                                   //
		    start_time_ns,                                            //
		    end_time_ns,                                              //
		    out_samples,                                              //
		    out_num_samples,                                          //
		    out_first_after);

		if (result != ImuCollectResult::Lapped) {
			return result;
		}
	}

	return ImuCollectResult::Lapped;
}

struct StaticCameraObservationCostFunctor
{
	camera_model params;
	DeviceObservation observation;
	Eigen::Transform<float, 3, Eigen::Isometry> Tcv_cam_world;

	template <typename T>
	bool
	operator()(const T *const keyframe_parameters,       //
	           const T *const imu_extrinsics_parameters, //
	           T *residuals) const                       //
	{
		const Pose<T> predicted_T_world_device{
		    Map<const Vector<T, Pose<T>::kNumParameters>>(keyframe_parameters)};

		const ImuExtrinsics<T> predicted_imu_extrinsics{
		    Map<const Vector<T, ImuExtrinsics<T>::kNumParameters>>(imu_extrinsics_parameters)};

		const auto predicted_T_camera_imu = Tcv_cam_world.cast<T>() * predicted_T_world_device.toTransform();

		const Quaternion<T> Q_cam_model =
		    Quaternion<T>(predicted_T_camera_imu.rotation()) * predicted_imu_extrinsics.Q_imu_model;

		for (size_t i = 0; i < observation.num_points; i++) {
			const auto blob_position_2d = map_vec2(this->observation.points2d[i]).cast<T>();
			const auto T_model_led = map_vec3(this->observation.points3d[i]).cast<T>();

			Map<Vector2<T>> residual(residuals + (i * kNumLedResiduals));
			computeLedResidual<T>(params,                               //
			                      predicted_T_camera_imu.translation(), //
			                      Q_cam_model,                          //
			                      blob_position_2d,                     //
			                      T_model_led,                          //
			                      residual);                            //

			residual /= T(kBlobPositionSigmaPixels);
		}

		return true;
	}
};

typedef ceres::AutoDiffCostFunction<StaticCameraObservationCostFunctor,       //
                                    ceres::DYNAMIC,                           //
                                    PoseWithVelocity<double>::kNumParameters, //
                                    ImuExtrinsics<double>::kNumParameters>    //
    StaticCameraObservationCostFunction;

/*!
 * Evaluates @p inner's residuals where the states actually are, but its jacobians at the points those states are
 * anchored at.
 *
 * Only the states a prior has covered are anchored. Everything else linearizes where it is, because nothing frozen
 * disagrees with it.
 *
 * Evaluating twice is the price: the residuals have to come from where the states are, since that is the error the
 * solve is there to drive down, while the jacobians have to come from the anchor, since that is the whole point.
 * What it buys is every factor over an anchored state agreeing with the prior about which linearization they are
 * all talking about, at the cost of jacobians a little staler than the ones a re-linearizing factor would use.
 */
class FirstEstimateJacobianCostFunction final : public ceres::CostFunction
{
public:
	//! The most parameter blocks any factor in this problem touches.
	static constexpr size_t kMaxParameterBlocks = 8;

	//! The widest ambient parameter block in this problem, @ref PoseWithVelocity.
	static constexpr int kMaxAmbientSize = 10;

	//! What one parameter block of the wrapped factor is linearized at.
	struct Anchor
	{
		//! The point to linearize this block at, or null to linearize it wherever it currently is.
		const double *linearization_point{nullptr};

		/*!
		 * The manifold the block is added to the problem with, or null for a block that lives in a plain
		 * Euclidean space.
		 *
		 * Has to be that same manifold: the lift in @ref Evaluate only cancels against its own plus
		 * jacobian.
		 */
		const ceres::Manifold *manifold{nullptr};
	};

	/*!
	 * @param inner   The factor to evaluate. Ownership is taken.
	 * @param anchors One per parameter block of @p inner, in the same order.
	 */
	FirstEstimateJacobianCostFunction(ceres::CostFunction *inner, std::vector<Anchor> anchors)
	    : inner(inner), anchors(std::move(anchors))
	{
		assert(this->anchors.size() == this->inner->parameter_block_sizes().size());
		assert(this->anchors.size() <= kMaxParameterBlocks);

		this->set_num_residuals(this->inner->num_residuals());
		*this->mutable_parameter_block_sizes() = this->inner->parameter_block_sizes();
	}

	bool
	Evaluate(double const *const *parameters, double *residuals, double **jacobians) const override
	{
		if (jacobians == nullptr) {
			return this->inner->Evaluate(parameters, residuals, nullptr);
		}

		std::array<const double *, kMaxParameterBlocks> anchored{};
		for (size_t i = 0; i < this->anchors.size(); i++) {
			anchored[i] = this->anchors[i].linearization_point != nullptr
			                  ? this->anchors[i].linearization_point
			                  : parameters[i];
		}

		/*
		 * The jacobian pass goes first and its residuals land straight in the output, where the pass below
		 * overwrites them with the ones taken where the states actually are. Saves keeping a scratch buffer in
		 * a function Ceres calls from several threads at once.
		 */
		if (!this->inner->Evaluate(anchored.data(), residuals, jacobians)) {
			return false;
		}

		for (size_t i = 0; i < this->anchors.size(); i++) {
			const Anchor &anchor = this->anchors[i];

			if (anchor.linearization_point == nullptr || anchor.manifold == nullptr ||
			    jacobians[i] == nullptr) {
				continue;
			}

			/*
			 * Ceres turns an ambient jacobian into the tangent one the solver actually uses by multiplying
			 * it by the manifold's plus jacobian *at the point the block currently holds*. An ambient
			 * derivative taken at the anchor comes back expressed in the wrong tangent basis, off by
			 * however far the block has rotated since, which is not a small error and does not show up as
			 * anything except the tracking coming apart.
			 *
			 * Lifting through the anchor's own plus jacobian and back down through the current point's
			 * minus jacobian leaves Ceres holding exactly the tangent jacobian at the anchor, since the
			 * minus jacobian is the left inverse of the plus one at the same point.
			 */
			const int ambient_size = anchor.manifold->AmbientSize();
			const int tangent_size = anchor.manifold->TangentSize();

			SmallRowMajorMatrix plus(ambient_size, tangent_size);
			SmallRowMajorMatrix minus(tangent_size, ambient_size);

			if (!anchor.manifold->PlusJacobian(anchor.linearization_point, plus.data()) ||
			    !anchor.manifold->MinusJacobian(parameters[i], minus.data())) {
				return false;
			}

			Map<RowMajorMatrixXd> jacobian(jacobians[i], this->num_residuals(), ambient_size);
			jacobian = jacobian * (plus * minus);
		}

		return this->inner->Evaluate(parameters, residuals, nullptr);
	}

private:
	//! Big enough for any manifold jacobian in this problem, so the lift never leaves the stack.
	typedef Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor, kMaxAmbientSize, kMaxAmbientSize>
	    SmallRowMajorMatrix;

	std::unique_ptr<ceres::CostFunction> inner;

	std::vector<Anchor> anchors;
};

/*!
 * Names one parameter block in a way that stays valid even after the window has moved past it.
 *
 * The raw `double *` is only good for as long as the build that handed it out. The ring slot it points into is
 * reused by the next keyframe to take that slot, and whether the block is in the problem at all comes out of masks
 * that are recomputed from scratch every build. A marginalization prior outlives both of those and has to be able
 * to ask "is the block I was linearized around still in the solve, and where is it now", which a pointer cannot
 * answer and this can.
 */
struct ParameterBlockRef
{
	enum class Kind
	{
		WorldGravity,
		ImuExtrinsics,
		DeviceKeyframe,
		KeyframeImuBias,
	};

	Kind kind;

	//! Ring slot, as @ref SensorFusion::keyframeIdxLocked hands out. Only meaningful for the per-keyframe kinds.
	uint32_t ring_slot;
};

/*!
 * The marginalization prior carried from one solve to the next.
 */
struct MarginalizationPrior
{
	/*!
	 * Whether there is a prior to add to the problem at all.
	 *
	 * False both before one has ever been computed and after one has been discarded for no longer describing
	 * anything the solve still holds, see @ref SensorFusionProblem::dropPriorIfBlocksMissingLocked.
	 */
	bool valid{false};

	//! One retained block, and what it held at the moment the prior was linearized around it.
	struct Block
	{
		/*!
		 * Which block this is.
		 *
		 * Held as a reference rather than a pointer precisely because it has to outlive the build it came
		 * from. @ref ParameterBlocks::resolve turns it back into the pointer Ceres wants.
		 */
		ParameterBlockRef ref;

		/*!
		 * The ambient values the block held when it was marginalized.
		 *
		 * The factor is a statement about how far the block has moved from here, so this has to be a copy: the
		 * storage it came out of is reused by the next keyframe to take that slot.
		 */
		std::vector<double> linearization_point;
	};

	/*!
	 * The retained blocks the prior is a distribution over, in the order its information matrix is laid out in.
	 */
	std::vector<Block> blocks{};

	//! `J^*`, the square root of the marginalized Hessian, laid out over the tangent spaces of @ref blocks.
	MatrixXd j{};

	//! `r^*`, what the factor evaluates to with every block still sitting at its linearization point.
	VectorXd e0{};

	/*!
	 * The residual block the current build added for this prior, or null if the current problem does not hold it.
	 *
	 * Only good for as long as the build that handed it out, which is exactly the window in which the next
	 * marginalization asks whether this factor is among the residuals it is about to fold in.
	 */
	ceres::ResidualBlockId residual{nullptr};
};

/*!
 * The linear factor a marginalization leaves behind, `|J^* dx + r^*|^2`, evaluated against however far the blocks it
 * was linearized around have moved since.
 *
 * @ref marginalize() produces the `J^*` and the `r^*` this carries. The blocks have to be handed over in the order
 * their tangent spaces were laid out in `J^*`'s columns, which is the order @ref MarginalizationPrior::blocks holds
 * them in.
 */
class MarginalizationPriorCostFunction final : public ceres::CostFunction
{
public:
	//! One of the blocks the prior is a distribution over, frozen at what it held when it was linearized.
	struct Block
	{
		//! The ambient values of the block at the moment of marginalization.
		std::vector<double> linearization_point{};

		/*!
		 * The manifold the block sits on, or null for one that lives in a plain Euclidean space.
		 *
		 * Has to stay the manifold the block is added to the problem with: the lift in @ref Evaluate is only
		 * the inverse of that same manifold's plus jacobian.
		 */
		const ceres::Manifold *manifold{nullptr};
	};

	MarginalizationPriorCostFunction(MatrixXd j, VectorXd e0, std::vector<Block> blocks)
	    : j(std::move(j)), e0(std::move(e0)), blocks(std::move(blocks))
	{
		assert(this->j.rows() == this->e0.rows());
		// The states `J^*` was laid out over and the blocks named here have to be the same set.
		assert(totalTangentSize(this->blocks) == this->j.cols());

		this->set_num_residuals(static_cast<int>(this->e0.rows()));

		for (const Block &block : this->blocks) {
			assert(block.manifold == nullptr || block.manifold->AmbientSize() == ambientSize(block));

			this->mutable_parameter_block_sizes()->push_back(ambientSize(block));
		}
	}

	bool
	Evaluate(double const *const *parameters, double *residuals, double **jacobians) const override
	{
		// How far each block has moved from its linearization point, stacked in the column order of `J^*`.
		VectorXd delta_chi(this->j.cols());

		int tangent_offset = 0;
		for (size_t i = 0; i < this->blocks.size(); i++) {
			const Block &block = this->blocks[i];
			const int tangent_size = tangentSize(block);

			if (block.manifold != nullptr) {
				if (!block.manifold->Minus(parameters[i], block.linearization_point.data(),
				                           delta_chi.data() + tangent_offset)) {
					return false;
				}
			} else {
				delta_chi.segment(tangent_offset, tangent_size) =
				    Map<const VectorXd>(parameters[i], tangent_size) -
				    Map<const VectorXd>(block.linearization_point.data(), tangent_size);
			}

			tangent_offset += tangent_size;
		}

		Map<VectorXd>(residuals, this->num_residuals()) = this->e0 + (this->j * delta_chi);

		if (jacobians == nullptr) {
			return true;
		}

		tangent_offset = 0;
		for (size_t i = 0; i < this->blocks.size(); i++) {
			const Block &block = this->blocks[i];
			const int tangent_size = tangentSize(block);
			const int block_tangent_offset = tangent_offset;

			tangent_offset += tangent_size;

			if (jacobians[i] == nullptr) {
				continue;
			}

			Map<RowMajorMatrixXd> block_jacobian(jacobians[i], this->num_residuals(), ambientSize(block));

			if (block.manifold == nullptr) {
				block_jacobian = this->j.middleCols(block_tangent_offset, tangent_size);
				continue;
			}

			/*
			 * Ceres asks for the derivative against the ambient parameters and multiplies it by the
			 * manifold's plus jacobian itself. The minus jacobian is the left inverse of the plus one at
			 * the same point, so lifting with it here leaves the solver holding exactly the tangent-space
			 * `J^*` this was linearized as, wherever the block has drifted to since.
			 *
			 * Holding it fixed is the point rather than a shortcut: the exact derivative of `Minus`
			 * against a block that has moved off its linearization point picks up a right-jacobian
			 * term, first order in how far it moved, and a prior that re-linearized itself would stop
			 * being the factor the marginalization solved for. OKVIS and VINS both freeze it the same
			 * way.
			 */
			RowMajorMatrixXd minus_jacobian(tangent_size, ambientSize(block));
			if (!block.manifold->MinusJacobian(parameters[i], minus_jacobian.data())) {
				return false;
			}

			block_jacobian = this->j.middleCols(block_tangent_offset, tangent_size) * minus_jacobian;
		}

		return true;
	}

private:
	static int
	ambientSize(const Block &block)
	{
		return static_cast<int>(block.linearization_point.size());
	}

	static int
	tangentSize(const Block &block)
	{
		return block.manifold != nullptr ? block.manifold->TangentSize() : ambientSize(block);
	}

	static int
	totalTangentSize(const std::vector<Block> &blocks)
	{
		int total = 0;
		for (const Block &block : blocks) {
			total += tangentSize(block);
		}

		return total;
	}

	//! `J^*`, the square root of the marginalized Hessian, laid out over the tangent spaces of @ref blocks.
	MatrixXd j;

	//! `r^*`, what the factor evaluates to with every block still sitting at its linearization point.
	VectorXd e0;

	std::vector<Block> blocks;
};

struct ParameterBlocks
{
	// The world has a single magnitude of gravity (static parameter if kOptimizeWorldGravity)
	SeedableVector<double, WorldGravity<double>::kNumParameters> world_gravity;

	//! The IMU extrinsics for the device.
	SeedableVector<double, ImuExtrinsics<double>::kNumParameters> imu_extrinsics;

	std::array<         // Each keyframe an IMU bias
	    SeedableVector< // Each IMU bias has some parameters
	        double,
	        ImuBias<double>::kNumParameters>,
	    kSlidingWindowSize>
	    keyframe_imu_biases;

	std::array<         // Each keyframe has a pose with velocity
	    SeedableVector< // Each pose with velocity has some parameters
	        double,
	        PoseWithVelocity<double>::kNumParameters>,
	    kSlidingWindowSize>
	    keyframes;

	//! All the residuals whose oldest keyframe is this one.
	std::array< // Each keyframe has some list of marginalization parameters attached to it
	    std::vector<ceres::ResidualBlockId>,
	    kSlidingWindowSize>
	    keyframe_marginalization_residuals{};

	/*!
	 * All the parameters that are part of a specific keyframe. These are the parameters that are going to be
	 * marginalized when this keyframe gets dropped.
	 */
	std::array<std::vector<double *>, kSlidingWindowSize> per_keyframe_marginalization_parameters{};

	/*!
	 * The build that filled each ring slot's two marginalization lists above, checked against
	 * @ref SensorFusionProblem::build_generation before an eviction reads them.
	 *
	 * Zero once a slot's lists have been folded into a prior, so that a slot can never be folded twice from the
	 * same build. Folding it twice would count its residuals into the prior a second time.
	 */
	std::array<uint64_t, kSlidingWindowSize> marginalization_list_generation{};

	/*!
	 * Reverse lookup from the pointer Ceres knows a block by to what that block actually is, rebuilt as the
	 * blocks are added each build.
	 *
	 * Ceres hands back raw `double *` when asked what a residual touches, and marginalization has to turn those
	 * back into something that still means anything a build later.
	 */
	std::map<const double *, ParameterBlockRef> block_refs{};

	ParameterBlocks() = default;

	/*!
	 * The live storage for the block @p ref names, or null when that block is not currently seeded and so has
	 * nothing meaningful in it.
	 */
	double *
	resolve(const ParameterBlockRef &ref)
	{
		switch (ref.kind) {
		case ParameterBlockRef::Kind::WorldGravity:
			return this->world_gravity.seeded ? this->world_gravity.vec.data() : nullptr;

		case ParameterBlockRef::Kind::ImuExtrinsics: {
			auto &block = this->imu_extrinsics;
			return block.seeded ? block.vec.data() : nullptr;
		}

		case ParameterBlockRef::Kind::DeviceKeyframe: {
			auto &block = this->keyframes[ref.ring_slot];
			return block.seeded ? block.vec.data() : nullptr;
		}

		case ParameterBlockRef::Kind::KeyframeImuBias: {
			auto &block = this->keyframe_imu_biases[ref.ring_slot];
			return block.seeded ? block.vec.data() : nullptr;
		}
		}

		assert(!"Unreachable");
		return nullptr;
	}

	/*!
	 * Freezes the block @p ref names at its current value as the point everything linearizes it at, and hands
	 * that point back.
	 *
	 * Null for a block that is not currently seeded, and for the states that outlive the window.
	 */
	const double *
	anchor(const ParameterBlockRef &ref)
	{
		switch (ref.kind) {
		/*
		 * The states that outlive the window are left to re-linearize where they are, and to be
		 * re-snapshotted as the prior's linearization point at every marginalization.
		 *
		 * Anchoring is only a good trade for a state that stays near its anchor. These are static parameters
		 * seeded once and solved for across the whole session, so a point picked at the first marginalization
		 * is one they spend the rest of the run moving away from, while every factor that touches them would
		 * go on linearizing about it.
		 *
		 * In `Optimization-based VINS: Consistency, Marginalization, and FEJ`
		 * https://pgeneva.com/downloads/papers/Chen2023IROS.pdf
		 * Chen et al. finds that biases and auxiliary variables are safe to re-linearize, "This analytical
		 * nullspace also shows that only orientation, position, velocity, and features affect the
		 * dimensionality, and therefore, there is no need to FEJ the biases or auxiliary variables".
		 */
		case ParameterBlockRef::Kind::WorldGravity:
		case ParameterBlockRef::Kind::ImuExtrinsics:
		case ParameterBlockRef::Kind::KeyframeImuBias: return nullptr;

		case ParameterBlockRef::Kind::DeviceKeyframe: return anchorBlock(this->keyframes[ref.ring_slot]);
		}

		assert(!"Unreachable");
		return nullptr;
	}

private:
	template <typename Block>
	static const double *
	anchorBlock(Block &block)
	{
		if (!block.seeded) {
			return nullptr;
		}

		block.anchor();

		return block.anchorData();
	}
};

/*!
 * The observation-derived world pose of the device at `keyframe`, or `false` if the keyframe holds no
 * observation of it.
 *
 * Where several cameras of the mosaic saw the device, the one with the most blobs wins: its PnP seed is the
 * best-conditioned of the set, and averaging across cameras would need a rotation mean this has no use for elsewhere.
 */
bool
observedDevicePoseLocked(const Keyframe &keyframe, xrt_pose &out_Tcv_world_device)
{
	const DeviceObservation *best = nullptr;

	for (uint32_t i = 0; i < keyframe.num_observations; i++) {
		const DeviceObservation &observation = keyframe.observations[i];

		if (best == nullptr || observation.num_points > best->num_points) {
			best = &observation;
		}
	}

	if (best == nullptr) {
		return false;
	}

	math_pose_transform(&best->Tcv_world_cam, &best->Tcv_cam_device_seed, &out_Tcv_world_device);

	return true;
}

/*!
 * Seeds the world-frame velocity of the device at keyframe age `keyframe_age`, whose own pose seed is
 * `Tcv_world_device`, by differencing the observation-derived positions of the nearest keyframe on either side that
 * also saw it.
 *
 * Zero is a poor seed: it disagrees with what the IMU preintegrated by the whole of the device's actual motion, and
 * the preintegration factor has to spend its first steps absorbing that before it can say anything about bias. A
 * difference of two PnP seeds is noisy, but it is centred on the right answer, which is all a seed has to be.
 */
Vector3d
seedVelocityLocked(const SensorFusion &fusion, uint32_t keyframe_age, const xrt_pose &Tcv_world_device)
{
	/*
	 * Age counts backwards from the newest keyframe, so the newer of any pair is the one with the smaller age.
	 * Both endpoints start collapsed onto the keyframe being seeded, which is what makes a one-sided difference
	 * fall out of the same arithmetic as a central one.
	 */
	const Keyframe &keyframe = fusion.keyframeLocked(keyframe_age);

	xrt_pose Tcv_world_device_newer = Tcv_world_device;
	xrt_pose Tcv_world_device_older = Tcv_world_device;
	timepoint_ns newer_timestamp_ns = keyframe.timestamp_ns;
	timepoint_ns older_timestamp_ns = keyframe.timestamp_ns;
	bool have_neighbour = false;

	for (uint32_t age = keyframe_age; age-- > 0;) {
		const Keyframe &newer = fusion.keyframeLocked(age);

		if (!observedDevicePoseLocked(newer, Tcv_world_device_newer)) {
			continue;
		}

		newer_timestamp_ns = newer.timestamp_ns;
		have_neighbour = true;
		break;
	}

	for (uint32_t age = keyframe_age + 1; age < fusion.num_keyframes; age++) {
		const Keyframe &older = fusion.keyframeLocked(age);

		if (!observedDevicePoseLocked(older, Tcv_world_device_older)) {
			continue;
		}

		older_timestamp_ns = older.timestamp_ns;
		have_neighbour = true;
		break;
	}

	// The only keyframe in the window that sees this device, so there is nothing to difference against.
	if (!have_neighbour) {
		return Vector3d::Zero();
	}

	const time_duration_ns dt_ns = newer_timestamp_ns - older_timestamp_ns;

	/*
	 * Keyframes closer together than the tolerance that groups an exposure are the same instant as far as this is
	 * concerned, and dividing by that interval turns blob noise into an arbitrarily large velocity.
	 */
	if (dt_ns < kKeyframeGroupingToleranceNs) {
		return Vector3d::Zero();
	}

	Vector3d velocity = (map_vec3(Tcv_world_device_newer.position).cast<double>() -
	                     map_vec3(Tcv_world_device_older.position).cast<double>()) /
	                    (static_cast<double>(dt_ns) * 1e-9);

	/*
	 * One bad PnP seed is enough to produce a velocity that leaves the solve worse off than starting it at rest.
	 * We really don't want devices flying off.
	 */
	if (velocity.norm() > kMaxSeedVelocityMetersPerSecond) {
		return Vector3d::Zero();
	}

	return velocity;
}

/*!
 * The RMS of the problem's residuals in standard deviations, recovered from a Ceres cost.
 *
 * Ceres reports cost as one half of the sum of the squared residuals, so the factor of two undoes that before the
 * mean. Dividing by the residual count is what makes the number comparable between solves: the window holds a
 * different set of keyframes and blobs every tick, and a raw cost moves with all of them.
 */
double
residualRmsSigma(double cost, int num_residuals)
{
	if (num_residuals <= 0) {
		return 0.0;
	}

	return std::sqrt((2.0 * cost) / static_cast<double>(num_residuals));
}

/*!
 * Stops the solve the moment its residuals reach @ref kTargetResidualRmsSigma, rather than letting it spend the rest
 * of the iteration budget polishing a fit that is already inside the noise.
 *
 * The threshold is held as a cost so that the check is a comparison rather than a square root per iteration. Nothing
 * here reads parameter state, which is what lets the solve run without `update_state_every_iteration`.
 */
class ResidualRmsTargetCallback final : public ceres::IterationCallback
{
public:
	ResidualRmsTargetCallback(int num_residuals, double target_rms_sigma)
	    /*
	     * The inverse of `residualRmsSigma`. A problem with no residuals gets a target no cost can meet, so an
	     * empty solve runs its course instead of being reported as an instant success.
	     */
	    : target_cost(num_residuals > 0
	                      ? 0.5 * static_cast<double>(num_residuals) * target_rms_sigma * target_rms_sigma
	                      : -1.0)
	{}

	ceres::CallbackReturnType
	operator()(const ceres::IterationSummary &summary) override
	{
		/*
		 * We always want to compute one full iteration so that it takes into account all cameras, not just the
		 * seed that is "good enough".
		 */
		if (summary.iteration < 2) {
			return ceres::SOLVER_CONTINUE;
		}

		/*
		 * Ceres carries the last accepted cost through an unsuccessful step, so this never mistakes a
		 * rejected trust region step for progress.
		 */
		if (summary.cost <= this->target_cost) {
			return ceres::SOLVER_TERMINATE_SUCCESSFULLY;
		}

		return ceres::SOLVER_CONTINUE;
	}

private:
	double target_cost;
};

extern "C" void *
thread_runner(void *ptr)
{
	SensorFusion *fusion = static_cast<SensorFusion *>(ptr);

	std::vector<constellation::CameraSample> pending;
	// Reserve the maximum amount of samples up front, so we don't keep allocating over time
	pending.reserve(kMaxQueuedCameraSamples);

	os_thread_helper_lock(&fusion->thread);
	while (os_thread_helper_is_running_locked(&fusion->thread)) {
		timepoint_ns now_ns = os_monotonic_get_ns();

		auto &queue = fusion->sample_queue;
		auto is_ready = [](timepoint_ns now_ns, timepoint_ns latest_imu_ts_ns) {
			// Only samples the IMU has passed can have the interval ending at them integrated.
			return [now_ns, latest_imu_ts_ns](const auto &sample) {
				// If the imu is 100ms old, just accept anything so that the fusion thread doesn't spill
				// over.
				if (now_ns - latest_imu_ts_ns > U_TIME_1MS_IN_NS * 100L) {
					return true;
				}

				return sample.timestamp_ns < latest_imu_ts_ns;
			};
		};

		if (std::none_of(queue.begin(), queue.end(),
		                 is_ready(now_ns, fusion->device.latest_imu_ts_ns.load()))) {
			os_thread_helper_wait_locked(&fusion->thread);

			// Shutdown wakes us the same way work does, so just exit out if we're no longer supposed to run
			if (!os_thread_helper_is_running_locked(&fusion->thread)) {
				break;
			}
		}

		auto ready_end = std::stable_partition(queue.begin(), queue.end(),
		                                       is_ready(now_ns, fusion->device.latest_imu_ts_ns.load()));

		pending.clear();
		pending.insert(pending.end(), queue.begin(), ready_end);
		queue.erase(queue.begin(), ready_end);

		// Fold in timestamp order, so a late arrival doesn't miss its keyframe.
		std::stable_sort(pending.begin(), pending.end(),
		                 [](const auto &a, const auto &b) { return a.timestamp_ns < b.timestamp_ns; });

		/*
		 * Camera samples are the only thing a solve is worth running for. A pass with none of them was woken
		 * by pushImuSample() to unpark a preintegration, and only needs to get as far as integrating it.
		 */
		const bool have_new_input = !pending.empty();

		for (const auto &sample : pending) {
			fusion->processCameraSampleLocked(sample);
		}

		if (have_new_input) {
			fusion->dirty = true;
		}

		if (fusion->tickLocked(have_new_input)) {
			fusion->runFusionLocked();
		}

		// Make sure this is set if we still have more in the queue
		if (!queue.empty()) {
			fusion->waiting_on_imu.store(true, std::memory_order_relaxed);
		}
	}
	os_thread_helper_unlock(&fusion->thread);

	return nullptr;
}

//! Does a single IMU sample integration and updates the passed state parameters
static void
integrateOnce(timepoint_ns &pose_ns,
              const xrt_imu_sample &sample,
              Vector3d &T_world_device,
              Quaterniond &Q_world_device,
              Vector3d &world_velocity,
              const Vector3d &gravity_vec,
              const Vector3d &accel_bias,
              const Vector3d &gyro_bias,
              const Vector3d &accel_scale)
{
	double dt = time_ns_to_s(sample.timestamp_ns - pose_ns);
	// @todo: Handle this properly
	// assert(dt >= 0);
	dt = MAX(0.0, dt);

	Vector3d cur_accel = (map_vec3_f64(sample.accel_m_s2) - accel_bias).cwiseProduct(accel_scale);
	auto cur_gyro = map_vec3_f64(sample.gyro_rad_secs) - gyro_bias;

	auto world_accel = Q_world_device * cur_accel;

	// Cancel out the force of gravity that is being applied
	world_accel += gravity_vec;

	T_world_device += (world_velocity * dt) + (0.5 * world_accel * dt * dt);
	world_velocity += world_accel * dt;

	Q_world_device *= quat_exp_so3(cur_gyro * dt);

	pose_ns = sample.timestamp_ns;
}

static void
integrateToTimestampHistory(const double world_gravity_mag,
                            const timepoint_ns solve_time_ns,
                            const xrt_pose &Tcv_world_device_solve,
                            const xrt_vec3_f64 &solve_world_velocity,
                            const xrt_vec3_f64 &solve_accel_bias,
                            const xrt_vec3_f64 &solve_gyro_bias,
                            const xrt_vec3_f64 &solve_accel_scale,
                            const std::span<const xrt_imu_sample> imu_samples,
                            m_relation_history *relation_history)
{
	assert(imu_samples.size() > 0);

	const Vector3d gravity_vec = {0, world_gravity_mag, 0};

	timepoint_ns pose_ns = solve_time_ns;
	Vector3d T_world_device = map_vec3(Tcv_world_device_solve.position).cast<double>();
	Quaterniond Q_world_device = map_quat(Tcv_world_device_solve.orientation).cast<double>();

	const auto accel_bias = map_vec3_f64(solve_accel_bias);
	const auto gyro_bias = map_vec3_f64(solve_gyro_bias);
	const auto accel_scale = map_vec3_f64(solve_accel_scale);

	Vector3d world_velocity = map_vec3_f64(solve_world_velocity);
	for (const auto &sample : imu_samples) {
		integrateOnce(pose_ns,        //
		              sample,         //
		              T_world_device, //
		              Q_world_device, //
		              world_velocity, //
		              gravity_vec,    //
		              accel_bias,     //
		              gyro_bias,      //
		              accel_scale);   //

		// Normalize the orientation
		Q_world_device.normalize();

		xrt_space_relation relation = {
		    .relation_flags = static_cast<xrt_space_relation_flags>(
		        XRT_SPACE_RELATION_ORIENTATION_VALID_BIT | XRT_SPACE_RELATION_POSITION_VALID_BIT |
		        XRT_SPACE_RELATION_LINEAR_VELOCITY_VALID_BIT | XRT_SPACE_RELATION_ANGULAR_VELOCITY_VALID_BIT |
		        XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT | XRT_SPACE_RELATION_POSITION_TRACKED_BIT),
		    .pose = XRT_POSE_IDENTITY,
		    .linear_velocity = XRT_VEC3_ZERO,
		    .angular_velocity = XRT_VEC3_ZERO,
		};
		map_vec3(relation.pose.position) = T_world_device.cast<float>();
		map_quat(relation.pose.orientation) = Q_world_device.cast<float>();
		map_vec3(relation.linear_velocity) = world_velocity.cast<float>();
		map_vec3(relation.angular_velocity) =
		    (Q_world_device * (map_vec3_f64(sample.gyro_rad_secs) - gyro_bias)).cast<float>();

		m_relation_history_push(relation_history, &relation, pose_ns);
	}
}

static void
integrateToTimestamp(const double world_gravity_mag,
                     const timepoint_ns solve_time_ns,
                     const xrt_pose &Tcv_world_device_solve,
                     const xrt_vec3_f64 &solve_world_velocity,
                     const xrt_vec3_f64 &solve_accel_bias,
                     const xrt_vec3_f64 &solve_gyro_bias,
                     const xrt_vec3_f64 &solve_accel_scale,
                     const std::span<const xrt_imu_sample> imu_samples,
                     const std::optional<xrt_imu_sample> &first_imu_sample_after,
                     xrt_space_relation &relation)
{
	assert(imu_samples.size() > 0);

	Vector3d gravity_vec = {0, world_gravity_mag, 0};

	timepoint_ns pose_ns = solve_time_ns;
	Vector3d T_world_device = map_vec3(Tcv_world_device_solve.position).cast<double>();
	Quaterniond Q_world_device = map_quat(Tcv_world_device_solve.orientation).cast<double>();

	const auto accel_bias = map_vec3_f64(solve_accel_bias);
	const auto gyro_bias = map_vec3_f64(solve_gyro_bias);
	const auto accel_scale = map_vec3_f64(solve_accel_scale);

	xrt_imu_sample end_sample = first_imu_sample_after.value_or(imu_samples.back());

	Vector3d world_velocity = map_vec3_f64(solve_world_velocity);
	for (const auto &sample : imu_samples) {
		integrateOnce(pose_ns,        //
		              sample,         //
		              T_world_device, //
		              Q_world_device, //
		              world_velocity, //
		              gravity_vec,    //
		              accel_bias,     //
		              gyro_bias,      //
		              accel_scale);   //
	}

	// Normalize the orientation
	Q_world_device.normalize();

	relation = {
	    .relation_flags = static_cast<xrt_space_relation_flags>(
	        XRT_SPACE_RELATION_ORIENTATION_VALID_BIT | XRT_SPACE_RELATION_POSITION_VALID_BIT |
	        XRT_SPACE_RELATION_LINEAR_VELOCITY_VALID_BIT | XRT_SPACE_RELATION_ANGULAR_VELOCITY_VALID_BIT |
	        XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT | XRT_SPACE_RELATION_POSITION_TRACKED_BIT),
	    .pose = XRT_POSE_IDENTITY,
	    .linear_velocity = XRT_VEC3_ZERO,
	    .angular_velocity = XRT_VEC3_ZERO,
	};
	map_vec3(relation.pose.position) = T_world_device.cast<float>();
	map_quat(relation.pose.orientation) = Q_world_device.cast<float>();
	map_vec3(relation.linear_velocity) = world_velocity.cast<float>();
	map_vec3(relation.angular_velocity) =
	    (Q_world_device * (map_vec3_f64(end_sample.gyro_rad_secs) - gyro_bias)).cast<float>();
}

}; // namespace

namespace xrt::tracking::constellation::optimizer::sensor_fusion {

/*
 *
 * SensorFusionProblem
 *
 */

struct SensorFusionProblem
{
	ParameterBlocks parameter_blocks;

	/*!
	 * The camera losses, keyed by the residual count they were sized for. Borrowed by the problem rather than
	 * owned by it, so they have to outlive it.
	 */
	std::map<int, ceres::HuberLoss> camera_losses;

	ceres::Problem problem;

	QuaternionManifold quaternion_manifold;
	PoseWithVelocityManifold pose_with_velocity_manifold;

	/*!
	 * The ring slot each keyframe age mapped to when the snapshot was taken, so that the solved blocks can be
	 * found again after the lock has been dropped. Only meaningful for ages below `snapshot.keyframes.size()`.
	 */
	std::array<uint32_t, kSlidingWindowSize> snapshot_block_idx{};

	//! The marginalization prior for the device.
	MarginalizationPrior prior{};

	/*!
	 * Counts builds, so the marginalization lists can be tied to the build that filled them.
	 * Zero before the first.
	 */
	uint64_t build_generation{0};

	/*!
	 * Set once an eviction has marginalized against the current problem, and cleared by the next build.
	 *
	 * An eviction folds part of this problem into a prior that the problem itself does not hold, and consumes that
	 * slot's lists. A second eviction against the same problem would build its prior without the first one, losing
	 * everything it carried, and once the ring wraps it would fold a slot's residuals a second time.
	 * @ref SensorFusion::pushKeyframeLocked rebuilds before evicting whenever this is set.
	 */
	bool marginalization_lists_stale{false};

	SensorFusionProblem()
	    : parameter_blocks(),           //
	      quaternion_manifold(),        //
	      pose_with_velocity_manifold() //
	{}

	/*!
	 * The ambient size a block of this kind is added with.
	 *
	 * A prior outlives the pointer it was built from, so recovering the size a block later has to go through
	 * what the block *is*. @ref addParameterBlockLocked checks the parameter passes against this.
	 */
	static int
	ambientSizeForKind(ParameterBlockRef::Kind kind)
	{
		switch (kind) {
		case ParameterBlockRef::Kind::WorldGravity: return WorldGravity<double>::kNumParameters;
		case ParameterBlockRef::Kind::ImuExtrinsics: return ImuExtrinsics<double>::kNumParameters;
		case ParameterBlockRef::Kind::DeviceKeyframe: return PoseWithVelocity<double>::kNumParameters;
		case ParameterBlockRef::Kind::KeyframeImuBias: return ImuBias<double>::kNumParameters;
		}

		assert(!"Unreachable");
		return 0;
	}

	//! The manifold a block of this kind is added with, or null for one that lives in a plain Euclidean space.
	ceres::Manifold *
	manifoldForKind(ParameterBlockRef::Kind kind)
	{
		switch (kind) {
		case ParameterBlockRef::Kind::WorldGravity: return nullptr;
		case ParameterBlockRef::Kind::ImuExtrinsics: return &this->quaternion_manifold;
		case ParameterBlockRef::Kind::DeviceKeyframe: return &this->pose_with_velocity_manifold;
		case ParameterBlockRef::Kind::KeyframeImuBias: return nullptr;
		}

		assert(!"Unreachable");
		return nullptr;
	}

	/*!
	 * Wraps @p cost so its jacobians are taken at the anchored linearization points of @p anchors.
	 *
	 * The blocks have to be given in the order @p cost takes them, which is the order they are handed to
	 * `AddResidualBlock` below.
	 */
	static ceres::CostFunction *
	anchorJacobians(ceres::CostFunction *cost, std::vector<FirstEstimateJacobianCostFunction::Anchor> anchors)
	{
		return new FirstEstimateJacobianCostFunction(cost, std::move(anchors));
	}

	/*!
	 * Adds a parameter block and records what it is, so that marginalization can name it again once the pointer
	 * on its own is no longer valid.
	 *
	 * Every block in the problem goes in through here. A block added behind its back would be one a residual can
	 * reach and @ref ParameterBlocks::block_refs cannot explain, which is exactly the case marginalization has no
	 * way to handle, anyway.
	 */
	void
	addParameterBlockLocked(double *values, int size, ceres::Manifold *manifold, const ParameterBlockRef &ref)
	{
		// A prior rebuilds its factor from the kind alone, so the two have to agree about every block.
		assert(size == ambientSizeForKind(ref.kind));
		assert(manifold == this->manifoldForKind(ref.kind));

		if (manifold != nullptr) {
			this->problem.AddParameterBlock(values, size, manifold);
		} else {
			this->problem.AddParameterBlock(values, size);
		}

		this->parameter_blocks.block_refs.insert_or_assign(values, ref);
	}

	//! Throws the prior away, for the cases where what it was linearized around has stopped being true.
	void
	invalidatePriorLocked()
	{
		auto &prior = this->prior;

		prior.valid = false;
		prior.blocks.clear();
		prior.residual = nullptr;
	}

	/*!
	 * Whether the build that is computing @p device_keyframe_mask is going to instantiate the block @p ref
	 * names.
	 *
	 * Mirrors the conditions the parameter passes below actually add blocks under, so the two cannot disagree
	 * about which states exist.
	 */
	static bool
	blockWillBeInstantiatedLocked(const SensorFusion &fusion,
	                              uint32_t device_keyframe_mask,
	                              const ParameterBlockRef &ref)
	{
		switch (ref.kind) {
		case ParameterBlockRef::Kind::WorldGravity:
			// Added unconditionally, every build.
			return true;

		case ParameterBlockRef::Kind::ImuExtrinsics:
			// Added for any device holding at least one state in the window.
			return device_keyframe_mask != 0;

		case ParameterBlockRef::Kind::DeviceKeyframe:
		case ParameterBlockRef::Kind::KeyframeImuBias: {
			uint32_t age = 0;
			if (!fusion.keyframeAgeForIdxLocked(ref.ring_slot, age)) {
				// The keyframe that held this slot has fallen out of the window entirely.
				return false;
			}

			return (device_keyframe_mask & (1u << age)) != 0;
		}
		}

		assert(!"Unreachable");
		return false;
	}

	/*!
	 * Discards the prior unless this build is going to instantiate every block it was linearized around.
	 */
	void
	dropPriorIfBlocksMissingLocked(const SensorFusion &fusion, uint32_t device_keyframe_mask)
	{
		auto &prior = this->prior;

		if (!prior.valid) {
			return;
		}

		for (const MarginalizationPrior::Block &block : prior.blocks) {
			const ParameterBlockRef &ref = block.ref;

			if (blockWillBeInstantiatedLocked(fusion, device_keyframe_mask, ref)) {
				continue;
			}

			SF_DEBUG((&fusion),
			         "Marginalization prior names a block (kind %d, slot %u) this build does "
			         "not instantiate, dropping the prior.",
			         static_cast<int>(ref.kind), ref.ring_slot);

			this->invalidatePriorLocked();
			break;
		}
	}

	/*!
	 * Whether the device's prior is one this build is going to add, and it is a distribution over the block that
	 * @p kind and @p ring_slot name.
	 *
	 * Only meaningful once the prior pass has run, which is why that pass goes first.
	 */
	bool
	priorNamesBlockLocked(ParameterBlockRef::Kind kind, uint32_t ring_slot) const
	{
		if (!this->prior.valid) {
			return false;
		}

		for (const MarginalizationPrior::Block &block : this->prior.blocks) {
			if (block.ref.kind == kind && block.ref.ring_slot == ring_slot) {
				return true;
			}
		}

		return false;
	}

	/*!
	 * Releases the anchor of every keyframe state the prior does not cover.
	 */
	void
	releaseAnchorsOutsidePriorLocked()
	{
		for (uint32_t slot = 0; slot < kSlidingWindowSize; slot++) {
			if (this->priorNamesBlockLocked(ParameterBlockRef::Kind::DeviceKeyframe, slot)) {
				continue;
			}

			this->parameter_blocks.keyframes[slot].releaseAnchor();
		}
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
	 * Rebuilds the problem from the window. Call with `fusion`'s lock held.
	 *
	 * @return `true` when the problem has something to solve. A window that observes no tracked device produces
	 *         nothing but the constant gravity block, and handing that to Ceres is pure overhead.
	 */
	bool
	buildLocked(SensorFusion &fusion)
	{
		ceres::Problem::Options options{};
		options.manifold_ownership = ceres::DO_NOT_TAKE_OWNERSHIP;
		options.loss_function_ownership = ceres::DO_NOT_TAKE_OWNERSHIP;
		options.enable_fast_removal = true;

		this->problem = ceres::Problem(options);

		// A fresh problem, so the next eviction has a whole build to marginalize against again.
		this->build_generation += 1;
		this->marginalization_lists_stale = false;

		ParameterBlocks &blocks = this->parameter_blocks;

		/*
		 * Parameters
		 */

		// Clear all the old parameter blocks
		for (auto &parameter_blocks : blocks.per_keyframe_marginalization_parameters) {
			parameter_blocks.clear();
		}

		/*
		 * The pointers these mapped belonged to the problem that was just thrown away. They are refilled as the
		 * blocks go back in below.
		 */
		blocks.block_refs.clear();

		// World gravity
		{
			// Seed before adding as parameter
			if (!blocks.world_gravity.seeded) {
				// @todo Pull the calibrated world gravity from the rig calibration
				WorldGravity<double>().pack(blocks.world_gravity.seedVec());
			}

			this->addParameterBlockLocked(blocks.world_gravity.data(),          //
			                              WorldGravity<double>::kNumParameters, //
			                              nullptr,                              //
			                              {ParameterBlockRef::Kind::WorldGravity, 0});

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

		/*
		 * Which keyframe ages hold a state for the device, as a bit per age.
		 */
		static_assert(kSlidingWindowSize <= 32, "the device state masks have a bit per keyframe age");
		uint32_t keyframe_mask = 0;

		auto &device = fusion.device;

		for (uint32_t age = 0; (age + 1) < fusion.num_keyframes; age++) {
			const auto &preintegration = fusion.preintegrationLocked(age);

			// Skip preintegrations that have no real data
			if (preintegration.state == DevicePreintegrationState::Dirty ||
			    preintegration.state == DevicePreintegrationState::Empty) {
				continue;
			}

			/*
			 * Both ends of a usable interval observe the device, so this never marks a keyframe
			 * that holds no observation of it.
			 */
			keyframe_mask |= 1u << age;
			keyframe_mask |= 1u << (age + 1);
		}

		/*
		 * The keyframe masks are now final, so we can drop the prior if we're now missing something the prior
		 * *was* referencing.
		 */
		this->dropPriorIfBlocksMissingLocked(fusion, keyframe_mask);

		if (keyframe_mask != 0) {
			auto &imu_extrinsics_block = blocks.imu_extrinsics;

			if (!imu_extrinsics_block.seeded) {
				ImuExtrinsics<double>(map_quat(fusion.device.latest_Qcv_imu_model).cast<double>())
				    .pack(imu_extrinsics_block.seedVec());
			}

			// IMU extrinsics
			this->addParameterBlockLocked(imu_extrinsics_block.data(),                  //
			                              ImuExtrinsics<double>::kNumParameters,        //
			                              &this->quaternion_manifold,                   //
			                              {ParameterBlockRef::Kind::ImuExtrinsics, 0}); //

			// Add one device keyframe for each state the IMU chain reaches
			auto &device_keyframe_blocks = blocks.keyframes;
			for (uint32_t keyframe_age = 0; keyframe_age < fusion.num_keyframes; keyframe_age++) {
				const auto &keyframe = fusion.keyframeLocked(keyframe_age);
				const auto keyframe_idx = fusion.keyframeIdxLocked(keyframe_age);

				auto &keyframe_block = device_keyframe_blocks[keyframe_idx];
				auto &imu_bias_block = blocks.keyframe_imu_biases[keyframe_idx];

				if ((keyframe_mask & (1u << keyframe_age)) == 0) {
					// This keyframe is not in the solve
					continue;
				}

				if (!imu_bias_block.seeded) {
					ImuBias<double>(map_vec3_f64(device.latest_accel_bias),
					                map_vec3_f64(device.latest_gyro_bias),
					                map_vec3_f64(device.latest_accel_scale))
					    .pack(imu_bias_block.seedVec());
				}

				// IMU bias for this keyframe
				this->addParameterBlockLocked(                 //
				    imu_bias_block.data(),                     //
				    ImuBias<double>::kNumParameters,           //
				    nullptr,                                   //
				    {ParameterBlockRef::Kind::KeyframeImuBias, //
				     static_cast<uint32_t>(keyframe_idx)});    //
				blocks.per_keyframe_marginalization_parameters[keyframe_idx].push_back(
				    imu_bias_block.data());

				// Set the bounds
				for (int i = 0; i < 3; i++) {
					problem.SetParameterLowerBound(imu_bias_block.data(),
					                               ImuBias<double>::kAccelScaleIndex + i,
					                               kMinAccelScale);
					problem.SetParameterUpperBound(imu_bias_block.data(),
					                               ImuBias<double>::kAccelScaleIndex + i,
					                               kMaxAccelScale);

					problem.SetParameterLowerBound(imu_bias_block.data(),
					                               ImuBias<double>::kAccelBiasIndex + i,
					                               -kMaxAccelBias);
					problem.SetParameterUpperBound(
					    imu_bias_block.data(), ImuBias<double>::kAccelBiasIndex + i, kMaxAccelBias);

					problem.SetParameterLowerBound(
					    imu_bias_block.data(), ImuBias<double>::kGyroBiasIndex + i, -kMaxGyroBias);
					problem.SetParameterUpperBound(
					    imu_bias_block.data(), ImuBias<double>::kGyroBiasIndex + i, kMaxGyroBias);
				}

				if (!keyframe_block.seeded) {
					xrt_pose Tcv_world_device;
					if (observedDevicePoseLocked(keyframe, Tcv_world_device)) {
						const Vector3d velocity =
						    seedVelocityLocked(fusion, keyframe_age, Tcv_world_device);

						xrt_pose Tcv_device_imu = {
						    .orientation = fusion.device.latest_Qcv_imu_model,
						    .position = XRT_VEC3_ZERO,
						};
						math_pose_invert(&Tcv_device_imu, &Tcv_device_imu);

						xrt_pose Tcv_world_imu;
						math_pose_transform(&Tcv_world_device, &Tcv_device_imu, &Tcv_world_imu);

						// Seed the pose
						PoseWithVelocity<double>(Tcv_world_imu, velocity)
						    .pack(keyframe_block.seedVec());
					}
				}

				// Pose
				this->addParameterBlockLocked(                //
				    keyframe_block.data(),                    //
				    PoseWithVelocity<double>::kNumParameters, //
				    &this->pose_with_velocity_manifold,       //
				    {ParameterBlockRef::Kind::DeviceKeyframe, //
				     static_cast<uint32_t>(keyframe_idx)});   //
				blocks.per_keyframe_marginalization_parameters[keyframe_idx].push_back(
				    keyframe_block.data());
			}
		}

		/*
		 * Residuals
		 */

		// Clear all the old residual blocks
		for (auto &keyframe_residual_blocks : blocks.keyframe_marginalization_residuals) {
			keyframe_residual_blocks.clear();
		}

		// Both lists of every slot are now empty, and everything added to them from here on belongs to this
		// build.
		blocks.marginalization_list_generation.fill(this->build_generation);

		// Marginalization priors
		auto &prior = this->prior;

		// Whatever id it held belonged to the problem that was just thrown away.
		prior.residual = nullptr;

		if (prior.valid) {
			std::vector<double *> prior_parameters;
			std::vector<MarginalizationPriorCostFunction::Block> prior_blocks;
			prior_parameters.reserve(prior.blocks.size());
			prior_blocks.reserve(prior.blocks.size());

			/*
			 * The oldest keyframe the prior reaches, so that it is folded into the next marginalization at
			 * the same point the residuals it was built from would have been.
			 */
			uint32_t oldest_age = 0;
			bool has_keyframe_block = false;

			for (const MarginalizationPrior::Block &block : prior.blocks) {
				double *values = blocks.resolve(block.ref);

				/*
				 * dropPriorIfBlocksMissingLocked() has already thrown away any prior naming a block
				 * this build does not instantiate, and the parameter passes seed a block before adding
				 * it.
				 */
				assert(values != nullptr);
				if (values == nullptr) {
					this->invalidatePriorLocked();
					break;
				}

				prior_parameters.push_back(values);
				prior_blocks.push_back(
				    {block.linearization_point, this->manifoldForKind(block.ref.kind)});

				const bool per_keyframe = block.ref.kind == ParameterBlockRef::Kind::DeviceKeyframe ||
				                          block.ref.kind == ParameterBlockRef::Kind::KeyframeImuBias;

				uint32_t age = 0;
				if (per_keyframe && fusion.keyframeAgeForIdxLocked(block.ref.ring_slot, age) &&
				    (!has_keyframe_block || age > oldest_age)) {
					// Age counts backwards from the newest keyframe, so the oldest is the largest.
					oldest_age = age;
					has_keyframe_block = true;
				}
			}

			prior.residual = problem.AddResidualBlock(
			    new MarginalizationPriorCostFunction(prior.j, prior.e0, std::move(prior_blocks)), //
			    nullptr,                                                                          //
			    prior_parameters);                                                                //

			/*
			 * A prior over nothing but states the window cannot move out from under has no keyframe to be
			 * folded in at. Nothing can invalidate it either, so it rides along until it is replaced.
			 */
			if (has_keyframe_block) {
				blocks.keyframe_marginalization_residuals[fusion.keyframeIdxLocked(oldest_age)]
				    .push_back(prior.residual);
			}
		}

		/*
		 * The prior is final for this build, so any anchor it no longer covers goes before a factor can pick it
		 * up.
		 */
		this->releaseAnchorsOutsidePriorLocked();

		/*
		 * LED residuals, one factor per observation with every LED correspondence of it stacked in, so that
		 * the loss applies to the observation as a whole.
		 */
		for (uint32_t keyframe_age = 0; keyframe_age < fusion.num_keyframes; keyframe_age++) {
			const auto &keyframe = fusion.keyframeLocked(keyframe_age);

			for (uint32_t obs_i = 0; obs_i < keyframe.num_observations; obs_i++) {
				const auto &observation = keyframe.observations[obs_i];
				const auto &camera = fusion.cameras[keyframe.mosaic_idx][observation.camera_idx];

				/*
				 * The solve holds no state for this device at this keyframe to attach the observation
				 * to, so there is nothing for it to constrain.
				 */
				if ((keyframe_mask & (1u << keyframe_age)) == 0) {
					continue;
				}

				const auto keyframe_idx = fusion.keyframeIdxLocked(keyframe_age);

				auto &keyframe_block = parameter_blocks.keyframes[keyframe_idx];
				auto &device_imu_extrinsics_block = parameter_blocks.imu_extrinsics;

				xrt_pose Tcv_cam_world;
				math_pose_invert(&observation.Tcv_world_cam, &Tcv_cam_world);

				const int num_residuals = static_cast<int>(observation.num_points) * kNumLedResiduals;
				ceres::LossFunction &camera_loss = this->cameraLossForResidualCount(num_residuals);

				blocks.keyframe_marginalization_residuals[keyframe_idx].push_back(
				    problem.AddResidualBlock(                        //
				        anchorJacobians(                             //
				            new StaticCameraObservationCostFunction( //
				                new StaticCameraObservationCostFunctor{
				                    camera.params, observation, isometryFromPose(Tcv_cam_world)}, //
				                num_residuals),                                                   //
				            {{keyframe_block.anchorData(), &this->pose_with_velocity_manifold},
				             {device_imu_extrinsics_block.anchorData(), &this->quaternion_manifold}}),
				        &camera_loss,                         //
				        keyframe_block.data(),                //
				        device_imu_extrinsics_block.data())); //
			}
		}

		// IMU preintegration residuals

		// Only do this if the device is in the solve at all
		if (keyframe_mask != 0) {
			for (uint32_t age = 0; (age + 1) < fusion.num_keyframes; age++) {
				const auto &preintegration = fusion.preintegrationLocked(age);

				// Skip preintegrations that have no real data
				if (preintegration.state == DevicePreintegrationState::Dirty ||
				    preintegration.state == DevicePreintegrationState::Empty) {
					continue;
				}

				const uint32_t start_age = age + 1;

				/*
				 * Both ends were instantiated off this same set of intervals, so a factor can never
				 * reach a state the parameter pass did not add, which Ceres would paper over by
				 * adding it itself, without the manifold this block needs.
				 */
				assert((keyframe_mask & (1u << age)) != 0);
				assert((keyframe_mask & (1u << start_age)) != 0);

				const auto start_age_idx = fusion.keyframeIdxLocked(start_age);
				const auto end_age_idx = fusion.keyframeIdxLocked(age);

				auto &start_keyframe_block = blocks.keyframes[start_age_idx];
				auto &end_keyframe_block = blocks.keyframes[end_age_idx];

				auto &start_imu_bias_block = blocks.keyframe_imu_biases[start_age_idx];
				auto &end_imu_bias_block = blocks.keyframe_imu_biases[end_age_idx];

				// Preintegration residual
				blocks.keyframe_marginalization_residuals[start_age_idx].push_back(
				    problem.AddResidualBlock(
				        anchorJacobians( //
				            new ImuCostFunction(
				                new ImuCostFunctor({preintegration.imu_preintegration})), //
				            {{blocks.world_gravity.anchorData(), nullptr},
				             {start_imu_bias_block.anchorData(), nullptr},
				             {start_keyframe_block.anchorData(), &this->pose_with_velocity_manifold},
				             {end_keyframe_block.anchorData(), &this->pose_with_velocity_manifold}}),
				        nullptr,
				        blocks.world_gravity.data(), //
				        // Preintegrate with the IMU bias of the *start* keyframe
				        start_imu_bias_block.data(), //
				        start_keyframe_block.data(), //
				        end_keyframe_block.data())); //

				// IMU bias random walk residual
				blocks.keyframe_marginalization_residuals[start_age_idx].push_back(
				    problem.AddResidualBlock(
				        anchorJacobians(
				            new ImuBiasRandomWalkCostFunction(new ImuBiasRandomWalkCostFunctor(
				                {preintegration.imu_preintegration.dt})), //
				            {{start_imu_bias_block.anchorData(), nullptr},
				             {end_imu_bias_block.anchorData(), nullptr}}),
				        nullptr,                     //
				        start_imu_bias_block.data(), //
				        end_imu_bias_block.data())); //
			}

			{ // Oldest keyframe anchor residuals
				const uint32_t oldest_keyframe_age = std::bit_width(keyframe_mask) - 1;

				const auto keyframe_idx = fusion.keyframeIdxLocked(oldest_keyframe_age);

				auto &oldest_bias = blocks.keyframe_imu_biases[keyframe_idx];

				/*
				 * IMU bias anchor residual, without marginalization IMU becomes unconstrained
				 * except by observation, this constrains it.
				 */
				const bool prior_carries_bias =
				    this->priorNamesBlockLocked(ParameterBlockRef::Kind::KeyframeImuBias, keyframe_idx);

				if (!prior_carries_bias) {
					blocks.keyframe_marginalization_residuals[keyframe_idx].push_back(
					    problem.AddResidualBlock(
					        anchorJacobians(
					            new ImuBiasAnchorCostFunction(new ImuBiasAnchorCostFunctor(
					                {ImuBias<double>(map_vec3_f64(device.latest_accel_bias),
					                                 map_vec3_f64(device.latest_gyro_bias),
					                                 map_vec3_f64(device.latest_accel_scale))})),
					            {{oldest_bias.anchorData(), nullptr}}), //
					        nullptr,                                    //
					        oldest_bias.data()));                       //
				}
			}
		}

		/*
		 * Last thing the build does, so the snapshot describes the problem exactly as it is about to be handed
		 * to the solver.
		 */
		this->captureSnapshotLocked(fusion, keyframe_mask);

		return this->problem.NumResidualBlocks() > 0;
	}

	/*!
	 * Copies the window out for the logger while the fusion lock is still held.
	 *
	 * The solved half of the snapshot is left alone here, there is nothing solved yet. That part is filled by @ref
	 * fillSolvedSnapshot once Ceres has returned. What this does have to record is the ring slot each keyframe age
	 * maps to, because recovering that later would mean reading `keyframe_head` without the lock.
	 */
	void
	captureSnapshotLocked(SensorFusion &fusion, uint32_t device_keyframe_mask)
	{
		SolveSnapshot &snapshot = fusion.snapshot;

		snapshot.num_late_samples_dropped = fusion.num_late_samples_dropped;

		const Device &device = fusion.device;
		SnapshotDevice &out = snapshot.device;

		out.keyframe_mask = device_keyframe_mask;
		out.id = device.id;
		// Overwritten from the solved extrinsics block, and left identity for a device the solve
		// skipped.
		out.Qcv_imu_model = XRT_QUAT_IDENTITY;
		out.calibrated_Qcv_imu_model = device.calibrated_Qcv_imu_model;

		snapshot.num_keyframes = fusion.num_keyframes;
		for (uint32_t age = 0; age < fusion.num_keyframes; age++) {
			const Keyframe &keyframe = fusion.keyframeLocked(age);
			SnapshotKeyframe &out = snapshot.keyframes[age];

			this->snapshot_block_idx[age] = static_cast<uint32_t>(fusion.keyframeIdxLocked(age));

			out.timestamp_ns = keyframe.timestamp_ns;
			out.mosaic_idx = keyframe.mosaic_idx;
			out.sealed = keyframe.sealed;
			out.device = {};

			out.num_observations = keyframe.num_observations;
			for (uint32_t i = 0; i < keyframe.num_observations; i++) {
				const DeviceObservation &observation = keyframe.observations[i];
				const CameraDescription &camera =
				    fusion.cameras[keyframe.mosaic_idx][observation.camera_idx];

				SnapshotObservation snapshot_observation{};
				snapshot_observation.mosaic_idx = keyframe.mosaic_idx;
				snapshot_observation.camera_idx = observation.camera_idx;
				snapshot_observation.Tcv_world_cam = observation.Tcv_world_cam;
				snapshot_observation.Tcv_cam_device_seed = observation.Tcv_cam_device_seed;
				snapshot_observation.params = camera.params;
				snapshot_observation.num_points = observation.num_points;
				snapshot_observation.points2d = observation.points2d;
				snapshot_observation.points3d = observation.points3d;

				out.observations[i] = snapshot_observation;
			}
		}
	}

	/*!
	 * Reads the solved parameter blocks back into the snapshot.
	 *
	 * Takes no lock and needs none: the blocks belong to this problem, and the ring indices they are reached
	 * through were recorded by @ref captureSnapshotLocked while the lock was held.
	 */
	void
	fillSolvedSnapshot(SolveSnapshot &snapshot)
	{
		const ParameterBlocks &blocks = this->parameter_blocks;

		snapshot.world_gravity_mag =
		    blocks.world_gravity.seeded ? WorldGravity<double>(blocks.world_gravity.vec).gravity_mag : 0.0;

		SnapshotDevice &device = snapshot.device;

		const auto &extrinsics_block = blocks.imu_extrinsics;
		if (extrinsics_block.seeded) {
			map_quat(device.Qcv_imu_model) =
			    ImuExtrinsics<double>(extrinsics_block.vec).Q_imu_model.coeffs().cast<float>();
		}

		for (uint32_t age = 0; age < snapshot.keyframes.size(); age++) {
			SolvedDeviceKeyframe &out = snapshot.keyframes[age].device;

			if ((device.keyframe_mask & (1u << age)) == 0) {
				continue;
			}

			const uint32_t block_idx = this->snapshot_block_idx[age];
			const auto &pose_block = blocks.keyframes[block_idx];
			const auto &bias_block = blocks.keyframe_imu_biases[block_idx];

			/*
			 * The mask says the solve reached this state, so both blocks were seeded on the way in.
			 * Checking anyway keeps a logging path from being the thing that trips an assert.
			 */
			if (!pose_block.seeded || !bias_block.seeded) {
				continue;
			}

			const PoseWithVelocity<double> pose{pose_block.vec};
			const ImuBias<double> bias{bias_block.vec};

			out.Tcv_world_device = pose.toXrtPose();
			map_vec3_f64(out.velocity) = pose.velocity;
			map_vec3_f64(out.accel_bias) = bias.accel_bias;
			map_vec3_f64(out.gyro_bias) = bias.gyro_bias;
			map_vec3_f64(out.accel_scale) = bias.accel_scale;
		}
	}

	/*!
	 * Drops everything seeded from a device's calibration, so the next build re-seeds it.
	 *
	 * The keyframe poses are deliberately left alone: they are seeded from observations, which a change of IMU
	 * intrinsics says nothing about, and throwing away a converged pose would cost the solve its warm start.
	 */
	void
	resetDeviceCalibrationSeedsLocked()
	{
		/*
		 * The prior was linearized against the biases this is throwing out, and there is no correcting it
		 * across a change that large.
		 */
		this->invalidatePriorLocked();

		this->parameter_blocks.imu_extrinsics.unseed();

		for (auto &bias : this->parameter_blocks.keyframe_imu_biases) {
			bias.unseed();
		}
	}

	void
	reset()
	{
		// The marginalization lists describe that same window, so the next eviction has to rebuild before it
		// folds any.
		this->marginalization_lists_stale = true;

		this->resetDeviceCalibrationSeedsLocked();

		for (auto &keyframe : this->parameter_blocks.keyframes) {
			keyframe.unseed();
		}
	}

	void
	evictLocked(SensorFusion &fusion, uint32_t age)
	{
		/*
		 * No lock of its own, and none needed: eviction reaches this from the fusion thread's fold pass, which
		 * is the same thread that builds and solves. Nothing else can be looking at the problem.
		 */
		uint32_t idx = fusion.keyframeIdxLocked(age);

		/*
		 * The lists have to come from a build that no earlier eviction has marginalized against yet, see
		 * @ref marginalization_lists_stale. SensorFusion::pushKeyframeLocked rebuilds before calling this to
		 * make sure of that, so this only trips if that guarantee has been broken.
		 */
		const bool lists_current =
		    !this->marginalization_lists_stale &&
		    this->parameter_blocks.marginalization_list_generation[idx] == this->build_generation;
		assert(lists_current);

		if (!lists_current) {
			SF_ERROR((&fusion),
			         "Evicting keyframe %u (slot %u) against marginalization lists from an earlier build, "
			         "dropping the prior rather than folding them.",
			         age, idx);

			/*
			 * Folding these lists would lose what the prior carries, or count a slot's residuals
			 * into it a second time. An emptier prior is better than an overconfident one.
			 */
			this->invalidatePriorLocked();
		} else {
			const auto &marginalized_residuals =
			    this->parameter_blocks.keyframe_marginalization_residuals[idx];
			const auto &full_marginalized_parameters_vec =
			    this->parameter_blocks.per_keyframe_marginalization_parameters[idx];

			//! The base parameters of this marginalization (M)
			std::set<double *> base_parameters;

			for (const auto marginalized_parameter : full_marginalized_parameters_vec) {
				// Skip constant parameter blocks
				if (this->problem.IsParameterBlockConstant(marginalized_parameter)) {
					continue;
				}

				base_parameters.emplace(marginalized_parameter);
			}

			std::set<double *> connected_parameters_set{};
			if (!base_parameters.empty()) {
				for (const auto marginalization_residual : marginalized_residuals) {
					std::vector<double *> connected_parameters_vec;
					this->problem.GetParameterBlocksForResidualBlock(marginalization_residual,
					                                                 &connected_parameters_vec);

					for (const auto connected_parameter : connected_parameters_vec) {
						// This parameter is a base parameter.
						if (base_parameters.count(connected_parameter) != 0) {
							continue;
						}

						// Skip constant parameter blocks
						if (this->problem.IsParameterBlockConstant(connected_parameter)) {
							continue;
						}

						connected_parameters_set.emplace(connected_parameter);
					}
				}
			}

			if (connected_parameters_set.empty() && !base_parameters.empty()) {
				/*
				 * Everything those residuals touched is being marginalized away, so there is no
				 * state left for a factor to be a statement about.
				 */
				this->invalidatePriorLocked();
			}

			if (!connected_parameters_set.empty()) {
				ceres::Problem::EvaluateOptions options;
				options.apply_loss_function = true;
				options.num_threads = 1; // @todo Figure out if multi-threading is needed or not.
				options.parameter_blocks.insert(options.parameter_blocks.end(), base_parameters.begin(),
				                                base_parameters.end());
				options.parameter_blocks.insert(options.parameter_blocks.end(),
				                                connected_parameters_set.begin(),
				                                connected_parameters_set.end());
				options.residual_blocks = marginalized_residuals;

				std::vector<double> residual_vec;
				ceres::CRSMatrix jacobian_crs;
				this->problem.Evaluate(options, nullptr, &residual_vec, nullptr, &jacobian_crs);

				auto residuals =
				    Eigen::Map<VectorXd, Eigen::Unaligned>(residual_vec.data(), residual_vec.size());

				MatrixXd jacobian;
				matrixCrsToEigen(jacobian_crs, jacobian);

				/*
				 * The Jacobian's columns are the tangent sizes of the parameter blocks, in the order
				 * they were put into options.parameter_blocks, so the base/remainder split has to be
				 * counted in tangent dimensions rather than in parameter blocks.
				 */
				int num_base_states = 0;
				for (double *base_parameter : base_parameters) {
					num_base_states += this->problem.ParameterBlockTangentSize(base_parameter);
				}

				VectorXd e0_marg;
				MatrixXd j_marg;
				marginalize(residuals, jacobian, num_base_states, e0_marg, j_marg);

				/*
				 * Whether the factor the device already carries is one of the residuals being folded
				 * in here.
				 */
				const bool had_prior = this->prior.valid;
				const auto previous_residual = this->prior.residual;
				const bool folded_in =
				    had_prior && previous_residual != nullptr &&
				    std::find(marginalized_residuals.begin(), marginalized_residuals.end(),
				              previous_residual) != marginalized_residuals.end();

				/*
				 * Replaced wholesale: whatever this device carried before is either folded into the
				 * factor being built here, or was dropped along with the residuals that are going away.
				 */
				this->invalidatePriorLocked();

				bool blocks_resolved = true;
				for (double *connected_parameter : connected_parameters_set) {
					const auto it = this->parameter_blocks.block_refs.find(connected_parameter);

					/*
					 * Every block in the problem went in through addParameterBlockLocked(), so a
					 * residual cannot reach one that is not in here.
					 */
					assert(it != this->parameter_blocks.block_refs.end());
					if (it == this->parameter_blocks.block_refs.end()) {
						blocks_resolved = false;
						break;
					}

					const ParameterBlockRef &ref = it->second;

					/*
					 * Anchoring the state here is what keeps this factor and the residuals that
					 * outlive it talking about the same linearization, and a state already anchored
					 * by an earlier marginalization keeps the point it was anchored at rather than
					 * being walked forward.
					 *
					 * Without first-estimate jacobians there is no anchor, and the factor is
					 * linearized around wherever the solve has just left the state.
					 */
					const double *anchored = this->parameter_blocks.anchor(ref);
					const double *linearization_point =
					    anchored != nullptr ? anchored : connected_parameter;

					this->prior.blocks.push_back(
					    {ref,
					     {linearization_point,
					      linearization_point + ambientSizeForKind(ref.kind)}});
				}

				if (!blocks_resolved) {
					this->invalidatePriorLocked();
				} else {
					this->prior.j = std::move(j_marg);
					this->prior.e0 = std::move(e0_marg);
					this->prior.valid = true;

					/*
					 * `r^*` is what the new factor evaluates to with every block still where it is
					 * right now, so a healthy one starts out near zero: the states it is a
					 * distribution over are the ones the solve just settled on. A large norm means
					 * the marginalized residuals disagreed with the solved estimate, and the prior
					 * is about to pull the window towards something it was never at.
					 */
					SF_DEBUG((&fusion),
					         "Marginalized keyframe %u into a %u state prior, |r*| %f, %s.", age,
					         static_cast<uint32_t>(prior.e0.rows()), prior.e0.norm(),
					         !had_prior ? "first one it has held"
					                    : (folded_in ? "folding in the one it held"
					                                 : "replacing the one it held"));

					if (had_prior && !folded_in) {
						SF_WARN((&fusion),
						        "Device's previous marginalization prior was not among the "
						        "residuals folded in here, so everything it carried is lost.");
					}
				}
			}
		}

		/*
		 * This slot's lists have been folded in, and the above built prior lives outside the problem. See
		 * @ref marginalization_lists_stale: nothing may marginalize against this problem again before the next
		 * build.
		 */
		this->parameter_blocks.per_keyframe_marginalization_parameters[idx].clear();
		this->parameter_blocks.marginalization_list_generation[idx] = 0;
		this->marginalization_lists_stale = true;

		/*
		 * Mark as needing seeding. The slot is about to be handed to a keyframe of its own, so whatever the
		 * state living here was anchored at stops meaning anything.
		 */
		this->parameter_blocks.keyframes[idx].unseed();
		this->parameter_blocks.keyframe_imu_biases[idx].unseed();
	}
};

/*
 *
 * Device
 *
 */

Device::Device(t_constellation_device_id_t id, const t_constellation_tracker_led_model *led_model)
    : id(id),                                      //
      led_model(led_model),                        //
      calibrated_accel_bias({0.0, 0.0, 0.0}),      //
      calibrated_gyro_bias({0.0, 0.0, 0.0}),       //
      calibrated_accel_scale({1.0, 1.0, 1.0}),     //
      calibrated_Qcv_imu_model(XRT_QUAT_IDENTITY), //
      imu_write_count(0),                          //
      imu_samples()                                //
{
	m_relation_history_create(&this->relation_history);

	this->latest_accel_bias = this->calibrated_accel_bias;
	this->latest_gyro_bias = this->calibrated_gyro_bias;
	this->latest_accel_scale = this->calibrated_accel_scale;
	this->latest_Qcv_imu_model = this->calibrated_Qcv_imu_model;

	this->resetLocked();
}

Device::~Device()
{
	m_relation_history_destroy(&this->relation_history);
}

void
Device::resetLocked()
{
	m_relation_history_clear(this->relation_history);

	this->has_latest_solve = false;

	for (auto &preintegration : this->preintegrations) {
		preintegration = DevicePreintegration::Identity(DevicePreintegrationState::Dirty);
	}

	// Wipe the amount of IMU writes
	this->imu_write_count.store(0, std::memory_order_relaxed);
}

/*
 *
 * Helper functions
 *
 */

size_t
SensorFusion::keyframeIdxLocked(uint32_t age) const
{
	return (this->keyframe_head - 1 - age) % kSlidingWindowSize;
}

bool
SensorFusion::keyframeAgeForIdxLocked(size_t idx, uint32_t &out_age) const
{
	// Bounded by the window, so a scan is easier than the real math to reason about and is cheap anyway.
	for (uint32_t age = 0; age < this->num_keyframes; age++) {
		if (this->keyframeIdxLocked(age) == idx) {
			out_age = age;
			return true;
		}
	}

	return false;
}

Keyframe &
SensorFusion::keyframeLocked(uint32_t age)
{
	assert(age < this->num_keyframes);

	return this->keyframe_storage[this->keyframeIdxLocked(age)];
}

const Keyframe &
SensorFusion::keyframeLocked(uint32_t age) const
{
	assert(age < this->num_keyframes);

	return this->keyframe_storage[this->keyframeIdxLocked(age)];
}

DevicePreintegration &
SensorFusion::preintegrationLocked(uint32_t age)
{
	assert(age < this->num_keyframes);

	return this->device.preintegrations[this->keyframeIdxLocked(age)];
}

Keyframe &
SensorFusion::pushKeyframeLocked(const constellation::CameraSample &sample)
{
	/*
	 * The slot about to be reused holds the keyframe falling out of the window, so evict it.
	 */
	if (this->num_keyframes == kSlidingWindowSize) {
		uint32_t evicted_age = kSlidingWindowSize - 1;

		/*
		 * Marginalization may only ever fold in sealed keyframes, and this is the last moment one is
		 * reachable. `kKeyframeSealLagKeyframes < kSlidingWindowSize` is what guarantees it.
		 */
		assert(this->keyframeLocked(evicted_age).sealed);

		/*
		 * The thread folds every queued sample before it builds, so when it has fallen behind, one pass can
		 * push several keyframes. Only the first of those evictions has a whole build to marginalize against.
		 */
		if (this->problem->marginalization_lists_stale) {
			this->rebuildForEvictionLocked();
		}

		// Evict the age for the fusion problem
		this->problem->evictLocked(*this, evicted_age);
	} else {
		this->num_keyframes += 1;
	}

	const uint64_t slot = this->keyframe_head % kSlidingWindowSize;
	this->keyframe_head += 1;

	Keyframe &keyframe = this->keyframe_storage[slot];
	keyframe.mosaic_idx = sample.mosaic_index;
	keyframe.timestamp_ns = sample.timestamp_ns;
	keyframe.sealed = false;
	/*
	 * Deliberately not clearing `observations`, would be very expensive.
	 */
	keyframe.num_observations = 0;

	/*
	 * Nothing has integrated the interval ending at this keyframe yet, and the slot still holds the preintegration
	 * belonging to the keyframe that just fell out of it.
	 */
	this->device.preintegrations[slot] = DevicePreintegration::Identity(DevicePreintegrationState::Dirty);

	this->sealAgedKeyframesLocked();

	return keyframe;
}

void
SensorFusion::sealAgedKeyframesLocked()
{
	for (uint32_t age = kKeyframeSealLagKeyframes; age < this->num_keyframes; age++) {
		Keyframe &keyframe = this->keyframeLocked(age);

		if (keyframe.sealed) {
			continue;
		}

		SF_DEBUG(this, "Sealing keyframe (mosaic %u) with age %u at ts %" PRIi64 ", cameras %u reported.",
		         keyframe.mosaic_idx, age, keyframe.timestamp_ns, keyframe.num_observations);

		keyframe.sealed = true;
	}
}

void
SensorFusion::markDirtyLocked()
{
	this->dirty = true;
	os_thread_helper_signal_locked(&this->thread);
}

bool
SensorFusion::buildFusionLocked()
{
	const bool has_something_to_solve = this->problem->buildLocked(*this);

	SF_TRACE(this, "Built sensor fusion optimization problem, %s to solve",
	         has_something_to_solve ? "something" : "nothing");

	return has_something_to_solve;
}

void
SensorFusion::rebuildForEvictionLocked()
{
	SF_DEBUG(this, "Rebuilding the problem to evict another keyframe in the same pass.");

	/*
	 * The flags are the end-of-pass tick's business, since it integrates whatever is still dirty and records
	 * what is left, so they are thrown away here.
	 */
	bool work_remaining = false;
	bool wants_imu_wakeup = false;

	this->preintegrateDeviceLocked(work_remaining, wants_imu_wakeup);

	(void)this->buildFusionLocked();
	// We aren't doing a solve here, just making sure our marginalization is up to date
}

void
SensorFusion::runFusionLocked()
{
	ceres::Problem &problem = this->problem->problem;

	/*
	 * Fixed for the life of this solve since this thread is the only ones to add residuals and we aren't doing that
	 * here.
	 */
	const int num_residuals = problem.NumResiduals();

	ceres::Solver::Options options{};
	options.max_num_iterations = 3;
	/*
	 * The problem is block sparse: a keyframe only touches its own camera observations and the two IMU factors
	 * either side of it.
	 */
	options.linear_solver_type = ceres::LinearSolverType::SPARSE_NORMAL_CHOLESKY;
	// Eigen's sparse backend is the one this Ceres is built with, see CERES_USE_EIGEN_SPARSE.
	options.sparse_linear_algebra_library_type = ceres::SparseLinearAlgebraLibraryType::EIGEN_SPARSE;
	options.minimizer_progress_to_stdout = false;
	options.logging_type = ceres::LoggingType::PER_MINIMIZER_ITERATION;
	options.num_threads = static_cast<int>(std::thread::hardware_concurrency());
	// @todo Tune the tolerances to actual numbers.

	// Ceres does not take ownership, and the callback only has to outlive the `Solve` below.
	ResidualRmsTargetCallback rms_target{num_residuals, kTargetResidualRmsSigma};

	if constexpr (kExitOnRmsFloor) {
		options.callbacks.push_back(&rms_target);
		/*
		 * The callback reads nothing but the cost Ceres has already computed, so the solve is spared pushing
		 * parameter state back out to the blocks every iteration.
		 */
		options.update_state_every_iteration = false;
	}

	ceres::Solver::Summary summary;

	/*
	 * Unlock the fusion thread, since we've built our Ceres problem and nothing else will touch it or the parameter
	 * pointers.
	 */
	os_thread_helper_unlock(&this->thread);
	ceres::Solve(options, &problem, &summary);
	os_thread_helper_lock(&this->thread);

	SolveSnapshot &snapshot = this->snapshot;
	this->problem->fillSolvedSnapshot(snapshot);

#ifdef XRT_FEATURE_RERUN
	if (this->rerun_context) {
		/*
		 * Logging a solve means serializing the whole window out to the stream, which is far too long to
		 * hold the queue shut for. Nothing here is reachable from another thread, so it does not need to be
		 * held for.
		 */
		os_thread_helper_unlock(&this->thread);

		snapshot.num_residuals = num_residuals;
		snapshot.num_parameter_blocks = problem.NumParameterBlocks();
		snapshot.num_iterations = static_cast<uint32_t>(summary.iterations.size());
		snapshot.initial_rms_sigma = residualRmsSigma(summary.initial_cost, num_residuals);
		snapshot.final_rms_sigma = residualRmsSigma(summary.final_cost, num_residuals);
		snapshot.solve_time_s = summary.total_time_in_seconds;
		snapshot.termination = ceres::TerminationTypeToString(summary.termination_type);

		this->rerun_context->logSolve(snapshot);

		/*
		 * Numbered after the fact, so the first solve is 0 and the logger's sequence timeline lines up with
		 * how many solves have actually been logged.
		 */
		snapshot.solve_index += 1;

		os_thread_helper_lock(&this->thread);
	}
#endif

	bool device_updated = false;
	for (uint32_t i = 0; i < snapshot.num_keyframes; i++) {
		const auto &keyframe = snapshot.keyframes[i];

		// Device has already been updated
		if (device_updated) {
			continue;
		}

		// If the device isn't in this keyframe, skip it
		if (!(snapshot.device.keyframe_mask & (1 << i))) {
			continue;
		}

		// we have a previous solve and it's older than the new one
		if (this->device.has_latest_solve && keyframe.timestamp_ns > this->device.latest_solve_time_ns) {
			std::array<xrt_imu_sample, kImuBufferSize> samples_buf;
			uint32_t num_samples = 0;
			xrt_imu_sample first_after;

			// Get the IMU samples
			const ImuCollectResult result = collectImuSamplesForRange(this->device,                      //
			                                                          this->device.latest_solve_time_ns, //
			                                                          keyframe.timestamp_ns,             //
			                                                          samples_buf,                       //
			                                                          num_samples,                       //
			                                                          first_after);                      //

			/*
			 * If we got it OK, or there's no sample *after*, that's fine, we're integrating
			 * until the last IMU sample *before* the stop time.
			 */
			if ((result == ImuCollectResult::Ok || result == ImuCollectResult::NoSampleAfterEnd) //
			    && num_samples > 0) {
				integrateToTimestampHistory(
				    this->snapshot.world_gravity_mag,                                 //
				    this->device.latest_solve_time_ns,                                //
				    this->device.Tcv_world_device_latest,                             //
				    this->device.latest_world_velocity,                               //
				    this->device.latest_accel_bias,                                   //
				    this->device.latest_gyro_bias,                                    //
				    this->device.latest_accel_scale,                                  //
				    std::span<const xrt_imu_sample>(samples_buf.data(), num_samples), //
				    this->device.relation_history);                                   //

				SF_DEBUG(this, "Filled relation history for device %d from %" PRIi64 " to %" PRIi64,
				         this->device.id, this->device.latest_solve_time_ns, keyframe.timestamp_ns);
			}
		}

		this->device.has_latest_solve = true;
		this->device.latest_solve_time_ns = keyframe.timestamp_ns;
		this->device.Tcv_world_device_latest = keyframe.device.Tcv_world_device;
		this->device.latest_world_velocity = keyframe.device.velocity;

		this->device.latest_accel_bias = keyframe.device.accel_bias;
		this->device.latest_gyro_bias = keyframe.device.gyro_bias;
		this->device.latest_accel_scale = keyframe.device.accel_scale;
		this->device.latest_Qcv_imu_model = snapshot.device.Qcv_imu_model;

		device_updated = true;

		SF_DEBUG(this, "Got new solved pose for device %d at %" PRIi64, this->device.id, keyframe.timestamp_ns);
	}

	SF_DEBUG(this, "Solved system in %zu iterations, %.3f -> %.3f sigma RMS over %d residuals (%s). Report: %s",
	         summary.iterations.size(),                                //
	         residualRmsSigma(summary.initial_cost, num_residuals),    //
	         residualRmsSigma(summary.final_cost, num_residuals),      //
	         num_residuals,                                            //
	         ceres::TerminationTypeToString(summary.termination_type), //
	         summary.FullReport().c_str());                            //
}

/*
 *
 * "Exported" functions
 *
 */

SensorFusion::SensorFusion(t_constellation_device_id_t id, const t_constellation_tracker_led_model *led_model)
    : log_level(debug_get_log_option_sensor_fusion_log()), //
      device(id, led_model),                               //
      sample_queue()                                       //
{
	this->problem = new SensorFusionProblem();

	if (debug_get_bool_option_sensor_fusion_enable_rerun()) {
#ifdef XRT_FEATURE_RERUN
		this->rerun_context = std::make_unique<RerunContext>();
		SF_INFO(this, "Sensor fusion Rerun stream enabled");

		if (debug_get_bool_option_sensor_fusion_rerun_spawn()) {
			this->rerun_context->spawnViewer();
		} else {
			this->rerun_context->stream->connect_grpc().exit_on_failure();
		}
#else
		SF_ERROR(this, "Rerun stream requested but XRT_FEATURE_RERUN is not enabled");
#endif
	}

	// Everything the fusion thread touches has to exist before it is let loose on it.
	os_thread_helper_init(&this->thread);
	os_thread_helper_start(&this->thread, thread_runner, this);
}

SensorFusion::~SensorFusion()
{
	// Joins the fusion thread, so nothing can be mid-solve by the time the problem and the stream go away.
	os_thread_helper_destroy(&this->thread);

	delete this->problem;
}

void
SensorFusion::addCamera(const CameraDescription &camera_description)
{
	os_thread_helper_lock(&this->thread);

	// Copy the camera description in
	this->cameras[camera_description.mosaic_idx][camera_description.camera_idx] = camera_description;

	os_thread_helper_unlock(&this->thread);
}

void
SensorFusion::setDeviceImuCalibration(const DeviceImuCalibration &calibration)
{
	os_thread_helper_lock(&this->thread);

	this->device.calibrated_accel_bias = calibration.accel_bias;
	this->device.calibrated_gyro_bias = calibration.gyro_bias;
	this->device.calibrated_accel_scale = calibration.accel_scale;
	this->device.calibrated_Qcv_imu_model = calibration.Qcv_imu_model;

	this->device.latest_accel_bias = calibration.accel_bias;
	this->device.latest_gyro_bias = calibration.gyro_bias;
	this->device.latest_accel_scale = calibration.accel_scale;
	this->device.latest_Qcv_imu_model = calibration.Qcv_imu_model;

	/*
	 * Blocks seeded from the old values, and the anchor built out of them, both belong to a calibration that no
	 * longer applies.
	 */
	this->problem->resetDeviceCalibrationSeedsLocked();

	/*
	 * Every interval this device holds was integrated at the old linearization point, so it has to be walked
	 * again rather than corrected across a gap this large.
	 */
	for (auto &preintegration : this->device.preintegrations) {
		preintegration.state = DevicePreintegrationState::Dirty;
	}

	SF_INFO(this,
	        "Device %d IMU calibration set. Accel bias [%f, %f, %f] m/s^2, gyro bias [%f, %f, %f] rad/s, accel "
	        "scale [%f, %f, %f].",
	        this->device.id,                                                                  //
	        calibration.accel_bias.x, calibration.accel_bias.y, calibration.accel_bias.z,     //
	        calibration.gyro_bias.x, calibration.gyro_bias.y, calibration.gyro_bias.z,        //
	        calibration.accel_scale.x, calibration.accel_scale.y, calibration.accel_scale.z); //

	this->markDirtyLocked();

	os_thread_helper_unlock(&this->thread);
}

void
SensorFusion::pushCameraSample(constellation::ConstellationTracker *tracker, const constellation::CameraSample &sample)
{
	/*
	 * Only ever needed `tracker` to act as a "badge" to make sure the caller actually holds the lock on the
	 * tracker, so let's just mark it unused.
	 */
	(void)tracker;

	// Ignore samples with no camera position
	if (!sample.Txr_world_cam.has_value()) {
		return;
	}

	SF_TRACE(this, "Camera sample %u/%u at %" PRIi64 " pushed.", sample.mosaic_index, sample.camera_index,
	         sample.timestamp_ns);

	os_thread_helper_lock(&this->thread);

	if (this->sample_queue.size() >= kMaxQueuedCameraSamples) {
		this->num_queued_samples_dropped += 1;
		const uint64_t num_dropped = this->num_queued_samples_dropped;

		os_thread_helper_unlock(&this->thread);

		SF_WARN(this,
		        "Fusion thread is %u camera samples behind, dropping %u/%u at %" PRIi64 ". (%" PRIu64
		        " dropped so far)",
		        kMaxQueuedCameraSamples, sample.mosaic_index, sample.camera_index, sample.timestamp_ns,
		        num_dropped);
		return;
	}

	this->sample_queue.push_back(sample);

	this->markDirtyLocked();

	os_thread_helper_unlock(&this->thread);
}

void
SensorFusion::processCameraSampleLocked(const constellation::CameraSample &sample)
{
	Keyframe *keyframe = nullptr;

	const constellation::DeviceState *device_state = nullptr;
	for (uint32_t i = 0; i < sample.device_count; i++) {
		const auto &sample_device = sample.device_states[i];

		if (sample_device.device_id != this->device.id) {
			continue;
		}

		if (sample_device.found_pose.has_value()) {
			device_state = &sample_device;

			// This pose has already been handled.
			if (sample_device.found_pose->handled) {
				assert(!"Unreachable"); // The tracker never pushes already handled samples.

				// Early exit in release modes
				return;
			}

			break;
		}
	}

	if (device_state == nullptr) {
		assert(!"Unreachable."); // We should only be given samples with poses?
		return;
	}

	DeviceObservation observation;
	if (!constructObservation(this, sample, *device_state, observation)) {
		// Something's gone wrong.
		return;
	}

	/*
	 * Look for the exposure this sample belongs to. The cameras of a mosaic expose together but finish at
	 * different times, so a sample routinely arrives after a keyframe has already been opened for its exposure -
	 * and with per-camera capture threads, occasionally after a newer exposure has opened one too.
	 */
	for (uint32_t age = 0; age < this->num_keyframes; age++) {
		Keyframe &candidate = this->keyframeLocked(age);

		if (sample.mosaic_index != candidate.mosaic_idx) {
			continue;
		}

		if (std::abs(sample.timestamp_ns - candidate.timestamp_ns) >= kKeyframeGroupingToleranceNs) {
			continue;
		}

		if (candidate.sealed) {
			/*
			 * Too late to take it. Its neighbours may already have been folded into a marginalization
			 * prior, and adding a factor to it now would count this exposure's information twice.
			 */
			this->num_late_samples_dropped += 1;

			SF_DEBUG(this,
			         "Camera sample %u/%u at %" PRIi64
			         " arrived after its keyframe was sealed, dropping it. (%" PRIu64 " dropped so far)",
			         sample.mosaic_index, sample.camera_index, sample.timestamp_ns,
			         this->num_late_samples_dropped);

			return;
		}

		keyframe = &candidate;
		break;
	}

	// Device was found, but this is a new keyframe
	if (keyframe == nullptr) {
		/*
		 * Nothing in the window wants it and it predates the newest keyframe, so it belongs somewhere
		 * in the middle of the window, which we cannot open a keyframe in without reordering everything
		 * after it.
		 */
		if (this->num_keyframes > 0 && sample.timestamp_ns < this->keyframeLocked(0).timestamp_ns) {
			this->num_late_samples_dropped += 1;

			SF_DEBUG(this,
			         "Camera sample %u/%u at %" PRIi64
			         " is older than the newest keyframe and matches none, dropping it. (%" PRIu64
			         " dropped so far)",
			         sample.mosaic_index, sample.camera_index, sample.timestamp_ns,
			         this->num_late_samples_dropped);

			return;
		}

		// If the gap is too wide, just reset fusion.
		if (this->num_keyframes > 0 &&
		    time_ns_to_s(sample.timestamp_ns - this->keyframeLocked(0).timestamp_ns) >
		        kMaxTimeBetweenKeyframesSeconds) {
			this->resetLocked();
		}

		keyframe = &this->pushKeyframeLocked(sample);
	}

	// Write the sample of the found device
	writeSampleToKeyframe(this, *keyframe, observation, sample.camera_index);
}

void
SensorFusion::pushImuSample(const xrt_imu_sample &xr_sample)
{
	/*
	 * @note This deliberately runs without taking `this->thread`. We *really* do not want a heavy lock on the IMU
	 *       path, so the ring is a single-producer/single-consumer structure instead: we fill the slot first and
	 *       then publish it by bumping the count, and the fusion thread reads in the mirrored order. See
	 *       `imu_write_count` for more details.
	 */

	SF_TRACE(this, "IMU sample %" PRIi64 " for %d pushed.", xr_sample.timestamp_ns, this->device.id);

	// Convert from OpenXR to OpenCV coordinate space for the IMU sample
	xrt_imu_sample cv_sample = xr_sample;
	math_vec3_f64_convert_from_opencv(&cv_sample.accel_m_s2, &cv_sample.accel_m_s2);
	math_vec3_f64_convert_from_opencv(&cv_sample.gyro_rad_secs, &cv_sample.gyro_rad_secs);

	// Relaxed is enough to read the count back: this thread is its only writer.
	const uint64_t count = this->device.imu_write_count.load(std::memory_order_relaxed);

	/*
	 * Release fence, pairing with the acquire fence a reader takes before re-checking the count: a reader
	 * that sees any word of the store below also sees `count` itself, published by the previous push, and
	 * so notices that the slot it read was being overwritten.
	 */
	std::atomic_thread_fence(std::memory_order_release);

	this->device.imu_samples[count % kImuBufferSize].store(cv_sample);

	/*
	 * Release, so that a reader acquiring the new count is guaranteed to see the slot we just filled and
	 * never an older or half-written one.
	 */
	this->device.imu_write_count.store(count + 1, std::memory_order_release);

	/*
	 * Release, so that the thread gets the timestamp always has the latest written imu write availabile.
	 */
	this->device.latest_imu_ts_ns.store(xr_sample.timestamp_ns, std::memory_order_release);

	/*
	 * The fusion thread parked a preintegration because the IMU had not reached the end of its interval
	 * yet, and this sample may be the one that closes it. Signalled *without* the mutex on purpose: taking
	 * it here would put the IMU thread behind whatever the fusion thread is chewing on, which is the one
	 * thing this path must never do. The cost is that a wakeup can be lost to that race, which just means
	 * waiting for the next sample at most a couple milliseconds later.
	 */
	if (this->waiting_on_imu.load(std::memory_order_relaxed)) {
		os_thread_helper_signal_locked(&this->thread);
	}

	return;
}

void
SensorFusion::getTrackedPose(timepoint_ns requested_time_ns,
                             timepoint_ns &out_time_ns,
                             xrt_space_relation &out_relation)
{
	out_relation = XRT_SPACE_RELATION_ZERO;

	os_thread_helper_lock(&this->thread);

	if (!device.has_latest_solve) {
		os_thread_helper_unlock(&this->thread);
		return;
	}

	double world_gravity_mag = this->snapshot.world_gravity_mag;
	timepoint_ns latest_solve_time_ns = this->device.latest_solve_time_ns;
	xrt_pose Tcv_world_device = this->device.Tcv_world_device_latest;
	xrt_vec3_f64 world_velocity = this->device.latest_world_velocity;
	xrt_vec3_f64 accel_bias = this->device.latest_accel_bias;
	xrt_vec3_f64 gyro_bias = this->device.latest_gyro_bias;
	xrt_vec3_f64 accel_scale = this->device.latest_accel_scale;
	xrt_quat Qcv_imu_model = this->device.latest_Qcv_imu_model;

	os_thread_helper_unlock(&this->thread);

	bool integrate_to = true;

	if (requested_time_ns <= latest_solve_time_ns) {
		SF_TRACE(this, "Requested time is before our latest sample, can't work backwards.");
		integrate_to = false;
	}

	std::array<xrt_imu_sample, kImuBufferSize> samples_buf;
	uint32_t num_samples = 0;
	std::optional<xrt_imu_sample> first_after;

	if (integrate_to) {
		xrt_imu_sample raw_first_after;
		const ImuCollectResult result = collectImuSamplesForRange(this->device,         //
		                                                          latest_solve_time_ns, //
		                                                          requested_time_ns,    //
		                                                          samples_buf,          //
		                                                          num_samples,          //
		                                                          raw_first_after);     //

		// Check if we got it OK
		if (result != ImuCollectResult::Ok && result != ImuCollectResult::NoSampleAfterEnd) {
			SF_TRACE(this, "IMU data collection for dead reckoning failed, got %d",
			         static_cast<int>(result));
			integrate_to = false;
		}

		if (num_samples == 0) {
			SF_TRACE(this, "No IMU data found for dead reckoning");
			integrate_to = false;
		}

		if (result == ImuCollectResult::NoSampleAfterEnd) {
			first_after = std::nullopt;
		} else {
			first_after = raw_first_after;
		}
	}

	if (integrate_to) {
		integrateToTimestamp(                                                 //
		    world_gravity_mag,                                                //
		    latest_solve_time_ns,                                             //
		    Tcv_world_device,                                                 //
		    world_velocity,                                                   //
		    accel_bias,                                                       //
		    gyro_bias,                                                        //
		    accel_scale,                                                      //
		    std::span<const xrt_imu_sample>(samples_buf.data(), num_samples), //
		    first_after,                                                      //
		    out_relation);                                                    //
		out_time_ns = samples_buf[num_samples - 1].timestamp_ns;

		SF_TRACE(this, "Used dead reckoning for pose at ts %" PRIi64, requested_time_ns);
	} else {
		m_relation_history_get(this->device.relation_history, requested_time_ns, &out_relation);
		out_time_ns = requested_time_ns;

		SF_TRACE(this, "Used relation history for pose at ts %" PRIi64, requested_time_ns);
	}

	math_quat_rotate(&out_relation.pose.orientation, &Qcv_imu_model, &out_relation.pose.orientation);

	// Convert to OpenXR coordinate space before outputting
	math_pose_convert_from_opencv(&out_relation.pose, &out_relation.pose);
	math_vec3_convert_from_opencv(&out_relation.linear_velocity, &out_relation.linear_velocity);
	math_vec3_convert_from_opencv(&out_relation.angular_velocity, &out_relation.angular_velocity);

	SF_TRACE(this,
	         "Pose get from %" PRIi64 " to %" PRIi64
	         " succeeded.\n"
	         "Pose: %f,%f,%f\t%f,%f,%f,%f\n"
	         "Velocity: %f,%f,%f\t%f,%f,%f",
	         latest_solve_time_ns, out_time_ns, out_relation.pose.position.x, out_relation.pose.position.y,
	         out_relation.pose.position.z, out_relation.pose.orientation.x, out_relation.pose.orientation.y,
	         out_relation.pose.orientation.z, out_relation.pose.orientation.w, out_relation.linear_velocity.x,
	         out_relation.linear_velocity.y, out_relation.linear_velocity.z, out_relation.angular_velocity.x,
	         out_relation.angular_velocity.y, out_relation.angular_velocity.z);

	return;
}

void
SensorFusion::resetLocked()
{
	// Reset the active sensor fusion problem
	this->problem->reset();

	/*
	 * Dropping the window is a counter reset. Nothing reads a slot beyond `num_keyframes`, so the stale
	 * contents are unreachable and do not need wiping.
	 */
	this->keyframe_head = 0;
	this->num_keyframes = 0;

	this->waiting_on_imu.store(false, std::memory_order_relaxed);
	this->num_late_samples_dropped = 0;

	this->device.resetLocked();
}

void
SensorFusion::preintegrateDeviceLocked(bool &work_remaining, bool &wants_imu_wakeup)
{
	ImuBias<ImuBiasJet> bias = ImuBias<double>(map_vec3_f64(this->device.latest_accel_bias),  //
	                                           map_vec3_f64(this->device.latest_gyro_bias),   //
	                                           map_vec3_f64(this->device.latest_accel_scale)) //
	                               .seed();

	/*
	 * `age` names the keyframe an interval *ends* at, so the oldest keyframe in the window is skipped:
	 * there is no predecessor left in the window to integrate it from.
	 */
	for (uint32_t end_age = 0; end_age + 1 < this->num_keyframes; end_age++) {
		auto &preintegration = this->preintegrationLocked(end_age);

		// Doesn't need to be worked on
		if (!preintegration.dirty()) {
			continue;
		}

		const uint32_t start_age = end_age + 1;

		const auto &end_keyframe = this->keyframeLocked(end_age);
		Keyframe *start_keyframe = &this->keyframeLocked(start_age);

		if (end_keyframe.timestamp_ns == start_keyframe->timestamp_ns) {
			/*
			 * Nothing to integrate over. Identity carries a zero whitening matrix, so the factor
			 * contributes nothing rather than contributing something wrong.
			 */
			preintegration = DevicePreintegration::Identity(DevicePreintegrationState::Empty);
			continue;
		}

		assert(end_keyframe.timestamp_ns > start_keyframe->timestamp_ns);

		std::array<xrt_imu_sample, kMaxImuSamplesPerKeyframe> samples_buf;
		uint32_t num_samples = 0;
		xrt_imu_sample first_after;

		const ImuCollectResult result = collectImuSamplesForRange(device,                       //
		                                                          start_keyframe->timestamp_ns, //
		                                                          end_keyframe.timestamp_ns,    //
		                                                          samples_buf,                  //
		                                                          num_samples,                  //
		                                                          first_after);                 //

		switch (result) {
		case ImuCollectResult::Ok: break;

		case ImuCollectResult::Lapped:
			SF_WARN(this, "IMU ring lapped while collecting samples, fusion is running slow.");
			/*
			 * A retry can succeed once the fusion thread has caught up, but hammering it from the
			 * IMU path would only make it slower, so this waits for the next camera sample.
			 */
			work_remaining = true;
			continue;

		case ImuCollectResult::Overflow:
			SF_WARN(this,
			        "More than %u IMU samples between keyframes at %" PRIi64 " and %" PRIi64
			        ", leaving the interval unconstrained.",
			        kMaxImuSamplesPerKeyframe, start_keyframe->timestamp_ns, end_keyframe.timestamp_ns);

			/*
			 * Retrying cannot help, since the range only ever grows. Make the interval explicitly
			 * empty so that it's not going to be checked again.
			 */
			preintegration = DevicePreintegration::Identity(DevicePreintegrationState::Empty);
			continue;

		case ImuCollectResult::Truncated:
			SF_WARN(this,
			        "IMU samples between keyframes at %" PRIi64 " and %" PRIi64
			        " have already left the ring, leaving the interval unconstrained.",
			        start_keyframe->timestamp_ns, end_keyframe.timestamp_ns);

			// Same as an overflow, the missing samples are never coming back.
			preintegration = DevicePreintegration::Identity(DevicePreintegrationState::Empty);
			continue;

		case ImuCollectResult::NoSampleAfterEnd:
			/*
			 * The IMU has not reached the end of this interval yet. The sample that lands past it
			 * is what unblocks this, so ask pushImuSample() to wake us when one does.
			 */
			work_remaining = true;
			wants_imu_wakeup = true;
			continue;
		}

		/*
		 * preintegrate() needs at least a single sample to run correctly.
		 *
		 * @todo Have it work with zero samples if we have an IMU sample *after*.
		 */
		if (num_samples < 1) {
			SF_DEBUG(this,
			         "Only %u IMU samples between keyframes at %" PRIi64 " and %" PRIi64
			         ", leaving the interval unconstrained.",
			         num_samples, start_keyframe->timestamp_ns, end_keyframe.timestamp_ns);

			preintegration = DevicePreintegration::Identity(DevicePreintegrationState::Empty);
			continue;
		}

		preintegration.imu_preintegration = preintegrate(std::span<const xrt_imu_sample>(samples_buf.data(),
		                                                                                 num_samples), //
		                                                 first_after,                                  //
		                                                 bias,                                         //
		                                                 start_keyframe->timestamp_ns,                 //
		                                                 end_keyframe.timestamp_ns);                   //

		preintegration.state = DevicePreintegrationState::Integrated;

		SF_TRACE(this, "Preintegrated IMU between %" PRIi64 " and %" PRIi64 ". (dt %lf)",
		         start_keyframe->timestamp_ns, end_keyframe.timestamp_ns, preintegration.imu_preintegration.dt);
	}
}

bool
SensorFusion::tickLocked(bool have_new_input)
{
	if (!this->dirty) {
		return false;
	}

	/*
	 * Set by anything that wants to be run again, so that a preintegration left unfinished is retried rather than
	 * silently forgotten until the next camera sample happens along.
	 */
	bool work_remaining = false;
	/*
	 * Narrower than the above: only work that a *new IMU sample* can unblock, which is the only work worth having
	 * the IMU path wake us up for.
	 */
	bool wants_imu_wakeup = false;

	this->preintegrateDeviceLocked(work_remaining, wants_imu_wakeup);

	this->waiting_on_imu.store(wants_imu_wakeup, std::memory_order_relaxed);

	/*
	 * Deliberately not gated on `work_remaining`. buildLocked() already leaves an interval that has not integrated
	 * out of the problem, so the worst an unfinished one costs is that its device's newest keyframe sits out this
	 * solve and joins the next.
	 */
	bool run_fusion = false;
	if (have_new_input) {
		run_fusion = this->buildFusionLocked();
	}

	this->dirty = work_remaining;

	return run_fusion;
}

}; // namespace xrt::tracking::constellation::optimizer::sensor_fusion
