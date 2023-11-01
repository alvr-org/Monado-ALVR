// Copyright 2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Builder helpers for Vive/Index devices.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_vive
 */

#pragma once

#include "xrt/xrt_prober.h"


#ifdef __cplusplus
extern "C" {
#endif


/*!
 * Helper function to do an estimate of a system.
 *
 * @ingroup aux_vive
 */
xrt_result_t
vive_builder_estimate(struct xrt_prober *xp,
                      bool have_6dof,
                      bool have_hand_tracking,
                      bool *out_valve_have_index,
                      struct xrt_builder_estimate *out_estimate);


#ifdef __cplusplus
}
#endif
