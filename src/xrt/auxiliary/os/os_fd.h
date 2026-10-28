// Copyright 2019, Collabora, Ltd.
// Copyright 2026, Beyley Cardellio
// Copyright 2026, Finn Zeruhn
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Reading from file descriptors, shared by the Linux hid and serial code.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @author Finn Zeruhn <fzeruhn@icloud.com>
 * @ingroup aux_os
 */

#pragma once

#include "xrt/xrt_config_os.h"

#ifndef XRT_OS_LINUX
#error Linux-specific file.
#endif

#include "os/os_time.h"

#include <stdint.h>
#include <errno.h>
#include <poll.h>
#include <unistd.h>


/*!
 * Waits up to @p milliseconds for @p fd to be readable, then reads from it.
 * A negative @p milliseconds reads without waiting. Retries on EINTR.
 *
 * @return Bytes read, 0 on timeout, -1 on error.
 */
static inline ssize_t
os_fd_read(int fd, uint8_t *data, size_t size, int milliseconds)
{
	ssize_t ret;

	if (milliseconds >= 0) {
		int64_t deadline_ns = os_monotonic_get_ns() + (int64_t)milliseconds * U_TIME_1MS_IN_NS;
		int timeout = milliseconds;
		struct pollfd fds = {
		    .fd = fd,
		    .events = POLLIN,
		};

		while ((ret = poll(&fds, 1, timeout)) < 0 && errno == EINTR) {
			int64_t left_ns = deadline_ns - os_monotonic_get_ns();
			if (left_ns <= 0) {
				ret = 0;
				break;
			}
			timeout = (int)((left_ns + U_TIME_1MS_IN_NS - 1) / U_TIME_1MS_IN_NS);
		}

		if (ret == -1 || ret == 0) {
			// Error or timeout.
			return ret;
		}
		if (fds.revents & (POLLERR | POLLHUP | POLLNVAL)) {
			// Device disconnect?
			return -1;
		}
	}

	do {
		ret = read(fd, data, size);
	} while (ret < 0 && errno == EINTR);

	if (ret < 0 && (errno == EAGAIN || errno == EINPROGRESS)) {
		// Process most likely received a signal.
		ret = 0;
	}

	return ret;
}
