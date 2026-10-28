// Copyright 2018-2024, Collabora, Ltd.
// Copyright 2023-2026, NVIDIA CORPORATION.
// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Holds input related functions.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup oxr_main
 */

#include "xrt/xrt_compiler.h"

#include "util/u_debug.h"
#include "util/u_time.h"
#include "util/u_misc.h"

#include "math/m_vec2.h"

#include "oxr_input.h"
#include "oxr_resolver_for_bindings.h"
#include "oxr_binding.h"
#include "oxr_binding_registry.h"
#include "oxr_pair_hashset.h"
#include "oxr_priority_group_cache.h"
#include "oxr_subaction.h"
#include "oxr_dpad_state.h"
#include "oxr_input_transform.h"
#include "oxr_instance_action_context.h"
#include "oxr_generated_bindings.h"
#include "oxr_handle_base.h"
#include "oxr_suggested_profile_array.h"
#include "oxr_session_action_context.h"

#include "../oxr_objects.h"
#include "../oxr_logger.h"
#include "../oxr_two_call.h"
#include "../oxr_conversions.h"
#include "../oxr_xret.h"
#include "../oxr_roles.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


/*
 *
 * Structs.
 *
 */

struct oxr_profiles_per_subaction
{
#define INFO_MEMBER(X) struct oxr_profile_resolver_info X;
	OXR_FOR_EACH_VALID_SUBACTION_PATH(INFO_MEMBER)
#undef INFO_MEMBER
};


/*
 *
 * Helpers
 *
 */

/*!
 * Given an Action Set handle, return the @ref oxr_action_set and the
 * associated @ref oxr_action_set_attachment in the given Session.
 */
static void
get_action_set_attachment(struct oxr_session_action_context *sess_context,
                          XrActionSet actionSet,
                          struct oxr_action_set_attachment **out_act_set_attached,
                          struct oxr_action_set **out_act_set)
{
	struct oxr_action_set *act_set = XRT_CAST_OXR_HANDLE_TO_PTR(struct oxr_action_set *, actionSet);

	// Make sure all are set.
	*out_act_set = act_set;
	*out_act_set_attached = NULL;

	// Safe to call even if nothing has been attached.
	oxr_session_action_context_find_set(sess_context, act_set->act_set_key, out_act_set_attached);
}


/*
 *
 * Pre declare functions.
 *
 */

static void
oxr_action_cache_update(struct oxr_logger *log,
                        struct oxr_session *sess,
                        uint32_t countActionSets,
                        const XrActiveActionSet *actionSets,
                        struct oxr_action_attachment *act_attached,
                        struct oxr_action_cache *cache,
                        int64_t time,
                        struct oxr_subaction_paths *subaction_path,
                        bool select);

static void
oxr_action_attachment_update(struct oxr_logger *log,
                             struct oxr_session *sess,
                             uint32_t countActionSets,
                             const XrActiveActionSet *actionSets,
                             struct oxr_action_attachment *act_attached,
                             int64_t time,
                             struct oxr_subaction_paths subaction_paths);

static void
oxr_action_bind_io(struct oxr_logger *log,
                   struct oxr_sink_logger *slog,
                   const struct oxr_session_action_context *sess_context,
                   const struct oxr_profile_resolver_info *info,
                   const struct xrt_io_cache_device *dev,
                   XrPath subaction_path,
                   struct oxr_action_cache *cache,
                   struct oxr_action_attachment *act_attached);

/*!
 * Helper function to combine @ref oxr_subaction_paths structs, but skipping
 * @ref oxr_subaction_paths::any.
 *
 * If a real (non-"any") subaction path in @p new_subaction_paths is true, it will be
 * true in @p subaction_paths.
 *
 * @private @memberof oxr_subaction_paths
 */
static inline void
oxr_subaction_paths_accumulate_except_any(struct oxr_subaction_paths *subaction_paths,
                                          const struct oxr_subaction_paths *new_subaction_paths)
{
#define ACCUMULATE_SUBACTION_PATHS(X) subaction_paths->X |= new_subaction_paths->X;
	OXR_FOR_EACH_SUBACTION_PATH(ACCUMULATE_SUBACTION_PATHS)
#undef ACCUMULATE_SUBACTION_PATHS
}

/*!
 * Helper function to combine @ref oxr_subaction_paths structs.
 *
 * If a subaction path in @p new_subaction_paths is true, it will be true in @p subaction_paths.
 *
 * @private @memberof oxr_subaction_paths
 */
static inline void
oxr_subaction_paths_accumulate(struct oxr_subaction_paths *subaction_paths,
                               const struct oxr_subaction_paths *new_subaction_paths)
{
	subaction_paths->any |= new_subaction_paths->any;
	oxr_subaction_paths_accumulate_except_any(subaction_paths, new_subaction_paths);
}

static void
reset_last_sync_priorities(struct oxr_session_action_context *action_context)
{
	for (size_t i = 0; i < action_context->action_set_attachment_count; i++) {
		struct oxr_action_set_attachment *act_set_attached = &action_context->act_set_attachments[i];
		act_set_attached->last_sync_priority = act_set_attached->act_set_ref->priority;
	}
}

static void
reset_inactive_sync_priority_state(struct oxr_session_action_context *action_context)
{
	oxr_priority_group_cache_zero_priorities(&action_context->priority_group_cache);
	reset_last_sync_priorities(action_context);

	for (size_t i = 0; i < action_context->action_set_attachment_count; i++) {
		U_ZERO(&action_context->act_set_attachments[i].requested_subaction_paths);
	}
}


/*
 *
 * Action attachment functions
 *
 */

/*!
 * De-initialize/de-allocate all dynamic members of @ref oxr_action_cache and
 * reset all fields. This function is used when destroying an action that has
 * been attached to the session (@ref oxr_action_attachment), or when bindings
 * are (re)made for the action. Bindings can be (re)made multiple times during
 * the runtime of the session, such as when a device is dynamically moved
 * between roles.
 *
 * @private @memberof oxr_action_cache
 */
static void
oxr_action_cache_teardown(struct oxr_action_cache *cache)
{
	// Clean up input transforms
	for (uint32_t i = 0; i < cache->input_count; i++) {
		struct oxr_action_input *action_input = &cache->inputs[i];
		oxr_input_transform_destroy(&(action_input->transforms));
		action_input->transform_count = 0;
	}

	free(cache->inputs);
	cache->inputs = NULL;

	free(cache->outputs);
	cache->outputs = NULL;

	U_ZERO(cache);
}

/*!
 * Tear down an action attachment struct.
 *
 * Does not deallocate the struct itself.
 *
 * @public @memberof oxr_action_attachment
 */
static void
oxr_action_attachment_teardown(struct oxr_action_attachment *act_attached)
{
	oxr_session_attached_actions_remove_action_attachment(act_attached->attached_actions, act_attached);

#define CACHE_TEARDOWN(X) oxr_action_cache_teardown(&(act_attached->X));
	OXR_FOR_EACH_SUBACTION_PATH(CACHE_TEARDOWN)
#undef CACHE_TEARDOWN

	// Unref this action's refcounted data
	oxr_refcounted_unref(&act_attached->act_ref->base);
}

/*!
 * Set up an action attachment struct.
 *
 * @public @memberof oxr_action_attachment
 */
static XrResult
oxr_action_attachment_init(struct oxr_logger *log,
                           struct oxr_action_attachment *act_attached,
                           struct oxr_session_attached_actions *attached_actions,
                           struct oxr_action_set_attachment *act_set_attached,
                           struct oxr_action_ref *act_ref)
{
	// Reference this action's refcounted data
	oxr_refcounted_ref(&act_ref->base);

