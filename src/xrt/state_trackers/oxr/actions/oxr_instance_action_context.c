// Copyright 2018-2024, Collabora, Ltd.
// Copyright 2023-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Holds per instance action context.
 * @ingroup oxr_main
 */

#include "oxr_instance_action_context.h"
#include "oxr_binding_registry.h"
#include "oxr_suggested_profile_array.h"

#include "util/u_misc.h"

#include "../oxr_logger.h"


static XrResult
context_init(struct oxr_logger *log, struct oxr_instance_action_context *context)
{
	XrResult ret = oxr_pair_hashset_init(log, &context->action_sets);
	if (ret != XR_SUCCESS) {
		return ret;
	}

	context->suggested_profiles.profiles = NULL;
	context->suggested_profiles.count = 0;

	return XR_SUCCESS;
}

static void
context_fini(struct oxr_instance_action_context *context)
{
	oxr_binding_registry_unref(context->binding_registry);
	context->binding_registry = NULL;
	oxr_pair_hashset_fini(&context->action_sets);
	oxr_suggested_profile_array_clear(&context->suggested_profiles);
}

static void
context_destroy_cb(struct oxr_refcounted *orc)
{
	struct oxr_instance_action_context *context = (struct oxr_instance_action_context *)orc;
	context_fini(context);
	free(context);
}

XrResult
oxr_instance_action_context_create(struct oxr_logger *log,
                                   struct oxr_binding_registry *registry,
                                   struct oxr_instance_action_context **out_context)
{
	struct oxr_instance_action_context *context = U_TYPED_CALLOC(struct oxr_instance_action_context);
	if (context == NULL) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to allocate instance action context");
	}

	context->base.base.count = 1;
	context->base.destroy = context_destroy_cb;

	XrResult ret = context_init(log, context);
	if (ret != XR_SUCCESS) {
		free(context);
		return ret;
	}

	context->binding_registry = registry;
	oxr_binding_registry_ref(context->binding_registry);

	*out_context = context;
	return XR_SUCCESS;
}
