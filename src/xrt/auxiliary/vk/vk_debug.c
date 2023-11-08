// Copyright 2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Debug helper code.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_vk
 */

#include "vk/vk_helpers.h"


#ifdef VK_EXT_debug_utils

void
vk_name_object(struct vk_bundle *vk, VkObjectType type, uint64_t object, const char *name)
{
	if (!vk->has_EXT_debug_utils) {
		return;
	}

	/*
	 * VUID-VkDebugUtilsObjectNameInfoEXT-objectType-02589
	 * If objectType is VK_OBJECT_TYPE_UNKNOWN, objectHandle must not be VK_NULL_HANDLE
	 */
	if (type == VK_OBJECT_TYPE_UNKNOWN && object == 0) {
		U_LOG_W("Unknown object type can't be VK_NULL_HANDLE");
		return;
	}

	const VkDebugUtilsObjectNameInfoEXT name_info = {
	    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
	    .objectType = type,
	    .objectHandle = object,
	    .pObjectName = name,
	};

	VkResult ret = vk->vkSetDebugUtilsObjectNameEXT(vk->device, &name_info);
	if (ret != VK_SUCCESS) {
		VK_ERROR(vk, "vkSetDebugUtilsObjectNameEXT: %s", vk_result_string(ret));
	}
}

void
vk_cmd_insert_label(struct vk_bundle *vk, VkCommandBuffer cmd_buffer, const char *name)
{
	if (!vk->has_EXT_debug_utils) {
		return;
	}

	const VkDebugUtilsLabelEXT debug_label = {
	    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
	    .pNext = NULL,
	    .pLabelName = name,
	    .color = {1.0f, 0.0f, 0.0f, 1.0f},
	};

	vk->vkCmdInsertDebugUtilsLabelEXT(cmd_buffer, &debug_label);
}

#endif
