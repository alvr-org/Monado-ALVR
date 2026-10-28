// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Runtime OpenXR-level rebinding profile data.
 * @ingroup oxr_main
 */

#pragma once

#include "xrt/xrt_device.h"

#include "oxr_extension_support.h"
#include "oxr_forward_declarations.h"
#include "oxr_generated_bindings.h"


#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Single source-to-target binding path mapping in a rebinding profile.
 */
struct oxr_rebinding
{
	XrPath source_path;
	XrPath target_path;
};

/*!
 * Maps binding paths from one interaction profile onto another.
 *
 * Used when an app suggests bindings for a source profile that the device does
 * not expose directly. Listed source paths are remapped to the target profile;
 * unmapped paths are dropped.
 *
 * This struct is never mutated after creation from the templates.
 */
struct oxr_rebinding_profile
{
	//! Immutable generated source template.
	const struct oxr_rebinding_profile_template *templ;
	//! Source interaction profile path.
	XrPath source_profile_path;
	//! Target interaction profile path.
	XrPath target_profile_path;

	//! Source-to-target binding path mappings.
	struct oxr_rebinding *bindings;
	size_t binding_count;
};

/*!
 * Find a target binding path in a rebinding profile for @p source_path.
 *
 * @memberof oxr_rebinding_profile
 */
bool
oxr_rebinding_profile_find_target(const struct oxr_rebinding_profile *profile,
                                  XrPath source_path,
                                  XrPath *out_target_path);


#ifdef __cplusplus
}
#endif
