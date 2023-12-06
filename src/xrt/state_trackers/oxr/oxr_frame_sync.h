// Copyright 2024, Collabora, Ltd.
// Copyright 2024, QUALCOMM CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  The objects that handle session running status and blocking of xrWaitFrame.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @author Yulou Liu <quic_yuloliu@quicinc.com>
 * @ingroup oxr_main
 */

#pragma once

#include "xrt/xrt_compiler.h"
#include "xrt/xrt_config_os.h"
#include "xrt/xrt_openxr_includes.h"

#include <stdbool.h>

#if defined(XRT_OS_LINUX) || defined(XRT_OS_WINDOWS)
#include <pthread.h>
#include <assert.h>
#else
#error "OS not supported"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Helper that handles synchronizing the xr{Wait,Begin,End}Frame calls.
 */
struct oxr_frame_sync
{
	pthread_mutex_t mutex;
	pthread_cond_t cond;

	bool canWaitFrameReturn;
	bool initialized;
	bool running;
};

/*!
 * Initialize the frame sync helper.
 *
 * @public @memberof oxr_frame_sync
 */
int
oxr_frame_sync_init(struct oxr_frame_sync *ofs);

/*!
 * Handle mutual exclusion in xrWaitFrame w.r.t. xrBeginFrame
 *
 * @public @memberof oxr_frame_sync
 */
XRT_CHECK_RESULT XrResult
oxr_frame_sync_wait_frame(struct oxr_frame_sync *ofs);

/*!
 * Release at most one blocked xrWaitFrame to run, e.g. from xrBeginFrame.
 *
 * @public @memberof oxr_frame_sync
 */
XRT_CHECK_RESULT XrResult
oxr_frame_sync_release(struct oxr_frame_sync *ofs);

/*!
 * Begin the session, resetting state accordingly.
 *
 * @public @memberof oxr_frame_sync
 */
XRT_CHECK_RESULT XrResult
oxr_frame_sync_begin_session(struct oxr_frame_sync *ofs);

/*!
 * End the session
 *
 * @public @memberof oxr_frame_sync
 */
XRT_CHECK_RESULT XrResult
oxr_frame_sync_end_session(struct oxr_frame_sync *ofs);

/*!
 * Is the session running?.
 *
 * @public @memberof oxr_frame_sync
 */
XRT_CHECK_RESULT bool
oxr_frame_sync_is_session_running(struct oxr_frame_sync *ofs);

/*!
 * Clean up.
 *
 * @public @memberof oxr_frame_sync
 */
void
oxr_frame_sync_fini(struct oxr_frame_sync *ofs);

#ifdef __cplusplus
} // extern "C"
#endif
