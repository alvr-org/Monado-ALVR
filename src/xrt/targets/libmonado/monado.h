// Copyright 2019-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Interface of libmonado
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 */

#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif


/*
 *
 * Enums, defines and objects.
 *
 */

//! Major version of the API.
#define MND_API_VERSION_MAJOR 1
//! Minor version of the API.
#define MND_API_VERSION_MINOR 2
//! Patch version of the API.
#define MND_API_VERSION_PATCH 0

/*!
 * Result codes for operations, negative are errors, zero or positives are
 * success.
 */
typedef enum mnd_result
{
	MND_SUCCESS = 0,
	MND_ERROR_INVALID_VERSION = -1,
	MND_ERROR_INVALID_VALUE = -2,
	MND_ERROR_CONNECTING_FAILED = -3,
	MND_ERROR_OPERATION_FAILED = -4,
	//! Supported in version 1.1 and above.
	MND_ERROR_RECENTERING_NOT_SUPPORTED = -5,
	//! Supported in version 1.2 and above.
	MND_ERROR_INVALID_PROPERTY = -6,
} mnd_result_t;

/*!
 * Bitflags for client application state.
 */
typedef enum mnd_client_flags
{
	MND_CLIENT_PRIMARY_APP = (1u << 0u),
	MND_CLIENT_SESSION_ACTIVE = (1u << 1u),
	MND_CLIENT_SESSION_VISIBLE = (1u << 2u),
	MND_CLIENT_SESSION_FOCUSED = (1u << 3u),
	MND_CLIENT_SESSION_OVERLAY = (1u << 4u),
	MND_CLIENT_IO_ACTIVE = (1u << 5u),
} mnd_client_flags_t;

/*!
 * A property to get from a thing (currently only devices).
 *
 * Supported in version 1.2 and above.
 */
typedef enum mnd_property
{
	//! Supported in version 1.2 and above.
	MND_PROPERTY_NAME_STRING = 0,
	//! Supported in version 1.2 and above.
	MND_PROPERTY_SERIAL_STRING = 1,
} mnd_property_t;

/*!
 * Opaque type for libmonado state
 */
typedef struct mnd_root mnd_root_t;


/*
 *
 * Functions
 *
 */

/*!
 * Returns the version of the API (not Monado itself), follows the versioning
 * semantics of https://semver.org/ standard. In short if the major version
 * mismatch then the interface is incompatible.
 *
 * @param[out] out_major Major version number, must be valid pointer.
 * @param[out] out_minor Minor version number, must be valid pointer.
 * @param[out] out_patch Patch version number, must be valid pointer.
 *
 * Always succeeds, or crashes if any pointer isn't valid.
 */
void
mnd_api_get_version(uint32_t *out_major, uint32_t *out_minor, uint32_t *out_patch);

/*!
 * Create libmonado state and connect to service
 *
 * @param[out] out_root Address to populate with the opaque state type.
 * @return MND_SUCCESS on success
 */
mnd_result_t
mnd_root_create(mnd_root_t **out_root);

/*!
 * Destroy libmonado state, disconnecting from the service, and zeroing the
 * pointer.
 *
 * @param root_ptr Pointer to your libmonado state. Null-checked, will be set to null.
 */
void
mnd_root_destroy(mnd_root_t **root_ptr);

/*!
 * Update our local cached copy of the client list
 *
 * @param root The libmonado state.
 * @return MND_SUCCESS on success
 */
mnd_result_t
mnd_root_update_client_list(mnd_root_t *root);

/*!
 * Get the number of active clients
 *
 * This value only changes on calls to @ref mnd_root_update_client_list
 *
 * @param root         The libmonado state.
 * @param[out] out_num Pointer to value to populate with the number of clients.
 *
 * @pre Called @ref mnd_root_update_client_list at least once
 *
 * @return MND_SUCCESS on success
 */
mnd_result_t
mnd_root_get_number_clients(mnd_root_t *root, uint32_t *out_num);

/*!
 * Get the id from the current client list.
 *
 * @param root               The libmonado state.
 * @param index              Index to retrieve id for.
 * @param[out] out_client_id Pointer to value to populate with the id at the given index.
 */
mnd_result_t
mnd_root_get_client_id_at_index(mnd_root_t *root, uint32_t index, uint32_t *out_client_id);

/*!
 * Get the name of the client at the given index.
 *
 * The string returned is only valid until the next call into libmonado.
 *
 * @param root          The libmonado state.
 * @param client_id     ID of client to retrieve name from.
 * @param[out] out_name Pointer to populate with the client name.
 *
 * @pre Called @ref mnd_root_update_client_list at least once
 *
 * @return MND_SUCCESS on success
 */
mnd_result_t
mnd_root_get_client_name(mnd_root_t *root, uint32_t client_id, const char **out_name);

/*!
 * Get the state flags of the client at the given index.
 *
 * This result only changes on calls to @ref mnd_root_update_client_list
 *
 * @param root           The libmonado state.
 * @param client_id      ID of client to retrieve flags from.
 * @param[out] out_flags Pointer to populate with the flags, a bitwise combination of @ref mnd_client_flags.
 *
 * @pre Called @ref mnd_root_update_client_list at least once
 *
 * @return MND_SUCCESS on success
 */
mnd_result_t
mnd_root_get_client_state(mnd_root_t *root, uint32_t client_id, uint32_t *out_flags);

