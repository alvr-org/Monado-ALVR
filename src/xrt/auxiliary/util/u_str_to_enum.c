// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  String to enum conversion for XRT enums.
 * @author Jakob Bornecrantz <tbornecrantz@nvidia.com>
 * @ingroup aux_util
 */

#include "xrt/xrt_macro_lists.h"

#include "util/u_str_to_enum.h"

#include <string.h>


/*
 *
 * Internal helpers.
 *
 */

// clang-format off
#define X_MACRO_STR_TO_XRT_INPUT_NAME(NAME) \
	if (strcmp(#NAME, str) == 0) { \
		*out_name = NAME; \
		return true; \
	}

#define X_MACRO_STR_TO_XRT_OUTPUT_NAME(NAME) \
	if (strcmp(#NAME, str) == 0) { \
		*out_name = NAME; \
		return true; \
	}
// clang-format on


/*
 *
 * 'Exported' functions.
 *
 */

bool
u_str_to_xrt_input_name(const char *str, enum xrt_input_name *out_name)
{
	XRT_INPUT_NAME_LIST(X_MACRO_STR_TO_XRT_INPUT_NAME)

	return false;
}

bool
u_str_to_xrt_output_name(const char *str, enum xrt_output_name *out_name)
{
	XRT_OUTPUT_NAME_LIST(X_MACRO_STR_TO_XRT_OUTPUT_NAME)

	return false;
}

enum xrt_input_name
u_str_to_xrt_input_name_or_default(const char *str)
{
	enum xrt_input_name name;

	if (u_str_to_xrt_input_name(str, &name)) {
		return name;
	}

	return XRT_INPUT_GENERIC_TRACKER_POSE;
}

enum xrt_output_name
u_str_to_xrt_output_name_or_default(const char *str)
{
	enum xrt_output_name name;

	if (u_str_to_xrt_output_name(str, &name)) {
		return name;
	}

	return XRT_OUTPUT_NAME_SIMPLE_VIBRATION;
}
