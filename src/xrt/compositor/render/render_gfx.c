// Copyright 2019-2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  The NEW compositor rendering code header.
 * @author Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup comp_render
 */

#include "vk/vk_mini_helpers.h"

#include "render/render_interface.h"

#include <stdio.h>


/*
 *
 * Common helpers
 *
 */

/*!
 * Get the @ref vk_bundle from @ref render_gfx_target_resources.
 */
static inline struct vk_bundle *
vk_from_rtr(struct render_gfx_target_resources *rtr)
{
	return rtr->r->vk;
}

/*gfx_target_resources framebuffer@ref render_gfx.
 */
static inline struct vk_bundle *
vk_from_rr(struct render_gfx *rr)
{
	return rr->r->vk;
}

XRT_CHECK_RESULT static VkResult
create_implicit_render_pass(struct vk_bundle *vk,
                            VkFormat format,
                            VkAttachmentLoadOp load_op,
                            VkImageLayout final_layout,
                            VkRenderPass *out_render_pass)
{
	VkResult ret;

	VkAttachmentDescription attachments[1] = {
	    {
	        .format = format,
	        .samples = VK_SAMPLE_COUNT_1_BIT,
	        .loadOp = load_op,
	        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
	        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
	        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
	        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	        .finalLayout = final_layout,
	        .flags = 0,
	    },
	};

	VkAttachmentReference color_reference = {
	    .attachment = 0,
	    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};

	VkSubpassDescription subpasses[1] = {
	    {
	        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
	        .inputAttachmentCount = 0,
	        .pInputAttachments = NULL,
	        .colorAttachmentCount = 1,
	        .pColorAttachments = &color_reference,
	        .pResolveAttachments = NULL,
	        .pDepthStencilAttachment = NULL,
	        .preserveAttachmentCount = 0,
	        .pPreserveAttachments = NULL,
	    },
	};

	/*
	 * We don't use any VkSubpassDependency structs, instead relying on the
	 * implicit dependencies inserted by the runtime implementation.
	 */

	VkRenderPassCreateInfo render_pass_info = {
	    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
	    .attachmentCount = ARRAY_SIZE(attachments),
	    .pAttachments = attachments,
	    .subpassCount = ARRAY_SIZE(subpasses),
	    .pSubpasses = subpasses,
	    .dependencyCount = 0,
	    .pDependencies = NULL,
	};

	VkRenderPass render_pass = VK_NULL_HANDLE;
	ret = vk->vkCreateRenderPass( //
	    vk->device,               //
	    &render_pass_info,        //
	    NULL,                     //
	    &render_pass);            //
	VK_CHK_AND_RET(ret, "vkCreateRenderPass");

	*out_render_pass = render_pass;

	return VK_SUCCESS;
}

XRT_CHECK_RESULT static VkResult
create_framebuffer(struct vk_bundle *vk,
                   VkImageView image_view,
                   VkRenderPass render_pass,
                   uint32_t width,
                   uint32_t height,
                   VkFramebuffer *out_external_framebuffer)
{
	VkResult ret;

	VkImageView attachments[1] = {image_view};

	VkFramebufferCreateInfo frame_buffer_info = {
	    .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
	    .renderPass = render_pass,
	    .attachmentCount = ARRAY_SIZE(attachments),
	    .pAttachments = attachments,
	    .width = width,
	    .height = height,
	    .layers = 1,
	};

	VkFramebuffer framebuffer = VK_NULL_HANDLE;
	ret = vk->vkCreateFramebuffer( //
	    vk->device,                //
	    &frame_buffer_info,        //
	    NULL,                      //
	    &framebuffer);             //
	VK_CHK_AND_RET(ret, "vkCreateFramebuffer");

	*out_external_framebuffer = framebuffer;

	return VK_SUCCESS;
}

