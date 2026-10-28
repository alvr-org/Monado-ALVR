// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Constellation tracker sensor fusion.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup tracking
 */

#pragma once

#include "xrt/xrt_config_build.h"

#include "os/os_threading.h"

#include "tracking/t_constellation.h"

#include "constellation/t_constellation_tracker.h"

#include "math/m_relation_history.h"

#include "imu_preintegration.hpp"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
#include <type_traits>


namespace xrt::tracking::constellation {

// Declared here rather than included, since the tracker's internal header is the one that includes this file.
struct CameraSample;
struct ConstellationTracker;

}; // namespace xrt::tracking::constellation

namespace xrt::tracking::constellation::optimizer::sensor_fusion {

using namespace xrt::auxiliary;
using namespace xrt::tracking;
using namespace xrt::tracking::constellation::optimizer;

//! How many keyframes to keep in our sliding window.
static constexpr uint32_t kSlidingWindowSize = 3;

//! The number of IMU samples to keep in our ring buffer
static constexpr uint32_t kImuBufferSize = 5000;

/*!
 * The most IMU samples we ever expect to find between two keyframes. At a 1KHz IMU and a keyframe every few tens of
 * milliseconds the real count is well under a hundred, so this is generous headroom rather than a tight bound. Sizing
 * the staging buffer off this instead of `kImuBufferSize` keeps it small enough to sit on the stack.
 */
static constexpr uint32_t kMaxImuSamplesPerKeyframe = 512;

/*!
 * How many newer keyframes have to pile up before a keyframe gets sealed and marginalized.
 */
static constexpr uint32_t kKeyframeSealLagKeyframes = 2;

static_assert(kKeyframeSealLagKeyframes < kSlidingWindowSize,
              "the oldest keyframe in the window has to be sealed by the time it is evicted, so that marginalization "
              "never has to fold an open keyframe into the prior");


/*!
 * How many camera samples may be queued for the fusion thread before new ones start being dropped.
 *
 * The fusion thread drains the whole queue every pass, so if this grows past even a handful, the thread has stalled a
 * lot and we should just warn and start dropping them to not use an ever-increasing amount of RAM.
 */
static constexpr uint32_t kMaxQueuedCameraSamples = 32;

struct DeviceObservation
{
	//! The camera that this observation is tied to
	uint32_t camera_idx;

	//! The camera pose at the timestamp
	xrt_pose Tcv_world_cam;

	//! The seed cam->device transform
	xrt_pose Tcv_cam_device_seed;

	//! 2D observations
	std::array<xrt_vec2, XRT_CONSTELLATION_MAX_LEDS_PER_DEVICE> points2d;
	//! 3D model correspondences
	std::array<xrt_vec3, XRT_CONSTELLATION_MAX_LEDS_PER_DEVICE> points3d;

	uint32_t num_points;
};

struct Keyframe
{
	//! The mosaic this keyframe corresponds to
	uint32_t mosaic_idx;
	//! The timestamp of this observation
	timepoint_ns timestamp_ns;

	/*!
	 * Set once this keyframe can no longer accept observations due to marginalization or age.
	 */
	bool sealed;

	std::array<DeviceObservation, XRT_TRACKING_MAX_CAMS> observations;
	uint32_t num_observations;
};

/*!
 * One IMU sample as it sits in @ref Device::imu_samples.
 *
 * The fusion thread reads these while the IMU thread is writing them, so the words are loaded and stored one at a time
 * through relaxed atomics. This can cause torn reads on the reader end, but it will not be UB.
 *
 * Relaxed is the right ordering for the words themselves. All the ordering that matters lives on
 * @ref Device::imu_write_count, whose release/acquire pair is what publishes a finished slot, plus a fence pair around
 * the words that lets a reader detect a slot being overwritten underneath it.
 */
struct RingImuSample
{
	static_assert(std::is_trivially_copyable_v<xrt_imu_sample>);
	static_assert(sizeof(xrt_imu_sample) % sizeof(uint64_t) == 0,
	              "the sample has to decompose into whole words to be stored and loaded atomically");
	static_assert(std::atomic<uint64_t>::is_always_lock_free,
	              "a locking atomic here would put a mutex back on the IMU push path");

	static constexpr size_t kNumWords = sizeof(xrt_imu_sample) / sizeof(uint64_t);

