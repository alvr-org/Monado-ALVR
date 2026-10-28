// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Base implementation of @ref xrt_io_cache.
 * @ingroup base
 */

#include "xrt/xrt_device.h"
#include "xrt/xrt_space.h"
#include "xrt/xrt_system.h"

#include "util/u_debug.h"
#include "util/u_logging.h"
#include "util/u_misc.h"

#include "b_io_cache.h"
#include "b_io_cache.hpp"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>


DEBUG_GET_ONCE_LOG_OPTION(io_cache_log, "B_IO_CACHE_LOG", U_LOGGING_INFO)

#define BIC_TRACE(...) U_LOG_IFL_T(debug_get_log_option_io_cache_log(), __VA_ARGS__)
#define BIC_DEBUG(...) U_LOG_IFL_D(debug_get_log_option_io_cache_log(), __VA_ARGS__)
#define BIC_INFO(...) U_LOG_IFL_I(debug_get_log_option_io_cache_log(), __VA_ARGS__)
#define BIC_WARN(...) U_LOG_IFL_W(debug_get_log_option_io_cache_log(), __VA_ARGS__)
#define BIC_ERROR(...) U_LOG_IFL_E(debug_get_log_option_io_cache_log(), __VA_ARGS__)

namespace {

static xrt_device *
resolve_xdev(xrt_system_devices *xsysd, xrt_device_id id)
{
	for (uint32_t i = 0; i < xsysd->static_xdev_count; i++) {
		xrt_device *xdev = xsysd->static_xdevs[i];
		if (xdev != nullptr && xdev->id.val == id.val) {
			return xdev;
		}
	}

	return nullptr;
}

static xrt_result_t
create_io_cache(xrt_system_devices *xsysd, xrt_space_overseer *xso, xrt_io_cache **out_xic)
{
	assert(xsysd != nullptr);
	assert(xso != nullptr);
	assert(out_xic != nullptr);

	*out_xic = nullptr;

	auto io_cache = std::make_unique<IoCache>(xsysd, xso);

	*out_xic = io_cache.release()->getXIC();

	BIC_INFO("Created io cache for %u static devices", (uint32_t)xsysd->static_xdev_count);

	return XRT_SUCCESS;
}

} // namespace

IoCache::IoCache(xrt_system_devices *xsysd, xrt_space_overseer *xso) : mSystemDevices(xsysd), mXso(xso) {}

xrt_result_t
IoCache::getInfo(xrt_io_cache_info *out_info)
{
	assert(out_info != nullptr);
	assert(mSystemDevices->static_xdev_count <= XRT_SYSTEM_MAX_DEVICES);

	*out_info = {
	    .generation_id = 0,
	    .devices = {},
	    .device_count = 0,
	    .input_count = 0,
	    .output_count = 0,
	    .head = -1,
	    .eyes = -1,
	    .left = -1,
	    .right = -1,
	    .gamepad = -1,
	};

	/*
	 * Map from static_xdevs index to the compacted devices[] index (skips
	 * null slots); -1 means the static device is not present in the table.
	 */
	int32_t static_to_cache[XRT_SYSTEM_MAX_DEVICES];
	for (uint32_t i = 0; i < XRT_SYSTEM_MAX_DEVICES; i++) {
		static_to_cache[i] = -1;
	}

	uint32_t total_inputs = 0;
	uint32_t total_outputs = 0;

	for (uint32_t i = 0; i < mSystemDevices->static_xdev_count; i++) {
		xrt_device *xdev = mSystemDevices->static_xdevs[i];
		if (xdev == nullptr) {
			continue;
		}

		xrt_io_cache_device *dev = &out_info->devices[out_info->device_count];

		dev->id = xdev->id;
		dev->name = xdev->name;
		snprintf(dev->str, sizeof(dev->str), "%s", xdev->str);

		dev->first_input = total_inputs;
		dev->input_count = (uint32_t)xdev->input_count;
		dev->first_output = total_outputs;
		dev->output_count = (uint32_t)xdev->output_count;

		total_inputs += dev->input_count;
		total_outputs += dev->output_count;

		static_to_cache[i] = (int32_t)out_info->device_count;

		out_info->device_count++;
	}

	out_info->input_count = total_inputs;
	out_info->output_count = total_outputs;

	/*
	 * The controller role assignment (and the cache generation) tracks the
	 * static system devices; the device layout above never changes for this
	 * implementation, so a role change is the only thing that bumps the
	 * generation.
	 */
	xrt_system_roles roles = XRT_SYSTEM_ROLES_INIT;
	xrt_result_t xret = xrt_system_devices_get_roles(mSystemDevices, &roles);
	if (xret != XRT_SUCCESS) {
		BIC_WARN("Failed to get device roles: %i", xret);
		return xret;
	}

	out_info->generation_id = roles.generation_id;

	if (roles.left >= 0 && roles.left < (int32_t)XRT_SYSTEM_MAX_DEVICES) {
		out_info->left = static_to_cache[roles.left];
	}
	if (roles.right >= 0 && roles.right < (int32_t)XRT_SYSTEM_MAX_DEVICES) {
		out_info->right = static_to_cache[roles.right];
	}
	if (roles.gamepad >= 0 && roles.gamepad < (int32_t)XRT_SYSTEM_MAX_DEVICES) {
		out_info->gamepad = static_to_cache[roles.gamepad];
	}

	/*
	 * Head and eyes are static roles stored as device pointers (not indices),
	 * so locate their compacted cache entry by device id. They are duplicated
	 * here so a consumer can resolve every top-level path from the cache.
	 */
	auto find_cache_index = [&](const xrt_device *xdev) -> int32_t {
		if (xdev == nullptr) {
			return -1;
		}
		for (uint32_t i = 0; i < out_info->device_count; i++) {
			if (out_info->devices[i].id.val == xdev->id.val) {
				return (int32_t)i;
			}
		}
		return -1;
	};
	out_info->head = find_cache_index(mSystemDevices->static_roles.head);
	out_info->eyes = find_cache_index(mSystemDevices->static_roles.eyes);

	return XRT_SUCCESS;
}

