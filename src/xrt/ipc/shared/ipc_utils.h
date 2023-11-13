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
#include "xrt/xrt_results.h"

#include "util/u_logging.h"

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

/*!
 * Helper to print the results of called functions that return xret results, if
 * the result is @p XRT_SUCCESS will log with info, otherwise error. Will also
 * check if logging should be done with @p cond_level.
 *
 * @param cond_level What the current logging level is.
 * @param file       Callee site (__FILE__).
 * @param line       Callee site (__LINE__).
 * @param calling_fn Callee site (__func__).
 * @param xret       Result from the called function.
 * @param called_fn  Which function that this return is from.
 *
 * @ingroup ipc_shared
 */
void
ipc_print_result(enum u_logging_level cond_level,
                 const char *file,
                 int line,
                 const char *calling_func,
                 xrt_result_t xret,
                 const char *called_func);

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
