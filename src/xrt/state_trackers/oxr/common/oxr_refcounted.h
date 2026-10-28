// Copyright 2018-2024, Collabora, Ltd.
// Copyright 2023-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Reference-counted base for OpenXR state tracker objects.
 * @ingroup oxr_main
 */

#pragma once

#include "xrt/xrt_defines.h"


#ifdef __cplusplus
extern "C" {
#endif


struct oxr_refcounted
{
	struct xrt_reference base;
	//! Destruction callback
	void (*destroy)(struct oxr_refcounted *);
};

/*!
 * Increase the reference count of @p orc.
 */
static inline void
oxr_refcounted_ref(struct oxr_refcounted *orc)
{
	xrt_reference_inc(&orc->base);
}

/*!
 * Decrease the reference count of @p orc, destroying it if it reaches 0.
 */
static inline void
oxr_refcounted_unref(struct oxr_refcounted *orc)
{
	if (xrt_reference_dec_and_is_zero(&orc->base)) {
		orc->destroy(orc);
	}
}


#ifdef __cplusplus
}
#endif
