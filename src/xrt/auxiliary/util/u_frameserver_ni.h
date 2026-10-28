// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Not implemented function helpers for frameservers.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup aux_util
 */

#pragma once

#include "xrt/xrt_frameserver.h"

#ifdef __cplusplus
extern "C" {
#endif


/*
 *
 * Not implemented function helpers.
 *
 */

/*!
 * Not implemented function for @ref xrt_fs::enumerate_modes.
 *
 * @ingroup aux_util
 */
xrt_result_t
u_frameserver_ni_enumerate_modes(struct xrt_fs *xfs, struct xrt_fs_mode **out_modes, uint32_t *out_count);

/*!
 * Not implemented function for @ref xrt_fs::configure_capture.
 *
 * @ingroup aux_util
 */
xrt_result_t
u_frameserver_ni_configure_capture(struct xrt_fs *xfs, struct xrt_fs_capture_parameters *cp);

/*!
 * Not implemented function for @ref xrt_fs::stream_start.
 *
 * @ingroup aux_util
 */
xrt_result_t
u_frameserver_ni_stream_start(struct xrt_fs *xfs,
                              struct xrt_frame_sink *xs,
                              enum xrt_fs_capture_type capture_type,
                              uint32_t descriptor_index);

/*!
 * Not implemented function for @ref xrt_fs::slam_stream_start.
 *
 * @ingroup aux_util
 */
xrt_result_t
u_frameserver_ni_slam_stream_start(struct xrt_fs *xfs, struct xrt_slam_sinks *sinks);

/*!
 * Not implemented function for @ref xrt_fs::stream_stop.
 *
 * @ingroup aux_util
 */
xrt_result_t
u_frameserver_ni_stream_stop(struct xrt_fs *xfs);

/*!
 * Not implemented function for @ref xrt_fs::is_running.
 *
 * @ingroup aux_util
 */
xrt_result_t
u_frameserver_ni_is_running(struct xrt_fs *xfs, bool *out_is_running);


#ifdef __cplusplus
}
#endif
