// Copyright 2022-2023, Collabora, Ltd.
// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Default implementation helpers for @ref xrt_system_devices.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup base
 */

#include "b_system_devices.h"
#include "b_body_tracker.h"
#include "b_hand_tracker.h"
#include "b_io_cache.h"

#include "util/u_device.h"
#include "util/u_logging.h"
#include "util/u_misc.h"
#include "util/u_system_devices.h"

#include "xrt/xrt_device.h"

#include <assert.h>
#include <limits.h>
#include <stdlib.h>

/*
 *
 * Helper functions.
 *
 */

static int32_t
get_index_for_device(const struct xrt_system_devices *xsysd, const struct xrt_device *xdev)
{
	assert(xsysd->static_xdev_count <= ARRAY_SIZE(xsysd->static_xdevs));
	assert(xsysd->static_xdev_count < INT_MAX);

	if (xdev == NULL) {
		return -1;
	}

	for (int32_t i = 0; i < (int32_t)xsysd->static_xdev_count; i++) {
		if (xsysd->static_xdevs[i] == xdev) {
			return i;
		}
	}

	return -1;
}

static const char *
type_to_small_string(enum xrt_device_feature_type type)
{
	switch (type) {
	case XRT_DEVICE_FEATURE_HAND_TRACKING_LEFT: return "hand_tracking_left";
	case XRT_DEVICE_FEATURE_HAND_TRACKING_RIGHT: return "hand_tracking_right";
	case XRT_DEVICE_FEATURE_EYE_TRACKING: return "eye_tracking";
	case XRT_DEVICE_FEATURE_FACE_TRACKING: return "face_tracking";
	default: return "invalid";
	}
}

static inline void
get_hand_tracking_devices(struct xrt_system_devices *xsysd, enum xrt_hand hand, struct xrt_device *out_ht_xdevs[2])
{
#define XRT_GET_U_HT(HAND) xsysd->static_roles.hand_tracking.unobstructed.HAND
#define XRT_GET_C_HT(HAND) xsysd->static_roles.hand_tracking.conforming.HAND
	if (hand == XRT_HAND_LEFT) {
		out_ht_xdevs[0] = XRT_GET_U_HT(left);
		out_ht_xdevs[1] = XRT_GET_C_HT(left);
	} else {
		out_ht_xdevs[0] = XRT_GET_U_HT(right);
		out_ht_xdevs[1] = XRT_GET_C_HT(right);
	}
#undef XRT_GET_C_HT
#undef XRT_GET_U_HT
}

static xrt_result_t
set_hand_tracking_enabled(struct xrt_system_devices *xsysd, enum xrt_hand hand, bool enable)
{
	struct xrt_device *ht_sources[2] = {0};
	get_hand_tracking_devices(xsysd, hand, ht_sources);

	uint32_t ht_sources_size = ARRAY_SIZE(ht_sources);
	// hand-tracking data-sources can all come from the same xrt-device instance
	if (ht_sources[0] == ht_sources[1]) {
		ht_sources_size = 1;
	}

	typedef xrt_result_t (*set_feature_t)(struct xrt_device *, enum xrt_device_feature_type);
	const set_feature_t set_feature = enable ? xrt_device_begin_feature : xrt_device_end_feature;

	const enum xrt_device_feature_type ht_feature =
	    (hand == XRT_HAND_LEFT) ? XRT_DEVICE_FEATURE_HAND_TRACKING_LEFT : XRT_DEVICE_FEATURE_HAND_TRACKING_RIGHT;

	xrt_result_t xret = XRT_SUCCESS;
	for (uint32_t i = 0; i < ht_sources_size; ++i) {
		if (ht_sources[i]) {
			xret = set_feature(ht_sources[i], ht_feature);
		}
		if (xret != XRT_SUCCESS) {
			break;
		}
	}
	return xret;
}

XRT_CHECK_RESULT static xrt_result_t
notify_chirality_pair(struct b_system_devices_static *bsysds, struct xrt_device *left, struct xrt_device *right)
{
	if (left != NULL && left->supported.notify_chirality) {
		xrt_result_t xret = xrt_device_notify_chirality(left, true, XRT_HAND_LEFT);
		if (xret != XRT_SUCCESS) {
			return xret;
		}
	}
	if (right != NULL && right->supported.notify_chirality) {
		xrt_result_t xret = xrt_device_notify_chirality(right, true, XRT_HAND_RIGHT);
		if (xret != XRT_SUCCESS) {
			return xret;
		}
	}

	return XRT_SUCCESS;
}

