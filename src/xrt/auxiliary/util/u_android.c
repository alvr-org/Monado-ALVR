// Copyright 2026, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Various helpers for doing Android specific things.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup aux_util
 */

#include "util/u_android.h"

#include <sys/resource.h>
#include <unistd.h>
#include <sys/syscall.h>


/*
 *
 * Helper functions.
 *
 */



/*
 *
 * 'Exported' functions.
 *
 */

bool
u_android_try_to_set_highest_priority_on_thread(enum u_logging_level log_level, const char *name)
{
	(void)log_level;
	(void)name;

	enum
	{
		ANDROID_PRIORITY_AUDIO = -16,
		ANDROID_PRIORITY_URGENT_DISPLAY = -8,
	};

	//! Priorities ordered from highest to lowest.
	const int priorities[2] = {
	    ANDROID_PRIORITY_AUDIO,
	    ANDROID_PRIORITY_URGENT_DISPLAY,
	};

	const pid_t tid = gettid();

	for (uint32_t i = 0; i < ARRAY_SIZE(priorities); ++i) {
		if (setpriority(PRIO_PROCESS, tid, priorities[i]) == 0) {
			return true;
		}
	}

	return false;
}
