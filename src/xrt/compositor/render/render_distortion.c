// Copyright 2019-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Code for handling distortion resources (not shaders).
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup comp_render
 */

#include "xrt/xrt_device.h"

#include "math/m_api.h"
#include "math/m_matrix_2x2.h"
#include "math/m_vec2.h"

#include "vk/vk_mini_helpers.h"

#include "render/render_interface.h"


/*
 *
 * Helper functions.
 *
 */

XRT_CHECK_RESULT static VkResult
create_distortion_image_and_view(struct vk_bundle *vk,
                                 VkExtent2D extent,
                                 VkDeviceMemory *out_device_memory,
                                 VkImage *out_image,
                                 VkImageView *out_image_view)
{
	VkFormat format = VK_FORMAT_R32G32_SFLOAT;
	VkImage image = VK_NULL_HANDLE;
	VkDeviceMemory device_memory = VK_NULL_HANDLE;
	VkImageView image_view = VK_NULL_HANDLE;
	VkImageViewType view_type = VK_IMAGE_VIEW_TYPE_2D;
	VkResult ret;

	ret = vk_create_image_simple(                                     //
	    vk,                                                           // vk_bundle
	    extent,                                                       // extent
	    format,                                                       // format
	    VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, // usage
	    &device_memory,                                               // out_device_memory
	    &image);                                                      // out_image
	VK_CHK_AND_RET(ret, "vk_create_image_simple");

	VK_NAME_DEVICE_MEMORY(vk, device_memory, "distortion device_memory");
	VK_NAME_IMAGE(vk, image, "distortion image");

	VkImageSubresourceRange subresource_range = {
	    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	    .baseMipLevel = 0,
	    .levelCount = VK_REMAINING_MIP_LEVELS,
	    .baseArrayLayer = 0,
	    .layerCount = VK_REMAINING_ARRAY_LAYERS,
	};

	ret = vk_create_view(  //
	    vk,                // vk_bundle
	    image,             // image
	    view_type,         // type
	    format,            // format
	    subresource_range, // subresource_range
	    &image_view);      // out_image_view
	VK_CHK_WITH_GOTO(ret, "vk_create_view", err_free);

	VK_NAME_IMAGE_VIEW(vk, image_view, "distortion image view");

	*out_device_memory = device_memory;
	*out_image = image;
	*out_image_view = image_view;

	return VK_SUCCESS;

err_free:
	D(Image, image);
	DF(Memory, device_memory);

	return ret;
}

static void
queue_upload_for_first_level_and_layer_locked(
    struct vk_bundle *vk, VkCommandBuffer cmd, VkBuffer src, VkImage dst, VkExtent2D extent)
{
	VkImageSubresourceRange subresource_range = {
	    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	    .baseMipLevel = 0,
	    .levelCount = VK_REMAINING_MIP_LEVELS,
	    .baseArrayLayer = 0,
	    .layerCount = VK_REMAINING_ARRAY_LAYERS,
	};

	vk_cmd_image_barrier_gpu_locked(          //
	    vk,                                   //
	    cmd,                                  //
	    dst,                                  //
	    0,                                    //
	    VK_ACCESS_TRANSFER_WRITE_BIT,         //
	    VK_IMAGE_LAYOUT_UNDEFINED,            //
	    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, //
	    subresource_range);                   //

	VkImageSubresourceLayers subresource_layers = {
	    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	    .mipLevel = 0,
	    .baseArrayLayer = 0,
	    .layerCount = 1,
	};

	VkBufferImageCopy region = {
	    .bufferOffset = 0,
	    .bufferRowLength = 0,
	    .bufferImageHeight = 0,
	    .imageSubresource = subresource_layers,
	    .imageOffset = {0, 0, 0},
	    .imageExtent = {extent.width, extent.height, 1},
	};

	vk->vkCmdCopyBufferToImage(               //
	    cmd,                                  // commandBuffer
	    src,                                  // srcBuffer
	    dst,                                  // dstImage
	    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // dstImageLayout
	    1,                                    // regionCount
	    &region);                             // pRegions

	vk_cmd_image_barrier_gpu_locked(              //
	    vk,                                       //
	    cmd,                                      //
	    dst,                                      //
	    VK_ACCESS_TRANSFER_WRITE_BIT,             //
	    VK_ACCESS_SHADER_READ_BIT,                //
	    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,     //
	    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, //
	    subresource_range);                       //
}

