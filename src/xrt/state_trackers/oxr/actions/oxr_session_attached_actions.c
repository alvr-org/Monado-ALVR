// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Holds per session attached actions.
 * @ingroup oxr_main
 */

#include "oxr_session_attached_actions.h"

#include "util/u_hashmap.h"

#include "../oxr_objects.h"


XrResult
oxr_session_attached_actions_init(struct oxr_session_attached_actions *attached)
{
	int ret = u_hashmap_int_create(&attached->act_attachments_by_key);
	if (ret != 0) {
		return XR_ERROR_RUNTIME_FAILURE;
	}
	ret = os_mutex_init(&attached->mutex);
	if (ret != 0) {
		u_hashmap_int_destroy(&attached->act_attachments_by_key);
		return XR_ERROR_RUNTIME_FAILURE;
	}

	return XR_SUCCESS;
}

void
oxr_session_attached_actions_fini(struct oxr_session_attached_actions *attached)
{
	os_mutex_destroy(&attached->mutex);
	u_hashmap_int_destroy(&attached->act_attachments_by_key);
}

XrResult
oxr_session_attached_actions_add_action_attachment(struct oxr_session_attached_actions *attached,
                                                   struct oxr_action_attachment *act_attachment)
{
	os_mutex_lock(&attached->mutex);
	int ret =
	    u_hashmap_int_insert(attached->act_attachments_by_key, (uint64_t)act_attachment->act_key, act_attachment);
	os_mutex_unlock(&attached->mutex);
	return ret >= 0 ? XR_SUCCESS : XR_ERROR_RUNTIME_FAILURE;
}

void
oxr_session_attached_actions_remove_action_attachment(struct oxr_session_attached_actions *attached,
                                                      struct oxr_action_attachment *act_attachment)
{
	os_mutex_lock(&attached->mutex);
	u_hashmap_int_erase(attached->act_attachments_by_key, (uint64_t)act_attachment->act_key);
	os_mutex_unlock(&attached->mutex);
}

void
oxr_session_attached_actions_find(struct oxr_session_attached_actions *attached,
                                  uint32_t act_key,
                                  struct oxr_action_attachment **out_act_attached)
{
	void *ptr = NULL;
	os_mutex_lock(&attached->mutex);
	int ret = u_hashmap_int_find(attached->act_attachments_by_key, (uint64_t)act_key, &ptr);
	os_mutex_unlock(&attached->mutex);
	if (ret >= 0) {
		*out_act_attached = (struct oxr_action_attachment *)ptr;
	} else {
		*out_act_attached = NULL;
	}
}