	//! The word `xrt_imu_sample::timestamp_ns` lives in, which is the only one the collector's bisection needs.
	static constexpr size_t kTimestampWord = offsetof(xrt_imu_sample, timestamp_ns) / sizeof(uint64_t);

	std::array<std::atomic<uint64_t>, kNumWords> words;

	void
	store(const xrt_imu_sample &sample) noexcept
	{
		std::array<uint64_t, kNumWords> raw;
		std::memcpy(raw.data(), &sample, sizeof(sample));

		for (size_t i = 0; i < kNumWords; i++) {
			this->words[i].store(raw[i], std::memory_order_relaxed);
		}
	}

	xrt_imu_sample
	load() const noexcept
	{
		std::array<uint64_t, kNumWords> raw;
		for (size_t i = 0; i < kNumWords; i++) {
			raw[i] = this->words[i].load(std::memory_order_relaxed);
		}

		xrt_imu_sample sample;
		std::memcpy(&sample, raw.data(), sizeof(sample));
		return sample;
	}

	//! Just the timestamp, so the bisection does not pull in six words it is only going to throw away.
	timepoint_ns
	loadTimestamp() const noexcept
	{
		const uint64_t raw = this->words[kTimestampWord].load(std::memory_order_relaxed);

		timepoint_ns timestamp;
		std::memcpy(&timestamp, &raw, sizeof(timestamp));
		return timestamp;
	}
};

enum class DevicePreintegrationState
{
	//! Needs to be recomputed
	Dirty,
	//! Does not refer to any real pre-integration. This term contributes nothing and can be skipped.
	Empty,
	//! This preintegration is complete and will never change.
	Integrated,
};

/*!
 * Stores state for a single preintegration residual for a device.
 */
struct DevicePreintegration
{
	PreintegratedImuSamples imu_preintegration;

	DevicePreintegrationState state;

	bool
	dirty() const
	{
		return this->state == DevicePreintegrationState::Dirty;
	}

	static DevicePreintegration
	Identity(DevicePreintegrationState state)
	{
		return {
		    .imu_preintegration = PreintegratedImuSamples::Identity(),
		    .state = state,
		};
	}
};

/*!
 * The IMU intrinsics the rig calibration happened to solve for, as the fusion takes them in as a seed.
 *
 * A copy of @ref rig_calibration::ImuCalibration rather than that type itself.
 */
struct DeviceImuCalibration
{
	//! Accelerometer bias, in meters per second squared.
	xrt_vec3_f64 accel_bias;
	//! Gyroscope bias, in radians per second.
	xrt_vec3_f64 gyro_bias;
	//! Per-axis accelerometer scale, unitless.
	xrt_vec3_f64 accel_scale;

	//! The IMU -> LED model rotation.
	xrt_quat Qcv_imu_model;
};

struct Device
{
public: // Fields
	const t_constellation_device_id_t id;

	const t_constellation_tracker_led_model *const led_model;

	bool has_latest_solve{false};
	timepoint_ns latest_solve_time_ns;
	xrt_pose Tcv_world_device_latest;

	//! World velocity, in meters per second.
	xrt_vec3_f64 latest_world_velocity;

	//! Accelerometer bias, in meters per second squared.
	xrt_vec3_f64 latest_accel_bias;
	//! Gyroscope bias, in radians per second.
	xrt_vec3_f64 latest_gyro_bias;
	//! Per-axis accelerometer scale, unitless.
	xrt_vec3_f64 latest_accel_scale;
	//! The IMU -> constellation offset
	xrt_quat latest_Qcv_imu_model;

	//! The relation history of the device, for historical poses older than the latest pose.
	m_relation_history *relation_history;

	//! Pre-calibrated accelerometer bias, in meters per second squared.
	xrt_vec3_f64 calibrated_accel_bias;
	//! Pre-calibrated gyroscope bias, in radians per second.
	xrt_vec3_f64 calibrated_gyro_bias;
	//! Pre-calibrated per-axis accelerometer scale, unitless.
	xrt_vec3_f64 calibrated_accel_scale;
	//! The calibrated IMU -> constellation offset
	xrt_quat calibrated_Qcv_imu_model;

