// Copyright 2018-2024, Collabora, Ltd.
// Copyright 2023-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Holds per session action context.
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @ingroup oxr_main
 */

#pragma once

#include "os/os_threading.h"

#include "oxr_forward_declarations.h"
#include "oxr_extension_support.h"
#include "oxr_io_cache.h"
#include "oxr_priority_group_cache.h"
#include "oxr_subaction.h"
#include "oxr_suggested_profile_array.h"


#ifdef __cplusplus
extern "C" {
#endif


struct xrt_system_devices;
struct xrt_space_overseer;
struct oxr_logger;


/*
 *
 * Helper macros
 *
 */

/*!
 * Helper macro to check if action sets are attached to a session.
 *
 * Returns XR_ERROR_ACTIONSET_NOT_ATTACHED if action sets have not been attached.
 *
 * @param SESS Session pointer
 * @param LOG Logger pointer
 * @ingroup oxr_main
 */
#define OXR_SESSION_CHECK_ATTACHED_AND_RET(SESS, LOG)                                                                  \
	do {                                                                                                           \
		if (!oxr_session_action_context_has_attached_act_sets(&(SESS)->action_context)) {                      \
			return oxr_error(LOG, XR_ERROR_ACTIONSET_NOT_ATTACHED,                                         \
			                 "ActionSet(s) have not been attached to this session");                       \
		}                                                                                                      \
	} while (false)


/*
 *
 * Structs
 *
 */

/*!
 * This holds all of the action state that belongs on the session level.
 *
 * Future extensions might enable multiple action contexts per session.
 */
struct oxr_session_action_context
{
	/*
	 * Action Set fields
	 */

	/*!
	 * A map of action set key to action set attachments.
	 *
	 * If non-null, this means action sets have been attached to this
	 * session, since this map points to elements of
	 * oxr_session_action_context::act_set_attachments
	 */
	struct u_hashmap_int *act_sets_attachments_by_key;

	/*!
	 * An array of action set attachments that this session owns.
	 *
	 * If non-null, this means action sets have been attached to this
	 * session.
	 */
	struct oxr_action_set_attachment *act_set_attachments;

	/*!
	 * Length of @ref oxr_session_action_context::act_set_attachments.
	 */
	size_t action_set_attachment_count;


	/*
	 * Interaction Profiles fields
	 */

	/*!
	 * Clone of suggested binding profiles at action set/session attachment, this
	 * is mandated by the OpenXR specification. It says that the suggested binding
	 * profiles should be snapshotted when action sets are attached to the session.
	 *
	 * @ref oxr_session_attach_action_sets
	 */
	struct oxr_suggested_profile_array profiles_on_attachment;

	/*!
	 * Binding registry used to resolve this session action context.
	 */
	struct oxr_binding_registry *binding_registry;

	//! Cache of the last known system roles generation_id
	uint64_t dynamic_roles_generation_id;

	//! Protects access to dynamic_roles_generation_id during sync actions
	struct os_mutex sync_actions_mutex;

	struct oxr_priority_group_cache priority_group_cache;

	struct oxr_io_cache io_cache;

	/*!
	 * Currently bound interaction profile.
	 * @{
	 */

#define OXR_PATH_MEMBER(X) XrPath X;

	OXR_FOR_EACH_VALID_SUBACTION_PATH(OXR_PATH_MEMBER)
#undef OXR_PATH_MEMBER
	/*!
	 * @}
	 */
};


/*
 *
 * Functions
 *
 */

/*!
 * Initialize the action context for a session.
 *
 * @public @memberof oxr_session_action_context
 */
XRT_CHECK_RESULT XrResult
oxr_session_action_context_init(struct oxr_session_action_context *action_context,
                                struct xrt_system_devices *xsysd,
                                struct xrt_space_overseer *xso);

/*!
 * Finalize and cleanup the action context for a session.
 *
 * Safe to call after a failed @ref oxr_session_action_context_init; only resources
 * that init successfully created are torn down.
 *
 * @public @memberof oxr_session_action_context
 */
XRT_NONNULL_ALL void
oxr_session_action_context_fini(struct oxr_session_action_context *action_context);

/*!
 * Check if action sets have been attached to the action context.
 *
 * @return true if action sets are attached, false otherwise
 * @public @memberof oxr_session_action_context
 */
XRT_NONNULL_ALL bool
oxr_session_action_context_has_attached_act_sets(const struct oxr_session_action_context *action_context);

/*!
 * Helper to find action set attachment.
 *
 * @return true if the action set was found, false otherwise
 * @public @memberof oxr_session_action_context
 */
XRT_NONNULL_ALL bool
oxr_session_action_context_find_set(struct oxr_session_action_context *action_context,
                                    uint32_t act_set_key,
                                    struct oxr_action_set_attachment **out_act_set_attached);


#ifdef __cplusplus
}
#endif