XRT_CHECK_RESULT static VkResult
create_and_queue_upload_locked(struct vk_bundle *vk,
                               struct vk_cmd_pool *pool,
                               VkCommandBuffer cmd,
                               VkBuffer src_buffer,
                               VkDeviceMemory *out_image_device_memory,
                               VkImage *out_image,
                               VkImageView *out_image_view)
{
	VkExtent2D extent = {RENDER_DISTORTION_IMAGE_DIMENSIONS, RENDER_DISTORTION_IMAGE_DIMENSIONS};
	VkDeviceMemory device_memory = VK_NULL_HANDLE;
	VkImage image = VK_NULL_HANDLE;
	VkImageView image_view = VK_NULL_HANDLE;
	VkResult ret;

	ret = create_distortion_image_and_view( //
	    vk,                                 // vk_bundle
	    extent,                             // extent
	    &device_memory,                     // out_device_memory
	    &image,                             // out_image
	    &image_view);                       // out_image_view
	VK_CHK_AND_RET(ret, "create_distortion_image_and_view");

	queue_upload_for_first_level_and_layer_locked( //
	    vk,                                        // vk_bundle
	    cmd,                                       // cmd
	    src_buffer,                                // src
	    image,                                     // dst
	    extent);                                   // extent

	*out_image_device_memory = device_memory;
	*out_image = image;
	*out_image_view = image_view;

	return VK_SUCCESS;
}

/*!
 * Helper struct to make code easier to read.
 */
struct texture
{
	struct xrt_vec2 pixels[RENDER_DISTORTION_IMAGE_DIMENSIONS][RENDER_DISTORTION_IMAGE_DIMENSIONS];
};

struct tan_angles_transforms
{
	struct xrt_vec2 offset;
	struct xrt_vec2 scale;
};

XRT_CHECK_RESULT static VkResult
create_and_fill_in_distortion_buffer_for_view(struct vk_bundle *vk,
                                              struct xrt_device *xdev,
                                              struct render_buffer *r_buffer,
                                              struct render_buffer *g_buffer,
                                              struct render_buffer *b_buffer,
                                              uint32_t view,
                                              bool pre_rotate)
{
	VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	VkResult ret;

	struct xrt_matrix_2x2 rot = xdev->hmd->views[view].rot;

	const struct xrt_matrix_2x2 rotation_90_cw = {{
	    .vecs =
	        {
	            {0, 1},
	            {-1, 0},
	        },
	}};

	if (pre_rotate) {
		m_mat2x2_multiply(&rot, &rotation_90_cw, &rot);
	}

	VkDeviceSize size = sizeof(struct texture);

	ret = render_buffer_init(vk, r_buffer, usage_flags, properties, size);
	VK_CHK_WITH_GOTO(ret, "render_buffer_init", err_buffers);
	VK_NAME_BUFFER(vk, r_buffer->buffer, "distortion r_buffer");
	ret = render_buffer_init(vk, g_buffer, usage_flags, properties, size);
	VK_CHK_WITH_GOTO(ret, "render_buffer_init", err_buffers);
	VK_NAME_BUFFER(vk, g_buffer->buffer, "distortion g_buffer");
	ret = render_buffer_init(vk, b_buffer, usage_flags, properties, size);
	VK_CHK_WITH_GOTO(ret, "render_buffer_init", err_buffers);
	VK_NAME_BUFFER(vk, b_buffer->buffer, "distortion b_buffer");

	ret = render_buffer_map(vk, r_buffer);
	VK_CHK_WITH_GOTO(ret, "render_buffer_map", err_buffers);
	ret = render_buffer_map(vk, g_buffer);
	VK_CHK_WITH_GOTO(ret, "render_buffer_map", err_buffers);
	ret = render_buffer_map(vk, b_buffer);
	VK_CHK_WITH_GOTO(ret, "render_buffer_map", err_buffers);

	struct texture *r = r_buffer->mapped;
	struct texture *g = g_buffer->mapped;
	struct texture *b = b_buffer->mapped;

	const double dim_minus_one_f64 = RENDER_DISTORTION_IMAGE_DIMENSIONS - 1;

	for (int row = 0; row < RENDER_DISTORTION_IMAGE_DIMENSIONS; row++) {
		// This goes from 0 to 1.0 inclusive.
		float v = (float)(row / dim_minus_one_f64);

		for (int col = 0; col < RENDER_DISTORTION_IMAGE_DIMENSIONS; col++) {
			// This goes from 0 to 1.0 inclusive.
			float u = (float)(col / dim_minus_one_f64);

			// These need to go from -0.5 to 0.5 for the rotation
			struct xrt_vec2 uv = {u - 0.5f, v - 0.5f};
			m_mat2x2_transform_vec2(&rot, &uv, &uv);
			uv.x += 0.5f;
			uv.y += 0.5f;

			struct xrt_uv_triplet result;
			xrt_device_compute_distortion(xdev, view, uv.x, uv.y, &result);

			r->pixels[row][col] = result.r;
			g->pixels[row][col] = result.g;
			b->pixels[row][col] = result.b;
		}
	}

	render_buffer_unmap(vk, r_buffer);
	render_buffer_unmap(vk, g_buffer);
	render_buffer_unmap(vk, b_buffer);

	return VK_SUCCESS;

err_buffers:
	render_buffer_close(vk, r_buffer);
	render_buffer_close(vk, g_buffer);
	render_buffer_close(vk, b_buffer);

	return ret;
}

