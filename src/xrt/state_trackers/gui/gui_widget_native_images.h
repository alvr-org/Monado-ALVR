// Copyright 2019-2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Swapchain rendering helper code.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup gui
 */

#pragma once

#include "xrt/xrt_compositor.h"
#include "xrt/xrt_defines.h"
#include "xrt/xrt_limits.h"

#ifdef __cplusplus
extern "C" {
#endif

struct gui_program;
struct u_native_images_debug;

#define GUI_WIDGET_SWAPCHAIN_INVALID_INDEX 0xffffffff

/*!
 * A small widget that interfaces a @ref u_native_images_debug struct, caching the
 * imports from the listed @ref xrt_image_native list.
 *
 * @ingroup gui
 */
struct gui_widget_native_images
{
	//! To check if swapchain has been changed.
	xrt_limited_unique_id_t cache_id;

	uint32_t memories[XRT_MAX_SWAPCHAIN_IMAGES];
	uint32_t textures[XRT_MAX_SWAPCHAIN_IMAGES];

	//! The current number of images that has been imported.
	uint32_t texture_count;

	//! Dimensions.
	uint32_t width, height;

	//! Set to GUI_WIDGET_SWAPCHAIN_INVALID_INDEX on invalid.
	uint32_t active_index;

	//! Should the image be flipped in y direction.
	bool flip_y;
};

/*!
 * A single record in a native image widget storage.
 *
 * @ingroup gui
 */
struct gui_widget_native_images_record
{
	void *ptr;
	struct gui_widget_native_images gwni;
};

/*!
 * Helper struct to cache @ref gui_widget_native_images.
 *
 * @ingroup gui
 */
struct gui_widget_native_images_storage
{
	struct gui_widget_native_images_record records[32];
};


/*
 *
 * Swapchain functions.
 *
 */

/*!
 * Initialise a embeddable record window.
 *
 * @ingroup gui
 */
void
gui_widget_native_images_init(struct gui_widget_native_images *gwni);

/*!
 * Update the swapchain widget.
 *
 * @ingroup gui
 */
void
gui_widget_native_images_update(struct gui_widget_native_images *gwni, struct u_native_images_debug *unid);

/*!
 * Renders all controls of a record window.
 *
 * @ingroup gui
 */
void
gui_widget_native_images_render(struct gui_widget_native_images *gwni, struct gui_program *p);

/*!
 * Draw the sink image as the background to the background of the render view.
 * Basically the main window in which all ImGui windows lives in, not to a
 * ImGui window.
 *
 * @ingroup gui
 */
void
gui_widget_native_images_to_background(struct gui_widget_native_images *gwni, struct gui_program *p);

/*!
 * Frees all resources associated with a record window. Make sure to only call
 * this function on the main gui thread, and that nothing is pushing into the
 * record windows sink.
 *
 * @ingroup gui
 */
void
gui_widget_native_images_close(struct gui_widget_native_images *gwni);


/*
 *
 * Storage functions.
 *
 */

/*!
 * Search the storage for the matching record for the debug swapchain and
 * return it, if not found and there is room create it.
 *
 * @ingroup gui
 */
struct gui_widget_native_images *
gui_widget_native_images_storage_ensure(struct gui_widget_native_images_storage *gwnis,
                                        struct u_native_images_debug *unid);

/*!
 * Close the storage.
 *
 * @ingroup gui
 */
struct gui_widget_native_images *
gui_widget_native_images_storage_close(struct gui_widget_native_images_storage *gwnis,
                                       struct u_native_images_debug *unid);


#ifdef __cplusplus
}
#endif
