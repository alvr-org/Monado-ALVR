// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Internal header for sensor fusion usage of rerun logging.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup tracking
 */

#pragma once

#include "constellation/constellation_rerun_common.hpp"
#include "sensor_fusion.hpp"

#include <rerun.hpp>


namespace xrt::tracking::constellation::optimizer::sensor_fusion {

//! Whether to log older keyframes or just the newest.
constexpr bool kLogOldKeyframes = false;

const std::string_view kRerunRecordingId = "constellation_sensor_fusion";

struct RerunContext : public RerunStream
{
public: // Methods
	RerunContext() : RerunStream(kRerunRecordingId) {}

	/*!
	 * Logs one sliding-window solve to rerun.
	 *
	 * Everything is logged against two timelines: a `solves` sequence, which is the only one that advances
	 * monotonically, and a `fusion_time` timestamp taken from the newest keyframe in the window.
	 *
	 * @param snapshot The window and its solved state, as captured around @ref SensorFusion::runFusionLocked.
	 */
	void
	logSolve(const SolveSnapshot &snapshot);

private: // Methods
	void
	logStaticScene();

	void
	logSolveSummary(const SolveSnapshot &snapshot);

	void
	logSolverMetrics(const SolveSnapshot &snapshot);

	void
	logCameras(const SolveSnapshot &snapshot);

	void
	logDevice(const SolveSnapshot &snapshot);

	void
	logDeviceWindow(const SolveSnapshot &snapshot);

	void
	logDeviceMetrics(const SolveSnapshot &snapshot);

	void
	logObservations(const SolveSnapshot &snapshot);
};

}; // namespace xrt::tracking::constellation::optimizer::sensor_fusion
