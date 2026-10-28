// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Per-device tracking state for the space overseer.
 *
 * @ingroup base
 */

#include "util/u_misc.h"

#include "b_tracked_device.h"

#include <assert.h>
#include <string.h>


struct b_tracked_device
{
	struct xrt_reference reference;
	struct xrt_space *space;

	struct xrt_space **pose_spaces;
	uint32_t pose_space_count;
	uint32_t pose_space_capacity;
};

static void
b_tracked_device_destroy(struct b_tracked_device *btd)
{
	assert(btd->pose_space_count == 0);

	free(btd->pose_spaces);

	struct xrt_space *space = btd->space;
	xrt_space_reference(&space, NULL);
	free(btd);
}

struct b_tracked_device *
b_tracked_device_create(struct xrt_space *space)
{
	assert(space != NULL);

	struct b_tracked_device *btd = U_TYPED_CALLOC(struct b_tracked_device);
	btd->reference.count = 1;

	xrt_space_reference(&btd->space, space);

	return btd;
}

void
b_tracked_device_reference(struct b_tracked_device **dst, struct b_tracked_device *src)
{
	struct b_tracked_device *old_dst = *dst;

	if (old_dst == src) {
		return;
	}

	if (src != NULL) {
		xrt_reference_inc(&src->reference);
	}

	*dst = src;

	if (old_dst != NULL) {
		if (xrt_reference_dec_and_is_zero(&old_dst->reference)) {
			b_tracked_device_destroy(old_dst);
		}
	}
}

struct xrt_space *
b_tracked_device_get_space(struct b_tracked_device *btd)
{
	assert(btd != NULL);
	return btd->space;
}

void
b_tracked_device_track_pose_space(struct b_tracked_device *btd, struct xrt_space *xs)
{
	assert(btd != NULL);
	assert(xs != NULL);

	for (uint32_t i = 0; i < btd->pose_space_count; i++) {
		if (btd->pose_spaces[i] == xs) {
			return;
		}
	}

	if (btd->pose_space_count >= btd->pose_space_capacity) {
		uint32_t new_capacity = btd->pose_space_capacity == 0 ? 4 : btd->pose_space_capacity * 2;
		U_ARRAY_REALLOC_OR_FREE(btd->pose_spaces, struct xrt_space *, new_capacity);
		btd->pose_space_capacity = new_capacity;
	}

	btd->pose_spaces[btd->pose_space_count++] = xs;
}

void
b_tracked_device_untrack_pose_space(struct b_tracked_device *btd, struct xrt_space *xs)
{
	assert(btd != NULL);
	assert(xs != NULL);

	for (uint32_t i = 0; i < btd->pose_space_count; i++) {
		if (btd->pose_spaces[i] != xs) {
			continue;
		}

		btd->pose_space_count--;
		btd->pose_spaces[i] = btd->pose_spaces[btd->pose_space_count];
		return;
	}
}

void
b_tracked_device_for_each_pose_space(struct b_tracked_device *btd, b_tracked_device_pose_space_cb cb, void *priv)
{
	assert(btd != NULL);
	assert(cb != NULL);

	for (uint32_t i = 0; i < btd->pose_space_count; i++) {
		cb(btd->pose_spaces[i], priv);
	}
}

void
b_tracked_device_clear_pose_spaces(struct b_tracked_device *btd)
{
	assert(btd != NULL);
	btd->pose_space_count = 0;
}
