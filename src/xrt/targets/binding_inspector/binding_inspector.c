// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Inspect generated OpenXR binding templates and runtime registries.
 */

#include "xrt/xrt_instance.h"

#include "util/u_pretty_print.h"

#include "actions/oxr_binding_profile.h"
#include "actions/oxr_binding_registry.h"
#include "actions/oxr_rebinding_profile.h"
#include "common/oxr_defines.h"
#include "oxr_logger.h"
#include "path/oxr_path_store.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static void
file_sink(void *ptr, const char *str, size_t length)
{
	fwrite(str, 1, length, (FILE *)ptr);
}

static u_pp_delegate_t
file_delegate(FILE *file)
{
	return (u_pp_delegate_t){
	    .ptr = file,
	    .func = file_sink,
	};
}

xrt_result_t
xrt_instance_create(struct xrt_instance_info *ii, struct xrt_instance **out_xinst)
{
	(void)ii;
	*out_xinst = NULL;
	return XRT_ERROR_NOT_IMPLEMENTED;
}

static void
path_to_string(struct oxr_path_store *store, XrPath path, const char **out_str)
{
	size_t length = 0;
	if (oxr_path_store_get_string(store, path, out_str, &length) != XR_SUCCESS) {
		*out_str = "(invalid)";
	}
}

static void
dump_binding_point(u_pp_delegate_t dg, struct oxr_path_store *store, const struct oxr_binding_point *point)
{
	const char *path = NULL;
	const char *priority_group = NULL;
	path_to_string(store, point->path, &path);
	path_to_string(store, point->priority_group, &priority_group);
	u_pp(dg, "\n    [%s] name=%s group=%s", oxr_binding_point_kind_to_string(point->kind), path, priority_group);

	switch (point->kind) {
	case OXR_BINDING_POINT_KIND_INPUT:
		// We print these even if they are 0, to show any bugs.
		u_pp(dg, " input=");
		u_pp_xrt_input_name(dg, point->input.name);
		return;
	case OXR_BINDING_POINT_KIND_OUTPUT:
		// We print these even if they are 0, to show any bugs.
		u_pp(dg, " output=");
		u_pp_xrt_output_name(dg, point->output.name);
		return;
	case OXR_BINDING_POINT_KIND_DPAD:
		if (point->dpad.emulation != NULL && point->dpad.emulation->input != XRT_INPUT_NAME_INVALID) {
			const char *dpad_path = NULL;
			path_to_string(store, point->dpad.emulation->path, &dpad_path);
			u_pp(dg, " dpad-source=%s", dpad_path);
		}
		return;
	case OXR_BINDING_POINT_KIND_UPGRADE:
		if (point->upgrade.boolean_path != XR_NULL_PATH) {
			const char *str = NULL;
			path_to_string(store, point->upgrade.boolean_path, &str);
			u_pp(dg, "\n      boolean: %s", str);
		}
		if (point->upgrade.float_path != XR_NULL_PATH) {
			const char *str = NULL;
			path_to_string(store, point->upgrade.float_path, &str);
			u_pp(dg, "\n      float: %s", str);
		}
		if (point->upgrade.vec2_path != XR_NULL_PATH) {
			const char *str = NULL;
			path_to_string(store, point->upgrade.vec2_path, &str);
			u_pp(dg, "\n      vec2: %s", str);
		}
		if (point->upgrade.pose_path != XR_NULL_PATH) {
			const char *str = NULL;
			path_to_string(store, point->upgrade.pose_path, &str);
			u_pp(dg, "\n      pose: %s", str);
		}
		if (point->upgrade.haptic_path != XR_NULL_PATH) {
			const char *str = NULL;
			path_to_string(store, point->upgrade.haptic_path, &str);
			u_pp(dg, "\n      haptic: %s", str);
		}
		return;
	}
	u_pp(dg, "\n      unknown kind: %d", point->kind);
	assert(false); // Unreachable.
}

static bool
path_is_in_list(const XrPath *paths, size_t path_count, XrPath path)
{
	for (size_t i = 0; i < path_count; i++) {
		if (paths[i] == path) {
			return true;
		}
	}

	return false;
}

static void
dump_priority_groups(u_pp_delegate_t dg, struct oxr_path_store *store, const struct oxr_binding_profile *profile)
{
	XrPath *priority_groups = calloc(profile->binding_point_count, sizeof(XrPath));
	if (priority_groups == NULL && profile->binding_point_count > 0) {
		return;
	}
	size_t priority_group_count = 0;

	for (size_t i = 0; i < profile->binding_point_count; i++) {
		XrPath priority_group = profile->binding_points[i].priority_group;
		if (priority_group == XR_NULL_PATH ||
		    path_is_in_list(priority_groups, priority_group_count, priority_group)) {
			continue;
		}

		if (priority_group_count >= profile->binding_point_count) {
			break;
		}

		priority_groups[priority_group_count++] = priority_group;
	}

	if (priority_group_count == 0) {
		return;
	}

	u_pp(dg, "\n  Priority groups:");
	for (size_t i = 0; i < priority_group_count; i++) {
		const char *priority_group = NULL;
		path_to_string(store, priority_groups[i], &priority_group);
		u_pp(dg, "\n    %s", priority_group);
	}

	free(priority_groups);
}