	/*!
	 * The preintegrated IMU samples between keyframes, as a ring running parallel to @ref
	 * SensorFusion::keyframe_storage: slot `i` holds the interval that *ends* at the keyframe in slot `i`.
	 *
	 * Sharing the keyframe ring's indexing is what makes a window shift free for these too. The slot belonging to
	 * the oldest keyframe in the window has no predecessor to integrate from and is simply never read. Index this
	 * through @ref SensorFusion::preintegrationLocked rather than directly.
	 */
	std::array<DevicePreintegration, kSlidingWindowSize> preintegrations;

	/*!
	 * The total number of IMU samples ever pushed for this device. Absolute sample `n` lives in ring slot
	 * `n % kImuBufferSize`, so this is both the write pointer and the count of samples that have ever been
	 * written. Incrementing forever means there's no wraparound behaviour to worry about when the reader gets
	 * lapped by the writer.
	 *
	 * Only the IMU push thread writes this, and it publishes it with a release store once the slot it names has
	 * been filled. Readers must load it with acquire before touching `imu_samples`, and re-load it after an acquire
	 * fence to confirm the writer did not overwrite what they just read; the writer issues the matching release
	 * fence before it starts overwriting a slot.
	 */
	std::atomic<uint64_t> imu_write_count;

	std::atomic<timepoint_ns> latest_imu_ts_ns;

	//! The buffered IMU samples, indexed by absolute sample index modulo `kImuBufferSize`.
	std::array<RingImuSample, kImuBufferSize> imu_samples;

public: // Methods
	Device(t_constellation_device_id_t id, const t_constellation_tracker_led_model *led_model);
	~Device();

	Device(const Device &) = delete;
	Device &
	operator=(const Device &) = delete;

	/*!
	 * Resets the device's state to a fresh state, assuming that the biases were still good.
	 */
	void
	resetLocked();
};

struct CameraDescription
{
public: // Fields
	size_t mosaic_idx;
	size_t camera_idx;

	//! Intrinsics and distortion, which is what turns a LED in the device's frame into a pixel.
	camera_model params;
};

/*
 * Solve snapshot
 *
 * A description of one solve that stands still while the next pass moves the window on underneath it. The fusion
 * thread owns the window outright.
 */

//! A device's solved state at one keyframe.
struct SolvedDeviceKeyframe
{
	//! Solved world -> IMU pose.
	xrt_pose Tcv_world_device;
	//! Solved world-frame velocity, in meters per second.
	xrt_vec3_f64 velocity;

	//! Solved accelerometer bias, in meters per second squared.
	xrt_vec3_f64 accel_bias;
	//! Solved gyroscope bias, in radians per second.
	xrt_vec3_f64 gyro_bias;
	//! Solved per-axis accelerometer scale, unitless.
	xrt_vec3_f64 accel_scale;
};

/*!
 * One camera's view of one device at one keyframe.
 *
 * Carries the camera intrinsics alongside the correspondences so that whatever consumes this can reproject the solved
 * pose without having to reach back into @ref SensorFusion::cameras for them.
 */
struct SnapshotObservation
{
	uint32_t mosaic_idx;
	uint32_t camera_idx;

	xrt_pose Tcv_world_cam;
	xrt_pose Tcv_cam_device_seed;

	camera_model params;

	uint32_t num_points;
	std::array<xrt_vec2, XRT_CONSTELLATION_MAX_LEDS_PER_DEVICE> points2d;
	std::array<xrt_vec3, XRT_CONSTELLATION_MAX_LEDS_PER_DEVICE> points3d;
};

struct SnapshotKeyframe
{
	timepoint_ns timestamp_ns;
	uint32_t mosaic_idx;
	bool sealed;

	uint32_t num_observations;
	std::array<SnapshotObservation, XRT_TRACKING_MAX_CAMS> observations;

	SolvedDeviceKeyframe device;
};

struct SnapshotDevice
{
	t_constellation_device_id_t id;

	//! A bit per keyframe age the solve held a state for.
	uint32_t keyframe_mask;

	//! Solved IMU -> LED model rotation.
	xrt_quat Qcv_imu_model;

	//! The IMU -> LED model rotation from the rig calibration, for measuring how far the solve has walked from it.
	xrt_quat calibrated_Qcv_imu_model;
};

/*!
 * One solve, whole: what went in, what came out, and how the solver got there.
 *
 * Filled in two halves. @ref SensorFusion::buildFusionLocked copies the window in as the problem is built, and
 * @ref SensorFusion::runFusionLocked fills the solved values and solver statistics once Ceres has returned.
 */
struct SolveSnapshot
{
	//! Counts solves over the life of the fusion, and is what the logger uses as a sequence timeline.
	uint64_t solve_index{0};

