// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of error code conversion for OpenVR.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_common
 */

#include "xrt/xrt_results.h"

#include "openvr_interfaces_unified.h"


namespace xrt::state_trackers::openvr {

vr::EVRInitError
xretToInitError(xrt_result_t xret)
{
	vr::EVRInitError init_error = vr::EVRInitError::VRInitError_Unknown;
	switch (xret) {
	case XRT_SUCCESS: init_error = vr::EVRInitError::VRInitError_None; break;
	case XRT_ERROR_IPC_FAILURE: init_error = vr::EVRInitError::VRInitError_IPC_Failed; break;
	default: break;
	}
	return init_error;
}

vr::EVRCompositorError
xretToCompositorError(xrt_result_t xret)
{
	vr::EVRCompositorError compositor_error = vr::EVRCompositorError::VRCompositorError_RequestFailed;
	switch (xret) {
	case XRT_SUCCESS: compositor_error = vr::EVRCompositorError::VRCompositorError_None; break;
	default: break;
	}
	return compositor_error;
}

}; // namespace xrt::state_trackers::openvr