xrt_result_t
IoCache::read(uint64_t generation_id,
              uint32_t input_count,
              xrt_input *inputs,
              uint32_t output_count,
              xrt_output *outputs,
              bool *out_stale)
{
	assert(out_stale != nullptr);
	*out_stale = false;

	xrt_system_roles roles = XRT_SYSTEM_ROLES_INIT;
	xrt_result_t roles_ret = xrt_system_devices_get_roles(mSystemDevices, &roles);
	if (roles_ret != XRT_SUCCESS) {
		BIC_WARN("Failed to get device roles: %i", roles_ret);
		return roles_ret;
	}

	if (generation_id != roles.generation_id) {
		*out_stale = true;
		return XRT_SUCCESS;
	}

	uint32_t total_inputs = 0;
	uint32_t total_outputs = 0;

	for (uint32_t i = 0; i < mSystemDevices->static_xdev_count; i++) {
		xrt_device *xdev = mSystemDevices->static_xdevs[i];
		if (xdev == nullptr) {
			continue;
		}

		total_inputs += (uint32_t)xdev->input_count;
		total_outputs += (uint32_t)xdev->output_count;
	}

	if (input_count != total_inputs || output_count != total_outputs) {
		return XRT_ERROR_INVALID_ARGUMENT;
	}

	uint32_t first_input = 0;
	uint32_t first_output = 0;

	for (uint32_t i = 0; i < mSystemDevices->static_xdev_count; i++) {
		xrt_device *xdev = mSystemDevices->static_xdevs[i];
		if (xdev == nullptr) {
			continue;
		}

		xrt_result_t xret = xrt_device_update_inputs(xdev);
		if (xret != XRT_SUCCESS) {
			BIC_WARN("Failed to update inputs for device '%s': %i", xdev->str, xret);
		}

		const uint32_t dev_input_count = (uint32_t)xdev->input_count;
		const uint32_t dev_output_count = (uint32_t)xdev->output_count;

		if (dev_input_count > 0) {
			memcpy(&inputs[first_input], xdev->inputs, dev_input_count * sizeof(xrt_input));
		}

		if (dev_output_count > 0) {
			memcpy(&outputs[first_output], xdev->outputs, dev_output_count * sizeof(xrt_output));
		}

		first_input += dev_input_count;
		first_output += dev_output_count;
	}

	return XRT_SUCCESS;
}

xrt_result_t
IoCache::createSpace(xrt_device_id id, xrt_input_name name, xrt_space **out_space)
{
	xrt_device *xdev = resolve_xdev(mSystemDevices, id);
	if (xdev == nullptr) {
		return XRT_ERROR_INVALID_ARGUMENT;
	}

	return xrt_space_overseer_create_pose_space(mXso, xdev, name, out_space);
}

xrt_result_t
IoCache::setOutput(xrt_device_id id, xrt_output_name name, const xrt_output_value *value)
{
	assert(value != nullptr);

	xrt_device *xdev = resolve_xdev(mSystemDevices, id);
	if (xdev == nullptr) {
		return XRT_ERROR_INVALID_ARGUMENT;
	}

	return xrt_device_set_output(xdev, name, value);
}

xrt_result_t
IoCache::getOutputLimits(xrt_device_id id, xrt_output_limits *out_limits)
{
	xrt_device *xdev = resolve_xdev(mSystemDevices, id);
	if (xdev == nullptr) {
		return XRT_ERROR_INVALID_ARGUMENT;
	}

	return xrt_device_get_output_limits(xdev, out_limits);
}

void
IoCache::destroyIoCache(struct xrt_io_cache *xic)
{
	delete IoCache::fromXIC(xic);
}


/*
 *
 * 'Exported' functions.
 *
 */

extern "C" xrt_result_t
b_io_cache_create(xrt_system_devices *xsysd, xrt_space_overseer *xso, xrt_io_cache **out_xic)
{
	return create_io_cache(xsysd, xso, out_xic);
}
