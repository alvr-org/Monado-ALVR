// Copyright 2022-2023, Collabora, Ltd.
// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Fallback builder the old method of probing devices.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup xrt_iface
 */

#include "xrt/xrt_config_drivers.h"
#include "xrt/xrt_prober.h"

#include "util/u_misc.h"
#include "util/u_device.h"
#include "util/u_system_helpers.h"

#include "target_builder_helpers.h"
#include "target_builder_interface.h"

#include <assert.h>

static const char *driver_list[] = {
#ifdef XRT_BUILD_DRIVER_CONTACTGLOVE
    "contactglove",
#endif

#ifdef XRT_BUILD_DRIVER_HYDRA
    "hydra",
#endif

#ifdef XRT_BUILD_DRIVER_HDK
    "hdk",
#endif

#ifdef XRT_BUILD_DRIVER_ULV2
    "ulv2",
#endif

#ifdef XRT_BUILD_DRIVER_DEPTHAI
    "depthai",
#endif

#ifdef XRT_BUILD_DRIVER_WMR
    "wmr",
#endif

#ifdef XRT_BUILD_DRIVER_ARDUINO
    "arduino",
#endif

#ifdef XRT_BUILD_DRIVER_DAYDREAM
    "daydream",
#endif

#ifdef XRT_BUILD_DRIVER_OHMD
    "oh",
#endif

#ifdef XRT_BUILD_DRIVER_NS
    "ns",
#endif

#ifdef XRT_BUILD_DRIVER_ALVR
    "alvr",
#endif

#ifdef XRT_BUILD_DRIVER_ANDROID
    "android",
#endif

#ifdef XRT_BUILD_DRIVER_ILLIXR
    "illixr",
#endif

#ifdef XRT_BUILD_DRIVER_REALSENSE
    "rs",
#endif

#ifdef XRT_BUILD_DRIVER_EUROC
    "euroc",
#endif

#ifdef XRT_BUILD_DRIVER_QWERTY
    "qwerty",
#endif

#if defined(XRT_BUILD_DRIVER_HANDTRACKING) && defined(XRT_BUILD_DRIVER_DEPTHAI)
    "ht",
#endif

#if defined(XRT_BUILD_DRIVER_SIMULATED)
    "simulated",
#endif
    NULL,
};


/*
 *
 * Member functions.
 *
 */

static xrt_result_t
legacy_estimate_system(struct xrt_builder *xb,
                       cJSON *config,
                       struct xrt_prober *xp,
                       struct xrt_builder_estimate *estimate)
{
	// If no driver is enabled, there is no way to create a HMD
	bool may_create_hmd = xb->driver_identifier_count > 0;

	estimate->maybe.head = may_create_hmd;
	estimate->maybe.left = may_create_hmd;
	estimate->maybe.right = may_create_hmd;
	estimate->priority = -20;

	return XRT_SUCCESS;
}

static xrt_result_t
legacy_open_system_impl(struct xrt_builder *xb,
                        cJSON *config,
                        struct xrt_prober *xp,
                        struct xrt_tracking_origin *origin,
                        struct xrt_system_devices *xsysd,
                        struct xrt_frame_context *xfctx,
                        struct t_builder_options *tbo)
{
	xrt_result_t xret;
	int ret;


	/*
	 * Create the devices.
	 */

	xret = xrt_prober_probe(xp);
	if (xret != XRT_SUCCESS) {
		return xret;
	}

	ret = xrt_prober_select(xp, xsysd->static_xdevs, ARRAY_SIZE(xsysd->static_xdevs));
	if (ret < 0) {
		return XRT_ERROR_DEVICE_CREATION_FAILED;
	}

	// Count the xdevs.
	for (uint32_t i = 0; i < ARRAY_SIZE(xsysd->static_xdevs); i++) {
		if (xsysd->static_xdevs[i] == NULL) {
			break;
		}

		xsysd->static_xdev_count++;
	}


	/*
	 * Setup the roles.
	 */

	int head_idx, eyes_idx, face_idx, left_idx, right_idx, gamepad_idx;
	u_device_assign_xdev_roles(xsysd->static_xdevs, xsysd->static_xdev_count, &head_idx, &eyes_idx, &face_idx,
	                           &left_idx, &right_idx, &gamepad_idx);

	struct xrt_device *head = NULL;
	struct xrt_device *eyes = NULL, *face = NULL;
	struct xrt_device *left = NULL, *right = NULL, *gamepad = NULL;
	struct xrt_device *unobstructed_left_ht = NULL, *unobstructed_right_ht = NULL;
	struct xrt_device *conforming_left_ht = NULL, *conforming_right_ht = NULL;

	if (head_idx >= 0) {
		head = xsysd->static_xdevs[head_idx];
	}
	if (eyes_idx >= 0) {
		eyes = xsysd->static_xdevs[eyes_idx];
	}
	if (face_idx >= 0) {
		face = xsysd->static_xdevs[face_idx];
	}
	if (left_idx >= 0) {
		left = xsysd->static_xdevs[left_idx];
	}
	if (right_idx >= 0) {
		right = xsysd->static_xdevs[right_idx];
	}
	if (gamepad_idx >= 0) {
		gamepad = xsysd->static_xdevs[gamepad_idx];
	}

	// Find hand tracking devices.
	unobstructed_left_ht = u_system_devices_get_ht_device_unobstructed_left(xsysd);
	unobstructed_right_ht = u_system_devices_get_ht_device_unobstructed_right(xsysd);

	conforming_left_ht = u_system_devices_get_ht_device_conforming_left(xsysd);
	conforming_right_ht = u_system_devices_get_ht_device_conforming_right(xsysd);

	// Assign to role(s).
	tbo->head = head;
	tbo->eyes = eyes;
	tbo->face = face;
	tbo->left = left;
	tbo->right = right;
	tbo->gamepad = gamepad;
	tbo->hand_tracking.unobstructed.left = unobstructed_left_ht;
	tbo->hand_tracking.unobstructed.right = unobstructed_right_ht;
	tbo->hand_tracking.conforming.left = conforming_left_ht;
	tbo->hand_tracking.conforming.right = conforming_right_ht;

	return XRT_SUCCESS;
}

static void
legacy_destroy(struct xrt_builder *xb)
{
	free(xb);
}


/*
 *
 * 'Exported' functions.
 *
 */

struct xrt_builder *
t_builder_legacy_create(void)
{
	struct t_builder *ub = U_TYPED_CALLOC(struct t_builder);

	// xrt_builder fields.
	ub->base.estimate_system = legacy_estimate_system;
	ub->base.open_system = t_builder_open_system_static_roles;
	ub->base.destroy = legacy_destroy;
	ub->base.identifier = "legacy";
	ub->base.name = "Legacy probing system";
	ub->base.driver_identifiers = driver_list;
	ub->base.driver_identifier_count = ARRAY_SIZE(driver_list) - 1;

	// t_builder fields.
	ub->open_system_static_roles = legacy_open_system_impl;

	return &ub->base;
}
