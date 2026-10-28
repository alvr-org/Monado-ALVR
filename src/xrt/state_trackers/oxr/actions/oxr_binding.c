// Copyright 2018-2020,2023 Collabora, Ltd.
// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Holds binding related functions.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup oxr_main
 */

#include "util/u_misc.h"

#include "oxr_binding.h"
#include "oxr_binding_profile.h"
#include "oxr_binding_registry.h"
#include "oxr_generated_bindings.h"
#include "oxr_dpad_state.h"
#include "oxr_session_action_context.h"
#include "oxr_suggested_profile_array.h"

#include "../oxr_objects.h"
#include "../oxr_logger.h"
#include "../oxr_two_call.h"

#include <stdio.h>
#include <string.h>


static bool
get_profile_template_from_path(const struct oxr_instance_path_cache *cache,
                               XrPath path,
                               struct oxr_profile_template **out_templ)
{
	static_assert(OXR_BINDINGS_PROFILE_TEMPLATE_COUNT == ARRAY_SIZE(oxr_profile_templates), "Must match");
	static_assert(OXR_BINDINGS_PROFILE_TEMPLATE_COUNT == ARRAY_SIZE(cache->template_paths), "Must match");

	for (size_t x = 0; x < OXR_BINDINGS_PROFILE_TEMPLATE_COUNT; x++) {
		if (cache->template_paths[x] != path) {
			continue;
		}

		*out_templ = &oxr_profile_templates[x];
		return true;
	}

	*out_templ = NULL;

	return false;
}

static bool
suggested_profile_find_or_create_in_instance(struct oxr_logger *log,
                                             const struct oxr_instance_path_cache *cache,
                                             struct oxr_instance_action_context *context,
                                             XrPath path,
                                             struct oxr_suggested_profile **out_p)
{
	if (oxr_suggested_profile_array_find_by_path(&context->suggested_profiles, path, out_p)) {
		return true;
	}

	struct oxr_profile_template *templ = NULL;
	if (!get_profile_template_from_path(cache, path, &templ)) {
		*out_p = NULL;
		return false;
	}

	struct oxr_suggested_profile *p = U_TYPED_CALLOC(struct oxr_suggested_profile);
	if (p == NULL) {
		oxr_log(log, "Failed to allocate suggested profile\n");
		*out_p = NULL;
		return false;
	}

	p->path = path;
	p->templ = templ;

	oxr_suggested_profile_array_add(&context->suggested_profiles, p);

	*out_p = p;

	return true;
}

static void
add_string(char *temp, size_t max, ssize_t *current, const char *str)
{
	if (*current > 0) {
		temp[(*current)++] = ' ';
	}

	ssize_t len = snprintf(temp + *current, max - *current, "%s", str);
	if (len > 0) {
		*current += len;
	}
}


/*
 *
 * 'Exported' functions.
 *
 */

XrResult
oxr_action_suggest_interaction_profile_bindings(struct oxr_logger *log,
                                                struct oxr_path_store *store,
                                                const struct oxr_instance_path_cache *cache,
                                                struct oxr_instance_action_context *inst_context,
                                                const XrInteractionProfileSuggestedBinding *suggestedBindings,
                                                struct oxr_dpad_state *dpad_state)
{
	struct oxr_suggested_profile *p = NULL;
	XrResult ret = XR_SUCCESS;

	// Path already validated.
	XrPath path = suggestedBindings->interactionProfile;
	suggested_profile_find_or_create_in_instance( //
	    log,                                      //
	    cache,                                    //
	    inst_context,                             //
	    path,                                     //
	    &p);                                      //

	// Valid path, but not used.
	if (p == NULL) {
		goto out;
	}

	free(p->bindings);
	p->bindings = NULL;
	p->binding_count = 0;

	// Transfer ownership of dpad state to profile
	oxr_dpad_state_deinit(&p->dpad_state);
	p->dpad_state = *dpad_state;
	U_ZERO(dpad_state);

	const struct oxr_binding_profile *registry_profile = NULL;
	oxr_binding_registry_find_profile(inst_context->binding_registry, path, &registry_profile);
	if (registry_profile == NULL) {
		ret = oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to find registry profile");
		goto out;
	}

