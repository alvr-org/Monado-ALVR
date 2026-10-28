// Copyright 2019-2020, Collabora, Ltd.
// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Implementation
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 */

#include "oxr_handle_base.h"

#include "util/u_debug.h"
#include "util/u_misc.h"

#include "oxr_objects.h"
#include "oxr_logger.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>


#define HANDLE_LIFECYCLE_LOG(log, ...)                                                                                 \
	if (log->inst != NULL && log->inst->lifecycle_verbose) {                                                       \
		oxr_log(log, " Handle Lifecycle: " __VA_ARGS__);                                                       \
	}

// Variation of HANDLE_LIFECYCLE_LOG() to wrap a handle free() which might
// potentially free the instance (in which logger info is stored).
#define HANDLE_LIFECYCLE_LOG_SCOPED_BEGIN(log)                                                                         \
	{                                                                                                              \
		const bool _log_lifecycle_verbose = log->inst != NULL && log->inst->lifecycle_verbose;
#define HANDLE_LIFECYCLE_LOG_SCOPED_END                                                                                \
	}                                                                                                              \
	(void)0
#define HANDLE_LIFECYCLE_LOG_SCOPED(log, ...)                                                                          \
	if (_log_lifecycle_verbose) {                                                                                  \
		oxr_log(log, " Handle Lifecycle: " __VA_ARGS__);                                                       \
	}


const char *
oxr_handle_state_to_string(enum oxr_handle_state state)
{
	switch (state) {
	case OXR_HANDLE_STATE_UNINITIALIZED: return "UNINITIALIZED";
	case OXR_HANDLE_STATE_LIVE: return "LIVE";
	case OXR_HANDLE_STATE_DESTROYED: return "DESTROYED";
	}

	return "<UNKNOWN>";
}


XrResult
oxr_handle_init(struct oxr_logger *log,
                struct oxr_handle_base *hb,
                uint64_t debug,
                oxr_handle_destroyer destroy,
                struct oxr_handle_parent_base *parent)
{
	assert(log != NULL);
	assert(hb != NULL);
	assert(destroy != NULL);
	assert(debug != 0);

	HANDLE_LIFECYCLE_LOG(log, "[init %p] Initializing handle, parent handle = %p", (void *)hb, (void *)parent);

	hb->state = OXR_HANDLE_STATE_UNINITIALIZED;

	if (parent != NULL) {
		if (parent->base.state != OXR_HANDLE_STATE_LIVE) {
			return oxr_error(log, XR_ERROR_RUNTIME_FAILURE,
			                 "Handle %p given parent %p in invalid state: %s", (void *)parent, (void *)hb,
			                 oxr_handle_state_to_string(parent->base.state));
		}

		if (!oxr_handle_array_add(&parent->children, hb)) {
			return oxr_error(log, XR_ERROR_OUT_OF_MEMORY,
			                 "Failed to add handle to parent's children array.");
		}
	}
	U_ZERO(hb);
	hb->debug = debug;
	hb->parent = parent;
	hb->state = OXR_HANDLE_STATE_LIVE;
	hb->destroy = destroy;
	return XR_SUCCESS;
}

XrResult
oxr_handle_parent_init(struct oxr_logger *log,
                       struct oxr_handle_parent_base *hpb,
                       uint64_t debug,
                       oxr_handle_destroyer destroy,
                       struct oxr_handle_parent_base *parent)
{
	XrResult result = oxr_handle_init(log, &hpb->base, debug, destroy, parent);
	if (result != XR_SUCCESS) {
		return result;
	}

	result = oxr_handle_array_init(log, &hpb->children);
	if (result != XR_SUCCESS) {
		return result;
	}

	return XR_SUCCESS;
}

XrResult
oxr_handle_allocate_and_init(struct oxr_logger *log,
                             size_t size,
                             uint64_t debug,
                             oxr_handle_destroyer destroy,
                             struct oxr_handle_parent_base *parent,
                             void **out)
{
	/*
	 * This allocation call, taking a size, not a type, is why this
	 * function isn't recommended for direct use.
	 */
	struct oxr_handle_base *hb = U_CALLOC_WITH_CAST(struct oxr_handle_base, size);
	XrResult result = oxr_handle_init(log, hb, debug, destroy, parent);
	if (result != XR_SUCCESS) {
		free(hb);
		return result;
	}
	*out = (void *)hb;
	return result;
}