static void
begin_render_pass(struct vk_bundle *vk,
                  VkCommandBuffer command_buffer,
                  VkRenderPass render_pass,
                  VkFramebuffer framebuffer,
                  uint32_t width,
                  uint32_t height,
                  const VkClearColorValue *color)
{
	VkClearValue clear_color[1] = {{
	    .color = *color,
	}};

	VkRenderPassBeginInfo render_pass_begin_info = {
	    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
	    .renderPass = render_pass,
	    .framebuffer = framebuffer,
	    .renderArea =
	        {
	            .offset =
	                {
	                    .x = 0,
	                    .y = 0,
	                },
	            .extent =
	                {
	                    .width = width,
	                    .height = height,
	                },
	        },
	    .clearValueCount = ARRAY_SIZE(clear_color),
	    .pClearValues = clear_color,
	};

	vk->vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

static void
update_ubo_and_src_descriptor_set(struct vk_bundle *vk,
                                  uint32_t ubo_binding,
                                  VkBuffer buffer,
                                  VkDeviceSize offset,
                                  VkDeviceSize size,
                                  uint32_t src_binding,
                                  VkSampler sampler,
                                  VkImageView image_view,
                                  VkDescriptorSet descriptor_set)
{
	VkDescriptorImageInfo image_info = {
	    .sampler = sampler,
	    .imageView = image_view,
	    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	};

	VkDescriptorBufferInfo buffer_info = {
	    .buffer = buffer,
	    .offset = offset,
	    .range = size,
	};

	VkWriteDescriptorSet write_descriptor_sets[2] = {
	    {
	        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	        .dstSet = descriptor_set,
	        .dstBinding = src_binding,
	        .descriptorCount = 1,
	        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	        .pImageInfo = &image_info,
	    },
	    {
	        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	        .dstSet = descriptor_set,
	        .dstBinding = ubo_binding,
	        .descriptorCount = 1,
	        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	        .pBufferInfo = &buffer_info,
	    },
	};

	vk->vkUpdateDescriptorSets(            //
	    vk->device,                        //
	    ARRAY_SIZE(write_descriptor_sets), // descriptorWriteCount
	    write_descriptor_sets,             // pDescriptorWrites
	    0,                                 // descriptorCopyCount
	    NULL);                             // pDescriptorCopies
}

XRT_CHECK_RESULT static VkResult
do_ubo_and_src_alloc_and_write(struct render_gfx *rr,
                               uint32_t ubo_binding,
                               const void *ubo_ptr,
                               VkDeviceSize ubo_size,
                               uint32_t src_binding,
                               VkSampler src_sampler,
                               VkImageView src_image_view,
                               VkDescriptorPool descriptor_pool,
                               VkDescriptorSetLayout descriptor_set_layout,
                               VkDescriptorSet *out_descriptor_set)
{
	VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
	struct render_sub_alloc ubo = XRT_STRUCT_INIT;
	struct vk_bundle *vk = vk_from_rr(rr);

	VkResult ret;


	/*
	 * Allocate and upload data.
	 */
	ret = render_sub_alloc_ubo_alloc_and_write( //
	    vk,                                     // vk_bundle
	    &rr->ubo_tracker,                       // rsat
	    ubo_ptr,                                // ptr
	    ubo_size,                               // size
	    &ubo);                                  // out_rsa
	VK_CHK_AND_RET(ret, "render_sub_alloc_ubo_alloc_and_write");


	/*
	 * Create and fill out destriptor.
	 */

	ret = vk_create_descriptor_set( //
	    vk,                         // vk_bundle
	    descriptor_pool,            // descriptor_pool
	    descriptor_set_layout,      // descriptor_set_layout
	    &descriptor_set);           // descriptor_set
	VK_CHK_AND_RET(ret, "vk_create_descriptor_set");

	update_ubo_and_src_descriptor_set( //
	    vk,                            // vk_bundle
	    ubo_binding,                   // ubo_binding
	    ubo.buffer,                    // buffer
	    ubo.offset,                    // offset
	    ubo.size,                      // size
	    src_binding,                   // src_binding
	    src_sampler,                   // sampler
	    src_image_view,                // image_view
	    descriptor_set);               // descriptor_set

	*out_descriptor_set = descriptor_set;

	return VK_SUCCESS;
}

static inline void
dispatch_no_vbo(struct render_gfx *rr, uint32_t vertex_count, VkPipeline pipeline, VkDescriptorSet descriptor_set)
{
	struct vk_bundle *vk = vk_from_rr(rr);
	struct render_resources *r = rr->r;


	VkDescriptorSet descriptor_sets[1] = {descriptor_set};
	vk->vkCmdBindDescriptorSets(             //
	    r->cmd,                              // commandBuffer
	    VK_PIPELINE_BIND_POINT_GRAPHICS,     // pipelineBindPoint
	    r->gfx.layer.shared.pipeline_layout, // layout
	    0,                                   // firstSet
	    ARRAY_SIZE(descriptor_sets),         // descriptorSetCount
	    descriptor_sets,                     // pDescriptorSets
	    0,                                   // dynamicOffsetCount
	    NULL);                               // pDynamicOffsets

	vk->vkCmdBindPipeline(               //
	    r->cmd,                          // commandBuffer
	    VK_PIPELINE_BIND_POINT_GRAPHICS, // pipelineBindPoint
	    pipeline);                       // pipeline

	// This pipeline doesn't have any VBO input or indices.

	vk->vkCmdDraw(    //
	    r->cmd,       // commandBuffer
	    vertex_count, // vertexCount
	    1,            // instanceCount
	    0,            // firstVertex
	    0);           // firstInstance
}


/*
 *
 * Layer
 *
 */

XRT_CHECK_RESULT static VkResult
create_layer_pipeline(struct vk_bundle *vk,
                      VkRenderPass render_pass,
                      VkPipelineLayout pipeline_layout,
                      VkPipelineCache pipeline_cache,
                      VkBlendFactor src_blend_factor,
                      VkShaderModule module_vert,
                      VkShaderModule module_frag,
                      VkPipeline *out_pipeline)
{
	VkResult ret;

	// Might be changed to line for debugging.
	VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;

	// Generate vertices inside of the vertex shader.
	const VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
	    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
	    .primitiveRestartEnable = VK_FALSE,
	};

	const VkPipelineVertexInputStateCreateInfo vertex_input_state = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
	    .vertexAttributeDescriptionCount = 0,
	    .pVertexAttributeDescriptions = NULL,
	    .vertexBindingDescriptionCount = 0,
	    .pVertexBindingDescriptions = NULL,
	};


	/*
	 * Target and rasterisation.
	 */

	const VkPipelineViewportStateCreateInfo viewport_state = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
	    .viewportCount = 1,
	    .scissorCount = 1,
	};

	const VkPipelineMultisampleStateCreateInfo multisample_state = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
	    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
	};

	const VkPipelineRasterizationStateCreateInfo rasterization_state = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
	    .depthClampEnable = VK_FALSE,
	    .rasterizerDiscardEnable = VK_FALSE,
	    .polygonMode = polygonMode,
	    .cullMode = VK_CULL_MODE_BACK_BIT,
	    .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
	    .lineWidth = 1.0f,
	};


	/*
	 * Blending.
	 */

	const VkColorComponentFlags all_components = //
	    VK_COLOR_COMPONENT_R_BIT |               //
	    VK_COLOR_COMPONENT_G_BIT |               //
	    VK_COLOR_COMPONENT_B_BIT |               //
	    VK_COLOR_COMPONENT_A_BIT;                //

	/*
	 * We are using VK_BLEND_FACTOR_ONE for the dst alpha write
	 * to make sure that there is a valid value there, makes
	 * the debug UI work when inspecting the scratch images.
	 */
	const VkPipelineColorBlendAttachmentState blend_attachment_state = {
	    .blendEnable = VK_TRUE,
	    .colorWriteMask = all_components,
	    .srcColorBlendFactor = src_blend_factor,
	    .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
	    .colorBlendOp = VK_BLEND_OP_ADD,
	    .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
	    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
	    .alphaBlendOp = VK_BLEND_OP_ADD,
	};

	const VkPipelineColorBlendStateCreateInfo color_blend_state = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
	    .attachmentCount = 1,
	    .pAttachments = &blend_attachment_state,
	};

	const VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
	    .depthTestEnable = VK_FALSE,
	    .depthWriteEnable = VK_FALSE,
	    .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
	    .front = {.compareOp = VK_COMPARE_OP_ALWAYS},
	    .back = {.compareOp = VK_COMPARE_OP_ALWAYS},
	};


	/*
	 * Dynamic state.
	 */

	const VkDynamicState dynamic_states[] = {
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_SCISSOR,
	};

	const VkPipelineDynamicStateCreateInfo dynamic_state = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
	    .dynamicStateCount = ARRAY_SIZE(dynamic_states),
	    .pDynamicStates = dynamic_states,
	};


	/*
	 * Shaders.
	 */

	const VkPipelineShaderStageCreateInfo shader_stages[2] = {
	    {
	        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	        .stage = VK_SHADER_STAGE_VERTEX_BIT,
	        .module = module_vert,
	        .pName = "main",
	    },
	    {
	        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
	        .module = module_frag,
	        .pName = "main",
	    },
	};


	/*
	 * Bringing it all together.
	 */

	const VkGraphicsPipelineCreateInfo pipeline_info = {
	    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
	    .stageCount = ARRAY_SIZE(shader_stages),
	    .pStages = shader_stages,
	    .pVertexInputState = &vertex_input_state,
	    .pInputAssemblyState = &input_assembly_state,
	    .pViewportState = &viewport_state,
	    .pRasterizationState = &rasterization_state,
	    .pMultisampleState = &multisample_state,
	    .pDepthStencilState = &depth_stencil_state,
	    .pColorBlendState = &color_blend_state,
	    .pDynamicState = &dynamic_state,
	    .layout = pipeline_layout,
	    .renderPass = render_pass,
	    .basePipelineHandle = VK_NULL_HANDLE,
	    .basePipelineIndex = -1,
	};

	VkPipeline pipeline = VK_NULL_HANDLE;
	ret = vk->vkCreateGraphicsPipelines( //
	    vk->device,                      //
	    pipeline_cache,                  //
	    1,                               //
	    &pipeline_info,                  //
	    NULL,                            //
	    &pipeline);                      //
	VK_CHK_AND_RET(ret, "vkCreateGraphicsPipelines");

	*out_pipeline = pipeline;

	return VK_SUCCESS;
}


