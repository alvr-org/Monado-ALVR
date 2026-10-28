// Copyright 2018-2024, Collabora, Ltd.
// Copyright 2023-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Holds input related functions.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup oxr_main
 */

#pragma once

#include "oxr_extension_support.h"
#include "oxr_forward_declarations.h"


#ifdef __cplusplus
extern "C" {
#endif


/*
 *
 * Action Set functions
 *
 */

/*!
 * @public @memberof oxr_instance
 */
XrResult
oxr_action_set_create(struct oxr_logger *log,
                      struct oxr_instance *inst,
                      struct oxr_instance_action_context *inst_context,
                      const XrActionSetCreateInfo *createInfo,
                      struct oxr_action_set **out_act_set);

/*!
 * De-initialize an action set attachment and its action attachments.
 *
 * Frees the action attachments, but does not de-allocate the action set
 * attachment.
 *
 * @public @memberof oxr_action_set_attachment
 */
void
oxr_action_set_attachment_teardown(struct oxr_action_set_attachment *act_set_attached);


/*
 *
 * Action functions
 *
 */

/*!
 * @public @memberof oxr_action
 */
XrResult
oxr_action_create(struct oxr_logger *log,
                  struct oxr_action_set *act_set,
                  const XrActionCreateInfo *createInfo,
                  struct oxr_action **out_act);


/*
 *
 * Action input/output functions
 *
 */

/*!
 * Find the pose input for the set of subaction_paths
 *
 * @public @memberof oxr_session
 */
XrResult
oxr_action_get_pose_input(struct oxr_session *sess,
                          uint32_t act_key,
                          const struct oxr_subaction_paths *subaction_paths_ptr,
                          struct oxr_action_input **out_input);

void
oxr_action_cache_stop_output(struct oxr_logger *log, struct oxr_session *sess, struct oxr_action_cache *cache);


/*
 *
 * Session action functions
 *
 */

/*!
 * Attach action sets from bindInfo to the given session/instance action context.
 * @public @memberof oxr_session
 */
XrResult
oxr_session_attach_action_sets(struct oxr_logger *log,
                               const struct oxr_instance_action_context *inst_context,
                               struct oxr_session_attached_actions *attached_actions,
                               struct oxr_session_action_context *sess_context,
                               const XrSessionActionSetsAttachInfo *bindInfo);

/*!
 * @public @memberof oxr_session
 */
XrResult
oxr_action_sync_data(struct oxr_logger *log,
                     struct oxr_session *sess,
                     uint32_t countActionSets,
                     const XrActiveActionSet *actionSets,
                     const XrActiveActionSetPrioritiesEXT *activePriorities);

XrResult
oxr_action_sync_data_with_context(struct oxr_logger *log,
                                  struct oxr_session *sess,
                                  struct oxr_session_action_context *sess_context,
                                  uint32_t countActionSets,
                                  const XrActiveActionSet *actionSets,
                                  const XrActiveActionSetPrioritiesEXT *activePriorities,
                                  bool *out_interaction_profile_changed);

/*!
 * @public @memberof oxr_session
 */
XrResult
oxr_action_enumerate_bound_sources(struct oxr_logger *log,
                                   struct oxr_session *sess,
                                   uint32_t act_key,
                                   uint32_t sourceCapacityInput,
                                   uint32_t *sourceCountOutput,
                                   XrPath *sources);


#ifdef __cplusplus
}
#endif
