// Copyright 2022-2023, Collabora, Ltd.
// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Helpers for probing and querying @ref xrt_system_devices.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_util
 */

#include "xrt/xrt_device.h"
#include "xrt/xrt_prober.h"

#include "util/u_debug.h"
#include "util/u_device.h"
#include "util/u_system_helpers.h"

#include <assert.h>
#include <string.h>


/*
 *
 * Env variable options.
 *
 */

DEBUG_GET_ONCE_OPTION(ht_left_unobstructed, "XRT_DEVICE_HAND_TRACKER_LEFT_UNOBSTRUCTED_SERIAL", NULL)
DEBUG_GET_ONCE_OPTION(ht_right_unobstructed, "XRT_DEVICE_HAND_TRACKER_RIGHT_UNOBSTRUCTED_SERIAL", NULL)
DEBUG_GET_ONCE_OPTION(ht_left_conforming, "XRT_DEVICE_HAND_TRACKER_LEFT_CONFORMING_SERIAL", NULL)
DEBUG_GET_ONCE_OPTION(ht_right_conforming, "XRT_DEVICE_HAND_TRACKER_RIGHT_CONFORMING_SERIAL", NULL)


/*
 *
 * General system helpers.
 *
 */

xrt_result_t
u_system_devices_create_from_prober(struct xrt_instance *xinst,
                                    struct xrt_session_event_sink *broadcast,
                                    struct xrt_system_devices **out_xsysd,
                                    struct xrt_space_overseer **out_xso)
{
	xrt_result_t xret;

	assert(out_xsysd != NULL);
	assert(*out_xsysd == NULL);

	/*
	 * Create the devices.
	 */

	struct xrt_prober *xp = NULL;
	xret = xrt_instance_get_prober(xinst, &xp);
	if (xret != XRT_SUCCESS) {
		return xret;
	}

	xret = xrt_prober_probe(xp);
	if (xret < 0) {
		return xret;
	}

	return xrt_prober_create_system(xp, broadcast, out_xsysd, out_xso);
}

struct xrt_device *
u_system_devices_get_ht_device(struct xrt_system_devices *xsysd, enum xrt_input_name name)
{
	const char *ht_serial = NULL;
	switch (name) {
	case XRT_INPUT_HT_UNOBSTRUCTED_LEFT: ht_serial = debug_get_option_ht_left_unobstructed(); break;
	case XRT_INPUT_HT_UNOBSTRUCTED_RIGHT: ht_serial = debug_get_option_ht_right_unobstructed(); break;
	case XRT_INPUT_HT_CONFORMING_LEFT: ht_serial = debug_get_option_ht_left_conforming(); break;
	case XRT_INPUT_HT_CONFORMING_RIGHT: ht_serial = debug_get_option_ht_right_conforming(); break;
	default: break;
	}

	for (uint32_t i = 0; i < xsysd->static_xdev_count; i++) {
		struct xrt_device *xdev = xsysd->static_xdevs[i];

		if (xdev == NULL || !xdev->supported.hand_tracking ||
		    (ht_serial != NULL && (strncmp(xdev->serial, ht_serial, XRT_DEVICE_NAME_LEN) != 0))) {
			continue;
		}

		for (uint32_t j = 0; j < xdev->input_count; j++) {
			struct xrt_input *input = &xdev->inputs[j];

			if (input->name == name) {
				return xdev;
			}
		}
	}

	return NULL;
}
