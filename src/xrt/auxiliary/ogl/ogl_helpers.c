// Copyright 2020, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Common OpenGL code.
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @ingroup aux_ogl
 */

#include "util/u_handles.h"
#include "util/u_logging.h"

#include "ogl_helpers.h"
#include "ogl_api.h" // IWYU pragma: keep

#include <inttypes.h>

/*!
 * Check for OpenGL errors, context needs to be current.
 *
 * @ingroup sdl_test
 */
#define CHECK_GL()                                                                                                     \
	do {                                                                                                           \
		GLint err = glGetError();                                                                              \
		if (err != 0) {                                                                                        \
			U_LOG_RAW("%s:%u: error: 0x%04x", __func__, __LINE__, err);                                    \
		}                                                                                                      \
	} while (false)


/*
 *
 * 'Exported' functions.
 *
 */

void
ogl_texture_target_for_swapchain_info(const struct xrt_swapchain_create_info *info,
                                      uint32_t *out_tex_target,
                                      uint32_t *out_tex_param_name)
{
	// see reference:
	// https://android.googlesource.com/platform/cts/+/master/tests/tests/nativehardware/jni/AHardwareBufferGLTest.cpp#1261
	if (info->face_count == 6) {
		if (info->array_size > 1) {
			*out_tex_target = GL_TEXTURE_CUBE_MAP_ARRAY;
			*out_tex_param_name = GL_TEXTURE_BINDING_CUBE_MAP_ARRAY;
			return;
		}
		*out_tex_target = GL_TEXTURE_CUBE_MAP;
		*out_tex_param_name = GL_TEXTURE_BINDING_CUBE_MAP;
		return;
	}
	// Note: on Android, some sources say always use
	// GL_TEXTURE_EXTERNAL_OES, but AHardwareBufferGLTest only uses it for
	// YUV buffers.
	//! @todo test GL_TEXTURE_EXTERNAL_OES on Android
	if (info->array_size > 1) {
		*out_tex_target = GL_TEXTURE_2D_ARRAY;
		*out_tex_param_name = GL_TEXTURE_BINDING_2D_ARRAY;
		return;
	}
	*out_tex_target = GL_TEXTURE_2D;
	*out_tex_param_name = GL_TEXTURE_BINDING_2D;
}

XRT_CHECK_RESULT uint32_t
ogl_vk_format_to_gl(int64_t vk_format)
{
	switch (vk_format) {
	case 4 /*   VK_FORMAT_R5G6B5_UNORM_PACK16      */: return 0;       // GL_RGB565?
	case 23 /*  VK_FORMAT_R8G8B8_UNORM             */: return GL_RGB8; // Should not be used, colour precision.
	case 29 /*  VK_FORMAT_R8G8B8_SRGB              */: return GL_SRGB8;
	case 30 /*  VK_FORMAT_B8G8R8_UNORM             */: return 0;
	case 37 /*  VK_FORMAT_R8G8B8A8_UNORM           */: return GL_RGBA8; // Should not be used, colour precision.
	case 43 /*  VK_FORMAT_R8G8B8A8_SRGB            */: return GL_SRGB8_ALPHA8;
	case 44 /*  VK_FORMAT_B8G8R8A8_UNORM           */: return 0;
	case 50 /*  VK_FORMAT_B8G8R8A8_SRGB            */: return 0;
	case 64 /*  VK_FORMAT_A2B10G10R10_UNORM_PACK32 */: return GL_RGB10_A2;
	case 84 /*  VK_FORMAT_R16G16B16_UNORM          */: return GL_RGB16;
	case 90 /*  VK_FORMAT_R16G16B16_SFLOAT         */: return GL_RGB16F;
	case 91 /*  VK_FORMAT_R16G16B16A16_UNORM       */: return GL_RGBA16;
	case 97 /*  VK_FORMAT_R16G16B16A16_SFLOAT      */: return GL_RGBA16F;
	case 100 /* VK_FORMAT_R32_SFLOAT               */: return 0;
	case 124 /* VK_FORMAT_D16_UNORM                */: return GL_DEPTH_COMPONENT16;
	case 125 /* VK_FORMAT_X8_D24_UNORM_PACK32      */: return 0; // GL_DEPTH_COMPONENT24?
	case 126 /* VK_FORMAT_D32_SFLOAT               */: return GL_DEPTH_COMPONENT32F;
	case 127 /* VK_FORMAT_S8_UINT                  */: return 0; // GL_STENCIL_INDEX8?
	case 129 /* VK_FORMAT_D24_UNORM_S8_UINT        */: return GL_DEPTH24_STENCIL8;
	case 130 /* VK_FORMAT_D32_SFLOAT_S8_UINT       */: return GL_DEPTH32F_STENCIL8;
	default: U_LOG_W("Cannot convert VK format %" PRIu64 " to GL format!", vk_format); return 0;
	}
}

XRT_CHECK_RESULT bool
ogl_import_from_native(struct xrt_image_native *natives,
                       uint32_t native_count,
                       const struct xrt_swapchain_create_info *info,
                       struct ogl_import_results *results)
{
	// Setup fields.
	results->width = info->width;
	results->height = info->height;
	results->image_count = native_count;

	GLuint binding_enum = 0;
	GLuint tex_target = 0;
	ogl_texture_target_for_swapchain_info(info, &tex_target, &binding_enum);

	GLuint gl_format = ogl_vk_format_to_gl(info->format);

	glCreateTextures(tex_target, native_count, results->textures);
	CHECK_GL();
	glCreateMemoryObjectsEXT(native_count, results->memories);
	CHECK_GL();

	for (uint32_t i = 0; i < native_count; i++) {
		GLint dedicated = natives[i].use_dedicated_allocation ? GL_TRUE : GL_FALSE;
		glMemoryObjectParameterivEXT(results->memories[i], GL_DEDICATED_MEMORY_OBJECT_EXT, &dedicated);
		CHECK_GL();

		// The below function consumes the handle, need to reference it.
		xrt_graphics_buffer_handle_t handle = u_graphics_buffer_ref(natives[i].handle);

		glImportMemoryFdEXT(              //
		    results->memories[i],         //
		    natives[i].size,              //
		    GL_HANDLE_TYPE_OPAQUE_FD_EXT, //
		    (GLint)handle);               //
		CHECK_GL();

		if (info->array_size == 1) {
			glTextureStorageMem2DEXT( //
			    results->textures[i], //
			    info->mip_count,      //
			    gl_format,            //
			    info->width,          //
			    info->height,         //
			    results->memories[i], //
			    0);                   //
		} else {
			glTextureStorageMem3DEXT( //
			    results->textures[i], //
			    info->mip_count,      //
			    gl_format,            //
			    info->width,          //
			    info->height,         //
			    info->array_size,     //
			    results->memories[i], //
			    0);                   //
		}
		CHECK_GL();
	}

	return true;
}
