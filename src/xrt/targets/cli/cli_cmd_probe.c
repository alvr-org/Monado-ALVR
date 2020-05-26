// Copyright 2019, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Just does a probe.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 */

#include <string.h>
#include <stdio.h>

#include "xrt/xrt_instance.h"
#include "xrt/xrt_device.h"
#include "cli_common.h"


static int
do_exit(struct xrt_instance **xi_ptr, int ret)
{
	xrt_instance_destroy(xi_ptr);

	printf(" :: Exiting '%i'\n", ret);

	return ret;
}

#define NUM_XDEVS 32

int
cli_cmd_probe(int argc, const char **argv)
{
	struct xrt_device *xdevs[NUM_XDEVS] = {0};
	struct xrt_instance *xi = NULL;
	int ret = 0;

	// Initialize the prober.
	printf(" :: Creating instance!\n");

	ret = xrt_instance_create(&xi);
	if (ret != 0) {
		return do_exit(&xi, 0);
	}

	// Need to prime the prober with devices before dumping and listing.
	printf(" :: Probing and selecting!\n");

	ret = xrt_instance_select(xi, xdevs, NUM_XDEVS);
	if (ret != 0) {
		return do_exit(&xi, ret);
	}

	// End of program
	printf(" :: All ok, shutting down.\n");

	for (size_t i = 0; i < NUM_XDEVS; i++) {
		if (xdevs[i] == NULL) {
			continue;
		}

		printf("\tDestroying '%s'\n", xdevs[i]->str);
		xrt_device_destroy(&xdevs[i]);
	}

	// Finally done
	return do_exit(&xi, 0);
}
