// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Suggested interaction profile state.
 * @ingroup oxr_main
 */

#include "util/u_misc.h"

#include "oxr_suggested_profile.h"

#include <string.h>


void
oxr_suggested_profile_destroy(struct oxr_suggested_profile *profile)
{
	if (profile == NULL) {
		return;
	}

	free(profile->bindings);
	profile->bindings = NULL;
	profile->binding_count = 0;

	oxr_dpad_state_deinit(&profile->dpad_state);

	free(profile);
}

struct oxr_suggested_profile *
oxr_suggested_profile_clone(const struct oxr_suggested_profile *src_profile)
{
	if (src_profile == NULL) {
		return NULL;
	}

	struct oxr_suggested_profile *dst_profile = U_TYPED_CALLOC(struct oxr_suggested_profile);
	if (dst_profile == NULL) {
		return NULL;
	}

	dst_profile->path = src_profile->path;
	dst_profile->templ = src_profile->templ;

	if (src_profile->bindings != NULL && src_profile->binding_count > 0) {
		dst_profile->binding_count = src_profile->binding_count;
		dst_profile->bindings = U_TYPED_ARRAY_CALLOC(struct oxr_suggested_binding, src_profile->binding_count);
		if (dst_profile->bindings == NULL) {
			oxr_suggested_profile_destroy(dst_profile);
			return NULL;
		}

		memcpy(dst_profile->bindings, src_profile->bindings,
		       sizeof(struct oxr_suggested_binding) * src_profile->binding_count);
	}

	if (src_profile->dpad_state.entries != NULL &&
	    !oxr_dpad_state_clone(&dst_profile->dpad_state, &src_profile->dpad_state)) {
		oxr_suggested_profile_destroy(dst_profile);
		return NULL;
	}

	return dst_profile;
}