/*
 *
 * Mesh
 *
 */

struct mesh_params
{
	uint32_t do_timewarp;
};

XRT_CHECK_RESULT static VkResult
create_mesh_pipeline(struct vk_bundle *vk,
                     VkRenderPass render_pass,
                     VkPipelineLayout pipeline_layout,
                     VkPipelineCache pipeline_cache,
                     uint32_t src_binding,
                     uint32_t mesh_index_count_total,
                     uint32_t mesh_stride,
                     const struct mesh_params *params,
                     VkShaderModule mesh_vert,
                     VkShaderModule mesh_frag,
                     VkPipeline *out_mesh_pipeline)
{
	VkResult ret;

	// Might be changed to line for debugging.
	VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;

	// Do we use triangle strips or triangles with indices.
	VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	if (mesh_index_count_total > 0) {
		topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	}

	VkPipelineInputAssemblyStateCreateInfo input_assembly_state = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
	    .topology = topology,
	    .primitiveRestartEnable = VK_FALSE,
	};

	VkPipelineRasterizationStateCreateInfo rasterization_state = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
	    .depthClampEnable = VK_FALSE,
	    .rasterizerDiscardEnable = VK_FALSE,
	    .polygonMode = polygonMode,
	    .cullMode = VK_CULL_MODE_BACK_BIT,
	    .frontFace = VK_FRONT_FACE_CLOCKWISE,
	    .lineWidth = 1.0f,
	};

	VkPipelineColorBlendAttachmentState blend_attachment_state = {
	    .blendEnable = VK_FALSE,
	    .colorWriteMask = 0xf,
	};

	VkPipelineColorBlendStateCreateInfo color_blend_state = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
	    .attachmentCount = 1,
	    .pAttachments = &blend_attachment_state,
	};

	VkPipelineDepthStencilStateCreateInfo depth_stencil_state = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
	    .depthTestEnable = VK_FALSE,
	    .depthWriteEnable = VK_FALSE,
	    .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
	    .front = {.compareOp = VK_COMPARE_OP_ALWAYS},
	    .back = {.compareOp = VK_COMPARE_OP_ALWAYS},
	};

	VkPipelineViewportStateCreateInfo viewport_state = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
	    .viewportCount = 1,
	    .scissorCount = 1,
	};

	VkPipelineMultisampleStateCreateInfo multisample_state = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
	    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
	};

	VkDynamicState dynamic_states[] = {
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_SCISSOR,
	};

	VkPipelineDynamicStateCreateInfo dynamic_state = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
	    .dynamicStateCount = ARRAY_SIZE(dynamic_states),
	    .pDynamicStates = dynamic_states,
	};

	// clang-format off
	VkVertexInputAttributeDescription vertex_input_attribute_descriptions[2] = {
	    {
	        .binding = src_binding,
	        .location = 0,
	        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
	        .offset = 0,
	    },
	    {
	        .binding = src_binding,
	        .location = 1,
	        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
	        .offset = 16,
	    },
	};

	VkVertexInputBindingDescription vertex_input_binding_description[1] = {
	    {
	        .binding = src_binding,
	        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
	        .stride = mesh_stride,
	    },
	};

	VkPipelineVertexInputStateCreateInfo vertex_input_state = {
	    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
	    .vertexAttributeDescriptionCount = ARRAY_SIZE(vertex_input_attribute_descriptions),
	    .pVertexAttributeDescriptions = vertex_input_attribute_descriptions,
	    .vertexBindingDescriptionCount = ARRAY_SIZE(vertex_input_binding_description),
	    .pVertexBindingDescriptions = vertex_input_binding_description,
	};
	// clang-format on

