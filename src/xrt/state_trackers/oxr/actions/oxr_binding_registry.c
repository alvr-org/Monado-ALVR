// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Runtime registry built from generated OpenXR binding templates.
 * @ingroup oxr_main
 */

#include "util/u_hashmap.h"
#include "util/u_misc.h"

#include "oxr_binding_registry.h"
#include "oxr_binding_point_store.h"
#include "oxr_binding_profile.h"
#include "oxr_rebinding_profile.h"
#include "oxr_session_action_context.h"
#include "oxr_subaction_store.h"

#include "../oxr_logger.h"
#include "../path/oxr_path_store.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#define MAKE_PATH(STR, OUT_PATH)                                                                                       \
	do {                                                                                                           \
		XrResult ret = make_path(store, (STR), (OUT_PATH));                                                    \
		if (ret != XR_SUCCESS) {                                                                               \
			return oxr_error(log, ret, "Failed to create '" #STR "' path");                                \
		}                                                                                                      \
	} while (false)


#define MAKE_PATH_OPT(STR, OUT_PATH)                                                                                   \
	do {                                                                                                           \
		XrResult ret = make_path_opt(store, (STR), (OUT_PATH));                                                \
		if (ret != XR_SUCCESS) {                                                                               \
			return oxr_error(log, ret, "Failed to create '" #STR "' path");                                \
		}                                                                                                      \
	} while (false)

static XrResult
make_path(struct oxr_path_store *store, const char *str, XrPath *out_path)
{
	if (str == NULL) {
		return XR_ERROR_RUNTIME_FAILURE;
	}

	return oxr_path_store_get_or_create(store, str, strlen(str), out_path);
}

static XrResult
make_path_opt(struct oxr_path_store *store, const char *str, XrPath *out_path)
{
	if (str == NULL) {
		*out_path = XR_NULL_PATH;
		return XR_SUCCESS;
	}

	return make_path(store, str, out_path);
}

static XrResult
binding_registry_add_subaction(struct oxr_logger *log,
                               struct oxr_path_store *store,
                               struct oxr_binding_registry *registry,
                               const char *subaction_path_str,
                               const char *localized_name)
{
	XrPath path = XR_NULL_PATH;
	XrResult ret = make_path(store, subaction_path_str, &path);
	if (ret != XR_SUCCESS) {
		return oxr_error(log, ret, "Failed to create registry subaction path");
	}

	ret = oxr_subaction_store_add(&registry->subactions, path, localized_name);
	if (ret != XR_SUCCESS) {
		return oxr_error(log, ret, "Failed to add registry binding subaction");
	}

	return XR_SUCCESS;
}

static XrResult
binding_registry_populate_subactions(struct oxr_logger *log,
                                     struct oxr_path_store *store,
                                     struct oxr_binding_registry *registry)
{
	/*
	 * The possible top-level subaction paths are hardcoded by
	 * OXR_FOR_EACH_VALID_SUBACTION_PATH_DETAILED, so it is safe to add them
	 * eagerly once and have binding points only look them up.
	 */
#define ADD_SUBACTION(lower, CAP, PATH, LOCALIZED_NAME)                                                                \
	do {                                                                                                           \
		XrResult ret = binding_registry_add_subaction(log, store, registry, PATH, LOCALIZED_NAME);             \
		if (ret != XR_SUCCESS) {                                                                               \
			return ret;                                                                                    \
		}                                                                                                      \
	} while (false);
	OXR_FOR_EACH_VALID_SUBACTION_PATH_DETAILED(ADD_SUBACTION)
#undef ADD_SUBACTION

	return XR_SUCCESS;
}

static void
binding_point_fini(struct oxr_binding_point *point)
{
	U_ZERO(point);
}

static void
dpad_emulation_fini(struct oxr_dpad_emulation *emulation)
{
	U_ZERO(emulation);
}

static void
binding_profile_fini(struct oxr_binding_profile *profile)
{
	if (profile->binding_points != NULL) {
		for (size_t i = 0; i < profile->binding_point_count; i++) {
			binding_point_fini(&profile->binding_points[i]);
		}
	}

	free(profile->binding_points);
	profile->binding_points = NULL;
	profile->binding_point_count = 0;

	if (profile->dpad_emulations != NULL) {
		for (size_t i = 0; i < profile->dpad_emulation_count; i++) {
			dpad_emulation_fini(&profile->dpad_emulations[i]);
		}
	}

	free(profile->dpad_emulations);
	profile->dpad_emulations = NULL;
	profile->dpad_emulation_count = 0;

	if (profile->binding_points_by_path != NULL) {
		u_hashmap_int_destroy(&profile->binding_points_by_path);
	}

	U_ZERO(profile);
}

static void
rebinding_profile_fini(struct oxr_rebinding_profile *profile)
{
	free(profile->bindings);
	U_ZERO(profile);
}

static void
registry_destroy_cb(struct oxr_refcounted *orc)
{
	struct oxr_binding_registry *registry = (struct oxr_binding_registry *)orc;

	if (registry->profiles != NULL) {
		for (size_t i = 0; i < registry->profile_count; i++) {
			binding_profile_fini(&registry->profiles[i]);
		}
	}

	free(registry->profiles);
	registry->profiles = NULL;
	registry->profile_count = 0;

	if (registry->rebindings != NULL) {
		for (size_t i = 0; i < registry->rebinding_count; i++) {
			rebinding_profile_fini(&registry->rebindings[i]);
		}
	}

	free(registry->rebindings);
	registry->rebindings = NULL;
	registry->rebinding_count = 0;

	if (registry->profiles_by_path != NULL) {
		u_hashmap_int_destroy(&registry->profiles_by_path);
	}

	oxr_binding_point_store_fini(&registry->binding_points);

	oxr_subaction_store_fini(&registry->subactions);

	free(registry);
}

static XrResult
dpad_emulation_init(struct oxr_logger *log,
                    struct oxr_path_store *store,
                    struct oxr_binding_profile *profile,
                    const struct oxr_dpad_emulation_template *templ,
                    struct oxr_dpad_emulation *emulation)
{
	emulation->profile = profile;
	emulation->templ = templ;
	emulation->input = templ->position;
	emulation->activate = templ->activate;

	MAKE_PATH(templ->path, &emulation->path);
	MAKE_PATH(templ->subaction_path, &emulation->subaction_path);

	return XR_SUCCESS;
}

static struct oxr_dpad_emulation *
binding_profile_find_dpad_emulation_by_path(struct oxr_binding_profile *profile, XrPath path)
{
	for (size_t i = 0; i < profile->dpad_emulation_count; i++) {
		if (profile->dpad_emulations[i].path == path) {
			return &profile->dpad_emulations[i];
		}
	}

	return NULL;
}

static XrResult
binding_registry_get_subaction(struct oxr_logger *log,
                               struct oxr_path_store *store,
                               struct oxr_binding_registry *registry,
                               const char *subaction_path_str,
                               const struct oxr_binding_subaction **out_subaction)
{
	XrPath path = XR_NULL_PATH;
	XrResult ret = make_path(store, subaction_path_str, &path);
	if (ret != XR_SUCCESS) {
		return oxr_error(log, ret, "Failed to create binding point subaction path");
	}

	if (!oxr_subaction_store_get(&registry->subactions, path, out_subaction)) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to find registry binding subaction");
	}

	return XR_SUCCESS;
}

static XrResult
binding_point_init(struct oxr_logger *log,
                   struct oxr_path_store *store,
                   struct oxr_binding_profile *profile,
                   const struct oxr_binding_point_template *templ,
                   struct oxr_binding_point *point)
{
	point->profile = profile;
	point->templ = templ;
	point->kind = templ->kind;
	point->localized_name = templ->localized_name;
	point->value_transform = templ->value_transform;

	XrResult ret =
	    binding_registry_get_subaction(log, store, profile->registry, templ->subaction_path, &point->subaction);
	if (ret != XR_SUCCESS) {
		return ret;
	}

	MAKE_PATH(templ->path, &point->path);

	ret = oxr_binding_point_store_add(                                                                      //
	    &profile->registry->binding_points, profile->path, point->path, store, point, &point->source_path); //
	if (ret != XR_SUCCESS) {
		return oxr_error(log, ret, "Failed to add binding point source path");
	}

	switch (point->kind) {
	case OXR_BINDING_POINT_KIND_INPUT: point->input.name = templ->input; break;
	case OXR_BINDING_POINT_KIND_OUTPUT: point->output.name = templ->output; break;
	case OXR_BINDING_POINT_KIND_DPAD:
		point->dpad.binding_point = templ->dpad_binding_point;
		XrPath dpad_emulation_path = XR_NULL_PATH;
		ret = make_path(store, templ->dpad_emulation_path, &dpad_emulation_path);
		if (ret != XR_SUCCESS) {
			return oxr_error(log, ret, "Failed to create DPAD emulation lookup path");
		}
		point->dpad.emulation = binding_profile_find_dpad_emulation_by_path(profile, dpad_emulation_path);
		if (point->dpad.emulation == NULL) {
			return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to find DPAD emulation");
		}
		break;
	case OXR_BINDING_POINT_KIND_UPGRADE: break;
	}

	ret = make_path(store, templ->priority_group, &point->priority_group);
	if (ret != XR_SUCCESS) {
		return oxr_error(log, ret, "Failed to create binding point priority group path");
	}

	void *existing = NULL;
	if (u_hashmap_int_find(profile->binding_points_by_path, point->path, &existing) != 0) {
		int h_ret = u_hashmap_int_insert(profile->binding_points_by_path, point->path, point);
		if (h_ret < 0) {
			return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to insert binding point path");
		}
	}

	if (point->kind == OXR_BINDING_POINT_KIND_UPGRADE) {
		MAKE_PATH_OPT(templ->upgrade_boolean_path, &point->upgrade.boolean_path);
		MAKE_PATH_OPT(templ->upgrade_float_path, &point->upgrade.float_path);
		MAKE_PATH_OPT(templ->upgrade_vec2_path, &point->upgrade.vec2_path);
		MAKE_PATH_OPT(templ->upgrade_pose_path, &point->upgrade.pose_path);
		MAKE_PATH_OPT(templ->upgrade_haptic_path, &point->upgrade.haptic_path);
	}

	return XR_SUCCESS;
}

static XrResult
binding_profile_init(struct oxr_logger *log,
                     struct oxr_path_store *store,
                     struct oxr_binding_registry *registry,
                     const struct oxr_profile_template *templ,
                     struct oxr_binding_profile *profile)
{
	profile->registry = registry;
	profile->templ = templ;
	profile->device_name = templ->name;
	profile->localized_name = templ->localized_name;

	MAKE_PATH(templ->path, &profile->path);

	int h_ret = u_hashmap_int_create(&profile->binding_points_by_path);
	if (h_ret < 0) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to create binding point hashmap");
	}

	profile->dpad_emulation_count = templ->dpad_emulation_count;
	profile->dpad_emulations = U_TYPED_ARRAY_CALLOC(struct oxr_dpad_emulation, profile->dpad_emulation_count);
	if (profile->dpad_emulations == NULL && profile->dpad_emulation_count > 0) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to allocate DPAD emulations");
	}

	for (size_t i = 0; i < profile->dpad_emulation_count; i++) {
		XrResult ret = dpad_emulation_init( //
		    log,                            //
		    store,                          //
		    profile,                        //
		    &templ->dpad_emulations[i],     //
		    &profile->dpad_emulations[i]);  //
		if (ret != XR_SUCCESS) {
			return ret;
		}
	}

	profile->binding_point_count = templ->binding_count;
	profile->binding_points = U_TYPED_ARRAY_CALLOC(struct oxr_binding_point, profile->binding_point_count);
	if (profile->binding_points == NULL && profile->binding_point_count > 0) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to allocate binding points");
	}

	for (size_t i = 0; i < profile->binding_point_count; i++) {
		XrResult ret = binding_point_init( //
		    log,                           //
		    store,                         //
		    profile,                       //
		    &templ->bindings[i],           //
		    &profile->binding_points[i]);  //
		if (ret != XR_SUCCESS) {
			return ret;
		}
	}

	h_ret = u_hashmap_int_insert(registry->profiles_by_path, profile->path, profile);
	if (h_ret < 0) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to insert profile path");
	}

	return XR_SUCCESS;
}

