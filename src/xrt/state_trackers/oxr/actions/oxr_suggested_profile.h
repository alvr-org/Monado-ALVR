// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Suggested interaction profile state.
 * @ingroup oxr_main
 */

#pragma once

#include "xrt/xrt_device.h"

#include "oxr_extension_support.h"
#include "oxr_dpad_state.h"
#include "oxr_generated_bindings.h"

#include <stddef.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif


/*!
 * One action-to-path binding suggested by the OpenXR client.
 *
 * This is the client's suggestion: bind this action to this path on an
 * interaction profile. It is created from xrSuggestInteractionProfileBindings
 * and grouped on @ref oxr_suggested_profile. The @ref binding_point field
 * points at the immutable @ref oxr_binding_point the suggested path resolved
 * to in the @ref oxr_binding_registry.
 */
struct oxr_suggested_binding
{
	//! Action this suggestion applies to, from @ref oxr_action::act_key.
	uint32_t act_key;
	//! Binding path suggested by the client.
	XrPath path;
	//! Registry binding point this client suggestion points to.
	const struct oxr_binding_point *binding_point;
};

/*!
 * Client-suggested bindings for one OpenXR interaction profile.
 *
 * Groups the @ref oxr_suggested_binding entries submitted by the application
 * for a single interaction profile (for example
 * `/interaction_profiles/khr/simple_controller`). Each suggested binding
 * points at an @ref oxr_binding_point in the @ref oxr_binding_registry.
 *
 * Profiles live on @ref oxr_instance_action_context::suggested_profiles and
 * are snapshotted onto the session at xrAttachSessionActionSets. The
 * snapshotting is required by the OpenXR spec.
 */
struct oxr_suggested_profile
{
	//! Interaction profile path this suggestion set is for.
	XrPath path;

	//! Generated interaction profile template for this suggestion.
	const struct oxr_profile_template *templ;

	//! Client-suggested action-to-path bindings for this profile.
	struct oxr_suggested_binding *bindings;
	size_t binding_count;

	//! Optional DPAD modifications from the same suggest call.
	struct oxr_dpad_state dpad_state;
};

void
oxr_suggested_profile_destroy(struct oxr_suggested_profile *profile);

struct oxr_suggested_profile *
oxr_suggested_profile_clone(const struct oxr_suggested_profile *src_profile);


#ifdef __cplusplus
}
#endif
