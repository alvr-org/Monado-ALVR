// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Implementation of ContactGlove prober functions.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_contactglove
 */

#include "util/u_logging.h"

#include "os/os_serial.h"

#include "contactglove_interface.h"

#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>


int
contactglove2_found(struct xrt_prober *xp,
                    struct xrt_prober_device **devices,
                    size_t device_count,
                    size_t index,
                    cJSON *attached_data,
                    struct xrt_device **out_xdevs)
{
	struct xrt_prober_device *device_found = devices[index];
	unsigned char serial_number[64];
	xrt_prober_get_string_descriptor(xp, device_found, XRT_PROBER_STRING_SERIAL_NUMBER, serial_number,
	                                 sizeof(serial_number));

	struct os_serial_device *serial_dev;
	if (xrt_prober_open_serial_device(xp, device_found, &CONTACTGLOVE2_SERIAL_PARAMETERS, &serial_dev) != 0) {
		U_LOG_E("Failed to open ContactGlove2 serial device");
		return 0;
	}

	struct contactglove_dongle *dongle;
	int created_device_count =
	    contactglove_create(CONTACTGLOVE_TYPE_CONTACTGLOVE2, (char *)serial_number, serial_dev, &dongle, out_xdevs);
	if (created_device_count < 0) {
		U_LOG_E("Failed to create ContactGlove2 device");
		os_serial_destroy(serial_dev);
		return 0;
	}

	return created_device_count;
}
