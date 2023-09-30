// Copyright 2019-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Super small defines that makes writing Vulkan code smaller.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_vk
 */

#pragma once

#include "vk/vk_helpers.h"


/*!
 * Calls `vkDestroy##TYPE` on @p THING if it is not @p VK_NULL_HANDLE, sets it
 * to @p VK_NULL_HANDLE afterwards. The implicit argument @p vk will be used to
 * look up the function, and @p vk->device will be used as the device.
 *
 * @param TYPE  The type of the thing to be destroyed.
 * @param THING Object to be destroyed.
 *
 * @ingroup aux_vk
 */
#define D(TYPE, THING)                                                                                                 \
	if (THING != VK_NULL_HANDLE) {                                                                                 \
		vk->vkDestroy##TYPE(vk->device, THING, NULL);                                                          \
		THING = VK_NULL_HANDLE;                                                                                \
	}

/*!
 * Calls `vkFree##TYPE` on @p THING` if it is not @p VK_NULL_HANDLE, sets it to
 * @p VK_NULL_HANDLE afterwards. The implicit argument @p vk will be used to
 * look up the function, and @p vk->device will be used as the device.
 *
 * @param TYPE  The type of the thing to be freed.
 * @param THING Object to be freed.
 *
 * @ingroup aux_vk
 */
#define DF(TYPE, THING)                                                                                                \
	if (THING != VK_NULL_HANDLE) {                                                                                 \
		vk->vkFree##TYPE(vk->device, THING, NULL);                                                             \
		THING = VK_NULL_HANDLE;                                                                                \
	}