#define ENTRY(ID, FIELD)                                                                                               \
	{                                                                                                              \
	    .constantID = ID,                                                                                          \
	    .offset = offsetof(struct mesh_params, FIELD),                                                             \
	    .size = sizeof(params->FIELD),                                                                             \
	}

	VkSpecializationMapEntry vert_entries[] = {
	    ENTRY(0, do_timewarp),
	};
#undef ENTRY

	VkSpecializationInfo vert_specialization_info = {
	    .mapEntryCount = ARRAY_SIZE(vert_entries),
	    .pMapEntries = vert_entries,
	    .dataSize = sizeof(*params),
	    .pData = params,
	};

	VkPipelineShaderStageCreateInfo shader_stages[2] = {
	    {
	        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	        .stage = VK_SHADER_STAGE_VERTEX_BIT,
	        .module = mesh_vert,
	        .pSpecializationInfo = &vert_specialization_info,
	        .pName = "main",
	    },
	    {
	        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
	        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
	        .module = mesh_frag,
	        .pName = "main",
	    },
	};

	VkGraphicsPipelineCreateInfo pipeline_info = {
	    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
	    .stageCount = ARRAY_SIZE(shader_stages),
	    .pStages = shader_stages,
	    .pVertexInputState = &vertex_input_state,
	    .pInputAssemblyState = &input_assembly_state,
	    .pViewportState = &viewport_state,
	    .pRasterizationState = &rasterization_state,
	    .pMultisampleState = &multisample_state,
	    .pDepthStencilState = &depth_stencil_state,
	    .pColorBlendState = &color_blend_state,
	    .pDynamicState = &dynamic_state,
	    .layout = pipeline_layout,
	    .renderPass = render_pass,
	    .basePipelineHandle = VK_NULL_HANDLE,
	    .basePipelineIndex = -1,
	};

	VkPipeline pipeline = VK_NULL_HANDLE;
	ret = vk->vkCreateGraphicsPipelines( //
	    vk->device,                      //
	    pipeline_cache,                  //
	    1,                               //
	    &pipeline_info,                  //
	    NULL,                            //
	    &pipeline);                      //
	VK_CHK_AND_RET(ret, "vkCreateGraphicsPipelines");

	*out_mesh_pipeline = pipeline;

	return VK_SUCCESS;
}


/*
 *
 * 'Exported' render pass functions.
 *
 */

