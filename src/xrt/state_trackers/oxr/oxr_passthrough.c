// Copyright 2023-2024, Qualcomm Innovation Center, Inc.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  passthrough related API entrypoint functions.
 * @author Knox Liu <quic_dengkail@qti.qualcomm.com>
 * @ingroup oxr_main
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "math/m_api.h"
#include "math/m_space.h"
#include "util/u_debug.h"
#include "util/u_misc.h"

#include "oxr_objects.h"
#include "oxr_logger.h"
#include "oxr_handle.h"
#include "oxr_input_transform.h"
#include "oxr_chain.h"
#include "oxr_pretty_print.h"

/*
 *
 * XR_FB_passthrough
 *
 */

static enum xrt_passthrough_create_flags
convert_create_flags(XrPassthroughFlagsFB xr_flags)
{
	enum xrt_passthrough_create_flags flags = 0;

	if ((xr_flags & XR_PASSTHROUGH_IS_RUNNING_AT_CREATION_BIT_FB) != 0) {
		flags |= XRT_PASSTHROUGH_IS_RUNNING_AT_CREATION;
	}
	if ((xr_flags & XR_PASSTHROUGH_LAYER_DEPTH_BIT_FB) != 0) {
		flags |= XRT_PASSTHROUGH_LAYER_DEPTH;
	}

	return flags;
}

static enum xrt_passthrough_purpose_flags
convert_purpose_flags(XrPassthroughLayerPurposeFB xr_flags)
{
	enum xrt_passthrough_purpose_flags flags = 0;

	if ((xr_flags & XR_PASSTHROUGH_LAYER_PURPOSE_RECONSTRUCTION_FB) != 0) {
		flags |= XRT_PASSTHROUGH_LAYER_PURPOSE_RECONSTRUCTION;
	}
	if ((xr_flags & XR_PASSTHROUGH_LAYER_PURPOSE_PROJECTED_FB) != 0) {
		flags |= XRT_PASSTHROUGH_LAYER_PURPOSE_PROJECTED;
	}
	if ((xr_flags & XR_PASSTHROUGH_LAYER_PURPOSE_TRACKED_KEYBOARD_HANDS_FB) != 0) {
		flags |= XRT_PASSTHROUGH_LAYER_PURPOSE_TRACKED_KEYBOARD_HANDS;
	}
	if ((xr_flags & XR_PASSTHROUGH_LAYER_PURPOSE_TRACKED_KEYBOARD_MASKED_HANDS_FB) != 0) {
		flags |= XRT_PASSTHROUGH_LAYER_PURPOSE_TRACKED_KEYBOARD_MASKED_HANDS;
	}

	return flags;
}

static XrResult
oxr_passthrough_destroy(struct oxr_logger *log, struct oxr_handle_base *hb)
{
	struct oxr_passthrough *passthrough = (struct oxr_passthrough *)hb;
	free(passthrough);
	return XR_SUCCESS;
}

static XrResult
oxr_passthrough_layer_destroy(struct oxr_logger *log, struct oxr_handle_base *hb)
{
	struct oxr_passthrough_layer *passthroughLayer = (struct oxr_passthrough_layer *)hb;
	free(passthroughLayer);
	return XR_SUCCESS;
}

XrResult
oxr_passthrough_create(struct oxr_logger *log,
                       struct oxr_session *sess,
                       const XrPassthroughCreateInfoFB *createInfo,
                       struct oxr_passthrough **out_passthrough)
{
	struct oxr_instance *inst = sess->sys->inst;

	struct oxr_passthrough *passthrough = NULL;
	OXR_ALLOCATE_HANDLE_OR_RETURN(log, passthrough, OXR_XR_DEBUG_PASSTHROUGH, oxr_passthrough_destroy,
	                              &sess->handle);

	passthrough->sess = sess;
	passthrough->flags = createInfo->flags;

	xrt_result_t xret = XRT_SUCCESS;

	struct xrt_passthrough_create_info info;
	info.create = convert_create_flags(createInfo->flags);
	xret = xrt_comp_create_passthrough(sess->compositor, &info);
	if (xret != XRT_SUCCESS) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to create passthrough");
	}

	*out_passthrough = passthrough;

	return XR_SUCCESS;
}

XrResult
oxr_passthrough_layer_create(struct oxr_logger *log,
                             struct oxr_session *sess,
                             const XrPassthroughLayerCreateInfoFB *createInfo,
                             struct oxr_passthrough_layer **out_layer)
{
	struct oxr_instance *inst = sess->sys->inst;

	struct oxr_passthrough_layer *passthroughLayer = NULL;
	OXR_ALLOCATE_HANDLE_OR_RETURN(log, passthroughLayer, OXR_XR_DEBUG_PASSTHROUGH_LAYER,
	                              oxr_passthrough_layer_destroy, &sess->handle);

	passthroughLayer->sess = sess;
	passthroughLayer->passthrough = createInfo->passthrough;
	passthroughLayer->flags = createInfo->flags;
	passthroughLayer->purpose = createInfo->purpose;

	xrt_result_t xret = XRT_SUCCESS;

	struct xrt_passthrough_layer_create_info info;
	info.create = convert_create_flags(createInfo->flags);
	info.purpose = convert_purpose_flags(createInfo->purpose);

	xret = xrt_comp_create_passthrough_layer(sess->compositor, &info);
	if (xret != XRT_SUCCESS) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to create passthrough layer");
	}

	*out_layer = passthroughLayer;

	return XR_SUCCESS;
}
