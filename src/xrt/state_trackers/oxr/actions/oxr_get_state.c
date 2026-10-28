// Copyright 2018-2024, Collabora, Ltd.
// Copyright 2023-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Holds action state get related functions.
 * @ingroup oxr_main
 */

#include "oxr_get_state.h"
#include "oxr_subaction.h"
#include "oxr_input.h"

#include "../oxr_objects.h"
#include "../oxr_logger.h"


#define OXR_ACTION_GET_XR_STATE_FROM_ACTION_STATE_COMMON(ACTION_STATE, DATA)                                           \
	do {                                                                                                           \
		DATA->lastChangeTime = time_state_monotonic_to_ts_ns(inst->timekeeping, ACTION_STATE->timestamp);      \
		DATA->changedSinceLastSync = ACTION_STATE->changed;                                                    \
		DATA->isActive = XR_TRUE;                                                                              \
	} while (0)

static void
get_xr_state_from_action_state_bool(struct oxr_instance *inst,
                                    struct oxr_action_state *state,
                                    XrActionStateBoolean *data)
{
	/* only get here if the action is active! */
	assert(state->active);
	OXR_ACTION_GET_XR_STATE_FROM_ACTION_STATE_COMMON(state, data);
	data->currentState = state->value.boolean;
}

static void
get_xr_state_from_action_state_vec1(struct oxr_instance *inst, struct oxr_action_state *state, XrActionStateFloat *data)
{
	/* only get here if the action is active! */
	assert(state->active);
	OXR_ACTION_GET_XR_STATE_FROM_ACTION_STATE_COMMON(state, data);
	data->currentState = state->value.vec1.x;
}

static void
get_xr_state_from_action_state_vec2(struct oxr_instance *inst,
                                    struct oxr_action_state *state,
                                    XrActionStateVector2f *data)
{
	/* only get here if the action is active! */
	assert(state->active);
	OXR_ACTION_GET_XR_STATE_FROM_ACTION_STATE_COMMON(state, data);
	data->currentState.x = state->value.vec2.x;
	data->currentState.y = state->value.vec2.y;
}

/*!
 * This populates the internals of action get state functions.
 *
 * @note Keep this synchronized with OXR_FOR_EACH_SUBACTION_PATH!
 */
#define OXR_ACTION_GET_FILLER(TYPE)                                                                                    \
	if (subaction_paths.any && act_attached->any_state.active) {                                                   \
		get_xr_state_from_action_state_##TYPE(sess->sys->inst, &act_attached->any_state, data);                \
	}                                                                                                              \
	if (subaction_paths.user && act_attached->user.current.active) {                                               \
		get_xr_state_from_action_state_##TYPE(sess->sys->inst, &act_attached->user.current, data);             \
	}                                                                                                              \
	if (subaction_paths.head && act_attached->head.current.active) {                                               \
		get_xr_state_from_action_state_##TYPE(sess->sys->inst, &act_attached->head.current, data);             \
	}                                                                                                              \
	if (subaction_paths.left && act_attached->left.current.active) {                                               \
		get_xr_state_from_action_state_##TYPE(sess->sys->inst, &act_attached->left.current, data);             \
	}                                                                                                              \
	if (subaction_paths.right && act_attached->right.current.active) {                                             \
		get_xr_state_from_action_state_##TYPE(sess->sys->inst, &act_attached->right.current, data);            \
	}                                                                                                              \
	if (subaction_paths.gamepad && act_attached->gamepad.current.active) {                                         \
		get_xr_state_from_action_state_##TYPE(sess->sys->inst, &act_attached->gamepad.current, data);          \
	}

/*!
 * Clear the actual data members of the XrActionState* types, to have the
 * correct return value in case of the action being not active
 */
#define OXR_ACTION_RESET_XR_ACTION_STATE(data)                                                                         \
	do {                                                                                                           \
		data->isActive = XR_FALSE;                                                                             \
		data->changedSinceLastSync = XR_FALSE;                                                                 \
		data->lastChangeTime = 0;                                                                              \
		U_ZERO(&data->currentState);                                                                           \
	} while (0)

XrResult
oxr_action_get_boolean(struct oxr_logger *log,
                       struct oxr_session *sess,
                       uint32_t act_key,
                       struct oxr_subaction_paths subaction_paths,
                       XrActionStateBoolean *data)
{
	struct oxr_action_attachment *act_attached = NULL;

	oxr_session_attached_actions_find(&sess->attached_actions, act_key, &act_attached);
	if (act_attached == NULL) {
		return oxr_error(log, XR_ERROR_ACTIONSET_NOT_ATTACHED, "Action has not been attached to this session");
	}

	OXR_ACTION_RESET_XR_ACTION_STATE(data);

	OXR_ACTION_GET_FILLER(bool);

	return oxr_session_success_result(sess);
}

XrResult
oxr_action_get_vector1f(struct oxr_logger *log,
                        struct oxr_session *sess,
                        uint32_t act_key,
                        struct oxr_subaction_paths subaction_paths,
                        XrActionStateFloat *data)
{
	struct oxr_action_attachment *act_attached = NULL;

	oxr_session_attached_actions_find(&sess->attached_actions, act_key, &act_attached);
	if (act_attached == NULL) {
		return oxr_error(log, XR_ERROR_ACTIONSET_NOT_ATTACHED, "Action has not been attached to this session");
	}

	OXR_ACTION_RESET_XR_ACTION_STATE(data);

	OXR_ACTION_GET_FILLER(vec1);

	return oxr_session_success_result(sess);
}

XrResult
oxr_action_get_vector2f(struct oxr_logger *log,
                        struct oxr_session *sess,
                        uint32_t act_key,
                        struct oxr_subaction_paths subaction_paths,
                        XrActionStateVector2f *data)
{
	struct oxr_action_attachment *act_attached = NULL;

	oxr_session_attached_actions_find(&sess->attached_actions, act_key, &act_attached);
	if (act_attached == NULL) {
		return oxr_error(log, XR_ERROR_ACTIONSET_NOT_ATTACHED, "Action has not been attached to this session");
	}

	OXR_ACTION_RESET_XR_ACTION_STATE(data);

	OXR_ACTION_GET_FILLER(vec2);

	return oxr_session_success_result(sess);
}

XrResult
oxr_action_get_pose(struct oxr_logger *log,
                    struct oxr_session *sess,
                    uint32_t act_key,
                    struct oxr_subaction_paths subaction_paths,
                    XrActionStatePose *data)
{
	struct oxr_action_attachment *act_attached = NULL;

	oxr_session_attached_actions_find(&sess->attached_actions, act_key, &act_attached);
	if (act_attached == NULL) {
		return oxr_error(log, XR_ERROR_ACTIONSET_NOT_ATTACHED, "Action has not been attached to this session");
	}

	// For poses on the any path we select a single path.
	if (subaction_paths.any) {
		subaction_paths = act_attached->any_pose_subaction_path;
	}

	data->isActive = XR_FALSE;

	/*
	 * The sub path any is used as a catch all here to see if any
	 */
#define COMPUTE_ACTIVE(X)                                                                                              \
	if (subaction_paths.X) {                                                                                       \
		data->isActive |= act_attached->X.current.active;                                                      \
	}

	OXR_FOR_EACH_VALID_SUBACTION_PATH(COMPUTE_ACTIVE)
#undef COMPUTE_ACTIVE

	return oxr_session_success_result(sess);
}