XrResult
oxr_handle_parent_allocate_and_init(struct oxr_logger *log,
                                    size_t size,
                                    uint64_t debug,
                                    oxr_handle_destroyer destroy,
                                    struct oxr_handle_parent_base *parent,
                                    void **out)
{
	/*
	 * This allocation call, taking a size, not a type, is why this
	 * function isn't recommended for direct use.
	 */
	struct oxr_handle_parent_base *hpb = U_CALLOC_WITH_CAST(struct oxr_handle_parent_base, size);
	XrResult result = oxr_handle_parent_init(log, hpb, debug, destroy, parent);
	if (result != XR_SUCCESS) {
		free(hpb);
		return result;
	}
	*out = (void *)hpb;
	return result;
}

XrResult
oxr_handle_destroy_internal(struct oxr_logger *log, struct oxr_handle_base *hb, int level)
{
	HANDLE_LIFECYCLE_LOG(log,
	                     "[%d: destroying %p] Destroying handle and all "
	                     "contained handles (recursively)",
	                     level, (void *)hb);

	// Remove from parent, if any.
	if (hb->parent != NULL) {
		bool found = false;
		struct oxr_handle_parent_base *parent = hb->parent;

		for (uint32_t i = 0; i < parent->children.count; ++i) {
			if (parent->children.handles[i] == hb) {
				HANDLE_LIFECYCLE_LOG(log,
				                     "[%d: destroying %p] Removing handle from "
				                     "child slot %d in parent %p",
				                     level, (void *)hb, i, (void *)hb->parent);

				if (!oxr_handle_array_remove(&parent->children, i)) {
					return oxr_error(log, XR_ERROR_RUNTIME_FAILURE,
					                 "Failed to remove handle from parent's children array. This "
					                 "should never happen.");
				}
				found = true;
				break;
			}
		}
		if (!found) {
			return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Parent handle does not refer to this handle");
		}

		// clear parent pointer
		hb->parent = NULL;
	}

	// Might destroy instance, which log needs, so use secured variant
	HANDLE_LIFECYCLE_LOG_SCOPED_BEGIN(log)
	{
		// Destroy self
		HANDLE_LIFECYCLE_LOG_SCOPED(log, "[%d: destroying %p] Calling handle object destructor", level,
		                            (void *)hb);
		hb->state = OXR_HANDLE_STATE_DESTROYED;
		XrResult result = hb->destroy(log, hb);
		if (result != XR_SUCCESS) {
			return result;
		}
		HANDLE_LIFECYCLE_LOG_SCOPED(log, "r%d: destroying %p] Done", level, (void *)hb);
	}
	HANDLE_LIFECYCLE_LOG_SCOPED_END;

	return XR_SUCCESS;
}

XrResult
oxr_handle_parent_destroy_internal(struct oxr_logger *log, struct oxr_handle_parent_base *hpb, int level)
{
	// Destroy child handles
	XrResult result = oxr_handle_array_destroy(log, &hpb->children, level);
	if (result != XR_SUCCESS) {
		return result;
	}

	return oxr_handle_destroy_internal(log, &hpb->base, level);
}

XrResult
oxr_handle_destroy(struct oxr_logger *log, struct oxr_handle_base *hb)
{
	assert(log != NULL);
	assert(hb != NULL);

	// Might destroy instance, which log needs, so use secured variant
	HANDLE_LIFECYCLE_LOG_SCOPED_BEGIN(log)
	{
		HANDLE_LIFECYCLE_LOG_SCOPED(log, "[~: destroying %p] oxr_handle_destroy starting", (void *)hb);

		XrResult result = oxr_handle_destroy_internal(log, hb, 0);

		HANDLE_LIFECYCLE_LOG_SCOPED(log, "[~: destroying %p] oxr_handle_destroy finished", (void *)hb);

		return result;
	}
	HANDLE_LIFECYCLE_LOG_SCOPED_END;
}

XrResult
oxr_handle_parent_destroy(struct oxr_logger *log, struct oxr_handle_parent_base *hpb)
{
	assert(log != NULL);
	assert(hpb != NULL);

	// Might destroy instance, which log needs, so use secured variant
	HANDLE_LIFECYCLE_LOG_SCOPED_BEGIN(log)
	{
		HANDLE_LIFECYCLE_LOG_SCOPED(log, "[~: destroying %p] oxr_handle_parent_destroy starting", (void *)hpb);

		XrResult result = oxr_handle_parent_destroy_internal(log, hpb, 0);

		HANDLE_LIFECYCLE_LOG_SCOPED(log, "[~: destroying %p] oxr_handle_parent_destroy finished", (void *)hpb);

		return result;
	}
	HANDLE_LIFECYCLE_LOG_SCOPED_END;
}