bool
render_gfx_render_pass_init(struct render_gfx_render_pass *rgrp,
                            struct render_resources *r,
                            VkFormat format,
                            VkAttachmentLoadOp load_op,
                            VkImageLayout final_layout)
{
	struct vk_bundle *vk = r->vk;
	VkResult ret;

	ret = create_implicit_render_pass( //
	    vk,                            // vk_bundle
	    format,                        // target_format
	    load_op,                       // load_op
	    final_layout,                  // final_layout
	    &rgrp->render_pass);           // out_render_pass
	VK_CHK_WITH_RET(ret, "create_implicit_render_pass", false);
	VK_NAME_RENDER_PASS(vk, rgrp->render_pass, "render_gfx_render_pass render pass");

	struct mesh_params simple_params = {
	    .do_timewarp = false,
	};

	ret = create_mesh_pipeline(    //
	    vk,                        // vk_bundle
	    rgrp->render_pass,         // render_pass
	    r->mesh.pipeline_layout,   // pipeline_layout
	    r->pipeline_cache,         // pipeline_cache
	    r->mesh.src_binding,       // src_binding
	    r->mesh.index_count_total, // mesh_index_count_total
	    r->mesh.stride,            // mesh_stride
	    &simple_params,            // params
	    r->shaders->mesh_vert,     // mesh_vert
	    r->shaders->mesh_frag,     // mesh_frag
	    &rgrp->mesh.pipeline);     // out_mesh_pipeline
	VK_CHK_WITH_RET(ret, "create_mesh_pipeline", false);
	VK_NAME_PIPELINE(vk, rgrp->mesh.pipeline, "render_gfx_render_pass mesh pipeline");

	struct mesh_params timewarp_params = {
	    .do_timewarp = true,
	};

	ret = create_mesh_pipeline(         //
	    vk,                             // vk_bundle
	    rgrp->render_pass,              // render_pass
	    r->mesh.pipeline_layout,        // pipeline_layout
	    r->pipeline_cache,              // pipeline_cache
	    r->mesh.src_binding,            // src_binding
	    r->mesh.index_count_total,      // mesh_index_count_total
	    r->mesh.stride,                 // mesh_stride
	    &timewarp_params,               // params
	    r->shaders->mesh_vert,          // mesh_vert
	    r->shaders->mesh_frag,          // mesh_frag
	    &rgrp->mesh.pipeline_timewarp); // out_mesh_pipeline
	VK_CHK_WITH_RET(ret, "create_mesh_pipeline", false);
	VK_NAME_PIPELINE(vk, rgrp->mesh.pipeline_timewarp, "render_gfx_render_pass mesh pipeline timewarp");

	const VkBlendFactor blend_factor_premultiplied_alpha = VK_BLEND_FACTOR_ONE;
	const VkBlendFactor blend_factor_unpremultiplied_alpha = VK_BLEND_FACTOR_SRC_ALPHA;

	// Cylinder
	ret = create_layer_pipeline(                    //
	    vk,                                         // vk
	    rgrp->render_pass,                          // render_pass
	    r->gfx.layer.shared.pipeline_layout,        // pipeline_layout
	    r->pipeline_cache,                          // pipeline_cache
	    blend_factor_premultiplied_alpha,           // src_blend_factor
	    r->shaders->layer_cylinder_vert,            // module_vert
	    r->shaders->layer_cylinder_frag,            // module_frag
	    &rgrp->layer.cylinder_premultiplied_alpha); // out_pipeline
	VK_CHK_WITH_RET(ret, "create_layer_pipeline", false);
	VK_NAME_PIPELINE(vk, rgrp->layer.cylinder_premultiplied_alpha,
	                 "render_gfx_render_pass cylinder premultiplied alpha");

	ret = create_layer_pipeline(                      //
	    vk,                                           // vk
	    rgrp->render_pass,                            // render_pass
	    r->gfx.layer.shared.pipeline_layout,          // pipeline_layout
	    r->pipeline_cache,                            // pipeline_cache
	    blend_factor_unpremultiplied_alpha,           // src_blend_factor
	    r->shaders->layer_cylinder_vert,              // module_vert
	    r->shaders->layer_cylinder_frag,              // module_frag
	    &rgrp->layer.cylinder_unpremultiplied_alpha); // out_pipeline
	VK_CHK_WITH_RET(ret, "create_layer_pipeline", false);
	VK_NAME_PIPELINE(vk, rgrp->layer.cylinder_unpremultiplied_alpha,
	                 "render_gfx_render_pass cylinder unpremultiplied alpha");

	// Equirect2
	ret = create_layer_pipeline(                     //
	    vk,                                          // vk
	    rgrp->render_pass,                           // render_pass
	    r->gfx.layer.shared.pipeline_layout,         // pipeline_layout
	    r->pipeline_cache,                           // pipeline_cache
	    blend_factor_premultiplied_alpha,            // src_blend_factor
	    r->shaders->layer_equirect2_vert,            // module_vert
	    r->shaders->layer_equirect2_frag,            // module_frag
	    &rgrp->layer.equirect2_premultiplied_alpha); // out_pipeline
	VK_CHK_WITH_RET(ret, "create_layer_pipeline", false);
	VK_NAME_PIPELINE(vk, rgrp->layer.equirect2_premultiplied_alpha,
	                 "render_gfx_render_pass equirect2 premultiplied alpha");

	ret = create_layer_pipeline(                       //
	    vk,                                            // vk
	    rgrp->render_pass,                             // render_pass
	    r->gfx.layer.shared.pipeline_layout,           // pipeline_layout
	    r->pipeline_cache,                             // pipeline_cache
	    blend_factor_unpremultiplied_alpha,            // src_blend_factor
	    r->shaders->layer_equirect2_vert,              // module_vert
	    r->shaders->layer_equirect2_frag,              // module_frag
	    &rgrp->layer.equirect2_unpremultiplied_alpha); // out_pipeline
	VK_CHK_WITH_RET(ret, "create_layer_pipeline", false);
	VK_NAME_PIPELINE(vk, rgrp->layer.equirect2_unpremultiplied_alpha,
	                 "render_gfx_render_pass equirect2 unpremultiplied alpha");

	// Projection.
	ret = create_layer_pipeline(                //
	    vk,                                     // vk
	    rgrp->render_pass,                      // render_pass
	    r->gfx.layer.shared.pipeline_layout,    // pipeline_layout
	    r->pipeline_cache,                      // pipeline_cache
	    blend_factor_premultiplied_alpha,       // src_blend_factor
	    r->shaders->layer_projection_vert,      // module_vert
	    r->shaders->layer_shared_frag,          // module_frag
	    &rgrp->layer.proj_premultiplied_alpha); // out_pipeline
	VK_CHK_WITH_RET(ret, "create_layer_pipeline", false);
	VK_NAME_PIPELINE(vk, rgrp->layer.proj_premultiplied_alpha,
	                 "render_gfx_render_pass projection premultiplied alpha");

	ret = create_layer_pipeline(                  //
	    vk,                                       // vk
	    rgrp->render_pass,                        // render_pass
	    r->gfx.layer.shared.pipeline_layout,      // pipeline_layout
	    r->pipeline_cache,                        // pipeline_cache
	    blend_factor_unpremultiplied_alpha,       // src_blend_factor
	    r->shaders->layer_projection_vert,        // module_vert
	    r->shaders->layer_shared_frag,            // module_frag
	    &rgrp->layer.proj_unpremultiplied_alpha); // out_pipeline
	VK_CHK_WITH_RET(ret, "create_layer_pipeline", false);
	VK_NAME_PIPELINE(vk, rgrp->layer.proj_unpremultiplied_alpha,
	                 "render_gfx_render_pass projection unpremultiplied alpha");

	// Quad
	ret = create_layer_pipeline(                //
	    vk,                                     // vk
	    rgrp->render_pass,                      // render_pass
	    r->gfx.layer.shared.pipeline_layout,    // pipeline_layout
	    r->pipeline_cache,                      // pipeline_cache
	    blend_factor_premultiplied_alpha,       // src_blend_factor
	    r->shaders->layer_quad_vert,            // module_vert
	    r->shaders->layer_shared_frag,          // module_frag
	    &rgrp->layer.quad_premultiplied_alpha); // out_pipeline
	VK_CHK_WITH_RET(ret, "create_layer_pipeline", false);
	VK_NAME_PIPELINE(vk, rgrp->layer.quad_premultiplied_alpha, "render_gfx_render_pass quad premultiplied alpha");

	ret = create_layer_pipeline(                  //
	    vk,                                       // vk
	    rgrp->render_pass,                        // render_pass
	    r->gfx.layer.shared.pipeline_layout,      // pipeline_layout
	    r->pipeline_cache,                        // pipeline_cache
	    blend_factor_unpremultiplied_alpha,       // src_blend_factor
	    r->shaders->layer_quad_vert,              // module_vert
	    r->shaders->layer_shared_frag,            // module_frag
	    &rgrp->layer.quad_unpremultiplied_alpha); // out_pipeline
	VK_CHK_WITH_RET(ret, "create_layer_pipeline", false);
	VK_NAME_PIPELINE(vk, rgrp->layer.quad_unpremultiplied_alpha,
	                 "render_gfx_render_pass quad unpremultiplied alpha");

	// Set fields.
	rgrp->r = r;
	rgrp->format = format;
	rgrp->sample_count = VK_SAMPLE_COUNT_1_BIT;
	rgrp->load_op = load_op;
	rgrp->final_layout = final_layout;

	return true;
}

