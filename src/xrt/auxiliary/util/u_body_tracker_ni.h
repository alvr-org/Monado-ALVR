// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Not implemented function helpers for @ref xrt_body_tracker.
 * @ingroup aux_util
 */

#pragma once

#include "xrt/xrt_body_tracker.h"


#ifdef __cplusplus
extern "C" {
#endif


xrt_result_t
u_body_tracker_ni_locate(struct xrt_body_tracker *xbt,
                         struct xrt_space_overseer *xso,
                         struct xrt_space *base_space,
                         const struct xrt_pose *base_offset,
                         int64_t at_timestamp_ns,
                         struct xrt_body_tracker_location *out_location);

xrt_result_t
u_body_tracker_ni_get_skeleton(struct xrt_body_tracker *xbt, struct xrt_body_skeleton *out_skeleton);

xrt_result_t
u_body_tracker_ni_reset_calibration_meta(struct xrt_body_tracker *xbt);

xrt_result_t
u_body_tracker_ni_set_calibration_override_meta(struct xrt_body_tracker *xbt, float body_height);

xrt_result_t
u_body_tracker_ni_set_fidelity_meta(struct xrt_body_tracker *xbt, enum xrt_body_tracking_fidelity_meta fidelity);


#ifdef __cplusplus
}
#endif
