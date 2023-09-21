// Copyright 2023-2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Special code for managing a variable tracked swapchain.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_util
 */

#pragma once

#include "os/os_threading.h"
#include "xrt/xrt_compositor.h"


#ifdef __cplusplus
extern "C" {
#endif


/*!
 * A struct for debugging one or more native images.
 *
 * @ingroup aux_util
 */
struct u_native_images_debug
{
	//! Is initialised/destroyed when added or root is removed.
	struct os_mutex mutex;

	/*!
	 * Process unique id for the set of images, protected by @p mutex,
	 * allows caching of imports. Created by @ref u_limited_unique_id_get.
	 */
	xrt_limited_unique_id_t limited_unique_id;

	//! List to current set of native images, protected by @p mutex.
	struct xrt_image_native *native_images;

	//! Count of @p native_images, protected by @p mutex.
	uint32_t native_image_count;

	/*!
	 * Information needed to import the native images, information in the
	 * struct is immutable, the pointer is protected by @p mutex.
	 */
	const struct xrt_swapchain_create_info *xscci;

	/*!
	 * The native image that was last filled in by the source, only
	 * valid if @p native_images is non-null, protected by @p mutex.
	 */
	uint32_t active_index;

	//! Should the image be flipped in y direction.
	bool flip_y;
};

/*!
 * Must be called before variable is tracked.
 *
 * @ingroup aux_util
 */
static inline void
u_native_images_debug_init(struct u_native_images_debug *unid)
{
	os_mutex_init(&unid->mutex);
}

/*!
 * Must not be called while variable longer tracked, after @p u_var_remove_root.
 *
 * @ingroup aux_util
 */
static inline void
u_native_images_debug_destroy(struct u_native_images_debug *unid)
{
	os_mutex_destroy(&unid->mutex);
	unid->native_images = NULL;
	unid->native_image_count = 0;
	unid->xscci = NULL;
	unid->active_index = 0;
	unid->flip_y = false;
}

/*!
 * Simple lock helper.
 *
 * @ingroup aux_util
 */
static inline void
u_native_images_debug_lock(struct u_native_images_debug *unid)
{
	os_mutex_lock(&unid->mutex);
}

/*!
 * Simple lock helper.
 *
 * @ingroup aux_util
 */
static inline void
u_native_images_debug_unlock(struct u_native_images_debug *unid)
{
	os_mutex_unlock(&unid->mutex);
}

/*!
 * Helper function to update all variables, must be called with the lock held.
 *
 * @ingroup aux_util
 */
static inline void
u_native_images_debug_set_locked(struct u_native_images_debug *unid,
                                 xrt_limited_unique_id_t limited_unique_id,
                                 struct xrt_image_native *native_images,
                                 uint32_t native_image_count,
                                 const struct xrt_swapchain_create_info *xscci,
                                 uint32_t active_index,
                                 bool flip_y)
{
	unid->limited_unique_id = limited_unique_id;
	unid->native_images = native_images;
	unid->native_image_count = native_image_count;
	unid->active_index = active_index;
	unid->xscci = xscci;
	unid->flip_y = flip_y;
}

/*!
 * Updates all variables atomically by holding the lock.
 *
 * @ingroup aux_util
 */
static inline void
u_native_images_debug_set(struct u_native_images_debug *unid,
                          xrt_limited_unique_id_t limited_unique_id,
                          struct xrt_image_native *native_images,
                          uint32_t native_image_count,
                          const struct xrt_swapchain_create_info *xscci,
                          uint32_t active_index,
                          bool flip_y)
{
	u_native_images_debug_lock(unid);
	u_native_images_debug_set_locked( //
	    unid,                         //
	    limited_unique_id,            //
	    native_images,                //
	    native_image_count,           //
	    xscci,                        //
	    active_index,                 //
	    flip_y);                      //
	u_native_images_debug_unlock(unid);
}

/*!
 * Clear all variables, must be called with the lock held.
 *
 * @ingroup aux_util
 */
static inline void
u_native_images_debug_clear_locked(struct u_native_images_debug *unid)
{
	unid->limited_unique_id.data = 0;
	unid->xscci = NULL;
	unid->active_index = 0;
	unid->native_images = NULL;
	unid->native_image_count = 0;
}

/*!
 * Clear all variables atomically by holding the lock, still valid to use.
 *
 * @ingroup aux_util
 */
static inline void
u_native_images_debug_clear(struct u_native_images_debug *unid)
{
	u_native_images_debug_lock(unid);
	u_native_images_debug_clear_locked(unid);
	u_native_images_debug_unlock(unid);
}


/*
 *
 * Swapchain.
 *
 */

/*!
 * Allows to debug image that is in GPU memory.
 *
 * @ingroup aux_util
 */
struct u_swapchain_debug
{
	//! Base for native image debugging.
	struct u_native_images_debug base;

	//! Protected by @p base::mutex.
	struct xrt_swapchain_native *xscn;
};

/*!
 * Must be called before variable is tracked.
 *
 * @ingroup aux_util
 */
static inline void
u_swapchain_debug_init(struct u_swapchain_debug *uscd)
{
	u_native_images_debug_init(&uscd->base);
}

/*!
 * Updates all variables atomically by holding the lock.
 *
 * @ingroup aux_util
 */
static inline void
u_swapchain_debug_set(struct u_swapchain_debug *uscd,
                      struct xrt_swapchain_native *xscn,
                      const struct xrt_swapchain_create_info *xscci,
                      uint32_t active_index,
                      bool flip_y)
{
	u_native_images_debug_lock(&uscd->base);

	u_native_images_debug_set_locked( //
	    &uscd->base,                  //
	    xscn->limited_unique_id,      //
	    xscn->images,                 //
	    xscn->base.image_count,       //
	    xscci,                        //
	    active_index,                 //
	    flip_y);                      //

	xrt_swapchain_native_reference(&uscd->xscn, xscn);

	u_native_images_debug_unlock(&uscd->base);
}

/*!
 * Clear all variables atomically by holding the lock, still valid to use.
 *
 * @ingroup aux_util
 */
static inline void
u_swapchain_debug_clear(struct u_swapchain_debug *uscd)
{
	u_native_images_debug_lock(&uscd->base);
	u_native_images_debug_clear_locked(&uscd->base);
	xrt_swapchain_native_reference(&uscd->xscn, NULL);
	u_native_images_debug_unlock(&uscd->base);
}

/*!
 * Must not be called while variable longer tracked, after @p u_var_remove_root.
 *
 * @ingroup aux_util
 */
static inline void
u_swapchain_debug_destroy(struct u_swapchain_debug *uscd)
{
	xrt_swapchain_native_reference(&uscd->xscn, NULL);
	uscd->base.active_index = 0;
	os_mutex_destroy(&uscd->base.mutex);
}

/*!
 * Simple lock helper.
 *
 * @ingroup aux_util
 */
static inline void
u_swapchain_debug_lock(struct u_swapchain_debug *uscd)
{
	u_native_images_debug_lock(&uscd->base);
}

/*!
 * Simple lock helper.
 *
 * @ingroup aux_util
 */
static inline void
u_swapchain_debug_unlock(struct u_swapchain_debug *uscd)
{
	u_native_images_debug_unlock(&uscd->base);
}


#ifdef __cplusplus
}
#endif
