// Copyright 2020-2023, Collabora, Ltd.
// Copyright 2022, Magic Leap, Inc.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  IPC util helpers, for internal use only
 * @author Julian Petrov <jpetrov@magicleap.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup ipc_shared
 */

#pragma once

#include "xrt/xrt_config_os.h"

#ifdef XRT_OS_WINDOWS
#include "util/u_windows.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif


/*
 *
 * Misc utils.
 *
 */

#if defined(XRT_OS_WINDOWS) || defined(XRT_DOXYGEN)
/*!
 * Helper to convert windows error codes to human readable strings for logging.
 * N.B. This routine is not thread safe.
 *
 * @param err windows error code
 * @return human readable string corresponding to the error code.
 *
 * @ingroup ipc_shared
 */
const char *
ipc_winerror(DWORD err);
#endif


#ifdef __cplusplus
}
#endif
