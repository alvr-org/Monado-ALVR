// Copyright 2019-2024, Collabora, Ltd.
// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Haptic related functions.
 * @ingroup oxr_api
 */

#pragma once

#include "oxr_forward_declarations.h"
#include "oxr_extension_support.h"


#ifdef __cplusplus
extern "C" {
#endif


/*!
 * Mostly used to implement the XR_FB_haptic_pcm extension.
 */
XrResult
oxr_haptic_get_attachment_pcm_sample_rate(struct oxr_action_attachment *act_attached,
                                          const struct oxr_subaction_paths subaction_paths,
                                          float *sample_rate);


#ifdef __cplusplus
}
#endif
