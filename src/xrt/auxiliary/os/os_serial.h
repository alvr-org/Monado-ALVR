// Copyright 2019, Collabora, Ltd.
// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Wrapper around OS native serial functions.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 *
 * @ingroup aux_os
 */

#pragma once

#include "xrt/xrt_config_os.h"
#include "xrt/xrt_compiler.h"

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif


/*!
 * @interface os_serial_device
 *
 * Representing a single serial interface on a device.
 */
struct os_serial_device
{
	ssize_t (*read)(struct os_serial_device *serial_dev, uint8_t *data, size_t size, int milliseconds);

	ssize_t (*write)(struct os_serial_device *serial_dev, const uint8_t *data, size_t size);

	int (*set_line_control)(struct os_serial_device *serial_dev, bool dtr, bool rts);

	void (*destroy)(struct os_serial_device *serial_dev);
};

enum os_serial_parity_mode
{
	OS_SERIAL_PARITY_NONE,
	OS_SERIAL_PARITY_EVEN,
	OS_SERIAL_PARITY_ODD,
};

/*!
 * Represents the parameters for opening a serial device, such as baud rate, parity, etc.
 */
struct os_serial_parameters
{
	uint32_t baud_rate;
	uint8_t data_bits; // must be 5, 6, 7, or 8 bits
	uint8_t stop_bits; // must be 1 or 2
	enum os_serial_parity_mode parity;
};

/*!
 * Read the next input report, if any, from the given serial device.
 *
 * If milliseconds are negative, this call blocks indefinitely, 0 polls,
 * and positive will block for that amount of milliseconds.
 *
 * @public @memberof os_serial_device
 */
static inline ssize_t
os_serial_read(struct os_serial_device *serial_dev, uint8_t *data, size_t size, int milliseconds)
{
	return serial_dev->read(serial_dev, data, size, milliseconds);
}

/*!
 * Write an output report to the given device.
 *
 * @public @memberof os_serial_device
 */
static inline ssize_t
os_serial_write(struct os_serial_device *serial_dev, const uint8_t *data, size_t size)
{
	return serial_dev->write(serial_dev, data, size);
}

/*!
 * Set the line control signals (DTR and RTS) for the given serial device.
 *
 * @public @memberof os_serial_device
 */
static inline int
os_serial_set_line_control(struct os_serial_device *serial_dev, bool dtr, bool rts)
{
	return serial_dev->set_line_control(serial_dev, dtr, rts);
}

/*!
 * Close and free the given device.
 *
 * @public @memberof os_serial_device
 */
static inline void
os_serial_destroy(struct os_serial_device *serial_dev)
{
	serial_dev->destroy(serial_dev);
}

int
os_serial_open(const char *path, const struct os_serial_parameters *parameters, struct os_serial_device **out_serial);

#ifdef __cplusplus
} // extern "C"
#endif
