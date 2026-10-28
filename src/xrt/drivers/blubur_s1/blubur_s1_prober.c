// Copyright 2025, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Implementation of Blubur S1 prober code.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_blubur_s1
 */

#include "blubur_s1_interface.h"
#include "blubur_s1_internal.h"


int
blubur_s1_found(struct xrt_prober *xp,
                struct xrt_prober_device **devices,
                size_t device_count,
                size_t index,
                cJSON *attached_data,
                struct xrt_device **out_xdev)
{
	struct xrt_prober_device *dev = devices[index];

	struct os_hid_device *hid = NULL;
	int result = xrt_prober_open_hid_interface(xp, dev, 0, &hid);
	if (result != 0) {
		return -1;
	}

	// TODO: figure out how to get the actual serial number of the device, since the official driver has multiple
	// methods, and the string descriptor doesn't work on the unit used for development.
	char serial[41] = {0};
	result = xrt_prober_get_string_descriptor(xp, dev, XRT_PROBER_STRING_SERIAL_NUMBER, (unsigned char *)serial,
	                                          sizeof(serial));
	if (result <= 0 || strlen(serial) == 0) {
		snprintf(serial, sizeof(serial), "%04x:%04x", dev->vendor_id, dev->product_id);
	}

	struct blubur_s1_hmd *hmd = blubur_s1_hmd_create(hid, serial);
	if (hmd == NULL) {
		return -1;
	}
	*out_xdev = &hmd->base;

	return 1;
}
