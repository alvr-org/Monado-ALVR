// Copyright 2022, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Holds binding related functions.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup oxr_main
 */

#include "oxr_objects.h"


/*
 *
 * Helper functions.
 *
 */

static void
destroy_callback(void *item, void *priv)
{
	free(item);
}

static void
clone_oxr_dpad_entry(uint64_t key, const void *src_data, void *ctx)
{
	assert(src_data != NULL && ctx != NULL);

	struct oxr_dpad_state *dst_dpad_state = (struct oxr_dpad_state *)ctx;
	const struct oxr_dpad_entry *src_dpad_entry = (const struct oxr_dpad_entry *)src_data;

	struct oxr_dpad_entry *dst_dpad_entry = oxr_dpad_state_get_or_add(dst_dpad_state, key);
	assert(dst_dpad_entry != NULL);

	memcpy(dst_dpad_entry, src_dpad_entry, sizeof(struct oxr_dpad_entry));
}

/*
 *
 * 'Exported' functions.
 *
 */

bool
oxr_dpad_state_init(struct oxr_dpad_state *state)
{
	if (u_hashmap_int_create(&state->uhi) < 0) {
		return false;
	}

	return true;
}

struct oxr_dpad_entry *
oxr_dpad_state_get(struct oxr_dpad_state *state, uint64_t key)
{
	void *ptr = NULL;
	u_hashmap_int_find(state->uhi, key, &ptr);
	return (struct oxr_dpad_entry *)ptr;
}

struct oxr_dpad_entry *
oxr_dpad_state_get_or_add(struct oxr_dpad_state *state, uint64_t key)
{
	struct oxr_dpad_entry *e = oxr_dpad_state_get(state, key);
	if (e == NULL) {
		e = U_TYPED_CALLOC(struct oxr_dpad_entry);
		XRT_MAYBE_UNUSED int ret = u_hashmap_int_insert(state->uhi, key, (void *)e);
		assert(ret >= 0);
	}

	return e;
}

void
oxr_dpad_state_deinit(struct oxr_dpad_state *state)
{
	if (state != NULL && state->uhi != NULL) {
		u_hashmap_int_clear_and_call_for_each(state->uhi, destroy_callback, NULL);
		u_hashmap_int_destroy(&state->uhi);
	}
}

bool
oxr_dpad_state_clone(struct oxr_dpad_state *dst_dpad_state, const struct oxr_dpad_state *src_dpad_state)
{
	if (dst_dpad_state == NULL || src_dpad_state == NULL) {
		return false;
	}

	oxr_dpad_state_deinit(dst_dpad_state);
	assert(dst_dpad_state->uhi == NULL);

	if (!oxr_dpad_state_init(dst_dpad_state))
		return false;

	u_hashmap_int_for_each(src_dpad_state->uhi, clone_oxr_dpad_entry, dst_dpad_state);

	return true;
}
