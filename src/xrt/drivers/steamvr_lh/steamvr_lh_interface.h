// Copyright 2023, Shawn Wallace
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief SteamVR driver device interface.
 * @author Shawn Wallace <yungwallace@live.com>
 * @ingroup drv_steamvr_lh
 */

#include <xrt/xrt_results.h>

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * @defgroup drv_steamvr_lh Wrapper for the SteamVR Lighthouse driver.
 * @ingroup drv
 *
 * @brief Wrapper driver around the SteamVR Lighthouse driver.
 */

/*!
 * @dir drivers/steamvr_lh
 *
 * @brief @ref drv_steamvr_lh files.
 */

/*!
 * Creates the steamvr system devices.
 *
 * @ingroup drv_steamvr_lh
 */
enum xrt_result
steamvr_lh_create_devices(struct xrt_session_event_sink *broadcast,
                          struct xrt_system_devices **out_xsysd,
                          struct xrt_space_overseer **out_xso);


#ifdef __cplusplus
}
#endif
