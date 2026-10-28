// Copyright 2023-2025, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header defining planes detector enum and structs.
 * @author Christoph Haag <christoph.haag@collabora.com>
 * @ingroup xrt_iface
 */


#pragma once

#include "xrt/xrt_defines.h"
#include "xrt/xrt_limits.h"

#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif


/*!
 * Caps for a plane detector, see @ref xrt_device.
 *
 * @ingroup xrt_iface
 */
enum xrt_plane_detection_capability_flags_ext
{
	XRT_PLANE_DETECTION_CAPABILITY_PLANE_DETECTION_BIT_EXT = 0x00000001,
	XRT_PLANE_DETECTION_CAPABILITY_PLANE_HOLES_BIT_EXT = 0x00000002,
	XRT_PLANE_DETECTION_CAPABILITY_SEMANTIC_CEILING_BIT_EXT = 0x00000004,
	XRT_PLANE_DETECTION_CAPABILITY_SEMANTIC_FLOOR_BIT_EXT = 0x00000008,
	XRT_PLANE_DETECTION_CAPABILITY_SEMANTIC_WALL_BIT_EXT = 0x00000010,
	XRT_PLANE_DETECTION_CAPABILITY_SEMANTIC_PLATFORM_BIT_EXT = 0x00000020,
	XRT_PLANE_DETECTION_CAPABILITY_ORIENTATION_BIT_EXT = 0x00000040,
};

/*!
 * Flags used when running plane detection.
 *
 * @ingroup xrt_iface
 */
enum xrt_plane_detector_flags_ext
{
	XRT_PLANE_DETECTOR_FLAGS_CONTOUR_EXT = 1,
};

/*!
 * Orientation of a plane.
 *
 * @ingroup xrt_iface
 */
enum xrt_plane_detector_orientation_ext
{
	XRT_PLANE_DETECTOR_ORIENTATION_HORIZONTAL_UPWARD_EXT = 0,
	XRT_PLANE_DETECTOR_ORIENTATION_HORIZONTAL_DOWNWARD_EXT = 1,
	XRT_PLANE_DETECTOR_ORIENTATION_VERTICAL_EXT = 2,
	XRT_PLANE_DETECTOR_ORIENTATION_ARBITRARY_EXT = 3,
};

/*!
 * Has this plane any semantic meaning?
 *
 * @ingroup xrt_iface
 */
enum xrt_plane_detector_semantic_type_ext
{
	XRT_PLANE_DETECTOR_SEMANTIC_TYPE_UNDEFINED_EXT = 0,
	XRT_PLANE_DETECTOR_SEMANTIC_TYPE_CEILING_EXT = 1,
	XRT_PLANE_DETECTOR_SEMANTIC_TYPE_FLOOR_EXT = 2,
	XRT_PLANE_DETECTOR_SEMANTIC_TYPE_WALL_EXT = 3,
	XRT_PLANE_DETECTOR_SEMANTIC_TYPE_PLATFORM_EXT = 4,
};

/*!
 * State of a plane detector, see @ref xrt_device.
 *
 * @ingroup xrt_iface
 */
enum xrt_plane_detector_state_ext
{
	XRT_PLANE_DETECTOR_STATE_NONE_EXT = 0,
	XRT_PLANE_DETECTOR_STATE_PENDING_EXT = 1,
	XRT_PLANE_DETECTOR_STATE_DONE_EXT = 2,
	XRT_PLANE_DETECTOR_STATE_ERROR_EXT = 3,
	XRT_PLANE_DETECTOR_STATE_FATAL_EXT = 4,
};

/*!
 * A query for a plane. Corresponds to XrPlaneDetectorBeginInfoEXT.
 *
 * @ingroup xrt_iface
 */
struct xrt_plane_detector_begin_info_ext
{
	enum xrt_plane_detector_flags_ext detector_flags;
	uint32_t orientation_count;
	enum xrt_plane_detector_orientation_ext orientations[XRT_MAX_PLANE_ORIENTATIONS_EXT];
	uint32_t semantic_type_count;
	enum xrt_plane_detector_semantic_type_ext semantic_types[XRT_MAX_PLANE_SEMANTIC_TYPE_EXT];
	uint32_t max_planes;
	float min_area;
	struct xrt_pose bounding_box_pose;
	//! width, height, depth
	struct xrt_vec3 bounding_box_extent;
};

