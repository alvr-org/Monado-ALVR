// Copyright 2022-2023, Collabora, Ltd.
// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Tracking-origin setup helpers for @ref xrt_builder implementations.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_util
 */

#pragma once

#include "xrt/xrt_prober.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Helper function for setting up tracking origins. Applies 3dof offsets for devices with XRT_TRACKING_TYPE_NONE.
 *
 * @ingroup aux_util
 */
void
u_builder_setup_tracking_origins(struct xrt_device *head,
                                 struct xrt_device *eyes,
                                 struct xrt_device *left,
                                 struct xrt_device *right,
                                 struct xrt_device *gamepad,
                                 struct xrt_vec3 *global_tracking_origin_offset);

#ifdef __cplusplus
}
#endif
