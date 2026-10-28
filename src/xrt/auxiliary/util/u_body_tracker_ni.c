// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Not implemented function helpers for @ref xrt_body_tracker.
 * @ingroup aux_util
 */

#include "util/u_body_tracker_ni.h"
#include "util/u_logging.h"


#define E(FN) U_LOG_E("Function " #FN " is not implemented for this body tracker implementation")

xrt_result_t
u_body_tracker_ni_locate(struct xrt_body_tracker *xbt,
                         struct xrt_space_overseer *xso,
                         struct xrt_space *base_space,
                         const struct xrt_pose *base_offset,
                         int64_t at_timestamp_ns,
                         struct xrt_body_tracker_location *out_location)
{
	(void)xbt;
	(void)xso;
	(void)base_space;
	(void)base_offset;
	(void)at_timestamp_ns;
	(void)out_location;
	E(locate);
	return XRT_ERROR_NOT_IMPLEMENTED;
}

xrt_result_t
u_body_tracker_ni_get_skeleton(struct xrt_body_tracker *xbt, struct xrt_body_skeleton *out_skeleton)
{
	(void)xbt;
	(void)out_skeleton;
	E(get_skeleton);
	return XRT_ERROR_NOT_IMPLEMENTED;
}

xrt_result_t
u_body_tracker_ni_reset_calibration_meta(struct xrt_body_tracker *xbt)
{
	(void)xbt;
	E(reset_calibration_meta);
	return XRT_ERROR_NOT_IMPLEMENTED;
}

xrt_result_t
u_body_tracker_ni_set_calibration_override_meta(struct xrt_body_tracker *xbt, float body_height)
{
	(void)xbt;
	(void)body_height;
	E(set_calibration_override_meta);
	return XRT_ERROR_NOT_IMPLEMENTED;
}

xrt_result_t
u_body_tracker_ni_set_fidelity_meta(struct xrt_body_tracker *xbt, enum xrt_body_tracking_fidelity_meta fidelity)
{
	(void)xbt;
	(void)fidelity;
	E(set_fidelity_meta);
	return XRT_ERROR_NOT_IMPLEMENTED;
}
