// Copyright 2017, Philipp Zabel
// Copyright 2025, Jan Schmidt
// Copyright 2025-2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Implementation of esp770u sensor initialization
 * @author Philipp Zabel <philipp.zabel@gmail.com>
 * @author Jan Schmidt <jan@centricular.com>
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_rift_sensor
 */

#pragma once

#include "xrt/xrt_defines.h"

#include <libusb.h>


/*!
 * Reads a buffer from the flash storage.
 *
 * @param[in] devh libusb device handle
 * @param[in] addr Address to read from
 * @param[out] data Buffer to read into
 * @param[in] len Number of bytes to read
 * @return 0 on success, negative error code on failure
 */
int
rift_sensor_esp770u_flash_read(libusb_device_handle *devh, uint32_t addr, uint8_t *data, uint16_t len);

/*!
 * Sets up the radio with the given ID.
 *
 * @param[in] devhandle libusb device handle
 * @param[in] radio_id 5-byte radio ID
 * @return 0 on success, negative error code on failure
 */
int
rift_sensor_esp770u_setup_radio(libusb_device_handle *devhandle, const uint8_t radio_id[5]);

/*!
 *  Initial register setup, only after camera plugin
 *
 * @param[in] devhandle libusb device handle
 * @return 0 on success, negative error code on failure
 */
int
rift_sensor_esp770u_init_regs(libusb_device_handle *devhandle);

/*!
 * Extra initialisation sent after UVC config when in USB2 / MJPEG mode
 *
 * @param[in] devhandle libusb device handle
 * @return 0 on success, negative error code on failure
 */
int
rift_sensor_esp770u_init_jpeg(libusb_device_handle *devhandle);
