// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Helpers for fixed arrays of @ref xrt_image_native.
 * @ingroup aux_util
 */

#pragma once

#include "xrt/xrt_compositor.h"

#include "util/u_handles.h"
#include "util/u_misc.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Initialise every entry to cleared / invalid handles, the structs should not
 * have been populated with valid values before calling this function.
 *
 * @ingroup aux_util
 */
static inline void
u_native_images_init_invalid(struct xrt_image_native *images, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		U_ZERO(&images[i]);
		images[i].handle = XRT_GRAPHICS_BUFFER_HANDLE_INVALID;
	}
}

/*!
 * Unref every buffer handle and clear each entry, recommended to have called
 * @ref u_native_images_init_invalid before calling this function.
 *
 * @ingroup aux_util
 */
static inline void
u_native_images_unref_all(struct xrt_image_native *images, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		u_graphics_buffer_unref(&images[i].handle);
		U_ZERO(&images[i]);
	}
}

#ifdef __cplusplus
}
#endif