	uint32_t num_keyframes;
	//! Newest first, so index `i` is keyframe age `i`.
	std::array<SnapshotKeyframe, kSlidingWindowSize> keyframes;

	SnapshotDevice device;

	//! Solved gravity magnitude, in meters per second squared.
	double world_gravity_mag{0.0};

	uint64_t num_late_samples_dropped{0};

	int num_residuals{0};
	int num_parameter_blocks{0};
	uint32_t num_iterations{0};

	//! Residual RMS in standard deviations, before and after the solve. See @ref kTargetResidualRmsSigma.
	double initial_rms_sigma{0.0};
	double final_rms_sigma{0.0};

	double solve_time_s{0.0};

	//! Ceres' termination type, already stringified, since the logger has no business including Ceres headers.
	std::string termination;
};

struct SensorFusion
{
public: // Fields
	u_logging_level log_level;

	/*!
	 * The fusion thread, and the one lock in this file, guarding data handoff only, not heavy operations. The IMU
	 * or camera push functions should not hold a lock that will be held during an expensive operation (such as a
	 * Ceres solve).
	 */
	os_thread_helper thread{};

	/*!
	 * Backing store for the keyframe ring. The keyframe with absolute index `n` lives in slot
	 * `n % kSlidingWindowSize`.
	 *
	 * Reach these through @ref keyframeLocked instead of indexing here, and never move the entries around.
	 */
	std::array<Keyframe, kSlidingWindowSize> keyframe_storage{};

	//! One past the absolute index of the newest keyframe, so the newest keyframe is `keyframe_head - 1`.
	uint64_t keyframe_head{0};

	//! How many slots of the ring hold real keyframes, saturating at `kSlidingWindowSize`.
	uint32_t num_keyframes{0};

	//! The cameras that are present in the system
	std::array<std::array<CameraDescription, XRT_TRACKING_MAX_CAMS>, XRT_CONSTELLATION_MAX_TRACKING_MOSAICS>
	    cameras{};

	Device device;

	/*!
	 * Camera samples handed over by @ref pushCameraSample, in arrival order, waiting to be folded into the
	 * window by the fusion thread.
	 */
	std::vector<constellation::CameraSample> sample_queue;

	/*!
	 * How many samples have been thrown away for arriving with @ref sample_queue already at
	 * @ref kMaxQueuedCameraSamples.
	 *
	 * If this grows, it indicates the fusion thread has likely stalled or is running too slowly.
	 */
	uint64_t num_queued_samples_dropped{0};

	//! Whether the current state is dirty and we need to run an optimization
	bool dirty{false};

	/*!
	 * Set when a tick had to park a preintegration purely because the IMU had not reached the end of its interval
	 * yet.
	 *
	 * Read by `pushImuSample()` without the lock, which is why it is atomic: the sample that closes the interval
	 * is the thing that unblocks the tick, so it is also the thing that has to wake it up.
	 *
	 * @note This only hurries the interval along so it is ready for the next solve. It deliberately does *not*
	 *       hold the solve back, see @ref SensorFusion::tickLocked.
	 */
	std::atomic<bool> waiting_on_imu{false};

	//! How many camera samples have been dropped for turning up after their keyframe was already sealed.
	uint64_t num_late_samples_dropped{0};

	//! The active Ceres problem.
	struct SensorFusionProblem *problem{nullptr};

#ifdef XRT_FEATURE_RERUN
	/*!
	 * Rerun logging, null unless enabled.
	 */
	std::unique_ptr<struct RerunContext> rerun_context{nullptr};
#endif

	/*!
	 * The window as it stood for the most recent solve, kept alive between the build and the solve.
	 *
	 * This is owned in full by the fusion thread, don't touch it outside of that.
	 */
	SolveSnapshot snapshot;

private: // Methods
	void
	markDirtyLocked();

	/*!
	 * Retires the keyframe falling out of the window and opens a fresh one at the head of the ring.
	 *
	 * @return The new keyframe, which is the newest in the window and starts out open.
	 */
	Keyframe &
	pushKeyframeLocked(const constellation::CameraSample &sample);

	//! Seals every keyframe that has aged past @ref kKeyframeSealLagKeyframes.
	void
	sealAgedKeyframesLocked();

