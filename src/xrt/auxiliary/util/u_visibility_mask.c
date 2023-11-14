// Copyright 2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Visibility mask utilitary
 * @author Simon Zeni <simon.zeni@collabora.com>
 * @ingroup aux_util
 */

#include "math/m_mathinclude.h"

#include "u_misc.h"
#include "u_visibility_mask.h"
#include "u_logging.h"


#include <string.h>

static const struct xrt_vec2 vertices_hidden[] = {
    {1.0, 0.75},   {1.0, 1.0},   {0.75, 1.0},   {-1.0, 1.0},  {-1.0, 0.75}, {-0.75, 1.0},
    {-1.0, -0.75}, {-1.0, -1.0}, {-0.75, -1.0}, {0.75, -1.0}, {1.0, -1.0},  {1.0, -0.75},
};

static const uint32_t indices_hidden[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

static const struct xrt_vec2 vertices_visible[] = {
    {1.f, -.75f}, {.75f, -1.f}, {-.75f, -1.f}, {-1.f, -.75f}, {-1.f, .75f},
    {-.75f, 1.f}, {.75f, 1.f},  {1.f, .75f},   {0.f, 0.f},
};

static const uint32_t indices_visible[] = {
    8, 2, 1, 3, 2, 8, 8, 1, 0, 6, 8, 7, 4, 8, 5, 8, 0, 7, 5, 8, 6, 4, 3, 8,
};

static const struct xrt_vec2 vertices_line[] = {
    {-.75f, -1.f}, {.75f, -1.f}, {1.f, -.75f}, {1.f, .75f}, {.75f, 1.f}, {-.75f, 1.f}, {-1.f, .75f}, {-1.f, -.75f},
};

static const uint32_t indices_line[] = {0, 1, 2, 3, 4, 5, 6, 7};

void
u_visibility_mask_get_default(enum xrt_visibility_mask_type type,
                              const struct xrt_fov *fov,
                              struct xrt_visibility_mask **out_mask)
{
	struct xrt_visibility_mask *mask = NULL;
	uint32_t nvertices, nindices;

	switch (type) {
	case XRT_VISIBILITY_MASK_TYPE_HIDDEN_TRIANGLE_MESH:
		nvertices = ARRAY_SIZE(vertices_hidden);
		nindices = ARRAY_SIZE(indices_hidden);
		break;
	case XRT_VISIBILITY_MASK_TYPE_VISIBLE_TRIANGLE_MESH:
		nvertices = ARRAY_SIZE(vertices_visible);
		nindices = ARRAY_SIZE(indices_visible);
		break;
	case XRT_VISIBILITY_MASK_TYPE_LINE_LOOP:
		nvertices = ARRAY_SIZE(vertices_line);
		nindices = ARRAY_SIZE(indices_line);
		break;
	}

	const size_t size =
	    sizeof(struct xrt_visibility_mask) + sizeof(uint32_t) * nindices + sizeof(struct xrt_vec2) * nvertices;
	mask = U_CALLOC_WITH_CAST(struct xrt_visibility_mask, size);
	if (mask == NULL) {
		U_LOG_E("failed to allocate out xrt_visibility_mask");
		goto out;
	}


	mask->index_count = nindices;
	mask->vertex_count = nvertices;

	const struct xrt_vec2 *vertices;
	const uint32_t *indices;

	switch (type) {
	case XRT_VISIBILITY_MASK_TYPE_HIDDEN_TRIANGLE_MESH:
		vertices = vertices_hidden;
		indices = indices_hidden;
		break;
	case XRT_VISIBILITY_MASK_TYPE_VISIBLE_TRIANGLE_MESH:
		vertices = vertices_visible;
		indices = indices_visible;
		break;
	case XRT_VISIBILITY_MASK_TYPE_LINE_LOOP:
		vertices = vertices_line;
		indices = indices_line;
		break;
	}

	memcpy(xrt_visibility_mask_get_indices(mask), indices, sizeof(uint32_t) * nindices);


	const struct xrt_fov copy = *fov;

	const double tan_left = tan(copy.angle_left);
	const double tan_right = tan(copy.angle_right);

	const double tan_down = tan(copy.angle_down);
	const double tan_up = tan(copy.angle_up);

	const double tan_half_width = (tan_right - tan_left);
	const double tan_half_height = (tan_up - tan_down);

	const double tan_offset_x = ((tan_right + tan_left) - tan_half_width) / 2;
	const double tan_offset_y = (-(tan_up + tan_down) - tan_half_height) / 2;

	struct xrt_vec2 *dst = xrt_visibility_mask_get_vertices(mask);
	for (uint32_t i = 0; i < nvertices; i++) {
		struct xrt_vec2 v = vertices[i];

		// Yes this is really the simplest form, WolframAlpha agrees.
		v.x = (v.x * 0.5 + 0.5) * tan_half_width + tan_offset_x;
		v.y = (v.y * 0.5 + 0.5) * tan_half_height + tan_offset_y;

		dst[i] = v;
	}

out:
	*out_mask = mask; // Always NULL or allocated data.
}