void
render_gfx_render_pass_close(struct render_gfx_render_pass *rgrp)
{
	struct vk_bundle *vk = rgrp->r->vk;

	D(RenderPass, rgrp->render_pass);
	D(Pipeline, rgrp->mesh.pipeline);
	D(Pipeline, rgrp->mesh.pipeline_timewarp);

	D(Pipeline, rgrp->layer.cylinder_premultiplied_alpha);
	D(Pipeline, rgrp->layer.cylinder_unpremultiplied_alpha);
	D(Pipeline, rgrp->layer.equirect2_premultiplied_alpha);
	D(Pipeline, rgrp->layer.equirect2_unpremultiplied_alpha);
	D(Pipeline, rgrp->layer.proj_premultiplied_alpha);
	D(Pipeline, rgrp->layer.proj_unpremultiplied_alpha);
	D(Pipeline, rgrp->layer.quad_premultiplied_alpha);
	D(Pipeline, rgrp->layer.quad_unpremultiplied_alpha);

	U_ZERO(rgrp);
}


/*
 *
 * 'Exported' target resources functions.
 *
 */

bool
render_gfx_target_resources_init(struct render_gfx_target_resources *rtr,
                                 struct render_resources *r,
                                 struct render_gfx_render_pass *rgrp,
                                 VkImageView target,
                                 VkExtent2D extent)
{
	struct vk_bundle *vk = r->vk;
	VkResult ret;
	rtr->r = r;

	ret = create_framebuffer( //
	    vk,                   // vk_bundle,
	    target,               // image_view,
	    rgrp->render_pass,    // render_pass,
	    extent.width,         // width,
	    extent.height,        // height,
	    &rtr->framebuffer);   // out_external_framebuffer
	VK_CHK_WITH_RET(ret, "create_framebuffer", false);
	VK_NAME_FRAMEBUFFER(vk, rtr->framebuffer, "render_gfx_target_resources framebuffer");

	// Set fields.
	rtr->rgrp = rgrp;
	rtr->extent = extent;

	return true;
}

void
render_gfx_target_resources_close(struct render_gfx_target_resources *rtr)
{
	struct vk_bundle *vk = vk_from_rtr(rtr);

	D(Framebuffer, rtr->framebuffer);

	U_ZERO(rtr);
}


/*
 *
 * 'Exported' rendering functions.
 *
 */

bool
render_gfx_init(struct render_gfx *rr, struct render_resources *r)
{
	// Init fields.
	rr->r = r;

	// Used to sub-allocate UBOs from, restart from scratch each frame.
	render_sub_alloc_tracker_init(&rr->ubo_tracker, &r->gfx.shared_ubo);

	return true;
}

bool
render_gfx_begin(struct render_gfx *rr)
{
	struct vk_bundle *vk = vk_from_rr(rr);
	VkResult ret;

	ret = vk->vkResetCommandPool(vk->device, rr->r->cmd_pool, 0);
	VK_CHK_WITH_RET(ret, "vkResetCommandPool", false);


	VkCommandBufferBeginInfo begin_info = {
	    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
	    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};

	ret = vk->vkBeginCommandBuffer( //
	    rr->r->cmd,                 // commandBuffer
	    &begin_info);               // pBeginInfo
	VK_CHK_WITH_RET(ret, "vkResetCommandPool", false);

	vk->vkCmdResetQueryPool( //
	    rr->r->cmd,          // commandBuffer
	    rr->r->query_pool,   // queryPool
	    0,                   // firstQuery
	    2);                  // queryCount

	vk->vkCmdWriteTimestamp(               //
	    rr->r->cmd,                        // commandBuffer
	    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, // pipelineStage
	    rr->r->query_pool,                 // queryPool
	    0);                                // query

	return true;
}

bool
render_gfx_end(struct render_gfx *rr)
{
	struct vk_bundle *vk = vk_from_rr(rr);
	VkResult ret;

	vk->vkCmdWriteTimestamp(                  //
	    rr->r->cmd,                           // commandBuffer
	    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, // pipelineStage
	    rr->r->query_pool,                    // queryPool
	    1);                                   // query

	ret = vk->vkEndCommandBuffer(rr->r->cmd);
	VK_CHK_WITH_RET(ret, "vkEndCommandBuffer", false);

	return true;
}

void
render_gfx_close(struct render_gfx *rr)
{
	struct vk_bundle *vk = vk_from_rr(rr);
	struct render_resources *r = rr->r;

	// Reclaim all descriptor sets.
	vk->vkResetDescriptorPool(              //
	    vk->device,                         //
	    r->gfx.ubo_and_src_descriptor_pool, //
	    0);                                 //

	// This "reclaims" the allocated UBOs.
	U_ZERO(rr);
}


/*
 *
 * 'Exported' draw functions.
 *
 */

bool
render_gfx_begin_target(struct render_gfx *rr, struct render_gfx_target_resources *rtr, const VkClearColorValue *color)
{
	struct vk_bundle *vk = vk_from_rr(rr);

	assert(rr->rtr == NULL);
	rr->rtr = rtr;

	VkRenderPass render_pass = rtr->rgrp->render_pass;
	VkFramebuffer framebuffer = rtr->framebuffer;
	VkExtent2D extent = rtr->extent;

	begin_render_pass( //
	    vk,            //
	    rr->r->cmd,    //
	    render_pass,   //
	    framebuffer,   //
	    extent.width,  //
	    extent.height, //
	    color);        //

	return true;
}

void
render_gfx_end_target(struct render_gfx *rr)
{
	struct vk_bundle *vk = vk_from_rr(rr);

	assert(rr->rtr != NULL);
	rr->rtr = NULL;

	// Stop the [shared] render pass.
	vk->vkCmdEndRenderPass(rr->r->cmd);
}