	//! @return `true` when the built problem has something to solve.
	bool
	buildFusionLocked();

	/*!
	 * Rebuilds the problem, without solving it, so that an eviction can marginalize against the window as it
	 * stands.
	 *
	 * Needed when an earlier eviction in the same pass has already marginalized against the current problem: the
	 * prior it produced lives outside that problem, so a second eviction against it would drop the prior.
	 */
	void
	rebuildForEvictionLocked();

	/*!
	 * Integrates every dirty IMU interval of the device.
	 *
	 * @param[in,out] work_remaining   Set when an interval could not be integrated yet and has to be retried.
	 * @param[in,out] wants_imu_wakeup Set when only a newer IMU sample can unblock an interval.
	 */
	void
	preintegrateDeviceLocked(bool &work_remaining, bool &wants_imu_wakeup);

	//! Triggers the sensor fusion to reset it's state
	void
	resetLocked();

public: // Methods
	SensorFusion(t_constellation_device_id_t id, const t_constellation_tracker_led_model *led_model);

	~SensorFusion();

	//! Returns the index of the keyframe in the ring buffer.
	size_t
	keyframeIdxLocked(uint32_t age) const;

	/*!
	 * The age of the keyframe currently living in ring slot @p idx, or `false` when no keyframe in the window
	 * does.
	 */
	bool
	keyframeAgeForIdxLocked(size_t idx, uint32_t &out_age) const;

	/*!
	 * The keyframe `age` steps back from the newest, so age 0 is the newest keyframe in the window and
	 * `num_keyframes - 1` is the oldest.
	 */
	Keyframe &
	keyframeLocked(uint32_t age);

	const Keyframe &
	keyframeLocked(uint32_t age) const;

	//! The preintegration covering the interval that *ends* at `keyframeLocked(age)`.
	DevicePreintegration &
	preintegrationLocked(uint32_t age);

	//! Adds a camera to the sensor fusion
	void
	addCamera(const CameraDescription &camera_description);

	/*!
	 * Hands a device the IMU intrinsics a rig calibration solved for. This is used as a better seed start than
	 * assuming an identity calibration.
	 */
	void
	setDeviceImuCalibration(const DeviceImuCalibration &calibration);

	/*!
	 * Hands a complete constellation camera sample to the fusion thread. Must be called with
	 * `tracker->device_lock` held.
	 *
	 * Does no work beyond the copy onto @ref sample_queue: everything the sample goes on to do to the window
	 * happens on the fusion thread, so a camera thread never blocks on anything more than the queue push.
	 */
	void
	pushCameraSample(constellation::ConstellationTracker *tracker, const constellation::CameraSample &sample);

	/*!
	 * Pushes an IMU sample for this particular device, IMU samples should be in OpenXR coordinate space. IMU biases
	 * found during rig calibration should not be applied to these samples.
	 */
	void
	pushImuSample(const xrt_imu_sample &sample);

	//! Provides the latest unpredicted device pose for the requested timestamp.
	void
	getTrackedPose(timepoint_ns requested_time_ns, timepoint_ns &out_time_ns, xrt_space_relation &out_relation);

	/*!
	 * Folds one queued camera sample into the window, or drops if it's no longer able to be merged in.
	 *
	 * This is the far side of @ref sample_queue, and the only thing that grows the window.
	 */
	void
	processCameraSampleLocked(const constellation::CameraSample &sample);

	/*!
	 * Runs preintegration, and when @p have_new_input says the window has gained something to solve over, builds
	 * the problem.
	 *
	 * @param have_new_input Whether this pass drained any camera samples, rather than being woken by an IMU
	 *                       sample arriving to unpark a preintegration. Preintegration runs either way; only new
	 *                       input is worth a solve, which is what keeps the solve rate tied to the cameras
	 *                       rather than to the IMU.
	 *
	 * @returns `true` if the problem needs to be run, meaning the built problem holds at least one residual.
	 */
	bool
	tickLocked(bool have_new_input);

	/*!
	 * Solves the problem `tickLocked` built and publishes what came out of it.
	 *
	 * Releases the lock while the expensive part (running the solve) executes to let the lock be held by other
	 * things when it's safe.
	 */
	void
	runFusionLocked();
};

}; // namespace xrt::tracking::constellation::optimizer::sensor_fusion
