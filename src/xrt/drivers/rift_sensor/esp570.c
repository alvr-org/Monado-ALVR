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

#include "util/u_logging.h"

#include "uvc/uvc_interface.h"

#include "esp570.h"

#include <string.h>
#include <errno.h>


#define ESP570_EXTENSION_UNIT 4

#define ESP570_SELECTOR_I2C 2
#define ESP570_SELECTOR_UNKNOWN_3 3
#define ESP570_SELECTOR_EEPROM 5

/*
 * Internal functions
 */

/*!
 * Calls SET_CUR and then GET_CUR on a given selector of the eSP570 UVC
 * extension unit.
 *
 * @param[in] devh libusb device handle
 * @param[in] selector Selector to call on
 * @param[in] buf Buffer to send with SET_CUR, and receive with GET_CUR
 * @param[in] len Length of the buffer
 * @return 0 on success, negative error code on failure
 */
static int
uvc_xu_set_get_cur(libusb_device_handle *devh, int selector, uint8_t *buf, uint8_t len)
{
	int ret = uvc_set_cur(devh, 0, ESP570_EXTENSION_UNIT, selector, buf, len);
	if (ret < 0) {
		U_LOG_E("uvc_set_cur failed: %d", ret);
		return ret;
	}

	return uvc_get_cur(devh, 0, ESP570_EXTENSION_UNIT, selector, buf, len);
}

/*
 * Exported functions
 */

int
esp570_eeprom_read(libusb_device_handle *devh, uint16_t addr, uint8_t *data, uint8_t len)
{
	uint8_t buf[59] = {0x82, 0xa0, addr >> 8, addr & 0xff, len};

	if (len > 32) {
		return -EINVAL;
	}

	int ret = uvc_xu_set_get_cur(devh, ESP570_SELECTOR_EEPROM, buf, sizeof buf);
	if (ret < 0) {
		U_LOG_E("uvc_xu_set_get_cur failed: %d", ret);
		return ret;
	}

	if (buf[0] != 0x82 || buf[1] != len) {
		U_LOG_E("read_buf: error: 0x%02x 0x%02x", buf[0], buf[1]);
		return -1;
	}

	memcpy(data, buf + 2, len);
	return len;
}

int
esp570_i2c_read(libusb_device_handle *devh, uint8_t addr, uint8_t reg, uint16_t *val)
{
	uint8_t buf[6] = {0x84, addr, reg};

	int ret = uvc_xu_set_get_cur(devh, ESP570_SELECTOR_I2C, buf, sizeof buf);
	if (ret < 0) {
		return ret;
	}

	if (buf[0] != 0x84 || buf[4] != 0x00 || buf[5] != 0x00) {
		U_LOG_W("(%02x): %02x %02x %02x %02x %02x %02x", reg, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
		return -1;
	}

	*val = (buf[1] << 8) | buf[2];

	return 0;
}

int
esp570_i2c_write(libusb_device_handle *devh, uint8_t addr, uint8_t reg, uint16_t val)
{
	uint8_t buf[6] = {0x04, addr, reg, val >> 8, val & 0xff};

	int ret = uvc_xu_set_get_cur(devh, ESP570_SELECTOR_I2C, buf, sizeof buf);
	if (ret < 0) {
		return ret;
	}

	if (buf[0] != 0x04 || buf[1] != addr || buf[2] != reg || buf[5] != 0) {
		U_LOG_W("(%02x): %02x %02x %02x %02x %02x %02x", reg, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
		return -1;
	}

	if (buf[3] != (val >> 8) || buf[4] != (val & 0xff)) {
		U_LOG_W("(%02x, %04x): read back mismatch 0x%04x", reg, val, (buf[3] << 8) | buf[4]);
	}

	return 0;
}

int
esp570_setup_unknown_3(libusb_device_handle *devh)
{
	uint8_t buf[3];
	int ret;

	buf[0] = 0x80;
	buf[1] = 0x14;
	buf[2] = 0x00;
	ret = uvc_xu_set_get_cur(devh, ESP570_SELECTOR_UNKNOWN_3, buf, 3);
	if (ret < 0) {
		return ret;
	}
	if (buf[0] != 0x80 || buf[1] != 0xdc || buf[2] != 0x00) {
		U_LOG_W("(80 14 00): %02x %02x %02x", buf[0], buf[1], buf[2]);
	}

	buf[0] = 0xa0;
	buf[1] = 0xf0;
	buf[2] = 0x00;
	ret = uvc_xu_set_get_cur(devh, ESP570_SELECTOR_UNKNOWN_3, buf, 3);
	if (ret < 0) {
		return ret;
	}
	if (buf[0] != 0xa0 || buf[1] != 0x98 || buf[2] != 0x00) {
		U_LOG_W("(80 14 00): %02x %02x %02x", buf[0], buf[1], buf[2]);
	}

	return 0;
}
