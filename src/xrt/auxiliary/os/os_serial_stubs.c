// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Serial implementation stub
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup aux_os
 */

#include "util/u_logging.h"

#include "os_serial.h"


int
os_serial_open(const char *path, const struct os_serial_parameters *parameters, struct os_serial_device **out_serial)
{
	U_LOG_E("Serial devices are not implemented on this platform.");
	return -1;
}
