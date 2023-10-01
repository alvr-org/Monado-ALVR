// Copyright 2019-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Shader loading code.
 * @author Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup comp_render
 */

#include "xrt/xrt_config_build.h"
#include "vk/vk_mini_helpers.h"

#include "render/render_interface.h"


/*
 *
 * Shader headers.
 *
 */

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wnewline-eof"
#endif

#include "shaders/blit.comp.h"
#include "shaders/clear.comp.h"
#include "shaders/layer.comp.h"
#include "shaders/distortion.comp.h"
#include "shaders/layer.frag.h"
#include "shaders/layer.vert.h"
#include "shaders/equirect1.frag.h"
#include "shaders/equirect1.vert.h"
#include "shaders/equirect2.frag.h"
#include "shaders/equirect2.vert.h"
#include "shaders/mesh.frag.h"
#include "shaders/mesh.vert.h"

#ifdef XRT_FEATURE_OPENXR_LAYER_CUBE
#include "shaders/cube.frag.h"
#include "shaders/cube.vert.h"
#endif

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif


/*
 *
 * Helpers
 *
 */

#define LOAD(SHADER)                                                                                                   \
	do {                                                                                                           \
		const uint32_t *code = shaders_##SHADER;                                                               \
		size_t size = sizeof(shaders_##SHADER);                                                                \
		VkResult ret = shader_load(vk,          /* vk_bundle */                                                \
		                           code,        /* code      */                                                \
		                           size,        /* size      */                                                \
		                           &s->SHADER); /* out       */                                                \
		if (ret != VK_SUCCESS) {                                                                               \
			VK_ERROR(vk, "Failed to load shader '" #SHADER "'");                                           \
			render_shaders_close(s, vk);                                                                   \
			return false;                                                                                  \
		}                                                                                                      \
	} while (false)


/*
 *
 * Functions.
 *
 */

XRT_CHECK_RESULT static VkResult
shader_load(struct vk_bundle *vk, const uint32_t *code, size_t size, VkShaderModule *out_module)
{
	VkResult ret;

	VkShaderModuleCreateInfo info = {
	    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
	    .codeSize = size,
	    .pCode = code,
	};

	VkShaderModule module;
	ret = vk->vkCreateShaderModule( //
	    vk->device,                 //
	    &info,                      //
	    NULL,                       //
	    &module);                   //
	VK_CHK_AND_RET(ret, "vkCreateShaderModule");

	*out_module = module;

	return VK_SUCCESS;
}

bool
render_shaders_load(struct render_shaders *s, struct vk_bundle *vk)
{
	LOAD(blit_comp);

	LOAD(clear_comp);

	LOAD(layer_comp);

	LOAD(distortion_comp);

	LOAD(mesh_vert);
	LOAD(mesh_frag);

	LOAD(equirect1_vert);
	LOAD(equirect1_frag);

	LOAD(equirect2_vert);
	LOAD(equirect2_frag);

#ifdef XRT_FEATURE_OPENXR_LAYER_CUBE
	LOAD(cube_vert);
	LOAD(cube_frag);
#endif

	LOAD(layer_vert);
	LOAD(layer_frag);

	VK_DEBUG(vk, "Shaders loaded!");

	return true;
}

void
render_shaders_close(struct render_shaders *s, struct vk_bundle *vk)
{
	D(ShaderModule, s->blit_comp);
	D(ShaderModule, s->clear_comp);
	D(ShaderModule, s->distortion_comp);
	D(ShaderModule, s->layer_comp);
	D(ShaderModule, s->mesh_vert);
	D(ShaderModule, s->mesh_frag);
	D(ShaderModule, s->equirect1_vert);
	D(ShaderModule, s->equirect1_frag);
	D(ShaderModule, s->equirect2_vert);
	D(ShaderModule, s->equirect2_frag);
#ifdef XRT_FEATURE_OPENXR_LAYER_CUBE
	D(ShaderModule, s->cube_vert);
	D(ShaderModule, s->cube_frag);
#endif
	D(ShaderModule, s->layer_vert);
	D(ShaderModule, s->layer_frag);

	VK_DEBUG(vk, "Shaders destroyed!");
}
