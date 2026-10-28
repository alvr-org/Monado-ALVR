// Copyright 2023-2026, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Set thread priorities in a cross-platform manner
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 *
 * @ingroup aux_util
 */

#include "xrt/xrt_config_os.h"

#include "util/u_thread_priority.h"

#if defined(XRT_OS_LINUX)
#include "util/u_linux.h"
#endif

#if defined(XRT_OS_ANDROID)
#include "util/u_android.h"
#endif

bool
u_try_to_set_realtime_priority_on_thread(enum u_logging_level log_level, const char *name)
{
#if defined(XRT_OS_LINUX)
	if (u_linux_try_to_set_realtime_priority_on_thread(log_level, name)) {
		return true;
	}
#endif

#if defined(XRT_OS_ANDROID)
	/*!
	 * On Android, SCHED_FIFO requires CAP_SYS_NICE or a privileged SELinux domain,
	 * neither of which is available to a regular app process. We therefore fall back
	 * to the highest priority achievable within standard app sandbox constraints.
	 */
	if (u_android_try_to_set_highest_priority_on_thread(log_level, name)) {
		return true;
	}
#endif

	return false;
}
