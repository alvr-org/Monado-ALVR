// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  An @ref xrt_blob_sink that visualizes blobs to a frame.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup aux_util
 */

#include "math/m_api.h"

#include "util/u_misc.h"
#include "util/u_sink.h"
#include "util/u_frame.h"

/*!
 * An @ref xrt_blob_sink that visualizes blobs to a frame
 * @implements xrt_blob_sink
 * @implements xrt_frame_node
 */
struct u_sink_blob_visualizer
{
	struct t_blob_sink base;
	struct xrt_frame_node node;

	struct t_blob_sink *downstream_blob_sink;
	struct u_sink_debug *downstream_frame_sink;

	uint32_t width;
	uint32_t height;
};

static inline void
set_pixel(uint8_t *ptr, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	ptr[0] = r; // R
	ptr[1] = g; // G
	ptr[2] = b; // B
	ptr[3] = a; // A
}

static void
draw_centered_box(
    struct xrt_frame *frame, int center_x, int center_y, int box_size, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	assert(frame->format == XRT_FORMAT_R8G8B8X8); // sanity check to ensure only frames we created get passed

	uint8_t *frame_data = frame->data;
	size_t frame_stride = frame->stride;

	int half_size = box_size / 2;
	int left = MAX(0, center_x - half_size);
	int right = MIN((int)(frame->width) - 1, center_x + half_size);
	int top = MAX(0, center_y - half_size);
	int bottom = MIN((int)(frame->height) - 1, center_y + half_size);

	for (int y = top; y <= bottom; y++) {
		for (int x = left; x <= right; x++) {
			uint8_t *pixel = frame_data + (y * frame_stride) + (x * 4); // 4 bytes per pixel for R8G8B8X8
			set_pixel(pixel, r, g, b, a);
		}
	}
}

static void
visualize_blobs(struct t_blob_sink *xbs, struct t_blob_observation *tbo)
{
	struct u_sink_blob_visualizer *bv = (struct u_sink_blob_visualizer *)xbs;

	if (bv->downstream_blob_sink) {
		bv->downstream_blob_sink->push_blobs(bv->downstream_blob_sink, tbo);
	}

	struct xrt_frame *frame = NULL;
	u_frame_create_one_off(XRT_FORMAT_R8G8B8X8, bv->width, bv->height, &frame);

	memset(frame->data, 0, frame->size); // Clear the frame to black

	// Draw blobs as white squares on the frame
	for (uint32_t i = 0; i < tbo->num_blobs; i++) {
		struct t_blob blob = tbo->blobs[i];
		blob.size.x = MAX(blob.size.x, 5.0f); // Ensure size is at least 5 pixels
		blob.size.y = MAX(blob.size.y, 5.0f);

		int left = (int)(blob.center_distorted.x - (blob.size.x / 2));
		int right = (int)(blob.center_distorted.x + (blob.size.x / 2));
		int top = (int)(blob.center_distorted.y - (blob.size.y / 2));
		int bottom = (int)(blob.center_distorted.y + (blob.size.y / 2));

		// Clamp to frame boundaries
		left = MAX(0, left);
		right = MIN((int)bv->width - 1, right);
		top = MAX(0, top);
		bottom = MIN((int)bv->height - 1, bottom);

		// Draw a grey box around the blob's bounds
		for (int y = top; y <= bottom; y++) {
			for (int x = left; x <= right; x++) {
				uint8_t *pixel =
				    frame->data + y * frame->stride + x * 4; // 4 bytes per pixel for R8G8B8X8
				set_pixel(pixel, 128, 128, 128, 255);        // Grey color
			}
		}

		// Draw a white dot at the blob's center
		int center_x = (int)blob.center_distorted.x;
		int center_y = (int)blob.center_distorted.y;
		draw_centered_box(frame, center_x, center_y, 3, 255, 255, 255, 255);

		// Draw a red dot for the motion vector
		int motion_x = (int)(blob.center_distorted.x + blob.motion_vector.x);
		int motion_y = (int)(blob.center_distorted.y + blob.motion_vector.y);
		draw_centered_box(frame, motion_x, motion_y, 3, 255, 0, 0, 255);
	}

	u_sink_debug_push_frame(bv->downstream_frame_sink, frame);

	xrt_frame_reference(&frame, NULL);
}

static void
break_apart(struct xrt_frame_node *node)
{}

static void
destroy(struct xrt_frame_node *node)
{
	struct u_sink_blob_visualizer *bv = container_of(node, struct u_sink_blob_visualizer, node);

	free(bv);
}


/*
 *
 * Exported functions.
 *
 */

void
u_sink_blob_visualizer_create(struct xrt_frame_context *xfctx,
                              struct t_blob_sink *downstream_blob_sink,
                              struct u_sink_debug *downstream_frame_sink,
                              uint32_t frame_width,
                              uint32_t frame_height,
                              struct t_blob_sink **out_xbs)
{
	struct u_sink_blob_visualizer *bv = U_TYPED_CALLOC(struct u_sink_blob_visualizer);

	bv->base.push_blobs = visualize_blobs;

	bv->node.break_apart = break_apart;
	bv->node.destroy = destroy;

	bv->downstream_blob_sink = downstream_blob_sink;
	bv->downstream_frame_sink = downstream_frame_sink;

	bv->width = frame_width;
	bv->height = frame_height;

	xrt_frame_context_add(xfctx, &bv->node);

	*out_xbs = &bv->base;
}
