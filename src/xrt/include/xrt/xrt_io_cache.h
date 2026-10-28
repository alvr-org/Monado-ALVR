// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header defining xrt input/output cache.
 * @ingroup xrt_iface
 */

#pragma once

#include "xrt/xrt_defines.h"
#include "xrt/xrt_results.h"
#include "xrt/xrt_device.h"
#include "xrt/xrt_limits.h"


#ifdef __cplusplus
extern "C" {
#endif

struct xrt_space;
struct xrt_space_overseer;

/*!
 * Per-device layout entry describing where a device's inputs and outputs
 * sit in the flattened cache arrays.
 *
 * @ingroup xrt_iface
 */
struct xrt_io_cache_device
{
	//! Unique identifier for this device.
	struct xrt_device_id id;

	//! Human-readable device name string.
	XRT_ALIGNAS(8) char str[XRT_DEVICE_NAME_LEN];

	//! Device name enum.
	enum xrt_device_name name;

	//! Index of this device's first input in the flattened input array.
	uint32_t first_input;

	//! Number of inputs contributed by this device.
	uint32_t input_count;

	//! Index of this device's first output in the flattened output array.
	uint32_t first_output;

	//! Number of outputs contributed by this device.
	uint32_t output_count;
};

/*!
 * Snapshot of the cache layout. Returned by @ref xrt_io_cache::get_info.
 *
 * @ref generation_id must be passed to @ref xrt_io_cache::read together with
 * @ref input_count and @ref output_count. If the generation no longer matches the
 * live cache layout, @ref xrt_io_cache::read sets @p out_stale to true and returns
 * @ref XRT_SUCCESS without writing the arrays.
 *
 * The @ref head, @ref eyes, @ref left, @ref right and @ref gamepad fields
 * describe the current role assignment as indices into @ref devices (mirroring
 * how @ref xrt_system_roles indexes into @ref xrt_system_devices::static_xdevs).
 * They let a consumer resolve interaction profiles for the top-level user paths
 * without touching any @ref xrt_device. The controller roles (left, right,
 * gamepad) are dynamic; the head and eyes roles are static but duplicated here
 * for convenience so that a consumer can resolve every top-level path purely
 * from the cache. A change to a dynamic role assignment also bumps
 * @ref generation_id.
 *
 * @ingroup xrt_iface
 */
struct xrt_io_cache_info
{
	/*!
	 * Monotonically increasing layout generation; changes when devices,
	 * their input/output counts, or the role assignment below change.
	 */
	uint64_t generation_id;

	//! Per-device layout entries.
	struct xrt_io_cache_device devices[XRT_SYSTEM_MAX_DEVICES];

	//! Number of entries in @ref devices that are valid.
	XRT_ALIGNAS(8) uint32_t device_count;

	//! Total number of inputs across all devices.
	uint32_t input_count;

	//! Total number of outputs across all devices.
	uint32_t output_count;

	//! Index into @ref devices for the user's head device, or -1 if none.
	int32_t head;

	//! Index into @ref devices for the user's eye tracker, or -1 if none.
	int32_t eyes;

	//! Index into @ref devices for the user's left controller, or -1 if none.
	int32_t left;

	//! Index into @ref devices for the user's right controller, or -1 if none.
	int32_t right;

	//! Index into @ref devices for the user's gamepad, or -1 if none.
	int32_t gamepad;
};

/*!
 * A cache of flattened device inputs and outputs.
 *
 * @ingroup xrt_iface
 */
struct xrt_io_cache
{
	/*!
	 * Query the current cache layout.
	 *
	 * @param xic      Pointer to self.
	 * @param out_info Resulting layout snapshot.
	 */
	xrt_result_t (*get_info)(struct xrt_io_cache *xic, struct xrt_io_cache_info *out_info);

	/*!
	 * Read the current input and output values into caller-provided arrays.
	 *
	 * @param xic           Pointer to self.
	 * @param generation_id Layout generation from the most recent @ref get_info call.
	 * @param input_count   Capacity of @p inputs; must match the layout snapshot.
	 * @param inputs        Output array for input values.
	 * @param output_count  Capacity of @p outputs; must match the layout snapshot.
	 * @param outputs       Output array for output metadata.
	 * @param out_stale     Set to true when @p generation_id no longer matches the
	 *                      live layout; the arrays are not written in that case.
	 *
	 * @returns @ref XRT_ERROR_INVALID_ARGUMENT if @p input_count or @p output_count
	 *          do not match the current layout while the generation still matches.
	 */
	xrt_result_t (*read)(struct xrt_io_cache *xic,
	                     uint64_t generation_id,
	                     uint32_t input_count,
	                     struct xrt_input *inputs,
	                     uint32_t output_count,
	                     struct xrt_output *outputs,
	                     bool *out_stale);

	/*!
	 * Create a space for an input on a specific device.
	 *
	 * @param xic       Pointer to self.
	 * @param id        Device to create the space on.
	 * @param name      Input name to bind the space to.
	 * @param out_space Created space.
	 */
	xrt_result_t (*create_space)(struct xrt_io_cache *xic,
	                             struct xrt_device_id id,
	                             enum xrt_input_name name,
	                             struct xrt_space **out_space);

	/*!
	 * Set an output value on a specific device.
	 *
	 * @param xic   Pointer to self.
	 * @param id    Device to set the output on.
	 * @param name  Output name.
	 * @param value Value to write.
	 */
	xrt_result_t (*set_output)(struct xrt_io_cache *xic,
	                           struct xrt_device_id id,
	                           enum xrt_output_name name,
	                           const struct xrt_output_value *value);

	/*!
	 * Query the (device-wide) output limits for a device.
	 *
	 * @param xic        Pointer to self.
	 * @param id         Device to query.
	 * @param out_limits Resulting limits.
	 */
	xrt_result_t (*get_output_limits)(struct xrt_io_cache *xic,
	                                  struct xrt_device_id id,
	                                  struct xrt_output_limits *out_limits);

	/*!
	 * Destroy this cache.
	 *
	 * Code consuming this interface should use @ref xrt_io_cache_destroy.
	 */
	void (*destroy)(struct xrt_io_cache *xic);
};

/*!
 * @copydoc xrt_io_cache::get_info
 *
 * @public @memberof xrt_io_cache
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_io_cache_get_info(struct xrt_io_cache *xic, struct xrt_io_cache_info *out_info)
{
	return xic->get_info(xic, out_info);
}

/*!
 * @copydoc xrt_io_cache::read
 *
 * @public @memberof xrt_io_cache
 */
static inline xrt_result_t
xrt_io_cache_read(struct xrt_io_cache *xic,
                  uint64_t generation_id,
                  uint32_t input_count,
                  struct xrt_input *inputs,
                  uint32_t output_count,
                  struct xrt_output *outputs,
                  bool *out_stale)
{
	return xic->read(xic, generation_id, input_count, inputs, output_count, outputs, out_stale);
}

/*!
 * @copydoc xrt_io_cache::create_space
 *
 * @public @memberof xrt_io_cache
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_io_cache_create_space(struct xrt_io_cache *xic,
                          struct xrt_device_id id,
                          enum xrt_input_name name,
                          struct xrt_space **out_space)
{
	return xic->create_space(xic, id, name, out_space);
}

/*!
 * @copydoc xrt_io_cache::set_output
 *
 * @public @memberof xrt_io_cache
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_io_cache_set_output(struct xrt_io_cache *xic,
                        struct xrt_device_id id,
                        enum xrt_output_name name,
                        const struct xrt_output_value *value)
{
	return xic->set_output(xic, id, name, value);
}

/*!
 * @copydoc xrt_io_cache::get_output_limits
 *
 * @public @memberof xrt_io_cache
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_io_cache_get_output_limits(struct xrt_io_cache *xic, struct xrt_device_id id, struct xrt_output_limits *out_limits)
{
	return xic->get_output_limits(xic, id, out_limits);
}

/*!
 * Destroy an xrt_io_cache - helper function.
 *
 * @param[in,out] xic_ptr A pointer to the xrt_io_cache struct pointer.
 *
 * Will destroy the cache if `*xic_ptr` is not NULL. Will then set
 * `*xic_ptr` to NULL.
 *
 * @public @memberof xrt_io_cache
 */
XRT_NONNULL_ALL static inline void
xrt_io_cache_destroy(struct xrt_io_cache **xic_ptr)
{
	struct xrt_io_cache *xic = *xic_ptr;
	if (xic == NULL) {
		return;
	}

	*xic_ptr = NULL;
	xic->destroy(xic);
}

#ifdef __cplusplus
}
#endif