void
render_gfx_begin_view(struct render_gfx *rr, uint32_t view, const struct render_viewport_data *viewport_data)
{
	struct vk_bundle *vk = vk_from_rr(rr);

	// We currently only support two views.
	assert(view == 0 || view == 1);
	assert(rr->rtr != NULL);


	/*
	 * Viewport
	 */

	VkViewport viewport = {
	    .x = (float)viewport_data->x,
	    .y = (float)viewport_data->y,
	    .width = (float)viewport_data->w,
	    .height = (float)viewport_data->h,
	    .minDepth = 0.0f,
	    .maxDepth = 1.0f,
	};

	vk->vkCmdSetViewport(rr->r->cmd, // commandBuffer
	                     0,          // firstViewport
	                     1,          // viewportCount
	                     &viewport); // pViewports

	/*
	 * Scissor
	 */

	VkRect2D scissor = {
	    .offset =
	        {
	            .x = viewport_data->x,
	            .y = viewport_data->y,
	        },
	    .extent =
	        {
	            .width = viewport_data->w,
	            .height = viewport_data->h,
	        },
	};

	vk->vkCmdSetScissor(rr->r->cmd, // commandBuffer
	                    0,          // firstScissor
	                    1,          // scissorCount
	                    &scissor);  // pScissors
}

void
render_gfx_end_view(struct render_gfx *rr)
{
	//! Must have a current target.
	assert(rr->rtr != NULL);
}

XRT_CHECK_RESULT VkResult
render_gfx_mesh_alloc_and_write(struct render_gfx *rr,
                                const struct render_gfx_mesh_ubo_data *data,
                                VkSampler src_sampler,
                                VkImageView src_image_view,
                                VkDescriptorSet *out_descriptor_set)
{
	struct render_resources *r = rr->r;

	return do_ubo_and_src_alloc_and_write(  //
	    rr,                                 // rr
	    r->mesh.ubo_binding,                // ubo_binding
	    data,                               // ubo_ptr
	    sizeof(*data),                      // ubo_size
	    r->mesh.src_binding,                // src_binding
	    src_sampler,                        // src_sampler
	    src_image_view,                     // src_image_view
	    r->gfx.ubo_and_src_descriptor_pool, // descriptor_pool
	    r->mesh.descriptor_set_layout,      // descriptor_set_layout
	    out_descriptor_set);                // out_descriptor_set
}

void
render_gfx_mesh_draw(struct render_gfx *rr, uint32_t mesh_index, VkDescriptorSet descriptor_set, bool do_timewarp)
{
	struct vk_bundle *vk = vk_from_rr(rr);
	struct render_resources *r = rr->r;


	/*
	 * Descriptors and pipeline.
	 */

	VkDescriptorSet descriptor_sets[1] = {descriptor_set};
	vk->vkCmdBindDescriptorSets(         //
	    r->cmd,                          // commandBuffer
	    VK_PIPELINE_BIND_POINT_GRAPHICS, // pipelineBindPoint
	    r->mesh.pipeline_layout,         // layout
	    0,                               // firstSet
	    ARRAY_SIZE(descriptor_sets),     // descriptorSetCount
	    descriptor_sets,                 // pDescriptorSets
	    0,                               // dynamicOffsetCount
	    NULL);                           // pDynamicOffsets

	// Select which pipeline we want.
	VkPipeline pipeline = do_timewarp ? rr->rtr->rgrp->mesh.pipeline_timewarp : rr->rtr->rgrp->mesh.pipeline;

	vk->vkCmdBindPipeline(               //
	    r->cmd,                          // commandBuffer
	    VK_PIPELINE_BIND_POINT_GRAPHICS, // pipelineBindPoint
	    pipeline);                       // pipeline


	/*
	 * Vertex buffer.
	 */

	VkBuffer buffers[1] = {r->mesh.vbo.buffer};
	VkDeviceSize offsets[1] = {0};
	assert(ARRAY_SIZE(buffers) == ARRAY_SIZE(offsets));

	vk->vkCmdBindVertexBuffers( //
	    r->cmd,                 // commandBuffer
	    0,                      // firstBinding
	    ARRAY_SIZE(buffers),    // bindingCount
	    buffers,                // pBuffers
	    offsets);               // pOffsets


	/*
	 * Draw with indices or not?
	 */

	if (r->mesh.index_count_total > 0) {
		vk->vkCmdBindIndexBuffer(  //
		    r->cmd,                // commandBuffer
		    r->mesh.ibo.buffer,    // buffer
		    0,                     // offset
		    VK_INDEX_TYPE_UINT32); // indexType

		vk->vkCmdDrawIndexed(                  //
		    r->cmd,                            // commandBuffer
		    r->mesh.index_counts[mesh_index],  // indexCount
		    1,                                 // instanceCount
		    r->mesh.index_offsets[mesh_index], // firstIndex
		    0,                                 // vertexOffset
		    0);                                // firstInstance
	} else {
		vk->vkCmdDraw(            //
		    r->cmd,               // commandBuffer
		    r->mesh.vertex_count, // vertexCount
		    1,                    // instanceCount
		    0,                    // firstVertex
		    0);                   // firstInstance
	}
}


/*
 *
 * 'Exported' layer functions.
 *
 */

XRT_CHECK_RESULT VkResult
render_gfx_layer_cylinder_alloc_and_write(struct render_gfx *rr,
                                          const struct render_gfx_layer_cylinder_data *data,
                                          VkSampler src_sampler,
                                          VkImageView src_image_view,
                                          VkDescriptorSet *out_descriptor_set)
{
	struct render_resources *r = rr->r;

	return do_ubo_and_src_alloc_and_write(         //
	    rr,                                        // rr
	    RENDER_BINDING_LAYER_SHARED_UBO,           // ubo_binding
	    data,                                      // ubo_ptr
	    sizeof(*data),                             // ubo_size
	    RENDER_BINDING_LAYER_SHARED_SRC,           // src_binding
	    src_sampler,                               // src_sampler
	    src_image_view,                            // src_image_view
	    r->gfx.ubo_and_src_descriptor_pool,        // descriptor_pool
	    r->gfx.layer.shared.descriptor_set_layout, // descriptor_set_layout
	    out_descriptor_set);                       // out_descriptor_set
}

