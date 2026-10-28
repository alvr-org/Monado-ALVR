// Copyright 2018-2021, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Holds D3D11 specific session functions.
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup oxr_main
 * @ingroup comp_client
 */

#include "xrt/xrt_instance.h"
#include "xrt/xrt_gfx_d3d11.h"

#include "util/u_misc.h"

#include "oxr_objects.h"
#include "oxr_logger.h"
#include "oxr_two_call.h"
#include "oxr_handle_base.h"

#include <stdlib.h>


XrResult
oxr_session_populate_d3d11(struct oxr_logger *log,
                           struct oxr_system *sys,
                           XrGraphicsBindingD3D11KHR const *next,
                           struct oxr_session *sess)
{
	struct xrt_compositor_native *xcn = sess->xcn;
	struct xrt_compositor *xc = xrt_gfx_d3d11_provider_create( //
	    xcn,                                                   //
	    next->device);                                         //

	if (xc == NULL) {
		return oxr_error(log, XR_ERROR_INITIALIZATION_FAILED, "Failed to create a d3d11 client compositor");
	}

	sess->compositor = xc;
	sess->create_swapchain = oxr_swapchain_d3d11_create;

	return XR_SUCCESS;
}
