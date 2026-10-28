// Copyright 2018-2024, Collabora, Ltd.
// Copyright 2023-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Holds per instance action context.
 * @ingroup oxr_main
 */

#pragma once

#include "oxr_forward_declarations.h"
#include "oxr_pair_hashset.h"
#include "oxr_refcounted.h"
#include "oxr_suggested_profile_array.h"


#ifdef __cplusplus
extern "C" {
#endif


/*!
 * Holds all action-related state that lives at the instance level (shared
 * across sessions). Used for duplicate checking and storage of client-suggested
 * that are later applied when action sets are attached to a session.
 *
 * Reference-counted; use @ref oxr_refcounted_ref and @ref oxr_refcounted_unref
 * on the @ref base member to manage references. Destroy is only called when
 * the reference count reaches zero.
 *
 * - @ref action_sets: Registry of action set name and localized name pairs.
 *   Used to enforce uniqueness in xrCreateActionSet and to remove entries when
 *   an action set is destroyed.
 * - @ref suggested_profiles: Interaction profile binding suggestions from the
 *   app. When the app calls xrAttachSessionActionSets, this array is cloned
 *   into the session’s action context.
 *
 * In a future extension, this context is intended to support namespaced action
 * sets and to allow plugins (e.g. in a game) to suggest their own bindings
 * without affecting the main application’s bindings.
 *
 * @ingroup oxr_main
 */
struct oxr_instance_action_context
{
	struct oxr_refcounted base;

	/*!
	 * Action set name and localized name stores.
	 */
	struct oxr_pair_hashset action_sets;

	/*!
	 * Interaction profile bindings that have been suggested by the client.
	 */
	struct oxr_suggested_profile_array suggested_profiles;

	/*!
	 * Runtime binding registry used by this action context.
	 *
	 * Multiple action contexts may share the same registry, and future action
	 * contexts may install a modified registry.
	 */
	struct oxr_binding_registry *binding_registry;
};


/*
 *
 * Functions
 *
 */

/*!
 * Create a new instance action context (reference count 1).
 *
 * @param log Logger
 * @param registry Binding registry used by the context, the context takes a
 *        reference to it.
 * @param out_context On success, the new context (refcount 1). Caller must
 *        call @ref oxr_refcounted_unref on context->base when done.
 * @public @memberof oxr_instance_action_context
 * @ingroup oxr_main
 */
XRT_CHECK_RESULT XrResult
oxr_instance_action_context_create(struct oxr_logger *log,
                                   struct oxr_binding_registry *registry,
                                   struct oxr_instance_action_context **out_context);


#ifdef __cplusplus
}
#endif
