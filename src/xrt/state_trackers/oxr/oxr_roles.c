// Copyright 2018-2024, Collabora, Ltd.
// Copyright 2023-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Implementation for device role functions.
 * @ingroup oxr_main
 */

#include "oxr_roles.h"
#include "oxr_logger.h"


XrResult
oxr_roles_init_on_stack(struct oxr_logger *log, struct oxr_roles *roles, struct oxr_system *sys)
{
	roles->sys = sys;

	xrt_result_t xret = xrt_system_devices_get_roles(sys->xsysd, &roles->roles);
	if (xret != XRT_SUCCESS) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to get device roles");
	}

	return XR_SUCCESS;
}
