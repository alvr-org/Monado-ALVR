// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  String to enum conversion for XRT enums.
 * @author Jakob Bornecrantz <tbornecrantz@nvidia.com>
 * @ingroup aux_util
 */

#pragma once

#include "xrt/xrt_defines.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Parse an @ref xrt_input_name from its string representation.
 *
 * @return true if @p str was recognized and @p out_name was set.
 *
 * @ingroup aux_util
 */
bool
u_str_to_xrt_input_name(const char *str, enum xrt_input_name *out_name);

/*!
 * Parse an @ref xrt_output_name from its string representation.
 *
 * @return true if @p str was recognized and @p out_name was set.
 *
 * @ingroup aux_util
 */
bool
u_str_to_xrt_output_name(const char *str, enum xrt_output_name *out_name);

/*!
 * Parse an @ref xrt_input_name from its string representation.
 *
 * Returns @ref XRT_INPUT_GENERIC_TRACKER_POSE if @p str is not recognized.
 *
 * @ingroup aux_util
 */
enum xrt_input_name
u_str_to_xrt_input_name_or_default(const char *str);

/*!
 * Parse an @ref xrt_output_name from its string representation.
 *
 * Returns @ref XRT_OUTPUT_NAME_SIMPLE_VIBRATION if @p str is not recognized.
 *
 * @ingroup aux_util
 */
enum xrt_output_name
u_str_to_xrt_output_name_or_default(const char *str);

#ifdef __cplusplus
}
#endif