	act_attached->attached_actions = attached_actions;
	act_attached->act_set_attached = act_set_attached;
	act_attached->act_key = act_ref->act_key; // Copy this for efficiency.
	act_attached->act_ref = act_ref;

	XrResult ret = oxr_session_attached_actions_add_action_attachment(attached_actions, act_attached);
	if (ret != XR_SUCCESS) {
		oxr_refcounted_unref(&act_attached->act_ref->base);
		U_ZERO(act_attached);
		return oxr_error(log, ret, "Failed to add action attachment to session attached actions");
	}

	return XR_SUCCESS;
}


/*
 *
 * Action set attachment functions
 *
 */

/*!
 * @public @memberof oxr_action_set_attachment
 */
static XrResult
oxr_action_set_attachment_init(struct oxr_logger *log,
                               struct oxr_action_set_attachment *act_set_attached,
                               struct oxr_session_action_context *action_context,
                               struct oxr_action_set_ref *act_set_ref)
{
	// Reference this action set's refcounted data
	oxr_refcounted_ref(&act_set_ref->base);

	act_set_attached->sess_context = action_context;
	act_set_attached->act_set_key = act_set_ref->act_set_key; // Copy this for efficiency.
	act_set_attached->act_set_ref = act_set_ref;
	act_set_attached->last_sync_priority = act_set_ref->priority;

	int h_ret = u_hashmap_int_insert(                //
	    action_context->act_sets_attachments_by_key, //
	    act_set_ref->act_set_key,                    //
	    act_set_attached);                           //
	if (h_ret < 0) {
		oxr_refcounted_unref(&act_set_attached->act_set_ref->base);
		U_ZERO(act_set_attached);
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE,
		                 "Failed to add action set attachment to session action context");
	}

	return XR_SUCCESS;
}

void
oxr_action_set_attachment_teardown(struct oxr_action_set_attachment *act_set_attached)
{
	for (size_t i = 0; i < act_set_attached->action_attachment_count; ++i) {
		oxr_action_attachment_teardown(&(act_set_attached->act_attachments[i]));
	}

	free(act_set_attached->act_attachments);
	act_set_attached->act_attachments = NULL;
	act_set_attached->action_attachment_count = 0;

	struct oxr_session_action_context *sess_context = act_set_attached->sess_context;
	u_hashmap_int_erase(sess_context->act_sets_attachments_by_key, act_set_attached->act_set_key);

	// Unref this action set's refcounted data
	oxr_refcounted_unref(&act_set_attached->act_set_ref->base);
}


/*
 *
 * Action set functions
 *
 */
static void
oxr_action_set_ref_destroy_cb(struct oxr_refcounted *orc)
{
	struct oxr_action_set_ref *act_set_ref = (struct oxr_action_set_ref *)orc;

	oxr_pair_hashset_fini(&act_set_ref->actions);

	free(act_set_ref);
}

static XrResult
oxr_action_set_destroy_cb(struct oxr_logger *log, struct oxr_handle_base *hb)
{
	struct oxr_action_set *act_set = (struct oxr_action_set *)hb;
	struct oxr_instance_action_context *inst_context = act_set->inst_context;

	oxr_refcounted_unref(&act_set->data->base);
	act_set->data = NULL;

	if (inst_context != NULL) {
		oxr_pair_hashset_erase_and_free(&inst_context->action_sets, &act_set->name_item, &act_set->loc_item);
		act_set->inst_context = NULL;
		oxr_refcounted_unref(&inst_context->base);
	}

	free(act_set);

	return XR_SUCCESS;
}

XrResult
oxr_action_set_create(struct oxr_logger *log,
                      struct oxr_instance *inst,
                      struct oxr_instance_action_context *inst_context,
                      const XrActionSetCreateInfo *createInfo,
                      struct oxr_action_set **out_act_set)
{
	// Mod music for all!
	static uint32_t key_gen = 1;
	XrResult ret;
	int h_ret;

	struct oxr_action_set *act_set = NULL;
	OXR_ALLOCATE_HANDLE_PARENT_OR_RETURN(log, act_set, OXR_XR_DEBUG_ACTIONSET, oxr_action_set_destroy_cb,
	                                     &inst->handle);

	act_set->inst_context = NULL;

	struct oxr_action_set_ref *act_set_ref = U_TYPED_CALLOC(struct oxr_action_set_ref);
	act_set_ref->permitted_subaction_paths.any = true;
	act_set_ref->base.destroy = oxr_action_set_ref_destroy_cb;
	oxr_refcounted_ref(&act_set_ref->base);
	act_set->data = act_set_ref;

	act_set_ref->act_set_key = key_gen++;
	act_set->act_set_key = act_set_ref->act_set_key;

	act_set->inst = inst;

	ret = oxr_pair_hashset_init(log, &act_set_ref->actions);
	if (ret != XR_SUCCESS) {
		oxr_handle_parent_destroy(log, &act_set->handle);
		return ret;
	}

	snprintf(act_set_ref->name, sizeof(act_set_ref->name), "%s", createInfo->actionSetName);

	h_ret = oxr_pair_hashset_insert_str_c(  //
	    &inst_context->action_sets,         //
	    createInfo->actionSetName,          //
	    createInfo->localizedActionSetName, //
	    &act_set->name_item,                //
	    &act_set->loc_item);                //
	if (h_ret != 0) {
		oxr_handle_parent_destroy(log, &act_set->handle);
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to insert action set name pair");
	}

	act_set_ref->priority = createInfo->priority;

	oxr_refcounted_ref(&inst_context->base);
	act_set->inst_context = inst_context;

	*out_act_set = act_set;

	return XR_SUCCESS;
}


/*
 *
 * Action functions
 *
 */

static void
oxr_action_ref_destroy_cb(struct oxr_refcounted *orc)
{
	struct oxr_action_ref *act_ref = (struct oxr_action_ref *)orc;
	free(act_ref);
}

static XrResult
oxr_action_destroy_cb(struct oxr_logger *log, struct oxr_handle_base *hb)
{
	struct oxr_action *act = (struct oxr_action *)hb;

	oxr_refcounted_unref(&act->data->base);
	act->data = NULL;

	oxr_pair_hashset_erase_and_free(&act->act_set->data->actions, &act->name_item, &act->loc_item);

	free(act);

	return XR_SUCCESS;
}

XrResult
oxr_action_create(struct oxr_logger *log,
                  struct oxr_action_set *act_set,
                  const XrActionCreateInfo *createInfo,
                  struct oxr_action **out_act)
{
	struct oxr_instance *inst = act_set->inst;
	struct oxr_subaction_paths subaction_paths = {0};

	// Mod music for all!
	static uint32_t key_gen = 1;
	int h_ret;

	bool bret = oxr_classify_subaction_paths( //
	    &inst->path_cache,                    //
	    createInfo->countSubactionPaths,      //
	    createInfo->subactionPaths,           //
	    &subaction_paths);                    //
	if (!bret) {
		//! @todo This is probably an internal runtime error since it should be validated before?
		return oxr_error(log, XR_ERROR_PATH_UNSUPPORTED, "Bad internal codepath taken (bug?)");
	}

	struct oxr_action *act = NULL;
	OXR_ALLOCATE_HANDLE_OR_RETURN(log, act, OXR_XR_DEBUG_ACTION, oxr_action_destroy_cb, &act_set->handle);


