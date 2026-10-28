// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Misc helpers for @ref xrt_fs implementations.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup aux_util
 */

#include "u_frameserver.h"
#include "u_frameserver_ni.h"

#include <assert.h>


void
u_frameserver_populate_function_pointers(struct xrt_fs *xfs)
{
	assert(xfs != NULL);

	xfs->enumerate_modes = u_frameserver_ni_enumerate_modes;
	xfs->configure_capture = u_frameserver_ni_configure_capture;
	xfs->stream_start = u_frameserver_ni_stream_start;
	xfs->slam_stream_start = u_frameserver_ni_slam_stream_start;
	xfs->stream_stop = u_frameserver_ni_stream_stop;
	xfs->is_running = u_frameserver_ni_is_running;
}
