// Copyright 2019-2024, Collabora, Ltd.
// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Implementation of libmonado
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Pete Black <pblack@collabora.com>
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 */

#include "monado.h"

#include "xrt/xrt_defines.h"
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

	//! The retrieved list of tracking origins.
	struct ipc_tracking_origin_list tracking_origin_list;

	//! Cached infos about tracking origins.
	struct ipc_tracking_origin_info origin_infos[XRT_SYSTEM_MAX_DEVICES];

	//! State of the device list.
	struct ipc_device_list device_list;

	//! Cached infos about devices.
	struct ipc_device_info device_infos[XRT_SYSTEM_MAX_DEVICES];

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

enum role_enum
{
	ROLE_HEAD,
	ROLE_EYES,
	ROLE_LEFT,
	ROLE_RIGHT,
	ROLE_GAMEPAD,
	ROLE_HAND_UNOBSTRUCTED_LEFT,
	ROLE_HAND_UNOBSTRUCTED_RIGHT,
	ROLE_HAND_CONFORMING_LEFT,
	ROLE_HAND_CONFORMING_RIGHT,
};

#define CHECK_NOT_NULL(ARG)                                                                                            \
	do {                                                                                                           \
		if (ARG == NULL) {                                                                                     \
			PE("Argument '" #ARG "' can not be null!\n");                                                  \
			return MND_ERROR_INVALID_VALUE;                                                                \
		}                                                                                                      \
	} while (false)

#define CHECK_CLIENT_ID(ID)                                                                                            \
	do {                                                                                                           \
		if (ID == 0 && ID > INT_MAX) {                                                                         \
			PE("Invalid client id (%u)\n", ID);                                                            \
			return MND_ERROR_INVALID_VALUE;                                                                \
		}                                                                                                      \
	} while (false)

#define CHECK_CLIENT_INDEX(INDEX)                                                                                      \
	do {                                                                                                           \
		if (INDEX >= root->clients.id_count) {                                                                 \
			PE("Invalid client index, too large (%u)\n", INDEX);                                           \
			return MND_ERROR_INVALID_VALUE;                                                                \
		}                                                                                                      \
	} while (false)

#define CHECK_ORIGIN_INDEX(INDEX)                                                                                      \
	do {                                                                                                           \
		if (INDEX >= root->tracking_origin_list.origin_count) {                                                \
			PE("Invalid itrack index (%u)\n", INDEX);                                                      \
			return MND_ERROR_INVALID_VALUE;                                                                \
		}                                                                                                      \
	} while (false)

#define CHECK_DEVICE_INDEX(INDEX)                                                                                      \
	do {                                                                                                           \
		if (INDEX >= root->device_list.device_count) {                                                         \
			PE("Invalid device index (%u)\n", INDEX);                                                      \
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

static mnd_result_t
update_tracking_origin_list_and_infos(mnd_root_t *root)
{
	xrt_result_t xret = ipc_call_tracking_origin_get_list(&root->ipc_c, &root->tracking_origin_list);
	if (xret != XRT_SUCCESS) {
		PE("Failed ipc_call_tracking_origin_get_list '%i'", xret);
		return MND_ERROR_OPERATION_FAILED;
	}

	for (uint32_t i = 0; i < root->tracking_origin_list.origin_count; i++) {
		uint32_t id = root->tracking_origin_list.origins[i].id;

		xret = ipc_call_tracking_origin_get_info(&root->ipc_c, id, &root->origin_infos[i]);
		if (xret != XRT_SUCCESS) {
			PE("Failed ipc_call_tracking_origin_get_list '%i'", xret);
			return MND_ERROR_OPERATION_FAILED;
		}
	}

	return MND_SUCCESS;
}

static mnd_result_t
update_device_list_and_infos(mnd_root_t *root)
{
	xrt_result_t xret = ipc_call_system_devices_get_list(&root->ipc_c, &root->device_list);
	if (xret != XRT_SUCCESS) {
		PE("Failed ipc_call_system_devices_get_list '%i'", xret);
		return MND_ERROR_OPERATION_FAILED;
	}

	for (uint32_t i = 0; i < root->device_list.device_count; i++) {
		uint32_t device_id = root->device_list.devices[i].id;

		xret = ipc_call_device_get_info_no_arrays(&root->ipc_c, device_id, &root->device_infos[i]);
		if (xret != XRT_SUCCESS) {
			PE("Failed to get device info for device %u.\n", device_id);
			return MND_ERROR_OPERATION_FAILED;
		}
	}

	return MND_SUCCESS;
}

static enum xrt_view_type
mnd_view_type_to_xrt(mnd_view_type_t mnd_view_type)
{
	switch (mnd_view_type) {
	case MND_VIEW_TYPE_INVALID: return XRT_VIEW_TYPE_INVALID;
	case MND_VIEW_TYPE_MONO: return XRT_VIEW_TYPE_MONO;
	case MND_VIEW_TYPE_STEREO: return XRT_VIEW_TYPE_STEREO;
	case MND_VIEW_TYPE_QUAD: return XRT_VIEW_TYPE_QUAD;
	}

	return XRT_VIEW_TYPE_INVALID;
}

static bool
xrt_view_type_to_mnd(enum xrt_view_type xrt_view_type, mnd_view_type_t *out_mnd_view_type)
{
	switch (xrt_view_type) {
	case XRT_VIEW_TYPE_MAX: break;
	case XRT_VIEW_TYPE_INVALID: *out_mnd_view_type = MND_VIEW_TYPE_INVALID; return true;
	case XRT_VIEW_TYPE_MONO: *out_mnd_view_type = MND_VIEW_TYPE_MONO; return true;
	case XRT_VIEW_TYPE_STEREO: *out_mnd_view_type = MND_VIEW_TYPE_STEREO; return true;
	case XRT_VIEW_TYPE_QUAD: *out_mnd_view_type = MND_VIEW_TYPE_QUAD; return true;
	}

	return false;
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
	info.app_info.immediate_disconnect = true;
	snprintf(info.app_info.application_name, sizeof(info.app_info.application_name), "%s", "libmonado");

	xrt_result_t xret = ipc_client_connection_init(&r->ipc_c, U_LOGGING_INFO, &info);
	if (xret != XRT_SUCCESS) {
		PE("Connection init error '%i'!\n", xret);
		free(r);
		return MND_ERROR_CONNECTING_FAILED;
	}

	bool is_system_available = false;
	xret = ipc_call_instance_is_system_available(&r->ipc_c, &is_system_available);
	if (xret != XRT_SUCCESS) {
		PE("ipc_call_instance_is_system_available: '%i'!\n", xret);
		return -1;
	}
	if (!is_system_available) {
		PE("System isn't available, devices won't be available!");
	}

	mnd_result_t mret = update_tracking_origin_list_and_infos(r);
	if (mret != MND_SUCCESS) {
		mnd_root_destroy(&r);
		return mret;
	}

	mret = update_device_list_and_infos(r);
	if (mret != MND_SUCCESS) {
		mnd_root_destroy(&r);
		return mret;
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

	const struct ipc_client_io_blocks *iob = &root->app_state.io_blocks;
	uint32_t flags = 0;
	flags |= (root->app_state.primary_application) ? MND_CLIENT_PRIMARY_APP : 0u;
	flags |= (root->app_state.session_active) ? MND_CLIENT_SESSION_ACTIVE : 0u;
	flags |= (root->app_state.session_visible) ? MND_CLIENT_SESSION_VISIBLE : 0u;
	flags |= (root->app_state.session_focused) ? MND_CLIENT_SESSION_FOCUSED : 0u;
	flags |= (root->app_state.session_overlay) ? MND_CLIENT_SESSION_OVERLAY : 0u;
	flags |= (!iob->block_poses && !iob->block_hand_tracking && !iob->block_inputs && !iob->block_outputs)
	             ? MND_CLIENT_IO_ACTIVE
	             : 0u;
	flags |= (iob->block_poses) ? MND_CLIENT_POSES_BLOCKED : 0u;
	flags |= (iob->block_hand_tracking) ? MND_CLIENT_HT_BLOCKED : 0u;
	flags |= (iob->block_inputs) ? MND_CLIENT_INPUTS_BLOCKED : 0u;
	flags |= (iob->block_outputs) ? MND_CLIENT_OUTPUTS_BLOCKED : 0u;
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
mnd_root_set_client_io_blocks(mnd_root_t *root, uint32_t client_id, mnd_io_block_flags_t block_flags)
{
	CHECK_NOT_NULL(root);
	CHECK_CLIENT_ID(client_id);

	struct ipc_client_io_blocks blocks = {0};
	blocks.block_poses = block_flags & MND_IO_BLOCK_POSES;
	blocks.block_hand_tracking = block_flags & MND_IO_BLOCK_HT;
	blocks.block_inputs = block_flags & MND_IO_BLOCK_INPUTS;
	blocks.block_outputs = block_flags & MND_IO_BLOCK_OUTPUTS;
	xrt_result_t r = ipc_call_system_set_client_io_blocks(&root->ipc_c, client_id, &blocks);
	if (r != XRT_SUCCESS) {
		PE("Failed to set io blocks for client id: %u.\n", client_id);
		return MND_ERROR_OPERATION_FAILED;
	}

	return MND_SUCCESS;
}

mnd_result_t
mnd_root_get_client_session_running_state(mnd_root_t *root, uint32_t client_id, mnd_session_state_t *out_session_state)
{
	CHECK_NOT_NULL(root);
	CHECK_CLIENT_ID(client_id);
	CHECK_NOT_NULL(out_session_state);

	struct xrt_compositor_session_running_state running_state;
	xrt_result_t r = ipc_call_system_get_client_session_running_state(&root->ipc_c, client_id, &running_state);
	if (r != XRT_SUCCESS) {
		PE("Failed to get session running state for client id: %u.\n", client_id);
		return MND_ERROR_OPERATION_FAILED;
	}

	out_session_state->running = running_state.running;

	if (!xrt_view_type_to_mnd(running_state.active_view_type, &out_session_state->active_view_type)) {
		PE("Unsupported view type '%u' for client id: %u.\n", running_state.active_view_type, client_id);
		return MND_ERROR_OPERATION_FAILED;
	}

	return MND_SUCCESS;
}

mnd_result_t
mnd_root_get_client_system_view_config(mnd_root_t *root,
                                       uint32_t client_id,
                                       mnd_view_type_t mnd_view_type,
                                       mnd_view_config_view_t *out_default_view_config,
                                       mnd_recommended_view_config_t *out_recommended_view_config)
{
	CHECK_NOT_NULL(root);
	CHECK_CLIENT_ID(client_id);
	CHECK_NOT_NULL(out_default_view_config);
	CHECK_NOT_NULL(out_recommended_view_config);

	enum xrt_view_type view_type = mnd_view_type_to_xrt(mnd_view_type);

	if (view_type == XRT_VIEW_TYPE_INVALID) {
		PE("Invalid view type '%u'!\n", mnd_view_type);
		return MND_ERROR_INVALID_VALUE;
	}

	uint32_t view_count = xrt_view_type_view_count(view_type);

	struct xrt_view_config view_config_default;
	struct xrt_recommended_view_config view_config_recommended;
	xrt_result_t r = ipc_call_system_get_client_view_config(&root->ipc_c, client_id, view_type,
	                                                        &view_config_default, &view_config_recommended);
	if (r != XRT_SUCCESS) {
		PE("Failed to get view config for client id: %u.\n", client_id);
		return MND_ERROR_OPERATION_FAILED;
	}

	if (view_count != view_config_default.view_count) {
		PE("View count mismatch for default config! Expected %u, got %u.\n", view_count,
		   view_config_default.view_count);
		return MND_ERROR_OPERATION_FAILED;
	}

	if (view_config_recommended.valid && view_count != view_config_recommended.view_count) {
		PE("View count mismatch for recommended config! Expected %u, got %u.\n", view_count,
		   view_config_recommended.view_count);
		return MND_ERROR_OPERATION_FAILED;
	}

	(*out_recommended_view_config) = (mnd_recommended_view_config_t){
	    .valid = view_config_recommended.valid,
	};
	for (uint32_t i = 0; i < view_count; i++) {
		out_default_view_config[i] = (mnd_view_config_view_t){
		    .width_pixels = view_config_default.views[i].recommended.width_pixels,
		    .height_pixels = view_config_default.views[i].recommended.height_pixels,
		    .sample_count = view_config_default.views[i].recommended.sample_count,
		};

		out_recommended_view_config->view_configs[i] = (mnd_view_config_view_t){
		    .width_pixels = view_config_recommended.views[i].width_pixels,
		    .height_pixels = view_config_recommended.views[i].height_pixels,
		    .sample_count = view_config_recommended.views[i].sample_count,
		};
	}

	return MND_SUCCESS;
}

mnd_result_t
mnd_root_set_client_recommended_view_config(mnd_root_t *root,
                                            uint32_t client_id,
                                            mnd_view_type_t mnd_view_type,
                                            const mnd_recommended_view_config_t *recommended_view_config)
{
	CHECK_NOT_NULL(root);
	CHECK_CLIENT_ID(client_id);
	CHECK_NOT_NULL(recommended_view_config);

	mnd_result_t mret = get_client_info(root, client_id);
	if (mret < 0) {
		return mret; // Prints error.
	}

	if (!root->app_state.info.view_configuration_views_change_supported) {
		PE("Client does not support setting recommended view config for client id: %u.\n", client_id);
		return MND_ERROR_UNSUPPORTED_OPERATION;
	}

	enum xrt_view_type view_type = mnd_view_type_to_xrt(mnd_view_type);

	if (view_type == XRT_VIEW_TYPE_INVALID) {
		PE("Invalid view type '%u'!\n", view_type);
		return MND_ERROR_INVALID_VALUE;
	}

	uint32_t view_count = xrt_view_type_view_count(view_type);

	struct xrt_recommended_view_config xrt_view_config = {
	    .view_count = view_count,
	    .valid = recommended_view_config->valid,
	};
	for (uint32_t i = 0; i < view_count; i++) {
		xrt_view_config.views[i].width_pixels = recommended_view_config->view_configs[i].width_pixels;
		xrt_view_config.views[i].height_pixels = recommended_view_config->view_configs[i].height_pixels;
		xrt_view_config.views[i].sample_count = recommended_view_config->view_configs[i].sample_count;
	}

	xrt_result_t r = ipc_call_system_set_client_recommended_view_config( //
	    &root->ipc_c,                                                    //
	    client_id,                                                       //
	    view_type,                                                       //
	    &xrt_view_config);                                               //
	if (r != XRT_SUCCESS) {
		PE("Failed to set recommended view config for client id: %u.\n", client_id);
		return MND_ERROR_OPERATION_FAILED;
	}

	return MND_SUCCESS;
}

mnd_result_t
mnd_root_get_client_property_bool(mnd_root_t *root, uint32_t client_id, mnd_property_t prop, bool *out_bool)
{
	CHECK_NOT_NULL(root);
	CHECK_NOT_NULL(out_bool);

	mnd_result_t mret = get_client_info(root, client_id);
	if (mret < 0) {
		return mret; // Prints error.
	}

	switch (prop) {
	case MND_PROPERTY_SUPPORTS_VIEW_CONFIGURATION_CHANGE_BOOL:
		*out_bool = root->app_state.info.view_configuration_views_change_supported;
		break;
	default: PE("Is not a valid boolean property (%u)", prop); return MND_ERROR_INVALID_PROPERTY;
	}

	return MND_SUCCESS;
}

mnd_result_t
mnd_root_get_device_count(mnd_root_t *root, uint32_t *out_device_count)
{
	CHECK_NOT_NULL(root);
	CHECK_NOT_NULL(out_device_count);

	*out_device_count = root->device_list.device_count;

	return MND_SUCCESS;
}

mnd_result_t
mnd_root_get_device_info_bool(mnd_root_t *root, uint32_t device_index, mnd_property_t prop, bool *out_bool)
{
	CHECK_NOT_NULL(root);
	CHECK_NOT_NULL(out_bool);
	CHECK_DEVICE_INDEX(device_index);

	const struct ipc_device_info *device_info = &root->device_infos[device_index];

	switch (prop) {
	case MND_PROPERTY_SUPPORTS_POSITION_BOOL: *out_bool = device_info->supported.position_tracking; break;
	case MND_PROPERTY_SUPPORTS_ORIENTATION_BOOL: *out_bool = device_info->supported.orientation_tracking; break;
	case MND_PROPERTY_SUPPORTS_BRIGHTNESS_BOOL: *out_bool = device_info->supported.brightness_control; break;
	default: PE("Is not a valid boolean property (%u)", prop); return MND_ERROR_INVALID_PROPERTY;
	}

	return MND_SUCCESS;
}

mnd_result_t
mnd_root_get_device_info_i32(mnd_root_t *root, uint32_t device_index, mnd_property_t prop, uint32_t *out_i32)
{
	CHECK_NOT_NULL(root);
	CHECK_NOT_NULL(out_i32);
	CHECK_DEVICE_INDEX(device_index);

	PE("Is not a valid i32 property (%u)", prop);

	return MND_ERROR_INVALID_PROPERTY;
}

mnd_result_t
mnd_root_get_device_info_u32(mnd_root_t *root, uint32_t device_index, mnd_property_t prop, uint32_t *out_u32)
{
	CHECK_NOT_NULL(root);
	CHECK_NOT_NULL(out_u32);
	CHECK_DEVICE_INDEX(device_index);

	const struct ipc_device_info *device_info = &root->device_infos[device_index];

	switch (prop) {
	case MND_PROPERTY_TRACKING_ORIGIN_U32:
		for (uint32_t i = 0; i < root->tracking_origin_list.origin_count; i++) {
			if (device_info->tracking_origin_id == root->tracking_origin_list.origins[i].id) {
				*out_u32 = i;
				return MND_SUCCESS;
			}
		}
		PE("Could not find tracking origin id in origins list '%u'!\n", device_info->tracking_origin_id);
		return MND_ERROR_INVALID_VALUE;
	default: PE("Is not a valid u32 property (%u)", prop); return MND_ERROR_INVALID_PROPERTY;
	}

	return MND_SUCCESS;
}

mnd_result_t
mnd_root_get_device_info_float(mnd_root_t *root, uint32_t device_index, mnd_property_t prop, float *out_float)
{
	CHECK_NOT_NULL(root);
	CHECK_NOT_NULL(out_float);
	CHECK_DEVICE_INDEX(device_index);

	PE("Is not a valid float property (%u)", prop);

	return MND_ERROR_INVALID_PROPERTY;
}

mnd_result_t
mnd_root_get_device_info_string(mnd_root_t *root, uint32_t device_index, mnd_property_t prop, const char **out_string)
{
	CHECK_NOT_NULL(root);
	CHECK_NOT_NULL(out_string);
	CHECK_DEVICE_INDEX(device_index);

	const struct ipc_device_info *device_info = &root->device_infos[device_index];

	switch (prop) {
	case MND_PROPERTY_NAME_STRING: *out_string = device_info->str; break;
	case MND_PROPERTY_SERIAL_STRING: *out_string = device_info->serial; break;
	default: PE("Is not a valid string property (%u)", prop); return MND_ERROR_INVALID_PROPERTY;
	}

	return MND_SUCCESS;
}

mnd_result_t
mnd_root_get_device_info(mnd_root_t *root, uint32_t device_index, uint32_t *out_device_id, const char **out_dev_name)
{
	CHECK_NOT_NULL(root);
	CHECK_NOT_NULL(out_device_id);
	CHECK_NOT_NULL(out_dev_name);
	CHECK_DEVICE_INDEX(device_index);

	const struct ipc_device_info *device_info = &root->device_infos[device_index];
	*out_device_id = device_info->name;
	*out_dev_name = device_info->str;

	return MND_SUCCESS;
}

mnd_result_t
mnd_root_get_device_from_role(mnd_root_t *root, const char *role_name, int32_t *out_index)
{
	CHECK_NOT_NULL(root);
	CHECK_NOT_NULL(role_name);
	CHECK_NOT_NULL(out_index);

	enum role_enum role;

#define TO_ENUM(STRING, ENUM)                                                                                          \
	if (strcmp(role_name, STRING) == 0) {                                                                          \
		role = ENUM;                                                                                           \
	} else

	TO_ENUM("head", ROLE_HEAD)
	TO_ENUM("eyes", ROLE_EYES)
	TO_ENUM("left", ROLE_LEFT)
	TO_ENUM("right", ROLE_RIGHT)
	TO_ENUM("gamepad", ROLE_GAMEPAD)
	TO_ENUM("hand-tracking-unobstructed-left", ROLE_HAND_UNOBSTRUCTED_LEFT)
	TO_ENUM("hand-tracking-unobstructed-right", ROLE_HAND_UNOBSTRUCTED_RIGHT)
	TO_ENUM("hand-tracking-conforming-left", ROLE_HAND_CONFORMING_LEFT)
	TO_ENUM("hand-tracking-conforming-right", ROLE_HAND_CONFORMING_RIGHT)
	//! *DEPRECATED** following roles name are deprecated, to be removed in the next major version
	TO_ENUM("hand-tracking-left", ROLE_HAND_UNOBSTRUCTED_LEFT)
	TO_ENUM("hand-tracking-right", ROLE_HAND_UNOBSTRUCTED_RIGHT)
	{
		PE("Invalid role name (%s)", role_name);
		return MND_ERROR_INVALID_VALUE;
	}
#undef TO_ENUM

	switch (role) {
	case ROLE_HEAD: *out_index = root->ipc_c.ism->roles.head; return MND_SUCCESS;
	case ROLE_EYES: *out_index = root->ipc_c.ism->roles.eyes; return MND_SUCCESS;
#define CASE_ROLE_HAND(UC_SRC, SRC)                                                                                    \
	case ROLE_HAND_##UC_SRC##_LEFT: *out_index = root->ipc_c.ism->roles.hand_tracking.SRC.left;                    \
	    return MND_SUCCESS;                                                                                        \
	case ROLE_HAND_##UC_SRC##_RIGHT:                                                                               \
		*out_index = root->ipc_c.ism->roles.hand_tracking.SRC.right;                                           \
		return MND_SUCCESS;
		CASE_ROLE_HAND(UNOBSTRUCTED, unobstructed)
		CASE_ROLE_HAND(CONFORMING, conforming)
#undef CASE_ROLE_HAND
	case ROLE_LEFT:
	case ROLE_RIGHT:
	case ROLE_GAMEPAD: break;
	}

	struct xrt_system_roles roles;
	xrt_result_t xret = ipc_call_system_devices_get_roles(&root->ipc_c, &roles);
	if (xret != XRT_SUCCESS) {
		PE("Failed to get dynamic roles");
		return MND_ERROR_OPERATION_FAILED;
	}

	// Assumes roles index match device id.
	switch (role) {
	case ROLE_LEFT: *out_index = roles.left; return MND_SUCCESS;
	case ROLE_RIGHT: *out_index = roles.right; return MND_SUCCESS;
	case ROLE_GAMEPAD: *out_index = roles.gamepad; return MND_SUCCESS;
	default: PE("Internal error, shouldn't get here"); return MND_ERROR_OPERATION_FAILED;
	}
}

mnd_result_t
mnd_root_recenter_local_spaces(mnd_root_t *root)
{
	xrt_result_t xret = ipc_call_space_recenter_local_spaces(&root->ipc_c);
	switch (xret) {
	case XRT_SUCCESS: return MND_SUCCESS;
	case XRT_ERROR_RECENTERING_NOT_SUPPORTED: return MND_ERROR_RECENTERING_NOT_SUPPORTED;
	case XRT_ERROR_IPC_FAILURE: PE("Connection error!"); return MND_ERROR_OPERATION_FAILED;
	default: PE("Internal error, shouldn't get here"); return MND_ERROR_OPERATION_FAILED;
	}
}

mnd_result_t
mnd_root_get_reference_space_offset(mnd_root_t *root, mnd_reference_space_type_t type, mnd_pose_t *out_offset)
{
	xrt_result_t xret = ipc_call_space_get_reference_space_offset(&root->ipc_c, (enum xrt_reference_space_type)type,
	                                                              (struct xrt_pose *)out_offset);
	switch (xret) {
	case XRT_SUCCESS: return MND_SUCCESS;
	case XRT_ERROR_UNSUPPORTED_SPACE_TYPE: return MND_ERROR_INVALID_OPERATION;
	case XRT_ERROR_IPC_FAILURE: PE("Connection error!"); return MND_ERROR_OPERATION_FAILED;
	default: PE("Internal error, shouldn't get here"); return MND_ERROR_OPERATION_FAILED;
	}
}

mnd_result_t
mnd_root_set_reference_space_offset(mnd_root_t *root, mnd_reference_space_type_t type, const mnd_pose_t *offset)
{
	xrt_result_t xret = ipc_call_space_set_reference_space_offset(&root->ipc_c, (enum xrt_reference_space_type)type,
	                                                              (struct xrt_pose *)offset);
	switch (xret) {
	case XRT_SUCCESS: return MND_SUCCESS;
	case XRT_ERROR_UNSUPPORTED_SPACE_TYPE: return MND_ERROR_INVALID_OPERATION;
	case XRT_ERROR_RECENTERING_NOT_SUPPORTED: return MND_ERROR_RECENTERING_NOT_SUPPORTED;
	case XRT_ERROR_IPC_FAILURE: PE("Connection error!"); return MND_ERROR_OPERATION_FAILED;
	default: PE("Internal error, shouldn't get here"); return MND_ERROR_OPERATION_FAILED;
	}
}

mnd_result_t
mnd_root_get_tracking_origin_offset(mnd_root_t *root, uint32_t origin_index, mnd_pose_t *out_offset)
{
	CHECK_NOT_NULL(root);
	CHECK_NOT_NULL(out_offset);
	CHECK_ORIGIN_INDEX(origin_index);

	uint32_t origin_id = root->tracking_origin_list.origins[origin_index].id;

	xrt_result_t xret =
	    ipc_call_space_get_tracking_origin_offset(&root->ipc_c, origin_id, (struct xrt_pose *)out_offset);
	switch (xret) {
	case XRT_SUCCESS: return MND_SUCCESS;
	case XRT_ERROR_UNSUPPORTED_SPACE_TYPE: return MND_ERROR_INVALID_OPERATION;
	case XRT_ERROR_IPC_FAILURE: PE("Connection error!"); return MND_ERROR_OPERATION_FAILED;
	default: PE("Internal error, shouldn't get here"); return MND_ERROR_OPERATION_FAILED;
	}
}

mnd_result_t
mnd_root_set_tracking_origin_offset(mnd_root_t *root, uint32_t origin_index, const mnd_pose_t *offset)
{
	CHECK_NOT_NULL(root);
	CHECK_NOT_NULL(offset);
	CHECK_ORIGIN_INDEX(origin_index);

	uint32_t origin_id = root->tracking_origin_list.origins[origin_index].id;

	xrt_result_t xret =
	    ipc_call_space_set_tracking_origin_offset(&root->ipc_c, origin_id, (struct xrt_pose *)offset);
	switch (xret) {
	case XRT_SUCCESS: return MND_SUCCESS;
	case XRT_ERROR_UNSUPPORTED_SPACE_TYPE: return MND_ERROR_INVALID_OPERATION;
	case XRT_ERROR_IPC_FAILURE: PE("Connection error!"); return MND_ERROR_OPERATION_FAILED;
	default: PE("Internal error, shouldn't get here"); return MND_ERROR_OPERATION_FAILED;
	}
}

mnd_result_t
mnd_root_get_tracking_origin_count(mnd_root_t *root, uint32_t *out_track_count)
{
	CHECK_NOT_NULL(root);
	CHECK_NOT_NULL(out_track_count);

	*out_track_count = root->tracking_origin_list.origin_count;

	return MND_SUCCESS;
}

mnd_result_t
mnd_root_get_tracking_origin_name(mnd_root_t *root, uint32_t origin_index, const char **out_string)
{
	CHECK_NOT_NULL(root);
	CHECK_NOT_NULL(out_string);
	CHECK_ORIGIN_INDEX(origin_index);

	const struct ipc_tracking_origin_info *itoi = &root->origin_infos[origin_index];

	*out_string = itoi->name;

	return MND_SUCCESS;
}

mnd_result_t
mnd_root_get_device_battery_status(
    mnd_root_t *root, uint32_t device_index, bool *out_present, bool *out_charging, float *out_charge)
{
	CHECK_NOT_NULL(root);
	CHECK_NOT_NULL(out_present);
	CHECK_NOT_NULL(out_charging);
	CHECK_NOT_NULL(out_charge);
	CHECK_DEVICE_INDEX(device_index);

	const struct ipc_device_info *device_info = &root->device_infos[device_index];

	if (!device_info->supported.battery_status) {
		return MND_ERROR_OPERATION_FAILED;
	}

	xrt_result_t xret =
	    ipc_call_device_get_battery_status(&root->ipc_c, device_index, out_present, out_charging, out_charge);
	switch (xret) {
	case XRT_SUCCESS: return MND_SUCCESS;
	case XRT_ERROR_IPC_FAILURE: PE("Connection error!"); return MND_ERROR_OPERATION_FAILED;
	default: PE("Internal error, shouldn't get here"); return MND_ERROR_OPERATION_FAILED;
	}
}

mnd_result_t
mnd_root_get_device_brightness(mnd_root_t *root, uint32_t device_index, float *out_brightness)
{
	CHECK_NOT_NULL(root);
	CHECK_DEVICE_INDEX(device_index);
	CHECK_NOT_NULL(out_brightness);

	const struct ipc_device_info *device_info = &root->device_infos[device_index];

	if (!device_info->supported.brightness_control) {
		PE("device_get_brightness unsupported\n");
		return MND_ERROR_UNSUPPORTED_OPERATION;
	}

	xrt_result_t xret = ipc_call_device_get_brightness(&root->ipc_c, device_index, out_brightness);
	switch (xret) {
	case XRT_SUCCESS: return MND_SUCCESS;
	case XRT_ERROR_IPC_FAILURE: PE("Connection error!"); return MND_ERROR_OPERATION_FAILED;
	default: PE("Internal error, shouldn't get here"); return MND_ERROR_OPERATION_FAILED;
	}
}

mnd_result_t
mnd_root_set_device_brightness(mnd_root_t *root, uint32_t device_index, float brightness, bool relative)
{
	CHECK_NOT_NULL(root);
	CHECK_DEVICE_INDEX(device_index);

	const struct ipc_device_info *device_info = &root->device_infos[device_index];

	if (!device_info->supported.brightness_control) {
		PE("device_set_brightness unsupported\n");
		return MND_ERROR_UNSUPPORTED_OPERATION;
	}

	xrt_result_t xret = ipc_call_device_set_brightness(&root->ipc_c, device_index, brightness, relative);
	switch (xret) {
	case XRT_SUCCESS: return MND_SUCCESS;
	case XRT_ERROR_IPC_FAILURE: PE("Connection error!"); return MND_ERROR_OPERATION_FAILED;
	default: PE("Internal error, shouldn't get here"); return MND_ERROR_OPERATION_FAILED;
	}
}

mnd_result_t
mnd_root_set_chroma_key_params(
    mnd_root_t *root, struct mnd_colour_hsv hsv_min, struct mnd_colour_hsv hsv_max, float curve, float despill)
{
	struct xrt_colour_hsv_f32 min = {hsv_min.h, hsv_min.s, hsv_min.v};
	struct xrt_colour_hsv_f32 max = {hsv_max.h, hsv_max.s, hsv_max.v};
	xrt_result_t xret = ipc_call_compositor_set_chroma_key_params(&root->ipc_c, &min, &max, curve, despill);
	switch (xret) {
	case XRT_SUCCESS: return MND_SUCCESS;
	case XRT_ERROR_UNSUPPORTED_SPACE_TYPE: return MND_ERROR_INVALID_OPERATION;
	case XRT_ERROR_IPC_FAILURE: PE("Connection error!"); return MND_ERROR_OPERATION_FAILED;
	default: PE("Internal error, shouldn't get here"); return MND_ERROR_OPERATION_FAILED;
	}
}
