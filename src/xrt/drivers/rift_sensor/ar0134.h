// Copyright 2017, Philipp Zabel
// Copyright 2019-2025, Jan Schmidt
// Copyright 2025-2026, Beyley Cardellio
// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Implementation of ar0134 sensor initialization
 * @author Philipp Zabel <philipp.zabel@gmail.com>
 * @author Jan Schmidt <jan@centricular.com>
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_rift_sensor
 */

#pragma once

#include "xrt/xrt_defines.h"

#include <libusb.h>


/*!
 * Sensor setup after stream start
 *
 * @param[in] devh libusb device handle
 * @param[in] usb2_mode Whether to configure for USB2 mode
 * @return 0 on success, negative error code on failure
 */
int
rift_sensor_ar0134_init(libusb_device_handle *devh, bool usb2_mode);
