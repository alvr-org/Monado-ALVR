// Copyright 2018-2024, Collabora, Ltd.
// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Hand tracking objects and functions.
 * @author Christoph Haag <christoph.haag@collabora.com>
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup oxr_main
 */

#pragma once

#include "xrt/xrt_hand_tracker.h"
#include "xrt/xrt_openxr_includes.h"

#include "oxr_handle_base.h"


/*
 *
 * Structs and defines.
 *
 */

struct oxr_session;

/*!
 * A hand tracker.
 *
 * Parent type/handle is @ref oxr_instance
 *
 *
 * @obj{XrHandTrackerEXT}
 * @extends oxr_handle_base
 */
struct oxr_hand_tracker
{
	//! Common structure for things referred to by OpenXR handles.
	struct oxr_handle_base handle;

	//! Owner of this hand tracker.
	struct oxr_session *sess;

	//! XRT hand tracker backing this OpenXR handle.
	struct xrt_hand_tracker *xht;

	//! Whether any sources were requested by the application.
	bool has_requested_sources;

	XrHandEXT hand;
	XrHandJointSetEXT hand_joint_set;
};


/*
 *
 * Functions.
 *
 */

XrResult
oxr_hand_tracker_create(struct oxr_logger *log,
                        struct oxr_session *sess,
                        const XrHandTrackerCreateInfoEXT *createInfo,
                        struct xrt_device *override_xdev,
                        struct oxr_hand_tracker **out_hand_tracker);

XrResult
oxr_hand_tracker_joints(struct oxr_logger *log,
                        struct oxr_hand_tracker *hand_tracker,
                        const XrHandJointsLocateInfoEXT *locateInfo,
                        XrHandJointLocationsEXT *locations);

XrResult
oxr_hand_tracker_apply_force_feedback(struct oxr_logger *log,
                                      struct oxr_hand_tracker *hand_tracker,
                                      const XrForceFeedbackCurlApplyLocationsMNDX *locations);
