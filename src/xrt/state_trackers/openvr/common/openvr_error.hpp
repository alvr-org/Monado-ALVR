// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of error code conversion for OpenVR.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_common
 */

#pragma once

#include "xrt/xrt_results.h"

#include "openvr_interfaces_unified.h"

#include "openvr_logger.hpp"


namespace xrt::state_trackers::openvr {

//! Helper macro to set an OpenVR error pointer if it's not null.
#define SET_ERROR(error_ptr, ERROR)                                                                                    \
	do {                                                                                                           \
		if (error_ptr) {                                                                                       \
			*error_ptr = ERROR;                                                                            \
		}                                                                                                      \
	} while (false)

/*!
 * Converts an xrt_result_t to a vr::EVRInitError.
 *
 * @param xret The xrt_result_t to convert.
 * @return The corresponding vr::EVRInitError.
 */
vr::EVRInitError
xretToInitError(xrt_result_t xret);

/*!
 * Converts an xrt_result_t to a vr::EVRCompositorError.
 *
 * @param xret The xrt_result_t to convert.
 * @return The corresponding vr::EVRCompositorError.
 */
vr::EVRCompositorError
xretToCompositorError(xrt_result_t xret);

}; // namespace xrt::state_trackers::openvr