XRT_CHECK_RESULT static xrt_result_t
notify_chirality(struct b_system_devices_static *bsysds)
{
	xrt_result_t xret;

	// left/right controllers
	xret = notify_chirality_pair(
	    bsysds, //
	    bsysds->cached.left != XRT_DEVICE_ROLE_UNASSIGNED ? bsysds->base.base.static_xdevs[bsysds->cached.left]
	                                                      : NULL, //
	    bsysds->cached.right != XRT_DEVICE_ROLE_UNASSIGNED ? bsysds->base.base.static_xdevs[bsysds->cached.right]
	                                                       : NULL); //
	if (xret != XRT_SUCCESS) {
		return xret;
	}

	// left/right conforming hand tracking
	xret = notify_chirality_pair(bsysds, bsysds->base.base.static_roles.hand_tracking.conforming.left,
	                             bsysds->base.base.static_roles.hand_tracking.conforming.right);
	if (xret != XRT_SUCCESS) {
		return xret;
	}

	// left/right unobstructed hand tracking
	xret = notify_chirality_pair(bsysds, bsysds->base.base.static_roles.hand_tracking.unobstructed.left,
	                             bsysds->base.base.static_roles.hand_tracking.unobstructed.right);
	if (xret != XRT_SUCCESS) {
		return xret;
	}

	return XRT_SUCCESS;
}

/*
 *
 * Internal functions.
 *
 */

static void
destroy(struct xrt_system_devices *xsysd)
{
	b_system_devices_close(xsysd);
	free(xsysd);
}

static xrt_result_t
get_roles(struct xrt_system_devices *xsysd, struct xrt_system_roles *out_roles)
{
	struct b_system_devices_static *bsysds = b_system_devices_static(xsysd);

	assert(bsysds->cached.generation_id == 1);

	*out_roles = bsysds->cached;

	return XRT_SUCCESS;
}

static xrt_result_t
feature_inc(struct xrt_system_devices *xsysd, enum xrt_device_feature_type type)
{
	struct b_system_devices_static *bsysds = b_system_devices_static(xsysd);

	if (type >= XRT_DEVICE_FEATURE_MAX_ENUM) {
		return XRT_ERROR_FEATURE_NOT_SUPPORTED;
	}

	// If it wasn't zero nothing to do.
	if (!xrt_reference_inc_and_was_zero(&bsysds->feature_use[type])) {
		return XRT_SUCCESS;
	}

	xrt_result_t xret = XRT_SUCCESS;
	if (type == XRT_DEVICE_FEATURE_HAND_TRACKING_LEFT) {
		xret = set_hand_tracking_enabled(xsysd, XRT_HAND_LEFT, true);
	} else if (type == XRT_DEVICE_FEATURE_HAND_TRACKING_RIGHT) {
		xret = set_hand_tracking_enabled(xsysd, XRT_HAND_RIGHT, true);
	} else if (type == XRT_DEVICE_FEATURE_EYE_TRACKING) {
		xret = xrt_device_begin_feature(xsysd->static_roles.eyes, type);
	} else if (type == XRT_DEVICE_FEATURE_FACE_TRACKING) {
		xret = xrt_device_begin_feature(xsysd->static_roles.face, type);
	} else {
		xret = XRT_ERROR_FEATURE_NOT_SUPPORTED;
	}
	if (xret != XRT_SUCCESS) {
		return xret;
	}

	U_LOG_D("Device-feature %s in use", type_to_small_string(type));

	return XRT_SUCCESS;
}

static xrt_result_t
feature_dec(struct xrt_system_devices *xsysd, enum xrt_device_feature_type type)
{
	struct b_system_devices_static *bsysds = b_system_devices_static(xsysd);

	if (type >= XRT_DEVICE_FEATURE_MAX_ENUM) {
		return XRT_ERROR_FEATURE_NOT_SUPPORTED;
	}

	// If it is not zero we are done.
	if (!xrt_reference_dec_and_is_zero(&bsysds->feature_use[type])) {
		return XRT_SUCCESS;
	}

	xrt_result_t xret;
	if (type == XRT_DEVICE_FEATURE_HAND_TRACKING_LEFT) {
		xret = set_hand_tracking_enabled(xsysd, XRT_HAND_LEFT, false);
	} else if (type == XRT_DEVICE_FEATURE_HAND_TRACKING_RIGHT) {
		xret = set_hand_tracking_enabled(xsysd, XRT_HAND_RIGHT, false);
	} else if (type == XRT_DEVICE_FEATURE_EYE_TRACKING) {
		// @todo When eyes are moved from the static roles, we need to end features on the old device when
		// swapping which device is in the eyes role
		xret = xrt_device_end_feature(xsysd->static_roles.eyes, type);
	} else if (type == XRT_DEVICE_FEATURE_FACE_TRACKING) {
		xret = xrt_device_end_feature(xsysd->static_roles.face, type);
	} else {
		xret = XRT_ERROR_FEATURE_NOT_SUPPORTED;
	}
	if (xret != XRT_SUCCESS) {
		return xret;
	}

	U_LOG_D("Device-feature %s no longer in use", type_to_small_string(type));

	return XRT_SUCCESS;
}

