// Copyright 2019-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  File holding helper for @ref xrt_result_t results.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup oxr_main
 */

#pragma once

#include "xrt/xrt_results.h"
#include "oxr_objects.h"


/*!
 * Helper define to check results from 'xrt_` functions (@ref xrt_result_t) and
 * also set any needed state.
 *
 * @todo We probably want to invalidate all sessions, not the current.
 *
 * @ingroup oxr_main
 */
#define OXR_CHECK_XRET(LOG, SESS, RESULTS, FUNCTION)                                                                   \
	do {                                                                                                           \
		xrt_result_t check_ret = (RESULTS);                                                                    \
		if (check_ret == XRT_ERROR_IPC_FAILURE && (SESS)) {                                                    \
			(SESS)->has_lost = true;                                                                       \
			return oxr_error((LOG), XR_ERROR_INSTANCE_LOST, "Call to " #FUNCTION " failed");               \
		}                                                                                                      \
		if (check_ret != XRT_SUCCESS) {                                                                        \
			return oxr_error((LOG), XR_ERROR_RUNTIME_FAILURE, "Call to " #FUNCTION " failed");             \
		}                                                                                                      \
	} while (false)

#define OXR_CHECK_XRET_ALWAYS_RET(LOG, SESS, RESULTS, FUNCTION)                                                        \
	do {                                                                                                           \
		OXR_CHECK_XRET(LOG, SESS, RESULTS, FUNCTION);                                                          \
		return XR_SUCCESS;                                                                                     \
	} while (false)

#define OXR_CHECK_XRET_GOTO(LOG, SESS, RESULTS, FUNCTION, XR_RES, GOTO_LABEL)                                          \
	do {                                                                                                           \
		xrt_result_t check_ret = (RESULTS);                                                                    \
		if (check_ret == XRT_ERROR_IPC_FAILURE) {                                                              \
			(SESS)->has_lost = true;                                                                       \
			XR_RES = oxr_error(LOG, XR_ERROR_INSTANCE_LOST, "Call to " #FUNCTION " failed");               \
			goto GOTO_LABEL;                                                                               \
		}                                                                                                      \
		if (check_ret != XRT_SUCCESS) {                                                                        \
			XR_RES = oxr_error(LOG, XR_ERROR_RUNTIME_FAILURE, "Call to " #FUNCTION " failed");             \
			goto GOTO_LABEL;                                                                               \
		}                                                                                                      \
	} while (false)
