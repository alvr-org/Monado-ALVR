// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Sorting helper with argument.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup tracking
 */

#include "image_point_sort.h"

#include <algorithm>
#include <vector>


static int
compare_blobs_distance(struct cs_image_point *anchor, const struct cs_image_point *b1, const struct cs_image_point *b2)
{
	double dist1, dist2;

	dist1 = (b1->blob->center_undistorted.y - anchor->blob->center_undistorted.y) *
	            (b1->blob->center_undistorted.y - anchor->blob->center_undistorted.y) +
	        (b1->blob->center_undistorted.x - anchor->blob->center_undistorted.x) *
	            (b1->blob->center_undistorted.x - anchor->blob->center_undistorted.x);
	dist2 = (b2->blob->center_undistorted.y - anchor->blob->center_undistorted.y) *
	            (b2->blob->center_undistorted.y - anchor->blob->center_undistorted.y) +
	        (b2->blob->center_undistorted.x - anchor->blob->center_undistorted.x) *
	            (b2->blob->center_undistorted.x - anchor->blob->center_undistorted.x);

	if (dist1 > dist2) {
		return 1;
	}
	if (dist1 < dist2) {
		return -1;
	}

	return 0;
}

void
sort_image_points(struct cs_image_point **points, size_t num_points, struct cs_image_point *anchor)
{
	std::sort(points, points + num_points,
	          [anchor](const struct cs_image_point *x, const struct cs_image_point *y) {
		          return compare_blobs_distance(anchor, x, y) < 0;
	          });
}
