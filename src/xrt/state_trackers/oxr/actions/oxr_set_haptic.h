// Copyright 2018-2024, Collabora, Ltd.
// Copyright 2023-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Holds action state get related functions.
 * @ingroup oxr_main
 */

#pragma once

#include "oxr_forward_declarations.h"
#include "oxr_extension_support.h"


#ifdef __cplusplus
extern "C" {
#endif


/*!
 * @public @memberof oxr_session
 */
XrResult
oxr_action_apply_haptic_feedback(struct oxr_logger *log,
                                 struct oxr_session *sess,
                                 uint32_t act_key,
                                 struct oxr_subaction_paths subaction_paths,
                                 const XrHapticBaseHeader *hapticEvent);
/*!
 * @public @memberof oxr_session
 */
XrResult
oxr_action_stop_haptic_feedback(struct oxr_logger *log,
                                struct oxr_session *sess,
                                uint32_t act_key,
                                struct oxr_subaction_paths subaction_paths);


#ifdef __cplusplus
}
#endif
