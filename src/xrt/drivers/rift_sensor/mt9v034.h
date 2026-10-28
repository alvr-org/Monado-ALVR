// Copyright 2014-2020, Philipp Zabel
// Copyright 2017, TheOnlyJoey
// Copyright 2025, Jan Schmidt
// Copyright 2025-2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Implementation of mt9v034 sensor initialization
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
 * Sets up the MT9V034 sensor for synchronized exposure, with minimal gain
 * and raised black level calibration.
 *
 * @param[in] devh libusb device handle
 * @return 0 on success, negative error code on failure
 */
int
mt9v034_setup(libusb_device_handle *devh);

/*!
 * Enables or disables the MT9V034's sync output
 *
 * @param[in] devh libusb device handle
 * @param[in] enabled Whether to enable or disable the sync output
 * @return 0 on success, negative error code on failure
 */
int
mt9v034_set_sync(libusb_device_handle *devh, bool enabled);
