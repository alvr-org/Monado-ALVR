// Copyright 2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Live stats tracking and printing.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_util
 */
#pragma once

#include "xrt/xrt_compiler.h"

#include "util/u_pretty_print.h"


#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Number of chars for the name of the live stats.
 *
 * @ingroup aux_util
 */
#define U_LIVE_STATS_NAME_COUNT (16)

/*!
 * Max number of values that can be put into the trackers.
 *
 * @ingroup aux_util
 */
#define U_LIVE_STATS_VALUE_COUNT (1024)

/*!
 * Struct to do live statistic tracking and printing of nano-seconds values,
 * used by amongst other the compositor pacing code.
 *
 * @ingroup aux_util
 */
struct u_live_stats_ns
{
	//! Small name used for printing.
	char name[U_LIVE_STATS_NAME_COUNT];

	//! Number of values currently in struct.
	uint32_t value_count;

	//! The values that will be used to calculate statistics.
	uint64_t values[U_LIVE_STATS_VALUE_COUNT];
};

/*!
 * Add a value to the live stats struct, returns true if the struct is full
 * either before or after adding the value.
 *
 * @public @memberof u_live_stats_ns
 * @ingroup aux_util
 */
static inline bool
u_ls_ns_add(struct u_live_stats_ns *uls, uint64_t value)
{
	if (uls->value_count >= ARRAY_SIZE(uls->values)) {
		return true;
	}

	uls->values[uls->value_count++] = value;

	if (uls->value_count >= ARRAY_SIZE(uls->values)) {
		return true;
	}

	return false;
}

/*!
 * Get the median, mean and worst of the current set of values,
 * then reset the struct.
 *
 * @public @memberof u_live_stats_ns
 */
void
u_ls_ns_get_and_reset(struct u_live_stats_ns *uls, uint64_t *out_median, uint64_t *out_mean, uint64_t *out_worst);

/*!
 * Prints a header that looks nice before @ref u_ls_print_and_reset,
 * adding details about columns. Doesn't include any newlines.
 *
 * @public @memberof u_live_stats_ns
 */
void
u_ls_ns_print_header(u_pp_delegate_t dg);

/*!
 * Prints the calculated values and resets the struct, can be used with
 * @ref u_ls_ns_print_header to get a nice header to the values. Doesn't
 * include any newlines.
 *
 * @public @memberof u_live_stats_ns
 */
void
u_ls_ns_print_and_reset(struct u_live_stats_ns *uls, u_pp_delegate_t dg);


#ifdef __cplusplus
}
#endif
