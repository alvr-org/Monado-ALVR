// Copyright 2019-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Prints information about the system.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 */

#include "xrt/xrt_space.h"
#include "xrt/xrt_system.h"
#include "xrt/xrt_device.h"
#include "xrt/xrt_prober.h"
#include "xrt/xrt_instance.h"
#include "xrt/xrt_config_drivers.h"

#include "util/u_git_tag.h"

#include "cli_common.h"

#include <string.h>
#include <stdio.h>

#define P(...) printf(__VA_ARGS__)
#define PT(...) printf("\t" __VA_ARGS__)
#define PTT(...) printf("\t\t" __VA_ARGS__)



static int
do_exit(struct xrt_instance **xi_ptr, int ret)
{
	xrt_instance_destroy(xi_ptr);

	printf(" :: Exiting '%i'\n", ret);

	return ret;
}

int
cli_cmd_info(int argc, const char **argv)
{
	struct xrt_instance *xi = NULL;
	xrt_result_t xret = XRT_SUCCESS;
	int ret = 0;

	P(" :: Basic info\n");
	PT("runtime: '%s'\n", u_runtime_description);
	PT("git-tag: '%s'\n", u_git_tag);


	/*
	 * Initialize the instance and prober.
	 */

	P(" :: Creating instance and prober\n");

	ret = xrt_instance_create(NULL, &xi);
	if (ret != 0) {
		PT("Failed to create instance!");
		return do_exit(&xi, 0);
	}

	PT("instance: Ok\n");

	struct xrt_prober *xp = NULL;
	xret = xrt_instance_get_prober(xi, &xp);
	if (xret != XRT_SUCCESS) {
		PT("No xrt_prober could be created!\n");
		return do_exit(&xi, -1);
	}

	PT("prober: Ok\n");


	/*
	 * List builders, drivers and any modules.
	 */

	P(" :: Built builders\n");

	size_t builder_count;
	struct xrt_builder **builders;
	size_t num_entries;
	struct xrt_prober_entry **entries;
	struct xrt_auto_prober **auto_probers;
	ret = xrt_prober_get_builders(xp, &builder_count, &builders, &num_entries, &entries, &auto_probers);
	if (ret != 0) {
		PT("Failed to get builders!");
		do_exit(&xi, ret);
	}

	for (size_t i = 0; i < builder_count; i++) {
		struct xrt_builder *builder = builders[i];
		if (builder == NULL) {
			continue;
		}

		PT("%s: %s\n", builder->identifier, builder->name);

		for (uint32_t k = 0; k < builder->driver_identifier_count; k++) {
			PTT("%s\n", builder->driver_identifiers[k]);
		}
	}

	P(" :: Built auto probers\n");
	for (size_t i = 0; i < XRT_MAX_AUTO_PROBERS; i++) {
		if (auto_probers[i] == NULL) {
			continue;
		}

		PT("%s\n", auto_probers[i]->name);
	}

	P(" :: Built modules and drivers\n");

#ifdef XRT_BUILD_DRIVER_HANDTRACKING
	PT("ht\n");
#endif

#ifdef XRT_BUILD_DRIVER_DEPTHAI
	PT("depthai\n");
#endif

#ifdef XRT_BUILD_DRIVER_V4L2
	PT("v4l2\n");
#endif

#ifdef XRT_BUILD_DRIVER_VF
	PT("vf\n");
#endif


	/*
	 * Dump hardware devices connected.
	 */

	P(" :: Dumping devices\n");

	// Need to first probe for devices.
	xrt_prober_probe(xp);

	// Then we can dump then.
	xrt_prober_dump(xp, true);


	/*
	 * Done.
	 */

	// End of program
	P(" :: All ok, shutting down.\n");

	// Finally done
	return do_exit(&xi, 0);
}
