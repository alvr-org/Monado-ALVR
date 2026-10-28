// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Not implemented function helpers for frameservers.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup aux_util
 */

#include "util/u_frameserver_ni.h"
#include "util/u_logging.h"


/*
 *
 * Not implemented function helpers.
 *
 */

#define E(FN) U_LOG_E("Function " #FN " is not implemented for '%s'", xfs->name)

xrt_result_t
u_frameserver_ni_enumerate_modes(struct xrt_fs *xfs, struct xrt_fs_mode **out_modes, uint32_t *out_count)
{
	E(enumerate_modes);
	return XRT_ERROR_NOT_IMPLEMENTED;
}

xrt_result_t
u_frameserver_ni_configure_capture(struct xrt_fs *xfs, struct xrt_fs_capture_parameters *cp)
{
	E(configure_capture);
	return XRT_ERROR_NOT_IMPLEMENTED;
}

xrt_result_t
u_frameserver_ni_stream_start(struct xrt_fs *xfs,
                              struct xrt_frame_sink *xs,
                              enum xrt_fs_capture_type capture_type,
                              uint32_t descriptor_index)
{
	E(stream_start);
	return XRT_ERROR_NOT_IMPLEMENTED;
}

xrt_result_t
u_frameserver_ni_slam_stream_start(struct xrt_fs *xfs, struct xrt_slam_sinks *sinks)
{
	E(slam_stream_start);
	return XRT_ERROR_NOT_IMPLEMENTED;
}

xrt_result_t
u_frameserver_ni_stream_stop(struct xrt_fs *xfs)
{
	E(stream_stop);
	return XRT_ERROR_NOT_IMPLEMENTED;
}

xrt_result_t
u_frameserver_ni_is_running(struct xrt_fs *xfs, bool *out_is_running)
{
	E(is_running);
	return XRT_ERROR_NOT_IMPLEMENTED;
}
