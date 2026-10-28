// Copyright 2018-2024, Collabora, Ltd.
// Copyright 2023-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Holds per session attached actions.
 * @ingroup oxr_main
 */

#pragma once

#include "os/os_threading.h"

#include "oxr_forward_declarations.h"
#include "oxr_extension_support.h"


#ifdef __cplusplus
extern "C" {
#endif


/*!
 * Per-session map of action key to action attachment.
 *
 * This is stored on the session (not on the action context) and tracks
 * all action attachments from all action contexts. State-get and lookup
 * functions (e.g. xrGetActionStateBoolean, haptics, pose) resolve an action
 * by act_key only; they do not need to know which context the action came
 * from. That way, a future extension that adds multiple action contexts
 * per session can keep feeding this single map, and those functions will
 * not need to change.
 *
 * @ingroup oxr_main
 */
struct oxr_session_attached_actions
{
	/*!
	 * A map of action key to action attachment.
	 *
	 * The action attachments are actually owned by the action set
	 * attachments, which are owned by the session action context.
	 *
	 * If non-null, this means an action context (default or otherwise)
	 * action sets have been attached to the owning session, since this map
	 * points to @p oxr_action_attachment members of
	 * @ref oxr_session_action_context::act_set_attachments elements.
	 */
	struct u_hashmap_int *act_attachments_by_key;

	struct os_mutex mutex;
};


/*!
 * Initialize the attached actions for a session.
 *
 * @public @memberof oxr_session_attached_actions
 */
XRT_NONNULL_ALL XrResult
oxr_session_attached_actions_init(struct oxr_session_attached_actions *attached);

/*!
 * Finalize and cleanup the attached actions.
 *
 * @public @memberof oxr_session_attached_actions
 */
XRT_NONNULL_ALL void
oxr_session_attached_actions_fini(struct oxr_session_attached_actions *attached);

/*!
 * Add a single action attachment to the map (thread-safe).
 *
 * @public @memberof oxr_session_attached_actions
 */
XRT_NONNULL_ALL XrResult
oxr_session_attached_actions_add_action_attachment(struct oxr_session_attached_actions *attached,
                                                   struct oxr_action_attachment *act_attachment);

/*!
 * Remove a single action attachment from the map (thread-safe).
 *
 * @public @memberof oxr_session_attached_actions
 */
XRT_NONNULL_ALL void
oxr_session_attached_actions_remove_action_attachment(struct oxr_session_attached_actions *attached,
                                                      struct oxr_action_attachment *act_attachment);

/*!
 * Find an action attachment by action key (thread-safe).
 *
 * @public @memberof oxr_session_attached_actions
 */
XRT_NONNULL_ALL void
oxr_session_attached_actions_find(struct oxr_session_attached_actions *attached,
                                  uint32_t act_key,
                                  struct oxr_action_attachment **out_act_attached);


#ifdef __cplusplus
}
#endif
