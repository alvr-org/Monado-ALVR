// Copyright 2017, Philipp Zabel
// Copyright 2019-2025, Jan Schmidt
// Copyright 2025-2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Implementation of ar0134 sensor initialization
 * @author Philipp Zabel <philipp.zabel@gmail.com>
 * @author Jan Schmidt <jan@centricular.com>
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_rift_sensor
 */

#include "os/os_time.h"

#include "util/u_logging.h"

#include "uvc/uvc_interface.h"

#include "ar0134.h"


#define XU_ENTITY 4
#define SENSOR_REG_SEL 2
#define PIXEL_CLOCKS_PER_LINE 1388
#define COARSE_INTEGRATION_TIME 26
#define FINE_INTEGRATION_TIME 646

/*
 * Internal functions
 */

static int
ar0134_read_reg(libusb_device_handle *dev, uint16_t reg, uint16_t *val)
{
	uint8_t buf[6] = {0x86, 0x20, reg >> 8, reg & 0xff, 0x00, 0x00};
	int ret;

	ret = uvc_set_cur(dev, 0, XU_ENTITY, SENSOR_REG_SEL, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	ret = uvc_get_cur(dev, 0, XU_ENTITY, SENSOR_REG_SEL, buf, 3);
	if (ret < 0) {
		return ret;
	}

	if (buf[0] != 0x86) {
		U_LOG_D("(0x%x): %02x %02x %02x", reg, buf[0], buf[1], buf[2]);
	}
	*val = (buf[2] << 8) | buf[1];
	return ret;
}

static int
ar0134_write_reg(libusb_device_handle *dev, uint16_t reg, uint16_t val)
{
	uint8_t buf[64] = {0x06, 0x20, reg >> 8, reg & 0xff, val >> 8, val & 0xff};
	int ret;

	ret = uvc_set_cur(dev, 0, XU_ENTITY, SENSOR_REG_SEL, buf, 6);
	if (ret < 0) {
		return ret;
	}

	ret = uvc_get_cur(dev, 0, XU_ENTITY, SENSOR_REG_SEL, buf, sizeof buf);
	if (ret < 0) {
		return ret;
	}
	if (buf[0] != 0x06 || buf[1] != 0x20 || buf[2] != (reg >> 8) || buf[3] != (reg & 0xff) ||
	    buf[4] != (val >> 8) || buf[5] != (val & 0xff)) {
		U_LOG_D("(0x%04x, 0x%04x): %02x %02x %02x %02x %02x %02x", reg, val, buf[0], buf[1], buf[2], buf[3],
		        buf[4], buf[5]);
	}
	return ret;
}

/*
 * Exported functions
 */

int
rift_sensor_ar0134_init(libusb_device_handle *devh, bool usb2_mode)
{
	uint16_t val;
	int ret;

#if 0 // Debug: dump all registers
	for (int i = 0x3000; i < 0x3100; i += 2) {
		if (i % 16 == 0)
			printf("%04x: ", i);
		ret = ar0134_read_reg(devh, i, &val);
		if (ret < 0) return ret;
		printf("%04x ", val);
		if (i % 16 == 14)
			printf("\n");
	}
#endif

	// Read chip version and revision number registers
	for (int i = 0; i < 10; i++) {
		ret = ar0134_read_reg(devh, 0x3000, &val);
		if (ret < 0 || val != 0x2406) {
			// 5000us sleep between retries
			os_nanosleep(U_TIME_1US_IN_NS * 5000);
			continue;
		}
		break;
	}
	if (ret < 0) {
		return ret;
	}
	if (val != 0x2406) {
		U_LOG_E("This is not an AR0134 sensor: 0x%04x", val);
	}

	ret = ar0134_read_reg(devh, 0x300e, &val);
	if (ret < 0) {
		return ret;
	}
	if (val != 0x1300) {
		U_LOG_W("Unexpected revision number: 0x%04x", val);
	}

	ret = ar0134_read_reg(devh, 0x30b0, &val);
	if (ret < 0) {
		return ret;
	}
	if (val != 0x0080) {
		U_LOG_W("Expected monochrome mode instead of 0x%04x", val);
	}

	// Disable embedded register data and statistics. For now we can't use this anyway.
	ret = ar0134_read_reg(devh, 0x3064, &val);
	if (ret < 0) {
		return ret;
	}
	if (val != 0x1882 && val != 0x1982) {
		U_LOG_W("Unexpected embedded data control: %04x", val);
	}
	ret = ar0134_write_reg(devh, 0x3064, val & ~0x0180);
	if (ret < 0) {
		return ret;
	}

	// Set data pedestal (black level) to zero
	ret = ar0134_write_reg(devh, 0x301e, 0);
	if (ret < 0) {
		return ret;
	}

	// Disable binning
	ret = ar0134_write_reg(devh, 0x3032, 0x0000);
	if (ret < 0) {
		return ret;
	}

	// Set all gain values to the default, in USB2 mode 0x0007 is used instead.
	ret = ar0134_write_reg(devh, 0x305e, usb2_mode ? 0x0007 : 0x0020);
	if (ret < 0) {
		return ret;
	}

	// This changes nothing, probably clear some already cleared bits.
	ret = ar0134_read_reg(devh, 0x30b0, &val);
	if (ret < 0) {
		return ret;
	}
	val = 0x0080; // val &= ~(?);
	ret = ar0134_write_reg(devh, 0x30b0, val);
	if (ret < 0) {
		return ret;
	}

	// Set vertical and horizontal capture ranges to maximum (0-959, 0-1279).
	ret = ar0134_write_reg(devh, 0x3002, 0);
	if (ret < 0) {
		return ret;
	}
	ret = ar0134_write_reg(devh, 0x3006, 959);
	if (ret < 0) {
		return ret;
	}
	ret = ar0134_write_reg(devh, 0x3004, 0);
	if (ret < 0) {
		return ret;
	}
	ret = ar0134_write_reg(devh, 0x3008, 1279);
	if (ret < 0) {
		return ret;
	}

	// Set minimum supported pixel clocks per line
	ret = ar0134_write_reg(devh, 0x300c, PIXEL_CLOCKS_PER_LINE);
	if (ret < 0) {
		return ret;
	}

	// Set a short line bit, needed for the 1388 pclk line length above.
	ret = ar0134_read_reg(devh, 0x30b0, &val);
	if (ret < 0) {
		return ret;
	}
	val |= (1 << 10);
	ret = ar0134_write_reg(devh, 0x30b0, val);
	if (ret < 0) {
		return ret;
	}

	// Set total number of lines, 37 lines vertical blanking
	ret = ar0134_write_reg(devh, 0x300a, 997);
	if (ret < 0) {
		return ret;
	}

	// Read number of pixel clocks per line, should be changed above.
	ret = ar0134_read_reg(devh, 0x300c, &val);
	if (ret < 0) {
		return ret;
	}

	if (val != PIXEL_CLOCKS_PER_LINE) {
		U_LOG_E("Too many pixel clocks per line: %u, wanted %u", val, PIXEL_CLOCKS_PER_LINE);
	}

	// Set coarse integration time (in multiples of lines) and
	// fine integration time (in multiples of the pixel clock).
	ret = ar0134_write_reg(devh, 0x3012, COARSE_INTEGRATION_TIME); // 26 lines
	if (ret < 0) {
		return ret;
	}
	ret = ar0134_write_reg(devh, 0x3014, FINE_INTEGRATION_TIME);
	if (ret < 0) {
		return ret;
	}
	ret = ar0134_read_reg(devh, 0x3012, &val);
	if (ret < 0) {
		return ret;
	}
	if (val != COARSE_INTEGRATION_TIME) {
		U_LOG_E("Failed to set coarse integration time: %u", val);
	}
	ret = ar0134_read_reg(devh, 0x3014, &val);
	if (ret < 0) {
		return ret;
	}
	if (val != FINE_INTEGRATION_TIME) {
		U_LOG_E("Failed to set fine integration time: %u", val);
	}

	// Stop streaming, trigger external exposure from nRF51288
	ret = ar0134_read_reg(devh, 0x301a, &val);
	if (ret < 0) {
		return ret;
	}
	if (val != 0x10dc) {
		U_LOG_W("Unexpected reset register value: 0x%04x", val);
	}
	// Force PLL always enabled
	val |= 1 << 11;
	// Enable trigger input pin
	val |= 1 << 8;
	// Disable streaming (switch to externally triggered mode)
	val &= ~(1 << 2);
	ret = ar0134_write_reg(devh, 0x301a, val);
	if (ret < 0) {
		return ret;
	}

	// Enable junction temperature sensor
	ret = ar0134_read_reg(devh, 0x30b4, &val);
	if (ret < 0) {
		return ret;
	}
	// Temperature sensor power
	val |= 1 << 0;
	// Start conversion
	val |= 1 << 4;
	ret = ar0134_write_reg(devh, 0x30b4, val);
	if (ret < 0) {
		return ret;
	}

	// Read 70 °C calibration point
	uint16_t calib_70c;
	ret = ar0134_read_reg(devh, 0x30c6, &calib_70c);
	if (ret < 0) {
		return ret;
	}

	// Read 50 °C calibration point
	uint16_t calib_50c;
	ret = ar0134_read_reg(devh, 0x30c8, &calib_50c);
	if (ret < 0) {
		return ret;
	}

	// Read temperature sensor
	ret = ar0134_read_reg(devh, 0x30b2, &val);
	if (ret < 0) {
		return ret;
	}

	U_LOG_D("Temperature: %.1f °C", 50.0 + 20.0 * (val - calib_50c) / (calib_70c - calib_50c));

	return 0;
}
