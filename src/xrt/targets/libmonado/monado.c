// Copyright 2019-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Implementation of libmonado
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Pete Black <pblack@collabora.com>
 * @author Ryan Pavlik <ryan.pavlik@collabora.com>
 */

#include "monado.h"

#include "xrt/xrt_results.h"

#include "util/u_misc.h"
#include "util/u_file.h"
#include "util/u_logging.h"

#include "shared/ipc_protocol.h"

#include "client/ipc_client_connection.h"
#include "client/ipc_client.h"
#include "ipc_client_generated.h"

#include <limits.h>
#include <stdint.h>
#include <assert.h>


struct mnd_root
{
	struct ipc_connection ipc_c;

	//! List of clients.
	struct ipc_client_list clients;

	/// State of most recent app asked about
	struct ipc_app_state app_state;
};

#define P(...) fprintf(stdout, __VA_ARGS__)
#define PE(...) fprintf(stderr, __VA_ARGS__)


/*
 *
 * Helper functions.
 *
 */

#define CHECK_NOT_NULL(ARG)                                                                                            \
	do {                                                                                                           \
		if (ARG == NULL) {                                                                                     \
			PE("Argument '" #ARG "' can not be null!");                                                    \
			return MND_ERROR_INVALID_VALUE;                                                                \
		}                                                                                                      \
	} while (false)

#define CHECK_CLIENT_ID(ID)                                                                                            \
	do {                                                                                                           \
		if (ID == 0 && ID > INT_MAX) {                                                                         \
			PE("Invalid client id (%u)", ID);                                                              \
			return MND_ERROR_INVALID_VALUE;                                                                \
		}                                                                                                      \
	} while (false)

#define CHECK_CLIENT_INDEX(INDEX)                                                                                      \
	do {                                                                                                           \
		if (INDEX >= root->clients.id_count) {                                                                 \
			PE("Invalid client index, too large (%u)", INDEX);                                             \
			return MND_ERROR_INVALID_VALUE;                                                                \
		}                                                                                                      \
	} while (false)

static int
get_client_info(mnd_root_t *root, uint32_t client_id)
{
	assert(root != NULL);

	xrt_result_t r = ipc_call_system_get_client_info(&root->ipc_c, client_id, &root->app_state);
	if (r != XRT_SUCCESS) {
		PE("Failed to get client info for client id: %u.\n", client_id);
		return MND_ERROR_INVALID_VALUE;
	}

	return MND_SUCCESS;
}


/*
 *
 * API API.
 *
 */

void
mnd_api_get_version(uint32_t *out_major, uint32_t *out_minor, uint32_t *out_patch)
{
	*out_major = MND_API_VERSION_MAJOR;
	*out_minor = MND_API_VERSION_MINOR;
	*out_patch = MND_API_VERSION_PATCH;
}


/*
 *
 * Root API
 *
 */

mnd_result_t
mnd_root_create(mnd_root_t **out_root)
{
	CHECK_NOT_NULL(out_root);

	mnd_root_t *r = U_TYPED_CALLOC(mnd_root_t);

	struct xrt_instance_info info = {0};
	snprintf(info.application_name, sizeof(info.application_name), "%s", "libmonado");

	xrt_result_t xret = ipc_client_connection_init(&r->ipc_c, U_LOGGING_INFO, &info);
	if (xret != XRT_SUCCESS) {
		PE("Connection init error '%i'!\n", xret);
		free(r);
		return MND_ERROR_CONNECTING_FAILED;
	}

	*out_root = r;

	return MND_SUCCESS;
}

void
mnd_root_destroy(mnd_root_t **root_ptr)
{
	if (root_ptr == NULL) {
		return;
	}

	mnd_root_t *r = *root_ptr;
	if (r == NULL) {
		return;
	}

	ipc_client_connection_fini(&r->ipc_c);
	free(r);

	*root_ptr = NULL;

	return;
}

mnd_result_t
mnd_root_update_client_list(mnd_root_t *root)
{
	CHECK_NOT_NULL(root);

	xrt_result_t r = ipc_call_system_get_clients(&root->ipc_c, &root->clients);
	if (r != XRT_SUCCESS) {
		PE("Failed to get client list.\n");
		return MND_ERROR_OPERATION_FAILED;
	}

	return MND_SUCCESS;
}

mnd_result_t
mnd_root_get_number_clients(mnd_root_t *root, uint32_t *out_num)
{
	CHECK_NOT_NULL(root);
	CHECK_NOT_NULL(out_num);

	*out_num = root->clients.id_count;

	return MND_SUCCESS;
}

mnd_result_t
mnd_root_get_client_id_at_index(mnd_root_t *root, uint32_t index, uint32_t *out_client_id)
{
	CHECK_NOT_NULL(root);
	CHECK_CLIENT_INDEX(index);

	*out_client_id = root->clients.ids[index];

	return MND_SUCCESS;
}

mnd_result_t
mnd_root_get_client_name(mnd_root_t *root, uint32_t client_id, const char **out_name)
{
	CHECK_NOT_NULL(root);
	CHECK_CLIENT_ID(client_id);
	CHECK_NOT_NULL(out_name);

	mnd_result_t mret = get_client_info(root, client_id);
	if (mret < 0) {
		return mret; // Prints error.
	}

	*out_name = &(root->app_state.info.application_name[0]);

	return MND_SUCCESS;
}

mnd_result_t
mnd_root_get_client_state(mnd_root_t *root, uint32_t client_id, uint32_t *out_flags)
{
	CHECK_NOT_NULL(root);
	CHECK_CLIENT_ID(client_id);
	CHECK_NOT_NULL(out_flags);

	mnd_result_t mret = get_client_info(root, client_id);
	if (mret < 0) {
		return mret; // Prints error.
	}

	uint32_t flags = 0;
	flags |= (root->app_state.primary_application) ? MND_CLIENT_PRIMARY_APP : 0u;
	flags |= (root->app_state.session_active) ? MND_CLIENT_SESSION_ACTIVE : 0u;
	flags |= (root->app_state.session_visible) ? MND_CLIENT_SESSION_VISIBLE : 0u;
	flags |= (root->app_state.session_focused) ? MND_CLIENT_SESSION_FOCUSED : 0u;
	flags |= (root->app_state.session_overlay) ? MND_CLIENT_SESSION_OVERLAY : 0u;
	flags |= (root->app_state.io_active) ? MND_CLIENT_IO_ACTIVE : 0u;
	*out_flags = flags;

	return MND_SUCCESS;
}

mnd_result_t
mnd_root_set_client_primary(mnd_root_t *root, uint32_t client_id)
{
	CHECK_NOT_NULL(root);
	CHECK_CLIENT_ID(client_id);

	xrt_result_t r = ipc_call_system_set_primary_client(&root->ipc_c, client_id);
	if (r != XRT_SUCCESS) {
		PE("Failed to set primary to client id: %u.\n", client_id);
		return MND_ERROR_OPERATION_FAILED;
	}

	return MND_SUCCESS;
}

mnd_result_t
mnd_root_set_client_focused(mnd_root_t *root, uint32_t client_id)
{
	CHECK_NOT_NULL(root);
	CHECK_CLIENT_ID(client_id);

	xrt_result_t r = ipc_call_system_set_focused_client(&root->ipc_c, client_id);
	if (r != XRT_SUCCESS) {
		PE("Failed to set focused to client id: %u.\n", client_id);
		return MND_ERROR_OPERATION_FAILED;
	}

	return MND_SUCCESS;
}

mnd_result_t
mnd_root_toggle_client_io_active(mnd_root_t *root, uint32_t client_id)
{
	CHECK_NOT_NULL(root);
	CHECK_CLIENT_ID(client_id);

	xrt_result_t r = ipc_call_system_toggle_io_client(&root->ipc_c, client_id);
	if (r != XRT_SUCCESS) {
		PE("Failed to toggle io for client id: %u.\n", client_id);
		return MND_ERROR_OPERATION_FAILED;
	}

	return MND_SUCCESS;
}

mnd_result_t
mnd_root_get_device_count(mnd_root_t *root, uint32_t *out_device_count)
{
	CHECK_NOT_NULL(root);
	CHECK_NOT_NULL(out_device_count);

	*out_device_count = root->ipc_c.ism->isdev_count;

	return MND_SUCCESS;
}

mnd_result_t
mnd_root_get_device_info(mnd_root_t *root, uint32_t device_index, uint32_t *out_device_id, const char **out_dev_name)
{
	CHECK_NOT_NULL(root);
	CHECK_NOT_NULL(out_device_id);
	CHECK_NOT_NULL(out_dev_name);

	if (device_index >= root->ipc_c.ism->isdev_count) {
		PE("Invalid device index (%u)", device_index);
		return MND_ERROR_INVALID_VALUE;
	}

	const struct ipc_shared_device *shared_device = &root->ipc_c.ism->isdevs[device_index];
	*out_device_id = shared_device->name;
	*out_dev_name = shared_device->str;

	return MND_SUCCESS;
}

mnd_result_t
mnd_root_get_device_from_role(mnd_root_t *root, const char *role_name, int32_t *out_device_id)
{
	CHECK_NOT_NULL(root);
	CHECK_NOT_NULL(role_name);
	CHECK_NOT_NULL(out_device_id);

#ifndef MND_PP_DEV_ID_FROM_ROLE
#define MND_PP_DEV_ID_FROM_ROLE(role)                                                                                  \
	if (strcmp(role_name, #role) == 0) {                                                                           \
		*out_device_id = (int32_t)root->ipc_c.ism->roles.role;                                                 \
		return MND_SUCCESS;                                                                                    \
	}
#endif
	MND_PP_DEV_ID_FROM_ROLE(head)
	MND_PP_DEV_ID_FROM_ROLE(left)
	MND_PP_DEV_ID_FROM_ROLE(right)
	MND_PP_DEV_ID_FROM_ROLE(gamepad)
	MND_PP_DEV_ID_FROM_ROLE(eyes)
#undef MND_PP_DEV_ID_FROM_ROLE
	if (strcmp(role_name, "hand-tracking-left") == 0) {
		*out_device_id = root->ipc_c.ism->roles.hand_tracking.left;
		return MND_SUCCESS;
	}
	if (strcmp(role_name, "hand-tracking-right") == 0) {
		*out_device_id = root->ipc_c.ism->roles.hand_tracking.right;
		return MND_SUCCESS;
	}

	PE("Invalid role name (%s)", role_name);
	return MND_ERROR_INVALID_VALUE;
}
