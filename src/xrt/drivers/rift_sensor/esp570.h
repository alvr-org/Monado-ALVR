// Copyright 2014-2020, Philipp Zabel
// Copyright 2017, TheOnlyJoey
// Copyright 2025, Jan Schmidt
// Copyright 2025-2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Implementation of esp570 sensor initialization
 * @author Philipp Zabel <philipp.zabel@gmail.com>
 * @author TheOnlyJoey <joeyferweda@gmail.com>
 * @author Jan Schmidt <jan@centricular.com>
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_rift_sensor
 */

#pragma once

#include "xrt/xrt_defines.h"

#include <libusb.h>


/*!
 * Performs a 16-bit read operation on the I2C bus.
 *
 * @param[in] devh libusb device handle
 * @param[in] addr I2C address
 * @param[in] reg Register to read from
 * @param[out] val Value read
 * @return 0 on success, negative error code on failure
 */
int
esp570_i2c_read(libusb_device_handle *devh, uint8_t addr, uint8_t reg, uint16_t *val);

/*!
 * Performs a 16-bit write operation on the I2C bus.
 *
 * @param[in] devh libusb device handle
 * @param[in] addr I2C address
 * @param[in] reg Register to write to
 * @param[in] val Value to write
 * @return 0 on success, negative error code on failure
 */
int
esp570_i2c_write(libusb_device_handle *devh, uint8_t addr, uint8_t reg, uint16_t val);

/*!
 * Reads a buffer from the Microchip 24AA128 EEPROM.
 *
 * @param[in] devh libusb device handle
 * @param[in] addr Address to read from
 * @param[out] buf Buffer to read into
 * @param[in] len Number of bytes to read (max 32)
 * @return 0 on success, negative error code on failure
 */
int
esp570_eeprom_read(libusb_device_handle *devh, uint16_t addr, uint8_t *buf, uint8_t len);

/*!
 * Calls SET_CUR and GET_CUR on the extension unit's selector 3 with values
 * captured from the Oculus Windows drivers to setup the device. I have no idea what these mean.
 *
 * @param[in] devh libusb device handle
 * @return 0 on success, negative error code on failure
 */
int
esp570_setup_unknown_3(libusb_device_handle *devh);
