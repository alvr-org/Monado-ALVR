// Copyright 2018-2024, Collabora, Ltd.
// Copyright 2023-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Holds action state get related functions.
 * @ingroup oxr_main
 */

#pragma once

#include "oxr_extension_support.h"
#include "oxr_forward_declarations.h"


#ifdef __cplusplus
extern "C" {
#endif


/*!
 * @public @memberof oxr_session
 */
XrResult
oxr_action_get_boolean(struct oxr_logger *log,
                       struct oxr_session *sess,
                       uint32_t act_key,
                       struct oxr_subaction_paths subaction_paths,
                       XrActionStateBoolean *data);

/*!
 * @public @memberof oxr_session
 */
XrResult
oxr_action_get_vector1f(struct oxr_logger *log,
                        struct oxr_session *sess,
                        uint32_t act_key,
                        struct oxr_subaction_paths subaction_paths,
                        XrActionStateFloat *data);

/*!
 * @public @memberof oxr_session
 */
XrResult
oxr_action_get_vector2f(struct oxr_logger *log,
                        struct oxr_session *sess,
                        uint32_t act_key,
                        struct oxr_subaction_paths subaction_paths,
                        XrActionStateVector2f *data);

/*!
 * @public @memberof oxr_session
 */
XrResult
oxr_action_get_pose(struct oxr_logger *log,
                    struct oxr_session *sess,
                    uint32_t act_key,
                    struct oxr_subaction_paths subaction_paths,
                    XrActionStatePose *data);


#ifdef __cplusplus
}
#endif