	struct oxr_action_ref *act_ref = U_TYPED_CALLOC(struct oxr_action_ref);
	act_ref->base.destroy = oxr_action_ref_destroy_cb;
	oxr_refcounted_ref(&act_ref->base);
	act->data = act_ref;

	act_ref->act_key = key_gen++;
	act->act_key = act_ref->act_key;

	act->act_set = act_set;
	act_ref->subaction_paths = subaction_paths;
	act_ref->action_type = createInfo->actionType;
	act_ref->subaction_paths = subaction_paths;

	// Any subaction paths allowed for this action are allowed for this
	// action set. But, do not accumulate "any" - it just means none were
	// specified for this action.
	oxr_subaction_paths_accumulate_except_any(&(act_set->data->permitted_subaction_paths), &subaction_paths);

	snprintf(act_ref->name, sizeof(act_ref->name), "%s", createInfo->actionName);

	h_ret = oxr_pair_hashset_insert_str_c( //
	    &act_set->data->actions,           //
	    createInfo->actionName,            //
	    createInfo->localizedActionName,   //
	    &act->name_item,                   //
	    &act->loc_item);                   //
	if (h_ret != 0) {
		oxr_handle_destroy(log, &act->handle);
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to insert action name pair");
	}

	*out_act = act;

	return XR_SUCCESS;
}


/*
 *
 * "Exported" helper functions.
 *
 */

XrResult
oxr_action_get_pose_input(struct oxr_session *sess,
                          uint32_t act_key,
                          const struct oxr_subaction_paths *subaction_paths_ptr,
                          struct oxr_action_input **out_input)
{
	struct oxr_action_attachment *act_attached = NULL;

	oxr_session_attached_actions_find(&sess->attached_actions, act_key, &act_attached);
	if (act_attached == NULL) {
		return XR_SUCCESS;
	}

	struct oxr_subaction_paths subaction_paths = *subaction_paths_ptr;
	if (subaction_paths.any) {
		subaction_paths = act_attached->any_pose_subaction_path;
	}

	// Priority of inputs.
#define GET_POSE_INPUT(X)                                                                                              \
	if (act_attached->X.current.active && subaction_paths.X) {                                                     \
		*out_input = act_attached->X.inputs;                                                                   \
		return XR_SUCCESS;                                                                                     \
	}
	OXR_FOR_EACH_VALID_SUBACTION_PATH(GET_POSE_INPUT)

	// plus a fallback invocation for user
	GET_POSE_INPUT(user)
#undef GET_POSE_INPUT

	return XR_SUCCESS;
}

/*!
 * Look up the cache device entry for a role slot, where @p index is one of the
 * @ref xrt_io_cache_info role indices (left, right, gamepad, head, eyes).
 *
 * Every top-level role is resolved purely through the @ref xrt_io_cache; returns
 * NULL when the role has no assigned device.
 */
static const struct xrt_io_cache_device *
binding_device_from_cache(const struct oxr_io_cache *io_cache, int32_t index)
{
	if (index < 0 || index >= (int32_t)io_cache->info.device_count) {
		return NULL;
	}

	return &io_cache->info.devices[index];
}

/*!
 * Every top-level role (left, right, gamepad, head, eyes) is looked up in the
 * io cache via the matching @ref xrt_io_cache_info role index.
 */
#define GET_BINDING_DEVICE(NAME, IOC) binding_device_from_cache((IOC), (IOC)->info.NAME)

