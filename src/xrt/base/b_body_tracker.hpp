// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief C++ base implementation of @ref xrt_body_tracker.
 * @ingroup base
 */

#pragma once

#include "glue/g_body_tracker.hpp"

#include "b_body_tracker.h"


class BodyTracker : public xrt::util::BodyTrackerBase<BodyTracker>
{
public: // Methods
	BodyTracker(const xrt_body_tracker_create_info &info, xrt_device *xdev);

	xrt_result_t
	locate(xrt_space_overseer *xso,
	       xrt_space *base_space,
	       const xrt_pose *base_offset,
	       int64_t at_timestamp_ns,
	       xrt_body_tracker_location *out_location);

	xrt_result_t
	getSkeleton(xrt_body_skeleton *out_skeleton);

	xrt_result_t
	resetCalibrationMeta();

	xrt_result_t
	setCalibrationOverrideMeta(float body_height);

	xrt_result_t
	setFidelityMeta(enum xrt_body_tracking_fidelity_meta fidelity);

	static void
	destroyBodyTracker(struct xrt_body_tracker *xbt);


private: // Members
	xrt_body_tracker_create_info mInfo{};
	xrt_device *mXdev{nullptr};
};
