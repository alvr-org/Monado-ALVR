// Copyright 2022-2023, Collabora, Ltd.
// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Tracking-origin setup helpers for @ref xrt_builder implementations.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_util
 */

#include "util/u_builder_helpers.h"

#include "util/u_misc.h"

#include "xrt/xrt_tracking.h"

#include <stdbool.h>

static void
apply_offset(struct xrt_vec3 *position, struct xrt_vec3 *offset)
{
	position->x += offset->x;
	position->y += offset->y;
	position->z += offset->z;
}

void
u_builder_setup_tracking_origins(struct xrt_device *head,
                                 struct xrt_device *eyes,
                                 struct xrt_device *left,
                                 struct xrt_device *right,
                                 struct xrt_device *gamepad,
                                 struct xrt_vec3 *global_tracking_origin_offset)
{
	struct xrt_tracking_origin *head_origin = head ? head->tracking_origin : NULL;
	struct xrt_tracking_origin *eyes_origin = eyes ? eyes->tracking_origin : NULL;
	struct xrt_tracking_origin *left_origin = left ? left->tracking_origin : NULL;
	struct xrt_tracking_origin *right_origin = right ? right->tracking_origin : NULL;
	struct xrt_tracking_origin *gamepad_origin = gamepad ? gamepad->tracking_origin : NULL;

	if (left_origin != NULL && left_origin->type == XRT_TRACKING_TYPE_NONE) {
		left_origin->initial_offset.position.x = -0.2f;
		left_origin->initial_offset.position.y = 1.3f;
		left_origin->initial_offset.position.z = -0.5f;
	}

	if (right_origin != NULL && right_origin->type == XRT_TRACKING_TYPE_NONE) {
		right_origin->initial_offset.position.x = 0.2f;
		right_origin->initial_offset.position.y = 1.3f;
		right_origin->initial_offset.position.z = -0.5f;
	}

	if (gamepad_origin != NULL && gamepad_origin->type == XRT_TRACKING_TYPE_NONE) {
		gamepad_origin->initial_offset.position.x = 0.0f;
		gamepad_origin->initial_offset.position.y = 1.3f;
		gamepad_origin->initial_offset.position.z = -0.5f;
	}

	// Head comes last, because left and right may share tracking origin.
	if (head_origin != NULL && head_origin->type == XRT_TRACKING_TYPE_NONE) {
		// "nominal height" 1.6m
		head_origin->initial_offset.position.x = 0.0f;
		head_origin->initial_offset.position.y = 1.6f;
		head_origin->initial_offset.position.z = 0.0f;
	}

	if (eyes_origin != NULL && eyes_origin->type == XRT_TRACKING_TYPE_NONE) {
		// "nominal height" 1.6m
		eyes_origin->initial_offset.position.x = 0.0f;
		eyes_origin->initial_offset.position.y = 1.6f;
		eyes_origin->initial_offset.position.z = 0.0f;
	}

	struct xrt_tracking_origin *origins[] = {head_origin, eyes_origin, left_origin, right_origin, gamepad_origin};
	bool applied[ARRAY_SIZE(origins)] = {false};

	for (size_t i = 0; i < ARRAY_SIZE(origins); i++) {
		struct xrt_tracking_origin *origin = origins[i];

		if (origin == NULL) {
			continue;
		}

		// Check if we already applied offset to this origin
		bool already_applied = false;
		for (size_t j = 0; j < i; j++) {
			if (origins[j] == origin && applied[j]) {
				already_applied = true;
				break;
			}
		}

		if (!already_applied) {
			apply_offset(&origin->initial_offset.position, global_tracking_origin_offset);
			applied[i] = true;
		}
	}
}
