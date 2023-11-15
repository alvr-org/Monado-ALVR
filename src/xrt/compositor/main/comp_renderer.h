// Copyright 2019-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Compositor rendering code header.
 * @author Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup comp_main
 */

#pragma once

#include "xrt/xrt_compiler.h"
#include "xrt/xrt_defines.h"
#include "xrt/xrt_vulkan_includes.h"


#ifdef __cplusplus
extern "C" {
#endif


struct comp_compositor;
struct comp_swapchain_image;
/*!
 * @brief Renderer used by compositor.
 */
struct comp_renderer;

/*!
 * Called by the main compositor code to create the renderer.
 *
 * @param c              Owning compositor.
 * @param scratch_extent Size for scratch image used when squashing layers.
 *
 * @public @memberof comp_renderer
 * @see comp_compositor
 * @ingroup comp_main
 */
struct comp_renderer *
comp_renderer_create(struct comp_compositor *c, VkExtent2D scratch_extent);

/*!
 * Clean up and free the renderer.
 *
 * Does null checking and sets to null after freeing.
 *
 * @public @memberof comp_renderer
 * @ingroup comp_main
 */
void
comp_renderer_destroy(struct comp_renderer **ptr_r);

/*!
 * Render frame.
 *
 * @public @memberof comp_renderer
 * @ingroup comp_main
 */
XRT_CHECK_RESULT xrt_result_t
comp_renderer_draw(struct comp_renderer *r);

void
comp_renderer_add_debug_vars(struct comp_renderer *self);

#ifdef __cplusplus
}
#endif