static void
resolve_binding_infos_from_roles(struct oxr_logger *log,
                                 const struct oxr_session_action_context *action_context,
                                 const struct oxr_suggested_profile_array *suggested_profiles,
                                 const struct oxr_roles *roles,
                                 bool debug_bindings,
                                 struct oxr_profiles_per_subaction *out_infos)
{
	struct oxr_sink_logger slog = {0};
	const struct oxr_io_cache *io_cache = &action_context->io_cache;

#define RESOLVE_INFO(NAME, NAME_CAPS, PATH, LOCALIZED_NAME)                                                            \
	do {                                                                                                           \
		const struct xrt_io_cache_device *dev = GET_BINDING_DEVICE(NAME, io_cache);                            \
		if (dev == NULL) {                                                                                     \
			oxr_slog(&slog, "\n\t" #NAME ": No device");                                                   \
			break;                                                                                         \
		}                                                                                                      \
		oxr_slog(&slog, "\n\t" #NAME ": (%s)", u_str_xrt_device_name(dev->name));                              \
		oxr_resolver_populate_profile(log, &slog, action_context->binding_registry, suggested_profiles, dev,   \
		                              GET_PROFILE_NAME_BY_ROLE(roles, NAME), &out_infos->NAME);                \
	} while (false);
	OXR_FOR_EACH_VALID_SUBACTION_PATH_DETAILED(RESOLVE_INFO)
#undef RESOLVE_INFO

	if (debug_bindings) {
		oxr_log_slog(log, &slog);
	} else {
		oxr_slog_cancel(&slog);
	}
}

/*!
 * @public @memberof oxr_action_attachment
 */
static void
oxr_action_attachment_bind(struct oxr_logger *log,
                           struct oxr_instance *inst,
                           struct oxr_action_attachment *act_attached,
                           const struct oxr_profiles_per_subaction *resolver_infos)
{
	struct oxr_sink_logger slog = {0};
	const struct oxr_action_ref *act_ref = act_attached->act_ref;

	// Start logging into a single buffer.
	oxr_slog(&slog, "Binding '%s/%s' %s\n",                      //
	         act_attached->act_set_attached->act_set_ref->name,  //
	         act_ref->name,                                      //
	         xr_action_type_to_short_str(act_ref->action_type)); //

	if (act_ref->subaction_paths.user) {
		assert(!"Unreachable");
		oxr_slog(&slog, "\tFor: user (HOW WAS THIS ENABLED?)\n");
	}

	/*
	 * The bare `/user` top-level path is never bound. No interaction
	 * profile targets it and it has no binding points of its own; it is a
	 * left over in both the spec and our code, so there is nothing to
	 * resolve for it.
	 *
	 * The "any" path is not a top-level path either: it means the action
	 * was created without specifying any subaction paths. Such an action is
	 * bound across every concrete top-level path (left, right, head,
	 * gamepad, eyes), which is why the BIND_SUBACTION loop below also binds
	 * a subaction path when `any` is set.
	 */

#define BIND_SUBACTION(NAME, NAME_CAPS, PATH, LOCALIZED_NAME)                                                          \
	if (act_ref->subaction_paths.NAME || act_ref->subaction_paths.any) {                                           \
		oxr_slog(&slog, "\tFor: %s\n", #NAME);                                                                 \
		struct oxr_session_action_context *sess_context = act_attached->act_set_attached->sess_context;        \
		const struct xrt_io_cache_device *dev = GET_BINDING_DEVICE(NAME, &sess_context->io_cache);             \
		oxr_action_bind_io(log, &slog, sess_context, &resolver_infos->NAME, dev, inst->path_cache.NAME,        \
		                   &act_attached->NAME, act_attached);                                                 \
	}
	OXR_FOR_EACH_VALID_SUBACTION_PATH_DETAILED(BIND_SUBACTION)
#undef BIND_SUBACTION


	/*!
	 * The any sub path is special cased for poses, it binds to one sub path
	 * and sticks with it.
	 */
	if (act_ref->action_type == XR_ACTION_TYPE_POSE_INPUT) {

#define RESET_ANY(NAME) act_attached->any_pose_subaction_path.NAME = false;
		OXR_FOR_EACH_VALID_SUBACTION_PATH(RESET_ANY)
#undef RESET_ANY

#define POSE_ANY(NAME)                                                                                                 \
	if ((act_ref->subaction_paths.NAME || act_ref->subaction_paths.any) && act_attached->NAME.input_count > 0) {   \
		act_attached->any_pose_subaction_path.NAME = true;                                                     \
		oxr_slog(&slog, "\tFor: <any>\n\t\tBinding any pose to " #NAME ".\n");                                 \
	} else
		OXR_FOR_EACH_VALID_SUBACTION_PATH(POSE_ANY)
#undef POSE_ANY

		{
			oxr_slog(&slog,
			         "\tFor: <any>\n\t\tNo active sub paths for "
			         "the any pose!\n");
		}
	}

	oxr_slog(&slog, "\tDone");

	// Also frees all data.
	if (inst->debug_bindings) {
		oxr_log_slog(log, &slog);
	} else {
		oxr_slog_cancel(&slog);
	}
}

void
oxr_action_cache_stop_output(struct oxr_logger *log, struct oxr_session *sess, struct oxr_action_cache *cache)
{
	// Set this as stopped.
	cache->stop_output_time = 0;

	struct xrt_output_value value = {.type = XRT_OUTPUT_VALUE_TYPE_VIBRATION, .vibration = {0}};

	for (uint32_t i = 0; i < cache->output_count; i++) {
		struct oxr_action_output *output = &cache->outputs[i];

		xrt_result_t xret =
		    xrt_io_cache_set_output(output->io_cache->xic, output->device_id, output->name, &value);
		if (xret != XRT_SUCCESS) {
			struct oxr_sink_logger slog = {0};
			oxr_slog(&slog, "Failed to stop output ");
			u_pp_xrt_output_name(oxr_slog_dg(&slog), output->name);
			oxr_log_slog(log, &slog);
		}
	}
}

static uint32_t
oxr_get_action_set_priority(const struct oxr_action_set_ref *act_set_ref,
                            const XrActiveActionSetPrioritiesEXT *activePriorities)
{
	if (activePriorities) {
		for (uint32_t i = 0; i < activePriorities->actionSetPriorityCount; i++) {
			XrActiveActionSetPriorityEXT act_set_priority = activePriorities->actionSetPriorities[i];

			struct oxr_action_set *action_set =
			    XRT_CAST_OXR_HANDLE_TO_PTR(struct oxr_action_set *, act_set_priority.actionSet);

			if (action_set->data == act_set_ref) {
				return act_set_priority.priorityOverride;
			}
		}
	}

	return act_set_ref->priority;
}

static void
accumulate_priority_group_cache(struct oxr_action_cache *cache, uint32_t priority)
{
	for (size_t i = 0; i < cache->input_count; i++) {
		struct oxr_action_input *input = &cache->inputs[i];
		if (input->priority_group_state == NULL) {
			continue;
		}

		if (priority > input->priority_group_state->highest_priority) {
			input->priority_group_state->highest_priority = priority;
		}
	}
}

static void
oxr_session_rebuild_priority_group_states(struct oxr_session_action_context *action_context,
                                          uint32_t countActionSets,
                                          const XrActiveActionSet *actionSets,
                                          const XrActiveActionSetPrioritiesEXT *activePriorities)
{
	oxr_priority_group_cache_zero_priorities(&action_context->priority_group_cache);

	/*
	 * Revert every attachment to its create-info priority before applying
	 * this sync's active sets. XR_EXT_active_action_set_priority overrides
	 * only last until the next xrSyncActions; sets omitted from the active
	 * list must not keep a stale override for bound-source enumeration.
	 */
	reset_last_sync_priorities(action_context);

	for (uint32_t i = 0; i < countActionSets; i++) {
		struct oxr_action_set *act_set = NULL;
		struct oxr_action_set_attachment *act_set_attached = NULL;
		get_action_set_attachment(action_context, actionSets[i].actionSet, &act_set_attached, &act_set);
		if (act_set_attached == NULL) {
			continue;
		}

		uint32_t priority = oxr_get_action_set_priority(act_set_attached->act_set_ref, activePriorities);
		act_set_attached->last_sync_priority = priority;

		for (size_t k = 0; k < act_set_attached->action_attachment_count; k++) {
			struct oxr_action_attachment *act_attached = &act_set_attached->act_attachments[k];

#define ACCUMULATE_CACHE(NAME, NAME_CAPS, PATH, LOCALIZED_NAME)                                                        \
	if (act_set_attached->requested_subaction_paths.any || act_set_attached->requested_subaction_paths.NAME) {     \
		accumulate_priority_group_cache(&act_attached->NAME, priority);                                        \
	}
			OXR_FOR_EACH_VALID_SUBACTION_PATH_DETAILED(ACCUMULATE_CACHE)
#undef ACCUMULATE_CACHE
		}
	}
}

static bool
input_suppressed(const struct oxr_action_attachment *act_attached, const struct oxr_action_input *action_input)
{
	assert(action_input->priority_group_state != NULL);

	if (action_input->priority_group_state == NULL) {
		return false;
	}

	uint32_t priority = act_attached->act_set_attached->last_sync_priority;

	return action_input->priority_group_state->highest_priority > priority;
}

static bool
oxr_input_combine_input(struct oxr_session *sess,
                        uint32_t countActionSets,
                        const XrActiveActionSet *actionSets,
                        struct oxr_action_attachment *act_attached,
                        struct oxr_subaction_paths *subaction_path,
                        struct oxr_action_cache *cache,
                        struct oxr_input_value_tagged *out_input,
                        int64_t *out_timestamp,
                        bool *out_is_active)
{
	struct oxr_action_input *inputs = cache->inputs;
	size_t input_count = cache->input_count;

	if (input_count == 0) {
		*out_is_active = false;
		return true;
	}

	bool any_active = false;
	struct oxr_input_value_tagged res = {0};
	int64_t res_timestamp = inputs[0].input->timestamp;

	for (size_t i = 0; i < input_count; i++) {
		struct oxr_action_input *action_input = &(inputs[i]);
		struct xrt_input *input = action_input->input;

		// suppress input if it is also bound to action in set with
		// higher priority
		if (input_suppressed(act_attached, action_input)) {
			continue;
		}

		if (input->active) {
			any_active = true;
		} else {
			continue;
		}

		struct oxr_input_value_tagged raw_input = {
		    .type = XRT_GET_INPUT_TYPE(input->name),
		    .value = input->value,
		};

		struct oxr_input_value_tagged transformed = {0};
		if (!oxr_input_transform_process(action_input->transforms, action_input->transform_count, &raw_input,
		                                 &transformed)) {
			// We couldn't transform, how strange. Reset all state.
			// At this level we don't know what action this is, etc.
			// so a warning message isn't very helpful.
			return false;
		}

		// at this stage type should be "compatible" to action
		res.type = transformed.type;

		switch (transformed.type) {
		case XRT_INPUT_TYPE_BOOLEAN:
			res.value.boolean |= transformed.value.boolean;

			/* Special case bool: all bool inputs are combined with
			 * OR. The action only changes to true on the earliest
			 * input that sets it to true, and to false on the
			 * latest input that is false. */
			if (res.value.boolean && transformed.value.boolean && input->timestamp < res_timestamp) {
				res_timestamp = input->timestamp;
			} else if (!res.value.boolean && !transformed.value.boolean &&
			           input->timestamp > res_timestamp) {
				res_timestamp = input->timestamp;
			}
			break;
		case XRT_INPUT_TYPE_VEC1_MINUS_ONE_TO_ONE:
		case XRT_INPUT_TYPE_VEC1_ZERO_TO_ONE:
			if (fabsf(transformed.value.vec1.x) > fabsf(res.value.vec1.x)) {
				res.value.vec1.x = transformed.value.vec1.x;
				res_timestamp = input->timestamp;
			}
			break;
		case XRT_INPUT_TYPE_VEC2_MINUS_ONE_TO_ONE: {
			float res_sq = res.value.vec2.x * res.value.vec2.x + res.value.vec2.y * res.value.vec2.y;
			float trans_sq = transformed.value.vec2.x * transformed.value.vec2.x +
			                 transformed.value.vec2.y * transformed.value.vec2.y;
			if (trans_sq > res_sq) {
				res.value.vec2 = transformed.value.vec2;
				res_timestamp = input->timestamp;
			}
		} break;
		case XRT_INPUT_TYPE_VEC3_MINUS_ONE_TO_ONE:
			// OpenXR has no vec3 right now.
			break;
		case XRT_INPUT_TYPE_POSE:
			// shouldn't be possible to get here
			break;
		case XRT_INPUT_TYPE_HAND_TRACKING:
			// shouldn't be possible to get here
			break;
		case XRT_INPUT_TYPE_FACE_TRACKING:
			// shouldn't be possible to get here
			break;
		case XRT_INPUT_TYPE_BODY_TRACKING:
			// shouldn't be possible to get here
			break;
		}
	}

	*out_is_active = any_active;
	*out_input = res;
	*out_timestamp = res_timestamp;

	return true;
}

/*!
 * Called during xrSyncActions.
 *
 * @private @memberof oxr_action_cache
 */
static void
oxr_action_cache_update(struct oxr_logger *log,
                        struct oxr_session *sess,
                        uint32_t countActionSets,
                        const XrActiveActionSet *actionSets,
                        struct oxr_action_attachment *act_attached,
                        struct oxr_action_cache *cache,
                        int64_t time,
                        struct oxr_subaction_paths *subaction_path,
                        bool selected)
{
	struct oxr_action_state last = cache->current;

	if (!selected) {
		if (cache->stop_output_time > 0) {
			oxr_action_cache_stop_output(log, sess, cache);
		}
		U_ZERO(&cache->current);
		return;
	}

	struct oxr_input_value_tagged combined;
	int64_t timestamp = time;

	/* a cache can only have outputs or inputs, not both */
	if (cache->output_count > 0) {
		cache->current.active = true;
		if (cache->stop_output_time > 0 && cache->stop_output_time < time) {
			oxr_action_cache_stop_output(log, sess, cache);
		}
	} else if (cache->input_count > 0) {

		bool is_active = false;
		bool bret = oxr_input_combine_input( //
		    sess,                            // sess
		    countActionSets,                 // countActionSets
		    actionSets,                      // actionSets
		    act_attached,                    // act_attached
		    subaction_path,                  // subaction_path
		    cache,                           // cache
		    &combined,                       // out_input
		    &timestamp,                      // out_timestamp
		    &is_active);                     // out_is_active
		if (!bret) {
			oxr_log(log, "Failed to get/combine input values '%s'", act_attached->act_ref->name);
			return;
		}

		bool is_focused = sess->state == XR_SESSION_STATE_FOCUSED;

		// If the input is not active signal or the session state is not in focused that.
		if (!is_focused || !is_active) {
			// Reset all state.
			U_ZERO(&cache->current);
			return;
		}

		// Signal that the input is active, always set just to be sure.
		cache->current.active = true;

		bool changed = false;
		switch (combined.type) {
		case XRT_INPUT_TYPE_VEC1_ZERO_TO_ONE:
		case XRT_INPUT_TYPE_VEC1_MINUS_ONE_TO_ONE: {
			changed = (combined.value.vec1.x != last.value.vec1.x);
			cache->current.value.vec1.x = combined.value.vec1.x;
			break;
		}
		case XRT_INPUT_TYPE_VEC2_MINUS_ONE_TO_ONE: {
			changed = (combined.value.vec2.x != last.value.vec2.x) ||
			          (combined.value.vec2.y != last.value.vec2.y);
			cache->current.value.vec2.x = combined.value.vec2.x;
			cache->current.value.vec2.y = combined.value.vec2.y;
			break;
		}
#if 0
		// Untested, we have no VEC3 input.
		case XRT_INPUT_TYPE_VEC3_MINUS_ONE_TO_ONE: {
			changed = (combined.value.vec3.x != last.vec3.x) ||
			          (combined.value.vec3.y != last.vec3.y) ||
			          (combined.value.vec3.z != last.vec3.z);
			cache->current.vec3.x = combined.value.vec3.x;
			cache->current.vec3.y = combined.value.vec3.y;
			cache->current.vec3.z = combined.value.vec3.z;
			break;
		}
#endif
		case XRT_INPUT_TYPE_BOOLEAN: {
			changed = (combined.value.boolean != last.value.boolean);
			cache->current.value.boolean = combined.value.boolean;
			break;
		}
		case XRT_INPUT_TYPE_POSE: return;
		default:
			// Should not end up here.
			assert(false);
		}

		if (last.active && changed) {
			// We were active last sync, and we've changed since
			// then
			cache->current.timestamp = timestamp;
			cache->current.changed = true;
		} else if (last.active) {
			// We were active last sync, but we haven't changed
			// since then.
			cache->current.timestamp = last.timestamp;
			cache->current.changed = false;
		} else {
			// We are active now but weren't active last time.
			cache->current.timestamp = timestamp;
			cache->current.changed = false;
		}
	}
}

static inline bool
oxr_state_equal_bool(const struct oxr_action_state *a, const struct oxr_action_state *b)
{
	return a->value.boolean == b->value.boolean;
}

static inline bool
oxr_state_equal_vec1(const struct oxr_action_state *a, const struct oxr_action_state *b)
{
	return a->value.vec1.x == b->value.vec1.x;
}

static inline bool
oxr_state_equal_vec2(const struct oxr_action_state *a, const struct oxr_action_state *b)
{
	return (a->value.vec2.x == b->value.vec2.x) && (a->value.vec2.y == b->value.vec2.y);
}

static inline void
oxr_state_update_bool(bool *active, bool *value, XrTime *timestamp, const struct oxr_action_state *new_state)
{
	if (new_state->active) {
		*active |= true;
		*value |= new_state->value.boolean;
		*timestamp = new_state->timestamp;
	}
}
#define BOOL_CHECK(NAME) oxr_state_update_bool(&active, &value, &timestamp, &act_attached->NAME.current);

static inline void
oxr_state_update_vec1(bool *active, float *value, XrTime *timestamp, const struct oxr_action_state *new_state)
{
	if (new_state->active) {
		*active |= true;
		if (*value < new_state->value.vec1.x) {
			*value = new_state->value.vec1.x;
			*timestamp = new_state->timestamp;
		}
	}
}
#define VEC1_CHECK(NAME) oxr_state_update_vec1(&active, &value, &timestamp, &act_attached->NAME.current);

static inline void
oxr_state_update_vec2(
    bool *active, float *x, float *y, float *distance, XrTime *timestamp, const struct oxr_action_state *new_state)
{
	if (new_state->active) {
		*active |= true;
		float curr_x = new_state->value.vec2.x;
		float curr_y = new_state->value.vec2.y;
		float curr_d = curr_x * curr_x + curr_y * curr_y;
		if (*distance < curr_d) {
			*x = curr_x;
			*y = curr_y;
			*distance = curr_d;
			*timestamp = new_state->timestamp;
		}
	}
}
#define VEC2_CHECK(NAME) oxr_state_update_vec2(&active, &x, &y, &distance, &timestamp, &act_attached->NAME.current);

/*!
 * Called during each xrSyncActions.
 *
 * @private @memberof oxr_action_attachment
 */
static void
oxr_action_attachment_update(struct oxr_logger *log,
                             struct oxr_session *sess,
                             uint32_t countActionSets,
                             const XrActiveActionSet *actionSets,
                             struct oxr_action_attachment *act_attached,
                             int64_t time,
                             struct oxr_subaction_paths subaction_paths)
{
	// This really shouldn't be happening.
	if (act_attached == NULL) {
		return;
	}

#define UPDATE_SELECT(X)                                                                                               \
	struct oxr_subaction_paths subaction_paths_##X = {0};                                                          \
	subaction_paths_##X.X = true;                                                                                  \
	bool select_##X = subaction_paths.X || subaction_paths.any;                                                    \
	oxr_action_cache_update(log, sess, countActionSets, actionSets, act_attached, &act_attached->X, time,          \
	                        &subaction_paths_##X, select_##X);

	OXR_FOR_EACH_VALID_SUBACTION_PATH(UPDATE_SELECT)
#undef UPDATE_SELECT

	/*
	 * Any state.
	 */
	struct oxr_action_state last = act_attached->any_state;
	bool active = false;
	bool changed = false;
	XrTime timestamp = time_state_monotonic_to_ts_ns(sess->sys->inst->timekeeping, time);
	U_ZERO(&act_attached->any_state);

	switch (act_attached->act_ref->action_type) {
	case XR_ACTION_TYPE_BOOLEAN_INPUT: {
		bool value = false;
		OXR_FOR_EACH_VALID_SUBACTION_PATH(BOOL_CHECK)

		act_attached->any_state.value.boolean = value;
		changed = active && !oxr_state_equal_bool(&last, &act_attached->any_state);
		break;
	}
	case XR_ACTION_TYPE_FLOAT_INPUT: {
		// Smaller than any possible real value
		float value = -2.0f; // NOLINT
		OXR_FOR_EACH_VALID_SUBACTION_PATH(VEC1_CHECK)

		act_attached->any_state.value.vec1.x = value;
		changed = active && !oxr_state_equal_vec1(&last, &act_attached->any_state);
		break;
	}
	case XR_ACTION_TYPE_VECTOR2F_INPUT: {
		float x = 0.0f;
		float y = 0.0f;
		float distance = -1.0f;
		OXR_FOR_EACH_VALID_SUBACTION_PATH(VEC2_CHECK)

		act_attached->any_state.value.vec2.x = x;
		act_attached->any_state.value.vec2.y = y;
		changed = active && !oxr_state_equal_vec2(&last, &act_attached->any_state);
		break;
	}
	default:
	case XR_ACTION_TYPE_POSE_INPUT:
	case XR_ACTION_TYPE_VIBRATION_OUTPUT:
		// Nothing to do
		//! @todo You sure?
		return;
	}

	act_attached->any_state.active = active;
	// We're only changed if the value differs and we're not newly
	// active.
	act_attached->any_state.changed = last.active && changed;
	if (active) {
		act_attached->any_state.timestamp = timestamp;
		if (!act_attached->any_state.changed && last.active) {
			// Use old timestamp if we're unchanged, but were active
			// last sync
			act_attached->any_state.timestamp = last.timestamp;
		}
	}
}

static void
oxr_action_bind_io(struct oxr_logger *log,
                   struct oxr_sink_logger *slog,
                   const struct oxr_session_action_context *sess_context,
                   const struct oxr_profile_resolver_info *info,
                   const struct xrt_io_cache_device *dev,
                   XrPath subaction_path,
                   struct oxr_action_cache *cache,
                   struct oxr_action_attachment *act_attached)
{
	// The resolver functions assumes that the cache is empty.
	oxr_action_cache_teardown(cache);

	if (dev == NULL) {
		oxr_slog(slog, "\t\tNo device!\n");
		return;
	}

	if (act_attached->act_ref->action_type == XR_ACTION_TYPE_VIBRATION_OUTPUT) {
		oxr_resolver_populate_output( //
		    log,                      //
		    slog,                     //
		    &sess_context->io_cache,  //
		    info,                     //
		    dev,                      //
		    subaction_path,           //
		    act_attached,             //
		    cache);                   //
		return;
	}

	oxr_resolver_populate_input( //
	    log,                     //
	    slog,                    //
	    &sess_context->io_cache, //
	    info,                    //
	    dev,                     //
	    subaction_path,          //
	    act_attached,            //
	    cache);                  //
}


/*
 *
 * Session functions.
 *
 */

XrResult
oxr_session_attach_action_sets(struct oxr_logger *log,
                               const struct oxr_instance_action_context *inst_context,
                               struct oxr_session_attached_actions *attached_actions,
                               struct oxr_session_action_context *sess_context,
                               const XrSessionActionSetsAttachInfo *bindInfo)
{
	XrResult ret;

	size_t count = bindInfo->countActionSets;

	sess_context->action_set_attachment_count = count;
	sess_context->act_set_attachments = U_TYPED_ARRAY_CALLOC(struct oxr_action_set_attachment, count);
	if (sess_context->act_set_attachments == NULL) {
		// Actual clean-up.
		sess_context->action_set_attachment_count = 0;
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to allocate action set attachments");
	}

	for (size_t i = 0; i < count; i++) {
		struct oxr_action_set *act_set =
		    XRT_CAST_OXR_HANDLE_TO_PTR(struct oxr_action_set *, bindInfo->actionSets[i]);
		assert(act_set != NULL);

		struct oxr_action_set_ref *act_set_ref = act_set->data;
		act_set_ref->ever_attached = true;

		struct oxr_action_set_attachment *act_set_attached = &sess_context->act_set_attachments[i];
		ret = oxr_action_set_attachment_init( //
		    log,                              //
		    act_set_attached,                 //
		    sess_context,                     //
		    act_set->data);                   //
		if (ret != XR_SUCCESS) {
			// @todo: unwind the loop and free the attachments.
			assert(false);
			return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "oxr_action_set_attachment_init failed");
		}

		// Allocate the action attachments for this set.
		act_set_attached->action_attachment_count = act_set->handle.children.count;
		act_set_attached->act_attachments =
		    U_TYPED_ARRAY_CALLOC(struct oxr_action_attachment, act_set_attached->action_attachment_count);
		if (act_set_attached->act_attachments == NULL) {
			// @todo: unwind the loop and free the attachments.
			assert(false);
			return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to allocate action attachments");
		}

		// Set up the per-session data for the actions.
		uint32_t child_index = 0;
		for (uint32_t k = 0; k < act_set->handle.children.count; k++) {
			struct oxr_action *act = (struct oxr_action *)act_set->handle.children.handles[k];
			if (act == NULL) {
				continue;
			}

			struct oxr_action_attachment *act_attached = &act_set_attached->act_attachments[child_index];
			ret = oxr_action_attachment_init( //
			    log,                          //
			    act_attached,                 //
			    attached_actions,             //
			    act_set_attached,             //
			    act->data);                   //
			if (ret != XR_SUCCESS) {
				// @todo: unwind the loop and free the attachments.
				assert(false);
				return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "oxr_action_attachment_init failed");
			}

			++child_index;
		}
	}

	ret = oxr_suggested_profile_array_clone(    //
	    &inst_context->suggested_profiles,      // src
	    &sess_context->profiles_on_attachment); // dst
	if (ret != XR_SUCCESS) {
		return oxr_error(log, ret, "Failed to clone suggested profiles");
	}

	sess_context->binding_registry = inst_context->binding_registry;
	oxr_binding_registry_ref(sess_context->binding_registry);

	/*
	 * We used to send XrEventDataInteractionProfileChanged here, but that's
	 * wrong. The OpenXR spec says we should only send them after a
	 * successful call to xrSyncActionData.
	 */

	return XR_SUCCESS;
}

/*!
 * Tear down the resolved input/output bindings of every attached action.
 *
 * Used before recreating the io cache: the bindings hold pointers into the
 * cache's snapshot arrays, so they must be dropped before those arrays go away.
 */
static void
session_clear_action_bindings(struct oxr_session_action_context *action_context)
{
	for (size_t i = 0; i < action_context->action_set_attachment_count; i++) {
		struct oxr_action_set_attachment *act_set_attached = &action_context->act_set_attachments[i];
		for (size_t k = 0; k < act_set_attached->action_attachment_count; k++) {
			struct oxr_action_attachment *act_attached = &act_set_attached->act_attachments[k];
#define CACHE_TEARDOWN(X) oxr_action_cache_teardown(&(act_attached->X));
			OXR_FOR_EACH_SUBACTION_PATH(CACHE_TEARDOWN)
#undef CACHE_TEARDOWN
		}
	}
}

static XrResult
session_update_action_bindings(struct oxr_logger *log,
                               struct oxr_instance *inst,
                               struct oxr_session_action_context *action_context,
                               const struct oxr_roles *roles,
                               bool *out_interaction_profile_changed)
{
	// Convenience.
	const struct oxr_suggested_profile_array *array = &action_context->profiles_on_attachment;

	struct oxr_profiles_per_subaction profiles = {0};
	resolve_binding_infos_from_roles( //
	    log,                          //
	    action_context,               //
	    array,                        //
	    roles,                        //
	    inst->debug_bindings,         //
	    &profiles);                   //

	oxr_priority_group_cache_clear(&action_context->priority_group_cache);

	for (size_t i = 0; i < action_context->action_set_attachment_count; i++) {
		struct oxr_action_set_attachment *act_set_attached = &action_context->act_set_attachments[i];
		for (size_t k = 0; k < act_set_attached->action_attachment_count; k++) {
			struct oxr_action_attachment *act_attached = &act_set_attached->act_attachments[k];
			oxr_action_attachment_bind(log, inst, act_attached, &profiles);
		}
	}

	bool interaction_profile_changed = false;

	/*
	 * This code will only send events (and update the bindings) if there
	 * is a profile mapped to the subaction path. Meaning it won't update
	 * the cache or generate events when a controller goes away, basically
	 * latching the interaction profile to the last active device.
	 */
#define POPULATE_PROFILE(X)                                                                                            \
	do {                                                                                                           \
		XrPath path = profiles.X.profile != NULL ? profiles.X.profile->path : XR_NULL_PATH;                    \
		if (path == XR_NULL_PATH) {                                                                            \
			break; /* Only update on "active" interaction profiles per sub-action path. */                 \
		}                                                                                                      \
		if (action_context->X != path) {                                                                       \
			action_context->X = path;                                                                      \
			interaction_profile_changed = true;                                                            \
		}                                                                                                      \
	} while (false);
	OXR_FOR_EACH_VALID_SUBACTION_PATH(POPULATE_PROFILE)
#undef POPULATE_PROFILE

	*out_interaction_profile_changed = interaction_profile_changed;

	return XR_SUCCESS;
}

static XrResult
session_sync_io_cache_locked(struct oxr_logger *log,
                             struct oxr_instance *inst,
                             struct oxr_session_action_context *sess_context,
                             const struct oxr_roles *roles,
                             bool *out_interaction_profile_changed)
{
	bool io_cache_stale = false;
	XrResult sync_ret = oxr_io_cache_sync(log, &sess_context->io_cache, &io_cache_stale);
	if (sync_ret != XR_SUCCESS || !io_cache_stale) {
		return sync_ret;
	}

	/*
	 * The set of devices changed (hotplug): the cache layout is out
	 * of date and the action bindings still point into the old
	 * snapshot arrays. Clear the bindings, recreate the cache for
	 * the new layout, rebind so inputs point into the new arrays,
	 * then read again.
	 */
	session_clear_action_bindings(sess_context);
	sync_ret = oxr_io_cache_recreate(log, &sess_context->io_cache);
	if (sync_ret != XR_SUCCESS) {
		return sync_ret;
	}

	sync_ret = session_update_action_bindings( //
	    log,                                   //
	    inst,                                  //
	    sess_context,                          //
	    roles,                                 //
	    out_interaction_profile_changed);      //
	if (sync_ret != XR_SUCCESS) {
		return sync_ret;
	}

	// If the layout changed yet again, the next xrSyncActions recreates it.
	return oxr_io_cache_sync(log, &sess_context->io_cache, &io_cache_stale);
}

XrResult
oxr_action_sync_data(struct oxr_logger *log,
                     struct oxr_session *sess,
                     uint32_t countActionSets,
                     const XrActiveActionSet *actionSets,
                     const XrActiveActionSetPrioritiesEXT *activePriorities)
{
	bool interaction_profile_changed = false;
	XrResult ret = oxr_action_sync_data_with_context( //
	    log,                                          //
	    sess,                                         //
	    &sess->action_context,                        //
	    countActionSets,                              //
	    actionSets,                                   //
	    activePriorities,                             //
	    &interaction_profile_changed);                //
	if (ret != XR_SUCCESS) {
		return ret;
	}

	if (interaction_profile_changed) {
		oxr_event_push_XrEventDataInteractionProfileChanged(log, sess);
	}

	return oxr_session_success_focused_result(sess);
}

XrResult
oxr_action_sync_data_with_context(struct oxr_logger *log,
                                  struct oxr_session *sess,
                                  struct oxr_session_action_context *sess_context,
                                  uint32_t countActionSets,
                                  const XrActiveActionSet *actionSets,
                                  const XrActiveActionSetPrioritiesEXT *activePriorities,
                                  bool *out_interaction_profile_changed)
{
	struct oxr_instance *inst = sess->sys->inst;

	struct oxr_action_set *act_set = NULL;
	struct oxr_action_set_attachment *act_set_attached = NULL;

	// Reset.
	*out_interaction_profile_changed = false;

	/*
	 * No side-effects allowed in this section as we are still
	 * validating and checking for errors at this point.
	 */

	// Check that all action sets has been attached.
	for (uint32_t i = 0; i < countActionSets; i++) {
		get_action_set_attachment(sess_context, actionSets[i].actionSet, &act_set_attached, &act_set);
		if (act_set_attached == NULL) {
			return oxr_error(log, XR_ERROR_ACTIONSET_NOT_ATTACHED,
			                 "(actionSets[%i].actionSet) action set '%s' has "
			                 "not been attached to this session",
			                 i, act_set != NULL ? act_set->data->name : "NULL");
		}
	}

	/*
	 * Can only call this function if the session state is focused. This is
	 * not an error and has to be checked after all validation, but before
	 * any side-effects happens.
	 */
	if (sess->state != XR_SESSION_STATE_FOCUSED) {
		return XR_SESSION_NOT_FOCUSED;
	}

	/*
	 * Side-effects allowed below, but not validation.
	 */

	struct oxr_roles roles = XRT_STRUCT_INIT;
	XrResult result = oxr_roles_init_on_stack(log, &roles, sess->sys);
	if (result != XR_SUCCESS) {
		// Only returns XR_ERROR_RUNTIME_FAILURE
		return result;
	}

	// Should we redo the bindings?
	{
		os_mutex_lock(&sess_context->sync_actions_mutex);
		if (sess_context->dynamic_roles_generation_id < roles.roles.generation_id) {
			sess_context->dynamic_roles_generation_id = roles.roles.generation_id;
			XrResult update_ret = session_update_action_bindings( //
			    log,                                              //
			    inst,                                             //
			    sess_context,                                     //
			    &roles,                                           //
			    out_interaction_profile_changed);                 //
			if (update_ret != XR_SUCCESS) {
				os_mutex_unlock(&sess_context->sync_actions_mutex);
				return update_ret;
			}
		}
		os_mutex_unlock(&sess_context->sync_actions_mutex);
	}

	if (countActionSets == 0) {
		reset_inactive_sync_priority_state(sess_context);
		return XR_SUCCESS;
	}

	// Synchronize outputs to this time.
	int64_t now = time_state_get_now(sess->sys->inst->timekeeping);

	/*
	 * Refresh this action context's input/output cache under the same lock
	 * used for binding updates, so stale layout handling cannot race them.
	 */
	XrResult sync_ret;
	os_mutex_lock(&sess_context->sync_actions_mutex);
	sync_ret = session_sync_io_cache_locked(log, inst, sess_context, &roles, out_interaction_profile_changed);
	os_mutex_unlock(&sess_context->sync_actions_mutex);
	if (sync_ret != XR_SUCCESS) {
		return sync_ret;
	}

	// Reset all action set attachments.
	for (size_t i = 0; i < sess_context->action_set_attachment_count; ++i) {
		act_set_attached = &sess_context->act_set_attachments[i];
		U_ZERO(&act_set_attached->requested_subaction_paths);
	}

	// Go over all requested action sets and update their
	// attachment.
	//! @todo can be listed more than once with different paths!
	for (uint32_t i = 0; i < countActionSets; i++) {
		struct oxr_subaction_paths subaction_paths;
		get_action_set_attachment(sess_context, actionSets[i].actionSet, &act_set_attached, &act_set);
		assert(act_set_attached != NULL);

		if (!oxr_classify_subaction_path(&inst->path_cache, actionSets[i].subactionPath, &subaction_paths)) {
			//! @todo Probably an internal runtime error since it should be validated before?
			return oxr_error(log, XR_ERROR_PATH_UNSUPPORTED, "Bad internal codepath taken (bug?)");
		}

		/* never error when requesting any subactionpath */
		bool any_action_with_subactionpath = subaction_paths.any;

		oxr_subaction_paths_accumulate(&(act_set_attached->requested_subaction_paths), &subaction_paths);

		/* check if we have at least one action for requested subactionpath */
		for (uint32_t k = 0; k < act_set_attached->action_attachment_count; k++) {
			struct oxr_action_attachment *act_attached = &act_set_attached->act_attachments[k];

			if (act_attached == NULL) {
				continue;
			}

#define ACCUMULATE_REQUESTED(X)                                                                                        \
	any_action_with_subactionpath |= subaction_paths.X && act_attached->act_ref->subaction_paths.X;
			OXR_FOR_EACH_SUBACTION_PATH(ACCUMULATE_REQUESTED)
#undef ACCUMULATE_REQUESTED
		}

		//! @TODO This validation is not allowed here, must done above.
		if (!any_action_with_subactionpath) {
			return oxr_error(log, XR_ERROR_PATH_UNSUPPORTED,
			                 "No action with specified subactionpath in actionset");
		}
	}

	oxr_session_rebuild_priority_group_states(sess_context, countActionSets, actionSets, activePriorities);

	// Now, update all action attachments
	for (size_t i = 0; i < sess_context->action_set_attachment_count; ++i) {
		act_set_attached = &sess_context->act_set_attachments[i];
		struct oxr_subaction_paths subaction_paths = act_set_attached->requested_subaction_paths;

		for (uint32_t k = 0; k < act_set_attached->action_attachment_count; k++) {
			struct oxr_action_attachment *act_attached = &act_set_attached->act_attachments[k];

			if (act_attached == NULL) {
				continue;
			}

			oxr_action_attachment_update(log, sess, countActionSets, actionSets, act_attached, now,
			                             subaction_paths);
		}
	}

	return XR_SUCCESS;
}

static void
add_path_to_set(XrPath path_set[OXR_MAX_BINDINGS_PER_ACTION], XrPath new_path, uint32_t *inout_path_count)
{
	const uint32_t n = *inout_path_count;

	// Shouldn't be full
	assert(n < OXR_MAX_BINDINGS_PER_ACTION);

	for (uint32_t i = 0; i < n; ++i) {
		if (new_path == path_set[i]) {
			return;
		}
		// Should have no gaps
		assert(path_set[i] != 0);
	}
	path_set[n] = new_path;
	(*inout_path_count)++;
}

static void
add_cache_bound_sources(const struct oxr_action_attachment *act_attached,
                        const struct oxr_action_cache *cache,
                        XrPath path_set[OXR_MAX_BINDINGS_PER_ACTION],
                        uint32_t *inout_path_count)
{
	for (uint32_t i = 0; i < cache->input_count; i++) {
		const struct oxr_action_input *action_input = &cache->inputs[i];
		if (input_suppressed(act_attached, action_input)) {
			continue;
		}
		add_path_to_set(path_set, action_input->binding_point->source_path, inout_path_count);
	}

	for (uint32_t i = 0; i < cache->output_count; i++) {
		add_path_to_set(path_set, cache->outputs[i].binding_point->source_path, inout_path_count);
	}
}

XrResult
oxr_action_enumerate_bound_sources(struct oxr_logger *log,
                                   struct oxr_session *sess,
                                   uint32_t act_key,
                                   uint32_t sourceCapacityInput,
                                   uint32_t *sourceCountOutput,
                                   XrPath *sources)
{
	struct oxr_action_attachment *act_attached = NULL;
	uint32_t path_count = 0;
	XrPath temp[OXR_MAX_BINDINGS_PER_ACTION] = {0};

	oxr_session_attached_actions_find(&sess->attached_actions, act_key, &act_attached);
	if (act_attached == NULL) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "act_key did not find any action");
	}

	/*
	 * Only enumerate sources for subaction paths that were active in the
	 * last sync. Priority-suppressed inputs are omitted; inactive paths
	 * contribute nothing.
	 */
#define ACCUMULATE_PATHS(X)                                                                                            \
	do {                                                                                                           \
		const struct oxr_action_set_attachment *act_set_attached = act_attached->act_set_attached;             \
		const bool subaction_path_active =                                                                     \
		    act_set_attached->requested_subaction_paths.any || act_set_attached->requested_subaction_paths.X;  \
		if (subaction_path_active) {                                                                           \
			add_cache_bound_sources(act_attached, &act_attached->X, temp, &path_count);                    \
		}                                                                                                      \
	} while (false);

	OXR_FOR_EACH_SUBACTION_PATH(ACCUMULATE_PATHS)
#undef ACCUMULATE_PATHS

	OXR_TWO_CALL_HELPER(log, sourceCapacityInput, sourceCountOutput, sources, path_count, temp,
	                    oxr_session_success_result(sess));
}
