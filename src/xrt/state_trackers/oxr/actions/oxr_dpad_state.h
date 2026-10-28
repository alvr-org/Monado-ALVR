// Copyright 2022, Collabora, Ltd.
// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Holds dpad state related functions.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup oxr_main
 */

#pragma once

#include "oxr_defines.h"
#include "oxr_extension_support.h"


#ifdef __cplusplus
extern "C" {
#endif


/*
 *
 * Structs
 *
 */

/*!
 * dpad settings we need extracted from XrInteractionProfileDpadBindingEXT
 *
 * @ingroup oxr_input
 */
struct oxr_dpad_settings
{
	float forceThreshold;
	float forceThresholdReleased;
	float centerRegion;
	float wedgeAngle;
	bool isSticky;
};

/*!
 * dpad binding extracted from XrInteractionProfileDpadBindingEXT
 */
struct oxr_dpad_binding_modification
{
	/*!
	 * Binding path from XrInteractionProfileDpadBindingEXT::binding.
	 *
	 * This is set while processing xrSuggestInteractionProfileBindings, when
	 * oxr_api_action.c copies each dpad binding modification into the
	 * interaction profile's dpad state. It names the DPAD emulator path that
	 * the modification applies to, for example
	 * `/user/hand/left/input/thumbstick`.
	 */
	XrPath binding;

	struct oxr_dpad_settings settings;
};

/*!
 * A entry in the dpad state for one action set.
 *
 * @ingroup oxr_input
 */
struct oxr_dpad_entry
{
#ifdef XR_EXT_dpad_binding
	struct oxr_dpad_binding_modification dpads[4];
	uint32_t dpad_count;
#endif

	uint64_t key;
};

/*!
 * Holds dpad binding state for a single interaction profile.
 *
 * @ingroup oxr_input
 */
struct oxr_dpad_state
{
	struct u_hashmap_int *entries;
};


/*
 *
 * Functions
 *
 */

/*!
 * Initialises a dpad state, has to be zero init before a call to this function.
 *
 * @public @memberof oxr_dpad_state
 */
bool
oxr_dpad_state_init(struct oxr_dpad_state *state);

/*!
 * Look for a entry in the state for the given action set key,
 * returns NULL if no entry has been made for that action set.
 *
 * @public @memberof oxr_dpad_state
 */
const struct oxr_dpad_entry *
oxr_dpad_state_get(const struct oxr_dpad_state *state, uint64_t key);

/*!
 * Look for a entry in the state for the given action set key,
 * allocates a new entry if none was found.
 *
 * @public @memberof oxr_dpad_state
 */
struct oxr_dpad_entry *
oxr_dpad_state_get_or_add(struct oxr_dpad_state *state, uint64_t key);

/*!
 * Frees all state and entries attached to this dpad state.
 *
 * @public @memberof oxr_dpad_state
 */
void
oxr_dpad_state_deinit(struct oxr_dpad_state *state);

/*!
 * Clones all oxr_dpad_state
 * @param dst_dpad_state destination of cloning
 * @param src_dpad_state source of cloning
 *
 * @public @memberof oxr_dpad_state
 */
bool
oxr_dpad_state_clone(struct oxr_dpad_state *dst_dpad_state, const struct oxr_dpad_state *src_dpad_state);


#ifdef __cplusplus
}
#endif