/*!
 * Set the client at the given index as "primary".
 *
 * @param root      The libmonado state.
 * @param client_id ID of the client set as primary.
 *
 * @pre Called @ref mnd_root_update_client_list at least once
 *
 * @return MND_SUCCESS on success
 */
mnd_result_t
mnd_root_set_client_primary(mnd_root_t *root, uint32_t client_id);

/*!
 * Set the client at the given index as "focused".
 *
 * @param root      The libmonado state.
 * @param client_id ID of the client set as focused.
 *
 * @pre Called @ref mnd_root_update_client_list at least once
 *
 * @return MND_SUCCESS on success
 */
mnd_result_t
mnd_root_set_client_focused(mnd_root_t *root, uint32_t client_id);

/*!
 * Toggle io activity for the client at the given index.
 *
 * @param root      The libmonado state.
 * @param client_id ID of the client to toggle IO for.
 *
 * @pre Called @ref mnd_root_update_client_list at least once
 *
 * @return MND_SUCCESS on success
 */
mnd_result_t
mnd_root_toggle_client_io_active(mnd_root_t *root, uint32_t client_id);

/*!
 * Get the number of devices
 *
 * @param root                  The libmonado state.
 * @param[out] out_device_count Pointer to value to populate with the number of devices.
 *
 * @return MND_SUCCESS on success
 */
mnd_result_t
mnd_root_get_device_count(mnd_root_t *root, uint32_t *out_device_count);

/*!
 * Get boolean property for the device at the given index.
 *
 * Supported in version 1.2 and above.
 *
 * @param root          The libmonado state.
 * @param device_index  Index of device to retrieve name from.
 * @param prop          A boolean property enum.
 * @param[out] out_bool Pointer to populate with the boolean.
 *
 * @return MND_SUCCESS on success
 */
mnd_result_t
mnd_root_get_device_info_bool(mnd_root_t *root, uint32_t device_index, mnd_property_t prop, bool *out_bool);

/*!
 * Get int32_t property for the device at the given index.
 *
 * Supported in version 1.2 and above.
 *
 * @param root         The libmonado state.
 * @param device_index Index of device to retrieve name from.
 * @param prop         A int32_t property enum.
 * @param[out] out_i32 Pointer to populate with the int32_t.
 *
 * @return MND_SUCCESS on success
 */
mnd_result_t
mnd_root_get_device_info_i32(mnd_root_t *root, uint32_t device_index, mnd_property_t prop, uint32_t *out_i32);

/*!
 * Get uint32_t property for the device at the given index.
 *
 * Supported in version 1.2 and above.
 *
 * @param root          The libmonado state.
 * @param device_index  Index of device to retrieve name from.
 * @param prop          A uint32_t property enum.
 * @param[out] out_u32 Pointer to populate with the uint32_t.
 *
 * @return MND_SUCCESS on success
 */
mnd_result_t
mnd_root_get_device_info_u32(mnd_root_t *root, uint32_t device_index, mnd_property_t prop, uint32_t *out_u32);

/*!
 * Get float property for the device at the given index.
 *
 * Supported in version 1.2 and above.
 *
 * @param root           The libmonado state.
 * @param device_index   Index of device to retrieve name from.
 * @param prop           A float property enum.
 * @param[out] out_float Pointer to populate with the float.
 *
 * @return MND_SUCCESS on success
 */
mnd_result_t
mnd_root_get_device_info_float(mnd_root_t *root, uint32_t device_index, mnd_property_t prop, float *out_float);

/*!
 * Get string property for the device at the given index.
 *
 * Supported in version 1.2 and above.
 *
 * @param root            The libmonado state.
 * @param device_index    Index of device to retrieve name from.
 * @param prop            A string property enum.
 * @param[out] out_string Pointer to populate with the string.
 *
 * @return MND_SUCCESS on success
 */
mnd_result_t
mnd_root_get_device_info_string(mnd_root_t *root, uint32_t device_index, mnd_property_t prop, const char **out_string);

/*!
 * Get device info at the given index.
 *
 * @deprecated Deprecated in version 1.2, scheduled for removal in version 2.0.0 currently.
 *
 * @param root               The libmonado state.
 * @param device_index       Index of device to retrieve name from.
 * @param[out] out_device_id Pointer to value to populate with the device id at the given index.
 * @param[out] out_dev_name  Pointer to populate with the device name.
 *
 * @return MND_SUCCESS on success
 */
mnd_result_t
mnd_root_get_device_info(mnd_root_t *root, uint32_t device_index, uint32_t *out_device_id, const char **out_dev_name);

/*!
 * Get the device index associated for a given role name.
 *
 * @param root           The libmonado state.
 * @param role_name      Name of the role, one-of: "head", "left", "right",
 *                       "gamepad", "eyes", "hand-tracking-left", and,
 *                       "hand-tracking-right":
 * @param[out] out_index Pointer to value to populate with the device index
 *                       associated with given role name, -1 if not role is set.
 *
 * @return MND_SUCCESS on success
 */
mnd_result_t
mnd_root_get_device_from_role(mnd_root_t *root, const char *role_name, int32_t *out_index);

/*!
 * Trigger a recenter of the local spaces.
 *
 * Supported in version 1.1 and above.
 *
 * @param root The libmonado state.
 *
 * @return MND_SUCCESS on success
 */
mnd_result_t
mnd_root_recenter_local_spaces(mnd_root_t *root);


#ifdef __cplusplus
}
#endif
