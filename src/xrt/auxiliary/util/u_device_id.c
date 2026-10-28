// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Generator for per-process unique @ref xrt_device_id values.
 * @author Jakob Bornecrantz <tbornecrantz@nvidia.com>
 * @ingroup aux_util
 */

#include "util/u_device_id.h"
#include "util/u_limited_unique_id.h"


struct xrt_device_id
u_device_id_generate(void)
{
	/*
	 * The xrt_limited_unique_id only guartantees that it is unique within
	 * one process, that's its limitation. Device IDs are only generated
	 * in one process, either the service for out-of-processs  or the app
	 * for in-process mode. That's the rule for xrt_device_id which makes
	 * this is safe to use here. And also makes device IDs unique with any
	 * other uses of limited unique id, or using it to generate an ID.
	 */
	return XRT_C11_COMPOUND(struct xrt_device_id){
	    .val = u_limited_unique_id_get().data,
	};
}

void
u_device_id_assign(struct xrt_device *xdev)
{
	xdev->id = u_device_id_generate();
}
