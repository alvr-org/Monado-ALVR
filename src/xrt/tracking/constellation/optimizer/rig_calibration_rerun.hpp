// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Internal header for sensor calibration usage of rerun logging.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup tracking
 */

#pragma once

#include "rig_calibration.hpp"
#include "constellation/constellation_rerun_common.hpp"

#include <rerun.hpp>


namespace xrt::tracking::constellation::optimizer::rig_calibration {

const std::string rerun_recording_id = "rig_calibration";

struct RerunContext : public RerunStream
{
public: // Methods
	RerunContext() : RerunStream(rerun_recording_id) {}

	/*!
	 * Logs the result of an rig calibration solve to rerun.
	 *
	 * The solved camera poses, the device trajectory and the solve summary are logged statically, so a later
	 * solve replaces them. Per-keyframe data is logged on a timeline keyed by the keyframe timestamps.
	 *
	 * @param result The result of the solve.
	 */
	void
	logResult(const CalibrationResult &result);

private: // Methods
	void
	logStaticScene();

	void
	logSolveSummary(const CalibrationResult &result);

	void
	logCameras(const CalibrationResult &result);

	void
	logDeviceTrajectory(const CalibrationResult &result);

	void
	logKeyframe(const CalibrationResult &result, const KeyframeCalibration &keyframe);

	void
	logKeyframeObservations(const CalibrationResult &result, const KeyframeCalibration &keyframe);

	void
	logKeyframeMetrics(const CalibrationResult &result, const KeyframeCalibration &keyframe);
};

}; // namespace xrt::tracking::constellation::optimizer::rig_calibration