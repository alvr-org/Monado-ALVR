// Copyright 2020-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Common things like defines for Vive and Index.
 * @author Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_vive
 */

#pragma once

#include "xrt/xrt_compiler.h"

#include "util/u_time.h"


#ifdef __cplusplus
extern "C" {
#endif


/*!
 * @defgroup aux_vive Shared code for @ref drv_vive and @ref drv_survive.
 * @ingroup aux
 *
 * @brief Shared functionality for @ref drv_vive and @ref drv_survive drivers
 * that supports the HTC Vive and Valve Index family of HMDs.
 */

/*!
 * @dir auxiliary/vive
 *
 * @brief @ref aux_vive files.
 */


/*!
 * Headset variant.
 *
 * @ingroup aux_vive
 */
enum VIVE_VARIANT
{
	VIVE_UNKNOWN = 0,
	VIVE_VARIANT_VIVE,
	VIVE_VARIANT_PRO,
	VIVE_VARIANT_INDEX,
	VIVE_VARIANT_PRO2,
	VIVE_VARIANT_BEYOND,
};

#define HTC_VID 0x0bb4
#define VALVE_VID 0x28de

#define VIVE_PID 0x2c87
#define VIVE_LIGHTHOUSE_FPGA_RX 0x2000

#define VIVE_PRO_MAINBOARD_PID 0x0309
#define VIVE_PRO2_MAINBOARD_PID 0x0342
#define VIVE_PRO_LHR_PID 0x2300

#define VIVE_WATCHMAN_DONGLE 0x2101
#define VIVE_WATCHMAN_DONGLE_GEN2 0x2102

enum VIVE_VARIANT
vive_determine_variant(const char *model_number);

bool
vive_variant_scanout_info(enum VIVE_VARIANT variant,
                          time_duration_ns frame_period_ns,
                          time_duration_ns *out_scanout_time_ns,
                          enum xrt_scanout_direction *out_scanout_direction,
                          enum xrt_panel_refresh_type *out_panel_refresh_type);

#ifdef __cplusplus
}
#endif