static XrResult
rebinding_profile_init(struct oxr_logger *log,
                       struct oxr_path_store *store,
                       const struct oxr_rebinding_profile_template *templ,
                       struct oxr_rebinding_profile *profile)
{
	profile->templ = templ;

	MAKE_PATH(templ->source_profile_path, &profile->source_profile_path);
	MAKE_PATH(templ->target_profile_path, &profile->target_profile_path);

	profile->binding_count = templ->binding_count;
	profile->bindings = U_TYPED_ARRAY_CALLOC(struct oxr_rebinding, profile->binding_count);
	if (profile->bindings == NULL && profile->binding_count > 0) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to allocate rebindings");
	}

	for (size_t i = 0; i < profile->binding_count; i++) {
		const struct oxr_rebinding_binding_template *binding = &templ->bindings[i];
		MAKE_PATH(binding->source_path, &profile->bindings[i].source_path);
		MAKE_PATH(binding->target_path, &profile->bindings[i].target_path);
	}

	return XR_SUCCESS;
}

XrResult
oxr_binding_registry_create(struct oxr_logger *log,
                            struct oxr_path_store *store,
                            struct oxr_binding_registry **out_registry)
{
	struct oxr_binding_registry *registry = U_TYPED_CALLOC(struct oxr_binding_registry);
	if (registry == NULL) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to allocate binding registry");
	}

	registry->base.base.count = 1;
	registry->base.destroy = registry_destroy_cb;

	int h_ret = u_hashmap_int_create(&registry->profiles_by_path);
	if (h_ret < 0) {
		free(registry);
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to create profile hashmap");
	}

	XrResult ret = oxr_binding_point_store_init(&registry->binding_points);
	if (ret != XR_SUCCESS) {
		oxr_binding_registry_unref(registry);
		return oxr_error(log, ret, "Failed to initialize registry binding point store");
	}

	static_assert(OXR_BINDINGS_PROFILE_TEMPLATE_COUNT == ARRAY_SIZE(oxr_profile_templates), "Must match");

	ret = oxr_subaction_store_init(&registry->subactions);
	if (ret != XR_SUCCESS) {
		oxr_binding_registry_unref(registry);
		return oxr_error(log, ret, "Failed to initialize registry binding subaction store");
	}

	ret = binding_registry_populate_subactions(log, store, registry);
	if (ret != XR_SUCCESS) {
		oxr_binding_registry_unref(registry);
		return ret;
	}

	/*
	 * Build the complete generated registry regardless of extension support or
	 * OpenXR version. Suggested binding validation rejects unavailable
	 * interaction profiles and binding paths before they are stored; the runtime
	 * registry should remain a complete lookup table.
	 */
	registry->profile_count = OXR_BINDINGS_PROFILE_TEMPLATE_COUNT;
	registry->profiles = U_TYPED_ARRAY_CALLOC(struct oxr_binding_profile, registry->profile_count);
	if (registry->profiles == NULL && registry->profile_count > 0) {
		oxr_binding_registry_unref(registry);
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to allocate binding profiles");
	}

	for (size_t i = 0; i < registry->profile_count; i++) {
		ret = binding_profile_init(    //
		    log,                       //
		    store,                     //
		    registry,                  //
		    &oxr_profile_templates[i], //
		    &registry->profiles[i]);   //
		if (ret != XR_SUCCESS) {
			oxr_binding_registry_unref(registry);
			return ret;
		}
	}

	static_assert(OXR_BINDINGS_REBINDING_PROFILE_TEMPLATE_COUNT == ARRAY_SIZE(oxr_rebinding_profile_templates),
	              "Must match");

	registry->rebinding_count = OXR_BINDINGS_REBINDING_PROFILE_TEMPLATE_COUNT;
	registry->rebindings = U_TYPED_ARRAY_CALLOC(struct oxr_rebinding_profile, registry->rebinding_count);
	if (registry->rebindings == NULL && registry->rebinding_count > 0) {
		oxr_binding_registry_unref(registry);
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to allocate rebinding profiles");
	}

	for (size_t i = 0; i < registry->rebinding_count; i++) {
		XrResult ret = rebinding_profile_init(   //
		    log,                                 //
		    store,                               //
		    &oxr_rebinding_profile_templates[i], //
		    &registry->rebindings[i]);           //
		if (ret != XR_SUCCESS) {
			oxr_binding_registry_unref(registry);
			return ret;
		}
	}

	*out_registry = registry;
	return XR_SUCCESS;
}

