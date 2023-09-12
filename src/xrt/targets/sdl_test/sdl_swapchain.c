// Copyright 2022-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Swapchain code for the sdl code.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup sdl_test
 */

#include "sdl_internal.h"

#include "util/u_handles.h"
#include "ogl/ogl_helpers.h"

#include <inttypes.h>


static void
post_init_setup(struct sdl_swapchain *ssc, struct sdl_program *sp, const struct xrt_swapchain_create_info *info)
{
	ST_DEBUG(sp, "CREATE");

	sdl_make_current(sp);

	struct ogl_import_results results = XRT_STRUCT_INIT;
	bool bret = ogl_import_from_native(  //
	    ssc->base.base.images,           // native_images
	    ssc->base.base.base.image_count, // native_count
	    info,                            // info
	    &results);                       // results
	assert(bret);
	(void)bret;

	sdl_make_uncurrent(sp);

	// Setup fields
	ssc->sp = sp;
	ssc->w = (int)results.width;
	ssc->h = (int)results.height;

	for (uint32_t i = 0; i < results.image_count; i++) {
		ssc->memory[i] = results.memories[i];
		ssc->textures[i] = results.textures[i];
	}
}

static void
really_destroy(struct comp_swapchain *sc)
{
	struct sdl_swapchain *ssc = (struct sdl_swapchain *)sc;
	struct sdl_program *sp = ssc->sp;

	ST_DEBUG(sp, "DESTROY");

	sdl_make_current(sp);

	uint32_t image_count = ssc->base.base.base.image_count;
	if (image_count > 0) {
		glDeleteTextures(image_count, ssc->textures);
		glDeleteMemoryObjectsEXT(image_count, ssc->memory);

		U_ZERO_ARRAY(ssc->textures);
		U_ZERO_ARRAY(ssc->memory);
	}

	sdl_make_uncurrent(sp);

	// Teardown the base swapchain, freeing all Vulkan resources.
	comp_swapchain_teardown(sc);

	// Teardown does not free the struct itself.
	free(ssc);
}


/*
 *
 * 'Exported' functions.
 *
 */

xrt_result_t
sdl_swapchain_create(struct xrt_compositor *xc,
                     const struct xrt_swapchain_create_info *info,
                     struct xrt_swapchain **out_xsc)
{
	struct sdl_program *sp = from_comp(xc);
	xrt_result_t xret;

	/*
	 * In case the default get properties function have been overridden
	 * make sure to correctly dispatch the call to get the properties.
	 */
	struct xrt_swapchain_create_properties xsccp = {0};
	xrt_comp_get_swapchain_create_properties(xc, info, &xsccp);

	struct sdl_swapchain *ssc = U_TYPED_CALLOC(struct sdl_swapchain);

	xret = comp_swapchain_create_init( //
	    &ssc->base,                    //
	    really_destroy,                //
	    &sp->c.base.vk,                //
	    &sp->c.base.cscs,              //
	    info,                          //
	    &xsccp);                       //
	if (xret != XRT_SUCCESS) {
		free(ssc);
		return xret;
	}

	// Init SDL fields and create OpenGL resources.
	post_init_setup(ssc, sp, info);

	// Correctly setup refcounts, init sets refcount to zero.
	xrt_swapchain_reference(out_xsc, &ssc->base.base.base);

	return xret;
}

xrt_result_t
sdl_swapchain_import(struct xrt_compositor *xc,
                     const struct xrt_swapchain_create_info *info,
                     struct xrt_image_native *native_images,
                     uint32_t native_image_count,
                     struct xrt_swapchain **out_xsc)
{
	struct sdl_program *sp = from_comp(xc);
	xrt_result_t xret;

	struct sdl_swapchain *ssc = U_TYPED_CALLOC(struct sdl_swapchain);

	xret = comp_swapchain_import_init( //
	    &ssc->base,                    //
	    really_destroy,                //
	    &sp->c.base.vk,                //
	    &sp->c.base.cscs,              //
	    info,                          //
	    native_images,                 //
	    native_image_count);           //
	if (xret != XRT_SUCCESS) {
		free(ssc);
		return xret;
	}

	// Init SDL fields and create OpenGL resources.
	post_init_setup(ssc, sp, info);

	// Correctly setup refcounts, init sets refcount to zero.
	xrt_swapchain_reference(out_xsc, &ssc->base.base.base);

	return xret;
}
