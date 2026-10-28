// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Per-session snapshot wrapper around @ref xrt_io_cache.
 * @ingroup oxr_main
 */

#include "xrt/xrt_system.h"

#include "util/u_misc.h"

#include "../oxr_logger.h"

#include "oxr_io_cache.h"

#include <stdlib.h>


/*!
 * Bounded number of get_info/allocate/read attempts when the device layout
 * keeps changing under us (rapid hotplug).
 */
#define OXR_IO_CACHE_RECREATE_MAX_ATTEMPTS 8

XrResult
oxr_io_cache_recreate(struct oxr_logger *log, struct oxr_io_cache *cache)
{
	XrResult ret = XR_SUCCESS;

	for (int attempt = 0; attempt < OXR_IO_CACHE_RECREATE_MAX_ATTEMPTS; attempt++) {
		xrt_result_t xret = xrt_io_cache_get_info(cache->xic, &cache->info);
		if (xret != XRT_SUCCESS) {
			ret = oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "xrt_io_cache_get_info failed");
			goto error;
		}

		free(cache->inputs);
		free(cache->outputs);
		cache->inputs = NULL;
		cache->outputs = NULL;

		if (cache->info.input_count > 0) {
			cache->inputs = U_TYPED_ARRAY_CALLOC(struct xrt_input, cache->info.input_count);
			if (cache->inputs == NULL) {
				ret = oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to allocate io cache inputs");
				goto error;
			}
		}

		if (cache->info.output_count > 0) {
			cache->outputs = U_TYPED_ARRAY_CALLOC(struct xrt_output, cache->info.output_count);
			if (cache->outputs == NULL) {
				ret = oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to allocate io cache outputs");
				goto error;
			}
		}

		/*
		 * Prime the snapshot so the input/output names are populated:
		 * action bindings are resolved by name against these arrays, and
		 * binding happens before the first xrSyncActions sync.
		 */
		bool stale = false;
		ret = oxr_io_cache_sync(log, cache, &stale);
		if (ret != XR_SUCCESS) {
			goto error;
		}
		if (!stale) {
			return XR_SUCCESS;
		}

		// The layout changed under us (hotplug); re-query and retry.
	}

	/*
	 * Could not get a stable snapshot in the allotted attempts; the names
	 * may be zeroed. The next xrSyncActions will observe the cache as stale
	 * and recreate again.
	 */
	return XR_SUCCESS;

error:
	free(cache->inputs);
	free(cache->outputs);
	cache->inputs = NULL;
	cache->outputs = NULL;
	return ret;
}

XrResult
oxr_io_cache_init(struct oxr_io_cache *cache, struct xrt_system_devices *xsysd, struct xrt_space_overseer *xso)
{
	*cache = (struct oxr_io_cache){0};

	xrt_result_t xret = xrt_system_devices_create_io_cache(xsysd, xso, &cache->xic);
	if (xret != XRT_SUCCESS) {
		return XR_ERROR_RUNTIME_FAILURE;
	}

	struct oxr_logger log;
	oxr_log_init(&log, "oxr_io_cache_init");
	XrResult ret = oxr_io_cache_recreate(&log, cache);
	if (ret != XR_SUCCESS) {
		oxr_io_cache_fini(cache);
		return ret;
	}

	return XR_SUCCESS;
}

void
oxr_io_cache_fini(struct oxr_io_cache *cache)
{
	xrt_io_cache_destroy(&cache->xic);

	free(cache->inputs);
	free(cache->outputs);
	cache->inputs = NULL;
	cache->outputs = NULL;
}

XrResult
oxr_io_cache_sync(struct oxr_logger *log, struct oxr_io_cache *cache, bool *out_stale)
{
	*out_stale = false;
	if (cache->xic == NULL) {
		return XR_SUCCESS;
	}

	xrt_result_t xret = xrt_io_cache_read(  //
	    cache->xic,                         //
	    cache->info.generation_id,          //
	    (uint32_t)cache->info.input_count,  //
	    cache->inputs,                      //
	    (uint32_t)cache->info.output_count, //
	    cache->outputs,                     //
	    out_stale);                         //
	if (xret != XRT_SUCCESS) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "xrt_io_cache_read failed");
	}

	return XR_SUCCESS;
}

const struct xrt_io_cache_device *
oxr_io_cache_find_device(const struct oxr_io_cache *cache, struct xrt_device_id id)
{
	for (uint32_t i = 0; i < cache->info.device_count; i++) {
		if (cache->info.devices[i].id.val == id.val) {
			return &cache->info.devices[i];
		}
	}

	return NULL;
}

struct xrt_input *
oxr_io_cache_find_input(const struct oxr_io_cache *cache,
                        const struct xrt_io_cache_device *dev,
                        enum xrt_input_name name)
{
	struct xrt_input *dev_inputs = oxr_io_cache_device_inputs(cache, dev);
	for (uint32_t k = 0; k < dev->input_count; k++) {
		if (dev_inputs[k].name == name) {
			return &dev_inputs[k];
		}
	}

	return NULL;
}

bool
oxr_io_cache_has_output(const struct oxr_io_cache *cache,
                        const struct xrt_io_cache_device *dev,
                        enum xrt_output_name name)
{
	struct xrt_output *dev_outputs = oxr_io_cache_device_outputs(cache, dev);
	for (uint32_t k = 0; k < dev->output_count; k++) {
		if (dev_outputs[k].name == name) {
			return true;
		}
	}

	return false;
}