XRT_CHECK_RESULT VkResult
render_gfx_layer_equirect2_alloc_and_write(struct render_gfx *rr,
                                           const struct render_gfx_layer_equirect2_data *data,
                                           VkSampler src_sampler,
                                           VkImageView src_image_view,
                                           VkDescriptorSet *out_descriptor_set)
{
	struct render_resources *r = rr->r;

	return do_ubo_and_src_alloc_and_write(         //
	    rr,                                        // rr
	    RENDER_BINDING_LAYER_SHARED_UBO,           // ubo_binding
	    data,                                      // ubo_ptr
	    sizeof(*data),                             // ubo_size
	    RENDER_BINDING_LAYER_SHARED_SRC,           // src_binding
	    src_sampler,                               // src_sampler
	    src_image_view,                            // src_image_view
	    r->gfx.ubo_and_src_descriptor_pool,        // descriptor_pool
	    r->gfx.layer.shared.descriptor_set_layout, // descriptor_set_layout
	    out_descriptor_set);                       // out_descriptor_set
}

XRT_CHECK_RESULT VkResult
render_gfx_layer_projection_alloc_and_write(struct render_gfx *rr,
                                            const struct render_gfx_layer_projection_data *data,
                                            VkSampler src_sampler,
                                            VkImageView src_image_view,
                                            VkDescriptorSet *out_descriptor_set)
{
	struct render_resources *r = rr->r;

	return do_ubo_and_src_alloc_and_write(         //
	    rr,                                        // rr
	    RENDER_BINDING_LAYER_SHARED_UBO,           // ubo_binding
	    data,                                      // ubo_ptr
	    sizeof(*data),                             // ubo_size
	    RENDER_BINDING_LAYER_SHARED_SRC,           // src_binding
	    src_sampler,                               // src_sampler
	    src_image_view,                            // src_image_view
	    r->gfx.ubo_and_src_descriptor_pool,        // descriptor_pool
	    r->gfx.layer.shared.descriptor_set_layout, // descriptor_set_layout
	    out_descriptor_set);                       // out_descriptor_set
}

XRT_CHECK_RESULT VkResult
render_gfx_layer_quad_alloc_and_write(struct render_gfx *rr,
                                      const struct render_gfx_layer_quad_data *data,
                                      VkSampler src_sampler,
                                      VkImageView src_image_view,
                                      VkDescriptorSet *out_descriptor_set)
{
	struct render_resources *r = rr->r;

	return do_ubo_and_src_alloc_and_write(         //
	    rr,                                        // rr
	    RENDER_BINDING_LAYER_SHARED_UBO,           // ubo_binding
	    data,                                      // ubo_ptr
	    sizeof(*data),                             // ubo_size
	    RENDER_BINDING_LAYER_SHARED_SRC,           // src_binding
	    src_sampler,                               // src_sampler
	    src_image_view,                            // src_image_view
	    r->gfx.ubo_and_src_descriptor_pool,        // descriptor_pool
	    r->gfx.layer.shared.descriptor_set_layout, // descriptor_set_layout
	    out_descriptor_set);                       // out_descriptor_set
}

void
render_gfx_layer_cylinder(struct render_gfx *rr, bool premultiplied_alpha, VkDescriptorSet descriptor_set)
{
	VkPipeline pipeline =                                          //
	    premultiplied_alpha                                        //
	        ? rr->rtr->rgrp->layer.cylinder_premultiplied_alpha    //
	        : rr->rtr->rgrp->layer.cylinder_unpremultiplied_alpha; //

	// One per degree.
	uint32_t subdivisions = 360;

	// One edge on either endstop and one between each subdivision.
	uint32_t edges = subdivisions + 1;

	// With triangle strip we get 2 vertices per edge.
	uint32_t vertex_count = edges * 2;

	dispatch_no_vbo(     //
	    rr,              // rr
	    vertex_count,    // vertex_count
	    pipeline,        // pipeline
	    descriptor_set); // descriptor_set
}

void
render_gfx_layer_equirect2(struct render_gfx *rr, bool premultiplied_alpha, VkDescriptorSet descriptor_set)
{
	VkPipeline pipeline =                                           //
	    premultiplied_alpha                                         //
	        ? rr->rtr->rgrp->layer.equirect2_premultiplied_alpha    //
	        : rr->rtr->rgrp->layer.equirect2_unpremultiplied_alpha; //

	// Hardcoded to 4 vertices.
	dispatch_no_vbo(     //
	    rr,              // rr
	    4,               // vertex_count
	    pipeline,        // pipeline
	    descriptor_set); // descriptor_set
}

void
render_gfx_layer_projection(struct render_gfx *rr, bool premultiplied_alpha, VkDescriptorSet descriptor_set)
{
	VkPipeline pipeline =                                      //
	    premultiplied_alpha                                    //
	        ? rr->rtr->rgrp->layer.proj_premultiplied_alpha    //
	        : rr->rtr->rgrp->layer.proj_unpremultiplied_alpha; //

	// Hardcoded to 4 vertices.
	dispatch_no_vbo(     //
	    rr,              // rr
	    4,               // vertex_count
	    pipeline,        // pipeline
	    descriptor_set); // descriptor_set
}

void
render_gfx_layer_quad(struct render_gfx *rr, bool premultiplied_alpha, VkDescriptorSet descriptor_set)
{
	VkPipeline pipeline =                                      //
	    premultiplied_alpha                                    //
	        ? rr->rtr->rgrp->layer.quad_premultiplied_alpha    //
	        : rr->rtr->rgrp->layer.quad_unpremultiplied_alpha; //

	// Hardcoded to 4 vertices.
	dispatch_no_vbo(     //
	    rr,              // rr
	    4,               // vertex_count
	    pipeline,        // pipeline
	    descriptor_set); // descriptor_set
}
