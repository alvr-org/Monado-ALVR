// Copyright 2018-2024, Collabora, Ltd.
// Copyright 2023-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Holds per session action context.
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @ingroup oxr_main
 */

#include "xrt/xrt_system.h"

#include "util/u_debug.h"
#include "util/u_hashmap.h"
#include "util/u_misc.h"

#include "oxr_session_action_context.h"
#include "oxr_binding_registry.h"
#include "oxr_io_cache.h"
#include "oxr_priority_group_cache.h"
#include "oxr_suggested_profile_array.h"
#include "../oxr_objects.h"
#include "oxr_input.h"


XrResult
oxr_session_action_context_init(struct oxr_session_action_context *action_context,
                                struct xrt_system_devices *xsysd,
                                struct xrt_space_overseer *xso)
{
	U_ZERO(action_context);

	os_mutex_init(&action_context->sync_actions_mutex);

	// Action system hashmaps.
	int h_ret = u_hashmap_int_create(&action_context->act_sets_attachments_by_key);
	if (h_ret != 0) {
		oxr_session_action_context_fini(action_context);
		return XR_ERROR_RUNTIME_FAILURE;
	}

	XrResult ret = oxr_priority_group_cache_init(&action_context->priority_group_cache);
	if (ret != XR_SUCCESS) {
		oxr_session_action_context_fini(action_context);
		return ret;
	}

	// Initialize action set attachments
	action_context->act_set_attachments = NULL;
	action_context->action_set_attachment_count = 0;

	// Initialize subaction paths
#define INIT_PATH(X) action_context->X = XR_NULL_PATH;
	OXR_FOR_EACH_VALID_SUBACTION_PATH(INIT_PATH)
#undef INIT_PATH

	ret = oxr_io_cache_init(&action_context->io_cache, xsysd, xso);
	if (ret != XR_SUCCESS) {
		oxr_session_action_context_fini(action_context);
		return ret;
	}

	return XR_SUCCESS;
}

void
oxr_session_action_context_fini(struct oxr_session_action_context *action_context)
{
	oxr_suggested_profile_array_clear(&action_context->profiles_on_attachment);
	oxr_binding_registry_unref(action_context->binding_registry);
	action_context->binding_registry = NULL;

	// Teardown action set attachments
	for (size_t i = 0; i < action_context->action_set_attachment_count; ++i) {
		oxr_action_set_attachment_teardown(&action_context->act_set_attachments[i]);
	}
	free(action_context->act_set_attachments);
	action_context->act_set_attachments = NULL;
	action_context->action_set_attachment_count = 0;

	oxr_priority_group_cache_fini(&action_context->priority_group_cache);

	oxr_io_cache_fini(&action_context->io_cache);

	// If we tore everything down correctly, these are empty now.
	assert(action_context->act_sets_attachments_by_key == NULL ||
	       u_hashmap_int_empty(action_context->act_sets_attachments_by_key));

	u_hashmap_int_destroy(&action_context->act_sets_attachments_by_key);

	// Destroy mutex
	os_mutex_destroy(&action_context->sync_actions_mutex);
}

bool
oxr_session_action_context_has_attached_act_sets(const struct oxr_session_action_context *action_context)
{
	return action_context->act_set_attachments != NULL;
}

XRT_NONNULL_ALL bool
oxr_session_action_context_find_set(struct oxr_session_action_context *action_context,
                                    uint32_t act_set_key,
                                    struct oxr_action_set_attachment **out_act_set_attached)
{
	void *ptr = NULL;

	// In case no action_sets have been attached.
	if (!oxr_session_action_context_has_attached_act_sets(action_context)) {
		return false;
	}

	int ret = u_hashmap_int_find(action_context->act_sets_attachments_by_key, act_set_key, &ptr);
	if (ret == 0) {
		*out_act_set_attached = (struct oxr_action_set_attachment *)ptr;
		return true;
	}

	return false;
}
