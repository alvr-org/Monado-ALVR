// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief C++ base implementation of @ref xrt_io_cache.
 * @ingroup base
 */

#pragma once

#include "b_io_cache.h"

#include "glue/g_io_cache.hpp"


class IoCache : public xrt::util::IoCacheBase<IoCache>
{
public: // Methods
	IoCache(xrt_system_devices *xsysd, xrt_space_overseer *xso);

	xrt_result_t
	getInfo(xrt_io_cache_info *out_info);

	xrt_result_t
	read(uint64_t generation_id,
	     uint32_t input_count,
	     xrt_input *inputs,
	     uint32_t output_count,
	     xrt_output *outputs,
	     bool *out_stale);

	xrt_result_t
	createSpace(xrt_device_id id, xrt_input_name name, xrt_space **out_space);

	xrt_result_t
	setOutput(xrt_device_id id, xrt_output_name name, const xrt_output_value *value);

	xrt_result_t
	getOutputLimits(xrt_device_id id, xrt_output_limits *out_limits);

	static void
	destroyIoCache(struct xrt_io_cache *xic);


private: // Members
	struct xrt_system_devices *mSystemDevices{nullptr};
	struct xrt_space_overseer *mXso{nullptr};
};