void
oxr_binding_registry_ref(struct oxr_binding_registry *registry)
{
	if (registry != NULL) {
		oxr_refcounted_ref(&registry->base);
	}
}

void
oxr_binding_registry_unref(struct oxr_binding_registry *registry)
{
	if (registry != NULL) {
		oxr_refcounted_unref(&registry->base);
	}
}

bool
oxr_binding_registry_find_profile(const struct oxr_binding_registry *registry,
                                  XrPath path,
                                  const struct oxr_binding_profile **out_profile)
{
	void *ptr = NULL;
	if (registry == NULL || u_hashmap_int_find(registry->profiles_by_path, path, &ptr) != 0) {
		*out_profile = NULL;
		return false;
	}

	*out_profile = (struct oxr_binding_profile *)ptr;
	return true;
}

bool
oxr_binding_registry_get_for_source_path(const struct oxr_binding_registry *registry,
                                         XrPath source_path,
                                         const struct oxr_binding_point **out_binding_point)
{
	return oxr_binding_point_store_get(&registry->binding_points, source_path, out_binding_point);
}

bool
oxr_binding_registry_find_rebinding_from_source_for_device(const struct oxr_binding_registry *registry,
                                                           XrPath source_profile_path,
                                                           enum xrt_device_name target_device_name,
                                                           const struct oxr_rebinding_profile **out_rebinding)
{
	if (registry == NULL) {
		*out_rebinding = NULL;
		return false;
	}

	for (size_t i = 0; i < registry->rebinding_count; i++) {
		const struct oxr_rebinding_profile *rebinding = &registry->rebindings[i];
		if (rebinding->source_profile_path != source_profile_path) {
			continue;
		}

		const struct oxr_binding_profile *target_profile = NULL;
		if (!oxr_binding_registry_find_profile(registry, rebinding->target_profile_path, &target_profile)) {
			continue;
		}

		if (target_profile->device_name == target_device_name) {
			*out_rebinding = rebinding;
			return true;
		}
	}

	*out_rebinding = NULL;
	return false;
}
