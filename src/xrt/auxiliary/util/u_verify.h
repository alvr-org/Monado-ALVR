// Copyright 2021, Collabora, Ltd.
// Copyright 2021, Moses Turner.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Tiny file to verify things
 * @author Moses Turner <mosesturner@protonmail.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_util
 */

#pragma once
#include "xrt/xrt_defines.h"
#include "xrt/xrt_device.h"
#include "xrt/xrt_compositor.h"

static inline bool
u_verify_blend_mode_valid(enum xrt_blend_mode blend_mode)
{
	return ((blend_mode == XRT_BLEND_MODE_OPAQUE) || (blend_mode == XRT_BLEND_MODE_ADDITIVE) ||
	        (blend_mode == XRT_BLEND_MODE_ALPHA_BLEND));
}

static inline bool
u_verify_blend_mode_supported(struct xrt_device *xdev, enum xrt_blend_mode blend_mode)
{
	for (size_t i = 0; i < xdev->hmd->blend_mode_count; i++) {
		if (xdev->hmd->blend_modes[i] == blend_mode) {
			return true;
		}
	}
	return false;
}

static inline bool
u_verify_blend_factor_valid(enum xrt_blend_factor blend_factor)
{
	return (blend_factor == XRT_BLEND_FACTOR_ZERO) || (blend_factor == XRT_BLEND_FACTOR_ONE) ||
	       (blend_factor == XRT_BLEND_FACTOR_SRC_ALPHA) || (blend_factor == XRT_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA) ||
	       (blend_factor == XRT_BLEND_FACTOR_DST_ALPHA) || (blend_factor == XRT_BLEND_FACTOR_ONE_MINUS_DST_ALPHA);
}
