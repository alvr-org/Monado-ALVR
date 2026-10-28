// Copyright 2018-2024, Collabora, Ltd.
// Copyright 2023-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Holds binding related functions.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup oxr_main
 */

#pragma once

#include "xrt/xrt_device.h"

#include "oxr_extension_support.h"
#include "oxr_forward_declarations.h"


#ifdef __cplusplus
extern "C" {
#endif


/*
 *
 * Binding functions
 *
 */

/*!
 * Suggest bindings for an interaction profile. Finds or creates the profile
 * for the given path, applies the suggested action-path bindings and takes
 * ownership of the given dpad state.
 *
 * @param log Logger
 * @param store Path store; paths may be created in the store.
 * @param cache Instance path cache; used to get the profile template.
 * @param inst_context Instance action context
 * @param suggestedBindings Suggested bindings for the interaction profile
 * @param state Dpad state (ownership transferred into the profile)
 * @return XR_SUCCESS on success
 * @public @memberof oxr_instance
 */
XrResult
oxr_action_suggest_interaction_profile_bindings(struct oxr_logger *log,
                                                struct oxr_path_store *store,
                                                const struct oxr_instance_path_cache *cache,
                                                struct oxr_instance_action_context *inst_context,
                                                const XrInteractionProfileSuggestedBinding *suggestedBindings,
                                                struct oxr_dpad_state *state);

/*!
 * Get the currently active interaction profile for a top-level user path
 * on the session action context (e.g. /user/hand/left). Requires action sets to
 * be attached.
 *
 * @param log Logger
 * @param cache Instance path cache
 * @param action_context Action context
 * @param topLevelUserPath Top-level user path (e.g. left hand)
 * @param interactionProfile Output interaction profile state
 * @return XR_SUCCESS on success
 * @public @memberof oxr_instance
 */
XrResult
oxr_action_get_current_interaction_profile(struct oxr_logger *log,
                                           const struct oxr_instance_path_cache *cache,
                                           const struct oxr_session_action_context *action_context,
                                           XrPath topLevelUserPath,
                                           XrInteractionProfileState *interactionProfile);

/*!
 * Get input source localized name.
 *
 * @param log Logger
 * @param path_store Needed to look up path strings.
 * @param action_context Action context
 * @param getInfo Input source localized name get info
 * @param bufferCapacityInput Buffer capacity
 * @param bufferCountOutput Buffer count output
 * @param buffer Buffer
 * @return XR_SUCCESS on success
 * @public @memberof oxr_session
 */
XrResult
oxr_action_get_input_source_localized_name(struct oxr_logger *log,
                                           const struct oxr_path_store *store,
                                           const struct oxr_session_action_context *action_context,
                                           const XrInputSourceLocalizedNameGetInfo *getInfo,
                                           uint32_t bufferCapacityInput,
                                           uint32_t *bufferCountOutput,
                                           char *buffer);


#ifdef __cplusplus
}
#endif