static void
dump_profile(u_pp_delegate_t dg,
             struct oxr_path_store *store,
             const struct oxr_binding_profile *profile,
             bool only_upgrades)
{
	const char *profile_path = NULL;
	path_to_string(store, profile->path, &profile_path);
	u_pp(dg, "\nProfile: %s (%s)", profile_path, profile->localized_name);

	if (!only_upgrades) {
		dump_priority_groups(dg, store, profile);
	}

	if (!only_upgrades && profile->dpad_emulation_count > 0) {
		u_pp(dg, "\n  DPAD emulations:");
		for (size_t i = 0; i < profile->dpad_emulation_count; i++) {
			const struct oxr_dpad_emulation *dpad = &profile->dpad_emulations[i];
			const char *dpad_path = NULL;
			path_to_string(store, dpad->path, &dpad_path);
			u_pp(dg, "\n    %s input=", dpad_path);
			u_pp_xrt_input_name(dg, dpad->input);
			if (dpad->activate != XRT_INPUT_NAME_INVALID) {
				u_pp(dg, " activate=");
				u_pp_xrt_input_name(dg, dpad->activate);
			}
		}
	}

	u_pp(dg, "\n  Binding points:");
	for (size_t i = 0; i < profile->binding_point_count; i++) {
		const struct oxr_binding_point *point = &profile->binding_points[i];
		if (only_upgrades && point->kind != OXR_BINDING_POINT_KIND_UPGRADE) {
			continue;
		}
		dump_binding_point(dg, store, point);
	}
	u_pp(dg, "\n");
}

static void
dump_rebindings(u_pp_delegate_t dg, struct oxr_path_store *store, const struct oxr_binding_registry *registry)
{
	for (size_t i = 0; i < registry->rebinding_count; i++) {
		const struct oxr_rebinding_profile *rebinding = &registry->rebindings[i];
		const char *source_profile = NULL;
		const char *target_profile = NULL;
		const struct oxr_binding_profile *target_binding_profile = NULL;
		path_to_string(store, rebinding->source_profile_path, &source_profile);
		path_to_string(store, rebinding->target_profile_path, &target_profile);
		oxr_binding_registry_find_profile(registry, rebinding->target_profile_path, &target_binding_profile);

		u_pp(dg, "\nRebinding: %s -> %s target_device=%d", source_profile, target_profile,
		     target_binding_profile != NULL ? target_binding_profile->device_name : XRT_DEVICE_INVALID);
		for (size_t k = 0; k < rebinding->binding_count; k++) {
			const char *source = NULL;
			const char *target = NULL;
			path_to_string(store, rebinding->bindings[k].source_path, &source);
			path_to_string(store, rebinding->bindings[k].target_path, &target);
			u_pp(dg, "\n  %s -> %s", source, target);
		}
		u_pp(dg, "\n");
	}
}

static void
usage(const char *name)
{
	fprintf(stderr, "Usage: %s [--all] [--profile PATH] [--priority-groups] [--upgrades] [--rebindings]\n", name);
}

int
main(int argc, char **argv)
{
	bool dump_all = argc == 1;
	bool dump_priorities = false;
	bool dump_upgrades = false;
	bool dump_rebinding_profiles = false;
	const char *profile_filter = NULL;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--all") == 0) {
			dump_all = true;
		} else if (strcmp(argv[i], "--priority-groups") == 0) {
			dump_priorities = true;
		} else if (strcmp(argv[i], "--upgrades") == 0) {
			dump_upgrades = true;
		} else if (strcmp(argv[i], "--rebindings") == 0) {
			dump_rebinding_profiles = true;
		} else if (strcmp(argv[i], "--profile") == 0 && i + 1 < argc) {
			profile_filter = argv[++i];
		} else {
			usage(argv[0]);
			return 1;
		}
	}

	struct oxr_logger log;
	oxr_log_init(&log, "binding_inspector");

	struct oxr_path_store store = {0};
	if (oxr_path_store_init(&store) != XR_SUCCESS) {
		fprintf(stderr, "Failed to initialize path store\n");
		return 1;
	}

	struct oxr_binding_registry *registry = NULL;
	if (oxr_binding_registry_create(&log, &store, &registry) != XR_SUCCESS) {
		fprintf(stderr, "Failed to create binding registry\n");
		oxr_path_store_fini(&store);
		return 1;
	}

	u_pp_delegate_t dg = file_delegate(stdout);

	if (dump_all || profile_filter != NULL || dump_upgrades || dump_priorities) {
		for (size_t i = 0; i < registry->profile_count; i++) {
			struct oxr_binding_profile *profile = &registry->profiles[i];
			if (profile_filter != NULL && strcmp(profile->templ->path, profile_filter) != 0) {
				continue;
			}
			dump_profile(dg, &store, profile, dump_upgrades && !dump_all);
		}
	}

	if (dump_all || dump_rebinding_profiles) {
		dump_rebindings(dg, &store, registry);
	}

	oxr_binding_registry_unref(registry);
	oxr_path_store_fini(&store);

	return 0;
}
