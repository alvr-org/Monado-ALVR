// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0

#include "util/u_git_tag.h"
#include "util/u_logging.h"

int
main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	U_LOG_E("Hello World %u.%u.%u (%s)", u_version_major, u_version_minor, u_version_patch, u_git_tag);

	return 0;
}
