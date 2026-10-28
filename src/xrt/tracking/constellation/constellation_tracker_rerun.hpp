// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Internal header for constellation tracker usage of the constellation tracker rerun logging.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup tracking
 */

#pragma once

// @note In general usage, we don't use the internal header, rerun is an opt-in debugging feature, so we are OK using
//       internal APIs here to keep code clean and non-invasive. Please look at target_builder_rift.c instead for normal
//       usage of these APIs.
#include "t_constellation_tracker_internal.hpp"
#include "constellation_rerun_common.hpp"

#include <rerun.hpp>


namespace xrt::tracking::constellation {

const std::string rerun_recording_id = "constellation_tracking";

struct RerunContext : public RerunStream
{
public: // Methods
	RerunContext() : RerunStream(rerun_recording_id) {}

	/*!
	 * Log a camera sample to rerun.
	 *
	 * @remarks Call with `tracker->device_lock` held.
	 *
	 * @param tracker       The constellation tracker.
	 * @param camera_sample The camera sample to log.
	 */
	void
	logSample(const ConstellationTracker &tracker, const CameraSample &camera_sample);

	/*!
	 * Logs a camera's image frame to rerun.
	 *
	 * @param mosaic_index The index of the mosaic the camera belongs to.
	 * @param camera_index The index of the camera in the mosaic.
	 * @param frame        The camera's image frame.
	 */
	void
	logImageFrame(const ConstellationTracker &tracker,
	              uint32_t mosaic_index,
	              uint32_t camera_index,
	              const xrt_frame &frame);

private: // Methods
	void
	logStaticScene(const CameraSample &camera_sample, const t_camera_calibration &calibration);

	void
	logLedModel(const std::string &entity_name,
	            t_constellation_device_id_t device_id,
	            const t_constellation_tracker_led_model &led_model,
	            bool prior);

	void
	logBlobSet(const CameraSample &camera_sample);

	void
	logFrameCameraMetrics(const CameraSample &camera_sample);

	void
	logFrameDeviceMetrics(const CameraSample &camera_sample, const DeviceState &device_state);

	void
	logFoundPose(const CameraSample &camera_sample,
	             const std::unique_ptr<Device> &device,
	             const FoundDevicePose &found_pose);
};

}; // namespace xrt::tracking::constellation
