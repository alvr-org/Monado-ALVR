// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Helpers for runtime interaction profile binding data.
 * @ingroup oxr_main
 */

#pragma once

#include "xrt/xrt_device.h"

#include "oxr_binding_point.h"
#include "oxr_extension_support.h"
#include "oxr_forward_declarations.h"


#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Runtime interaction profile binding data cloned from generated templates.
 */
struct oxr_binding_profile
{
	//! Owning registry.
	struct oxr_binding_registry *registry;
	//! Immutable generated source template.
	const struct oxr_profile_template *templ;

	//! Runtime XrPath for the interaction profile.
	XrPath path;
	//! XRT device name associated with the generated profile template.
	enum xrt_device_name device_name;
	//! User-facing localized profile name.
	const char *localized_name;

	//! All generated binding points for this profile.
	struct oxr_binding_point *binding_points;
	size_t binding_point_count;

	//! All generated DPAD emulations for this profile.
	struct oxr_dpad_emulation *dpad_emulations;
	size_t dpad_emulation_count;

	//! Maps every generated binding point XrPath to the first matching point.
	struct u_hashmap_int *binding_points_by_path;
};

/*!
 * Find a binding point in @p profile by its runtime binding point path.
 *
 * @memberof oxr_binding_profile
 */
bool
oxr_binding_profile_find_binding_point(const struct oxr_binding_profile *profile,
                                       XrPath path,
                                       const struct oxr_binding_point **out_binding_point);


#ifdef __cplusplus
}
#endif