/*!
 * Location and other info for a plane.
 *
 * @ingroup xrt_iface
 */
struct xrt_plane_detector_location_ext
{
	uint64_t planeId;
	struct xrt_space_relation relation;
	//! x = width, y = height
	struct xrt_vec2 extents;
	enum xrt_plane_detector_orientation_ext orientation;
	enum xrt_plane_detector_semantic_type_ext semantic_type;
	uint32_t polygon_buffer_count;
};

/*!
 * Helper struct to pair up metadata for one polygon.
 *
 * @ingroup xrt_iface
 */
struct xrt_plane_polygon_info_ext
{
	uint32_t vertex_count;

	//! Index into the continuous array of vertices for all planes of a query.
	uint32_t vertices_start_index;
};

/*!
 * Each plane has n polygons; ultimately plane metadata from @ref xrt_plane_detections_ext::locations and @ref
 * xrt_plane_detections_ext::vertices is reconstructed. Therefore lay out the data in flattened arrays:
 *
 * @ref xrt_plane_detections_ext::locations stores continuous metadata for each plane:
 * location 1 | location 2 | location 3 | location 4 | ...
 *
 * @ref xrt_plane_detections_ext::polygon_info_start_index is a helper array to go from a location entry to a
 * polygon_info entry.
 *
 * @ref xrt_plane_detections_ext::polygon_infos stores info (metadata) for each polygon, flattened:
 * plane 1 polygon 1 info | plane 1 polygon 2 info | ... | plane 2 polygon 1 info | ...
 *
 * @ref xrt_plane_polygon_info_ext::vertices_start_index is a helper array to go from a @p polygon_info entry to
 * @p vertices entry.
 *
 * @ref xrt_plane_detections_ext::vertices stores vertex data for each polygon, for each plane, flattened:
 * plane 1 polygon 1 vertex 1 | plane 1 polygon 1 vertex 2 | ... | plane 1 polygon 2 vertex 1 | ...
 *
 * To reconstruct the vertices of a certain plane polygon:
 * - Find the index i of the plane with the requested plane_id in the locations array.
 * - Use this index i to generate a new index j = polygon_info_start_index[i].
 * - polygon_info[j] is the info of the first polygon of the locations[i] plane.
 * - polygon_info[j + polygonBufferIndex] is the info of the requested polygon.
 * - polygon_info[j + polygonBufferIndex].vertex_count is the vertex count of this polygon.
 * - polygon_info[j + polygonBufferIndex].vertices_start_index is another new index k.
 * - vertices[k] is the first vertex of the requested polygon.
 *
 * Convention: Whoever writes to this struct checks the size values first and reallocates arrays if necessary.
 *
 * @ingroup xrt_iface
 */
struct xrt_plane_detections_ext
{
	//! How many locations were found.
	uint32_t location_count;

	//! size of @ref locations and @ref polygon_info_start_index arrays.
	uint32_t location_size;

	///! array of detected locations.
	struct xrt_plane_detector_location_ext *locations;

	//! Parallel array to @ref locations.
	//! Index into polygon_info of @ref polygon_infos for all planes of a query.
	uint32_t *polygon_info_start_index;

	//! size of @ref polygon_infos array.
	uint32_t polygon_info_size;

	//! Continuous array of polygon_infos of all polygons for all planes of a query.
	struct xrt_plane_polygon_info_ext *polygon_infos;

	//! size of @ref vertices array.
	uint32_t vertex_size;

	//! Continuous array of polygon vertices of all polygons for all planes of a query.
	struct xrt_vec2 *vertices;
};

/*!
 * Small helper to free any data of a @ref xrt_plane_detections_ext struct,
 * does not free the struct itself.
 *
 * @ingroup xrt_iface
 */
static inline void
xrt_plane_detections_ext_clear(struct xrt_plane_detections_ext *detections)
{
	free(detections->locations);
	free(detections->polygon_info_start_index);
	free(detections->polygon_infos);
	free(detections->vertices);
	memset(detections, 0, sizeof(*detections));
}


#ifdef __cplusplus
}
#endif
