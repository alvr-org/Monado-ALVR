// Copyright 2023, Joseph Albers.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief   Driver for Ultraleap's V5 API for the Leap Motion Controller.
 * @author  Joseph Albers <joseph.albers@outlook.de>
 * @ingroup drv_ulv5
 */

#pragma once

#include "math/m_api.h"
#include "xrt/xrt_device.h"
#include "xrt/xrt_prober.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ULV5_VID 0x2936
#define ULV5_PID 0x1202

/*!
 * @defgroup drv_ulv5 Leap Motion Controller driver
 * @ingroup drv
 *
 * @brief Leap Motion Controller driver using Ultraleap's V5 API
 */

/*!
 * Probing function for Leap Motion Controller.
 *
 * @ingroup drv_ulv5
 * @see xrt_prober_found_func_t
 */
xrt_result_t
ulv5_create_device(struct xrt_device **out_xdev);
#ifdef __cplusplus
}
#endif