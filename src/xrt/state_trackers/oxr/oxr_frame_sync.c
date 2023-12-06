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

#include "oxr_frame_sync.h"

#include <util/u_misc.h>

int
oxr_frame_sync_init(struct oxr_frame_sync *ofs)
{
	U_ZERO(ofs);

	int ret = pthread_mutex_init(&ofs->mutex, NULL);
	if (ret != 0) {
		return ret;
	}

	ret = pthread_cond_init(&ofs->cond, NULL);
	if (ret) {
		pthread_mutex_destroy(&ofs->mutex);
		return ret;
	}
	ofs->canWaitFrameReturn = 0;
	ofs->initialized = true;
	ofs->running = false;

	return 0;
}

XRT_CHECK_RESULT XrResult
oxr_frame_sync_wait_frame(struct oxr_frame_sync *ofs)
{
	pthread_mutex_lock(&ofs->mutex);
	while (ofs->running) {
		if (1 == ofs->canWaitFrameReturn) {
			ofs->canWaitFrameReturn = 0;
			break;
		} else if (0 == ofs->canWaitFrameReturn) {
			pthread_cond_wait(&ofs->cond, &ofs->mutex);
			continue;
		} else {
			// we are not suppose to be here
			// if canWaitFrameReturn is neither 1 nor 0
			// something goes wrong while session is running
			pthread_mutex_unlock(&ofs->mutex);
			return XR_ERROR_SESSION_NOT_RUNNING;
		}
	}
	if (ofs->running) {
		pthread_mutex_unlock(&ofs->mutex);
		return XR_SUCCESS;
	}
	pthread_mutex_unlock(&ofs->mutex);
	return XR_ERROR_SESSION_NOT_RUNNING;
}

XRT_CHECK_RESULT XrResult
oxr_frame_sync_release(struct oxr_frame_sync *ofs)
{
	pthread_mutex_lock(&ofs->mutex);
	if (ofs->running) {
		if (0 == ofs->canWaitFrameReturn) {
			ofs->canWaitFrameReturn = 1;
			pthread_cond_signal(&ofs->cond);
			pthread_mutex_unlock(&ofs->mutex);
			return XR_SUCCESS;
		}
	}
	pthread_mutex_unlock(&ofs->mutex);
	return XR_ERROR_SESSION_NOT_RUNNING;
}

XRT_CHECK_RESULT XrResult
oxr_frame_sync_begin_session(struct oxr_frame_sync *ofs)
{
	pthread_mutex_lock(&ofs->mutex);
	if (!ofs->running) {
		ofs->canWaitFrameReturn = 1;
		ofs->running = true;
		pthread_cond_signal(&ofs->cond);
		pthread_mutex_unlock(&ofs->mutex);
		return XR_SUCCESS;
	}
	pthread_mutex_unlock(&ofs->mutex);
	return XR_ERROR_SESSION_NOT_RUNNING;
}

XRT_CHECK_RESULT XrResult
oxr_frame_sync_end_session(struct oxr_frame_sync *ofs)
{
	pthread_mutex_lock(&ofs->mutex);
	if (ofs->running) {
		ofs->running = false;
		pthread_cond_signal(&ofs->cond);
		pthread_mutex_unlock(&ofs->mutex);
		return XR_SUCCESS;
	}
	pthread_mutex_unlock(&ofs->mutex);
	return XR_ERROR_SESSION_NOT_RUNNING;
}

XRT_CHECK_RESULT bool
oxr_frame_sync_is_session_running(struct oxr_frame_sync *ofs)
{
	pthread_mutex_lock(&ofs->mutex);
	bool ret = ofs->running;
	pthread_mutex_unlock(&ofs->mutex);
	return ret;
}

void
oxr_frame_sync_fini(struct oxr_frame_sync *ofs)
{
	// The fields are protected.
	pthread_mutex_lock(&ofs->mutex);
	assert(ofs->initialized);

	if (ofs->running) {
		// Stop the thread.
		ofs->running = false;
		// Wake up the thread if it is waiting.
		pthread_cond_signal(&ofs->cond);
	}

	ofs->canWaitFrameReturn = 0;
	ofs->running = false;

	// No longer need to protect fields.
	pthread_mutex_unlock(&ofs->mutex);

	// Destroy resources.
	pthread_mutex_destroy(&ofs->mutex);
	pthread_cond_destroy(&ofs->cond);
	ofs->initialized = false;
}