static xrt_result_t
create_body_tracker(struct xrt_system_devices *xsysd,
                    const struct xrt_body_tracker_create_info *info,
                    struct xrt_body_tracker **out_xbt)
{
	return b_body_tracker_create(xsysd, info, out_xbt);
}

static xrt_result_t
create_hand_tracker(struct xrt_system_devices *xsysd,
                    const struct xrt_hand_tracker_create_info *info,
                    struct xrt_hand_tracker **out_xht)
{
	return b_hand_tracker_create(xsysd, info, out_xht);
}

static xrt_result_t
create_io_cache(struct xrt_system_devices *xsysd, struct xrt_space_overseer *xso, struct xrt_io_cache **out_xic)
{
	return b_io_cache_create(xsysd, xso, out_xic);
}


/*
 *
 * 'Exported' functions.
 *
 */

struct b_system_devices *
b_system_devices_allocate(void)
{
	struct b_system_devices *bsysd = U_TYPED_CALLOC(struct b_system_devices);
	bsysd->base.destroy = destroy;
	bsysd->base.create_body_tracker = create_body_tracker;
	bsysd->base.create_hand_tracker = create_hand_tracker;
	bsysd->base.create_io_cache = create_io_cache;

	return bsysd;
}

void
b_system_devices_close(struct xrt_system_devices *xsysd)
{
	struct b_system_devices *bsysd = b_system_devices(xsysd);

	for (uint32_t i = 0; i < ARRAY_SIZE(bsysd->base.static_xdevs); i++) {
		xrt_device_destroy(&bsysd->base.static_xdevs[i]);
	}

	xrt_frame_context_destroy_nodes(&bsysd->xfctx);
}

struct b_system_devices_static *
b_system_devices_static_allocate(void)
{
	struct b_system_devices_static *bsysds = U_TYPED_CALLOC(struct b_system_devices_static);

	u_system_devices_populate_function_pointers(&bsysds->base.base, get_roles, destroy);
	bsysds->base.base.create_body_tracker = create_body_tracker;
	bsysds->base.base.create_hand_tracker = create_hand_tracker;
	bsysds->base.base.create_io_cache = create_io_cache;
	bsysds->base.base.feature_inc = feature_inc;
	bsysds->base.base.feature_dec = feature_dec;

	return bsysds;
}

xrt_result_t
b_system_devices_static_finalize(struct b_system_devices_static *bsysds,
                                 struct xrt_device *left,
                                 struct xrt_device *right,
                                 struct xrt_device *gamepad)
{
	struct xrt_system_devices *xsysd = &bsysds->base.base;
	int32_t left_index = get_index_for_device(xsysd, left);
	int32_t right_index = get_index_for_device(xsysd, right);
	int32_t gamepad_index = get_index_for_device(xsysd, gamepad);

	U_LOG_D(
	    "Devices:"
	    "\n\t%i: %p"
	    "\n\t%i: %p"
	    "\n\t%i: %p",
	    left_index, (void *)left,        //
	    right_index, (void *)right,      //
	    gamepad_index, (void *)gamepad); //

	// Consistency checking.
	assert(bsysds->cached.generation_id == 0);
	assert(left_index < 0 || left != NULL);
	assert(left_index >= 0 || left == NULL);
	assert(right_index < 0 || right != NULL);
	assert(right_index >= 0 || right == NULL);
	assert(gamepad_index < 0 || gamepad != NULL);
	assert(gamepad_index >= 0 || gamepad == NULL);

	// Completely clear the struct.
	bsysds->cached = (struct xrt_system_roles)XRT_SYSTEM_ROLES_INIT;
	bsysds->cached.generation_id = 1;
	bsysds->cached.left = left_index;
	bsysds->cached.right = right_index;
	bsysds->cached.gamepad = gamepad_index;

	return notify_chirality(bsysds);
}
