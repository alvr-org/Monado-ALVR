// Copyright 2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header defining visibility mask helper struct.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup xrt_iface
 */

#pragma once

#include "xrt/xrt_defines.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Visibility mask helper, the indices and vertices are tightly packed after
 * this struct.
 *
 * @ingroup xrt_iface
 */
struct xrt_visibility_mask
{
	enum xrt_visibility_mask_type type;
	uint32_t index_count;
	uint32_t vertex_count;
};

/*!
 * Visibility mask helper function to get the indices.
 *
 * @ingroup xrt_iface
 */
static inline uint32_t *
xrt_visibility_mask_get_indices(const struct xrt_visibility_mask *mask)
{
	return (uint32_t *)&mask[1];
}

/*!
 * Visibility mask helper function to get the vertices.
 *
 * @ingroup xrt_iface
 */
static inline struct xrt_vec2 *
xrt_visibility_mask_get_vertices(const struct xrt_visibility_mask *mask)
{
	const uint32_t *indices = xrt_visibility_mask_get_indices(mask);
	return (struct xrt_vec2 *)&indices[mask->index_count];
}

/*!
 * Visibility mask helper function to get the total size of the struct.
 *
 * @ingroup xrt_iface
 */
static inline size_t
xrt_visibility_mask_get_size(const struct xrt_visibility_mask *mask)
{
	return sizeof(*mask) +                               //
	       sizeof(uint32_t) * mask->index_count +        //
	       sizeof(struct xrt_vec2) * mask->vertex_count; //
}

#ifdef __cplusplus
}
#endif
