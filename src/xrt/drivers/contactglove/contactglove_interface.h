// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Interface to @ref drv_contactglove.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_contactglove
 */

#pragma once

#include "xrt/xrt_prober.h"
#include "xrt/xrt_compiler.h"

#include "os/os_serial.h"

#ifdef __cplusplus
extern "C" {
#endif


#define CONTACTGLOVE2_VID 0x10c4
#define CONTACTGLOVE2_PID 0x6b27

enum contactglove_type
{
	CONTACTGLOVE_TYPE_CONTACTGLOVE1,
	CONTACTGLOVE_TYPE_CONTACTGLOVE2,
};

#define CONTACTGLOVE2_SERIAL_PARAMETERS                                                                                \
	XRT_C11_COMPOUND(struct os_serial_parameters)                                                                  \
	{                                                                                                              \
		.baud_rate = 115200, .data_bits = 8, .stop_bits = 1, .parity = OS_SERIAL_PARITY_NONE,                  \
	}

struct contactglove_dongle;

struct contactglove_device;

int
contactglove2_found(struct xrt_prober *xp,
                    struct xrt_prober_device **devices,
                    size_t device_count,
                    size_t index,
                    cJSON *attached_data,
                    struct xrt_device **out_xdevs);

int
contactglove_create(enum contactglove_type type,
                    const char *serial_number,
                    struct os_serial_device *dongle_serial,
                    struct contactglove_dongle **out_dongle,
                    struct xrt_device **out_xdevs);

/*!
 * Enable pairing mode on the ContactGlove dongle.
 */
int
contactglove_dongle_pair(struct contactglove_dongle *dongle);

#ifdef __cplusplus
}
#endif
