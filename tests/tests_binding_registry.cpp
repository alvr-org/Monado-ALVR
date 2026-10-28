// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0

#include "catch_amalgamated.hpp"

#include <oxr/actions/oxr_binding_profile.h>
#include <oxr/actions/oxr_binding_registry.h>
#include <oxr/actions/oxr_rebinding_profile.h>
#include <oxr/oxr_logger.h>
#include <oxr/path/oxr_path_store.h>

#include "oxr_extension_support.h"

#include <cstring>

TEST_CASE("binding_registry")
{
	struct oxr_logger log;
	oxr_log_init(&log, "test");

	struct oxr_path_store store = {};
	REQUIRE(oxr_path_store_init(&store) == XR_SUCCESS);

	struct oxr_binding_registry *registry = nullptr;
	REQUIRE(oxr_binding_registry_create(&log, &store, &registry) == XR_SUCCESS);
	REQUIRE(registry != nullptr);

	CHECK(registry->rebinding_count >= 2);

#ifdef OXR_HAVE_KHR_generic_controller
	XrPath generic_profile_path = XR_NULL_PATH;
	REQUIRE(oxr_path_store_get_or_create(&store, "/interaction_profiles/khr/generic_controller",
	                                     strlen("/interaction_profiles/khr/generic_controller"),
	                                     &generic_profile_path) == XR_SUCCESS);

	const struct oxr_binding_profile *generic_profile = nullptr;
	REQUIRE(oxr_binding_registry_find_profile(registry, generic_profile_path, &generic_profile));
	REQUIRE(generic_profile != nullptr);

	bool saw_upgrade = false;
	bool saw_dpad = false;
	bool saw_xy = false;
	for (size_t i = 0; i < generic_profile->binding_point_count; i++) {
		const struct oxr_binding_point *point = &generic_profile->binding_points[i];
		saw_upgrade |= point->kind == OXR_BINDING_POINT_KIND_UPGRADE;
		saw_dpad |= point->kind == OXR_BINDING_POINT_KIND_DPAD;
		const char *str = nullptr;
		size_t length = 0;
		REQUIRE(oxr_path_store_get_string(&store, point->path, &str, &length) == XR_SUCCESS);
		saw_xy |= strcmp(str, "/user/hand/left/input/thumbstick/xy") == 0;
	}

	CHECK(saw_upgrade);
	CHECK(saw_dpad);
	CHECK(saw_xy);

	const struct oxr_rebinding_profile *rebinding = nullptr;
	REQUIRE(oxr_binding_registry_find_rebinding_from_source_for_device(registry, generic_profile_path,
	                                                                   XRT_DEVICE_INDEX_CONTROLLER, &rebinding));
	REQUIRE(rebinding != nullptr);

	XrPath source_primary = XR_NULL_PATH;
	XrPath target_a = XR_NULL_PATH;
	XrPath rebinding_target = XR_NULL_PATH;
	REQUIRE(oxr_path_store_get_or_create(&store, "/user/hand/right/input/primary/click",
	                                     strlen("/user/hand/right/input/primary/click"),
	                                     &source_primary) == XR_SUCCESS);
	REQUIRE(oxr_path_store_get_or_create(&store, "/user/hand/right/input/a/click",
	                                     strlen("/user/hand/right/input/a/click"), &target_a) == XR_SUCCESS);
	REQUIRE(oxr_rebinding_profile_find_target(rebinding, source_primary, &rebinding_target));
	CHECK(rebinding_target == target_a);
#endif // OXR_HAVE_KHR_generic_controller

	oxr_binding_registry_unref(registry);
	oxr_path_store_fini(&store);
}
