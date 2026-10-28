// Copyright 2018-2020,2023 Collabora, Ltd.
// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Holds suggested profile array related functions.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup oxr_main
 */

#include "util/u_misc.h"

#include "oxr_suggested_profile_array.h"
#include "oxr_binding_profile.h"
#include "oxr_binding_registry.h"
#include "oxr_rebinding_profile.h"


void
oxr_suggested_profile_array_clear(struct oxr_suggested_profile_array *array)
{
	for (size_t i = 0; i < array->count; i++) {
		struct oxr_suggested_profile *p = array->profiles[i];
		oxr_suggested_profile_destroy(p);
	}

	free(array->profiles);
	array->profiles = NULL;
	array->count = 0;
}

void
oxr_suggested_profile_array_add(struct oxr_suggested_profile_array *array, struct oxr_suggested_profile *profile)
{
	U_ARRAY_REALLOC_OR_FREE(array->profiles, struct oxr_suggested_profile *, (array->count + 1));
	array->profiles[array->count++] = profile;
}

XrResult
oxr_suggested_profile_array_clone(const struct oxr_suggested_profile_array *src,
                                  struct oxr_suggested_profile_array *dst)
{
	oxr_suggested_profile_array_clear(dst);

	if (src->profiles == NULL || src->count == 0) {
		return XR_SUCCESS;
	}

	dst->profiles = U_TYPED_ARRAY_CALLOC(struct oxr_suggested_profile *, src->count);
	if (dst->profiles == NULL) {
		// No need to clear dst, it's already empty.
		return XR_ERROR_RUNTIME_FAILURE;
	}
	dst->count = src->count;

	for (size_t i = 0; i < src->count; i++) {
		dst->profiles[i] = oxr_suggested_profile_clone(src->profiles[i]);
		if (dst->profiles[i] == NULL) {
			oxr_suggested_profile_array_clear(dst);
			return XR_ERROR_RUNTIME_FAILURE;
		}
	}

	return XR_SUCCESS;
}

bool
oxr_suggested_profile_array_find_by_path(const struct oxr_suggested_profile_array *array,
                                         XrPath path,
                                         struct oxr_suggested_profile **out_p)
{
	for (size_t x = 0; x < array->count; x++) {
		struct oxr_suggested_profile *p = array->profiles[x];
		if (p->path != path) {
			continue;
		}

		*out_p = p;
		return true;
	}

	*out_p = NULL;

	return false;
}

bool
oxr_suggested_profile_array_find_by_device_name(const struct oxr_suggested_profile_array *array,
                                                enum xrt_device_name name,
                                                struct oxr_suggested_profile **out_p)
{
	if (name == XRT_DEVICE_INVALID) {
		*out_p = NULL;
		return false;
	}

	for (size_t i = 0; i < array->count; i++) {
		struct oxr_suggested_profile *profile = array->profiles[i];
		assert(profile != NULL);
		assert(profile->templ != NULL);

		if (profile->templ->name == name) {
			*out_p = profile;
			return true;
		}
	}

	*out_p = NULL;
	return false;
}

bool
oxr_suggested_profile_array_find_rebinding_for_device_name(const struct oxr_suggested_profile_array *array,
                                                           const struct oxr_binding_registry *registry,
                                                           enum xrt_device_name target_device_name,
                                                           struct oxr_suggested_profile **out_p,
                                                           const struct oxr_rebinding_profile **out_rebinding)
{
	*out_p = NULL;
	*out_rebinding = NULL;

	if (target_device_name == XRT_DEVICE_INVALID) {
		return false;
	}

	for (size_t i = 0; i < registry->rebinding_count; i++) {
		const struct oxr_rebinding_profile *rebinding = &registry->rebindings[i];
		const struct oxr_binding_profile *target_profile = NULL;
		if (!oxr_binding_registry_find_profile(registry, rebinding->target_profile_path, &target_profile)) {
			continue;
		}
		if (target_profile->device_name != target_device_name) {
			continue;
		}

		if (oxr_suggested_profile_array_find_by_path(array, rebinding->source_profile_path, out_p)) {
			*out_rebinding = rebinding;
			return true;
		}
	}

	return false;
}
