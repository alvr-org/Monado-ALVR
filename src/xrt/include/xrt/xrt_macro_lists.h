// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief X_macro lists for various XRT enums.
 * @ingroup xrt_iface
 */

#pragma once

/*!
 * @defgroup xrt_macro_lists X_macro Lists
 * @ingroup xrt_iface
 * @brief X_macro definitions for XRT enums to facilitate code generation.
 *
 * These macros allow you to programmatically iterate over all values in an enum
 * without manually maintaining lists in multiple places.
 *
 * Each enum that has been processed will have a corresponding `<ENUM_NAME>_LIST` macro
 * defined in the included file.
 *
 * @section xrt_macro_usage Usage Examples
 *
 * Example 1 - Generate a string conversion function:
 * @code
 * const char* xrt_result_to_string(xrt_result_t result) {
 *     switch(result) {
 * #define CASE_STR(name) case name: return #name;
 *     XRT_RESULT_LIST(CASE_STR)
 * #undef CASE_STR
 *     default: return "UNKNOWN";
 *     }
 * }
 * @endcode
 *
 * Example 2 - Generate an array of all enum values:
 * @code
 * static const xrt_result_t all_results[] = {
 * #define ENUM_VALUE(name) name,
 *     XRT_RESULT_LIST(ENUM_VALUE)
 * #undef ENUM_VALUE
 * };
 * @endcode
 *
 * Example 3 - Count the number of enum values:
 * @code
 * #define COUNT_ONE(name) 1 +
 * static const int num_results = XRT_RESULT_LIST(COUNT_ONE) 0;
 * #undef COUNT_ONE
 * @endcode
 *
 * @{
 */

#include "xrt/xrt_macro_lists.h.inc"

/*!
 * @}
 */