static bool
render_distortion_buffer_init(struct render_resources *r,
                              struct vk_bundle *vk,
                              struct xrt_device *xdev,
                              bool pre_rotate)
{
	struct render_buffer bufs[RENDER_DISTORTION_IMAGES_SIZE];
	VkDeviceMemory device_memories[RENDER_DISTORTION_IMAGES_SIZE];
	VkImage images[RENDER_DISTORTION_IMAGES_SIZE];
	VkImageView image_views[RENDER_DISTORTION_IMAGES_SIZE];
	VkCommandBuffer upload_buffer = VK_NULL_HANDLE;
	VkResult ret;


	/*
	 * Basics
	 */

	for (uint32_t i = 0; i < r->view_count; ++i) {
		render_calc_uv_to_tangent_lengths_rect(&xdev->hmd->distortion.fov[i], &r->distortion.uv_to_tanangle[i]);
	}

	/*
	 * Buffers with data to upload.
	 * view_count=2,RRGGBB
	 * view_count=3,RRRGGGBBB
	 */
	for (uint32_t i = 0; i < r->view_count; ++i) {
		ret = create_and_fill_in_distortion_buffer_for_view(vk, xdev, &bufs[i], &bufs[r->view_count + i],
		                                                    &bufs[2 * r->view_count + i], i, pre_rotate);
		VK_CHK_WITH_GOTO(ret, "create_and_fill_in_distortion_buffer_for_view", err_resources);
	}

	/*
	 * Command submission.
	 */

	struct vk_cmd_pool *pool = &r->distortion_pool;

	vk_cmd_pool_lock(pool);

	ret = vk_cmd_pool_create_and_begin_cmd_buffer_locked(vk, pool, 0, &upload_buffer);
	VK_CHK_WITH_GOTO(ret, "vk_cmd_pool_create_and_begin_cmd_buffer_locked", err_unlock);
	VK_NAME_COMMAND_BUFFER(vk, upload_buffer, "render_resources distortion command buffer");

	for (uint32_t i = 0; i < RENDER_DISTORTION_IMAGES_COUNT; i++) {
		ret = create_and_queue_upload_locked( //
		    vk,                               // vk_bundle
		    pool,                             // pool
		    upload_buffer,                    // cmd
		    bufs[i].buffer,                   // src_buffer
		    &device_memories[i],              // out_image_device_memory
		    &images[i],                       // out_image
		    &image_views[i]);                 // out_image_view
		VK_CHK_WITH_GOTO(ret, "create_and_queue_upload_locked", err_cmd);
	}

	ret = vk_cmd_pool_end_submit_wait_and_free_cmd_buffer_locked(vk, pool, upload_buffer);
	VK_CHK_WITH_GOTO(ret, "vk_cmd_pool_end_submit_wait_and_free_cmd_buffer_locked", err_cmd);

	vk_cmd_pool_unlock(pool);

	/*
	 * Write results.
	 */

	r->distortion.pre_rotated = pre_rotate;

	for (uint32_t i = 0; i < RENDER_DISTORTION_IMAGES_COUNT; i++) {
		r->distortion.device_memories[i] = device_memories[i];
		r->distortion.images[i] = images[i];
		r->distortion.image_views[i] = image_views[i];
	}


	/*
	 * Tidy
	 */

	for (uint32_t i = 0; i < RENDER_DISTORTION_IMAGES_COUNT; i++) {
		render_buffer_close(vk, &bufs[i]);
	}

	return true;


err_cmd:
	vk->vkFreeCommandBuffers(vk->device, pool->pool, 1, &upload_buffer);

err_unlock:
	vk_cmd_pool_unlock(pool);

err_resources:
	for (uint32_t i = 0; i < RENDER_DISTORTION_IMAGES_COUNT; i++) {
		D(ImageView, image_views[i]);
		D(Image, images[i]);
		DF(Memory, device_memories[i]);
		render_buffer_close(vk, &bufs[i]);
	}

	return false;
}


/*
 *
 * 'Exported' functions.
 *
 */

void
render_distortion_images_close(struct render_resources *r)
{
	struct vk_bundle *vk = r->vk;

	for (uint32_t i = 0; i < RENDER_DISTORTION_IMAGES_COUNT; i++) {
		D(ImageView, r->distortion.image_views[i]);
		D(Image, r->distortion.images[i]);
		DF(Memory, r->distortion.device_memories[i]);
	}
}

bool
render_distortion_images_ensure(struct render_resources *r,
                                struct vk_bundle *vk,
                                struct xrt_device *xdev,
                                bool pre_rotate)
{
	if (r->distortion.image_views[0] == VK_NULL_HANDLE || pre_rotate != r->distortion.pre_rotated) {
		render_distortion_images_close(r);
		return render_distortion_buffer_init(r, vk, xdev, pre_rotate);
	}

	return true;
}