	for (size_t i = 0; i < suggestedBindings->countSuggestedBindings; i++) {
		const XrActionSuggestedBinding *s = &suggestedBindings->suggestedBindings[i];
		struct oxr_action *act = XRT_CAST_OXR_HANDLE_TO_PTR(struct oxr_action *, s->action);

		const struct oxr_binding_point *binding_point = NULL;
		if (!oxr_binding_profile_find_binding_point(registry_profile, s->binding, &binding_point)) {
			ret = oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to find binding point");
			goto out;
		}

		U_ARRAY_REALLOC_OR_FREE(p->bindings, struct oxr_suggested_binding, p->binding_count + 1);
		if (p->bindings == NULL) {
			p->binding_count = 0;
			ret = oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to allocate suggested binding");
			goto out;
		}

		struct oxr_suggested_binding *binding = &p->bindings[p->binding_count++];
		binding->act_key = act->act_key;
		binding->path = s->binding;
		binding->binding_point = binding_point;
	}

out:
	oxr_dpad_state_deinit(dpad_state); // if it hasn't been moved

	return ret;
}

XrResult
oxr_action_get_current_interaction_profile(struct oxr_logger *log,
                                           const struct oxr_instance_path_cache *cache,
                                           const struct oxr_session_action_context *action_context,
                                           XrPath topLevelUserPath,
                                           XrInteractionProfileState *interactionProfile)
{
	// Action sets attachment is checked in the API function.

#define IDENTIFY_TOP_LEVEL_PATH(X)                                                                                     \
	if (topLevelUserPath == cache->X) {                                                                            \
		interactionProfile->interactionProfile = action_context->X;                                            \
	} else

	OXR_FOR_EACH_VALID_SUBACTION_PATH(IDENTIFY_TOP_LEVEL_PATH)
	{
		// else clause
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Top level path not handled?!");
	}
#undef IDENTIFY_TOP_LEVEL_PATH
	return XR_SUCCESS;
}

XrResult
oxr_action_get_input_source_localized_name(struct oxr_logger *log,
                                           const struct oxr_path_store *store,
                                           const struct oxr_session_action_context *action_context,
                                           const XrInputSourceLocalizedNameGetInfo *getInfo,
                                           uint32_t bufferCapacityInput,
                                           uint32_t *bufferCountOutput,
                                           char *buffer)
{
	char temp[1024] = {0};
	ssize_t current = 0;

	const struct oxr_binding_point *binding_point = NULL;
	bool bret = oxr_binding_registry_get_for_source_path( //
	    action_context->binding_registry,                 //
	    getInfo->sourcePath,                              //
	    &binding_point);                                  //
	if (!bret) {
		return oxr_error(log, XR_ERROR_VALIDATION_FAILURE, "(sourcePath) is not a bound source path");
	}

	// Add which hand to use.
	if (getInfo->whichComponents & XR_INPUT_SOURCE_LOCALIZED_NAME_USER_PATH_BIT) {
		add_string(temp, sizeof(temp), &current, binding_point->subaction->localized_name);
	}

	// Add a human readable and localized name of the device.
	if ((getInfo->whichComponents & XR_INPUT_SOURCE_LOCALIZED_NAME_INTERACTION_PROFILE_BIT) != 0) {
		add_string(temp, sizeof(temp), &current, binding_point->profile->localized_name);
	}

	//! @todo This implementation is very very very inelegant.
	if ((getInfo->whichComponents & XR_INPUT_SOURCE_LOCALIZED_NAME_COMPONENT_BIT) != 0) {
		/*
		 * The preceding enum is misnamed: it should be called identifier
		 * instead of component. But, this is a spec bug.
		 */
		add_string(temp, sizeof(temp), &current, binding_point->localized_name);
	}

	// Include the null character.
	current += 1;

	OXR_TWO_CALL_HELPER(log, bufferCapacityInput, bufferCountOutput, buffer, (size_t)current, temp, XR_SUCCESS);
}
