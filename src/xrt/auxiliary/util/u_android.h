// Copyright 2026, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Various helpers for doing Android specific things.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup aux_util
 */

#pragma once

#include "xrt/xrt_compiler.h"
#include "util/u_logging.h"

#ifdef __cplusplus
extern "C" {
#endif


/*!
 * @brief Tries to set the highest possible priority on the current thread,
 *        within the constraints of the Android app sandbox.
 *
 * On Android, SCHED_FIFO requires CAP_SYS_NICE or a privileged SELinux domain,
 * neither of which is available to a regular app process. This function will not
 * give the thread real-time priority, but will attempt to set the highest priority.
 *
 * @param name      Thread name to be used in logging.
 * @param log_level Logging level to control chattiness.
 *
 * @return          Returns true/false if setting real-time priority was successful
 *
 * @ingroup aux_util
 */
bool
u_android_try_to_set_highest_priority_on_thread(enum u_logging_level log_level, const char *name);


#ifdef __cplusplus
}
#endif
