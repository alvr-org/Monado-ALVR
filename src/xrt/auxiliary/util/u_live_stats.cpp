// Copyright 2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Live stats tracking and printing.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_util
 */

#include "u_live_stats.h"

#include <inttypes.h>
#include <algorithm>


/*
 *
 * Helper functions.
 *
 */

static void
print_as_ms(u_pp_delegate_t dg, uint64_t value_ns)
{
	uint64_t in_us = value_ns / 1000;
	uint64_t in_ms = in_us / 1000;
	uint64_t in_1_000_ms = in_ms / 1000;
	uint64_t in_1_000_000_ms = in_1_000_ms / 1000;

	// Prints " M'TTT'###.FFFms"

	// " M'"
	if (in_1_000_000_ms >= 1) {
		u_pp(dg, " %" PRIu64 "'", in_1_000_000_ms);
	} else {
		//       " M'"
		u_pp(dg, "   ");
	}

	// "TTT'"
	if (in_1_000_ms >= 1000) {
		// Need to pad with zeros
		u_pp(dg, "%03" PRIu64 "'", in_1_000_ms % 1000);
	} else if (in_1_000_ms >= 1) {
		// Pad with spaces, we need to write a number.
		u_pp(dg, "%3" PRIu64 "'", in_1_000_ms);
	} else {
		//       "TTT'"
		u_pp(dg, "    ");
	}

	// "###"
	if (in_ms >= 1000) {
		// Need to pad with zeros
		u_pp(dg, "%03" PRIu64, in_ms % 1000);
	} else {
		// Pad with spaces, always need a numbere here.
		u_pp(dg, "%3" PRIu64, in_ms % 1000);
	}

	// ".FFFms"
	u_pp(dg, ".%03" PRIu64 "ms", in_us % 1000);
}


/*
 *
 * 'Exported' functions.
 *
 */

extern "C" void
u_ls_ns_get_and_reset(struct u_live_stats_ns *uls, uint64_t *out_median, uint64_t *out_mean, uint64_t *out_worst)
{
	uint32_t count = uls->value_count;

	if (count == 0) {
		*out_median = 0;
		*out_mean = 0;
		*out_worst = 0;
		return;
	}

	std::sort(&uls->values[0], &uls->values[count]);

	uint64_t worst = uls->values[count - 1]; // Always greater then 0.
	uint64_t median = uls->values[count / 2];

	uint64_t mean = 0;
	for (uint32_t i = 0; i < count; ++i) {
		mean += uls->values[i] / count;
	}

	uls->value_count = 0;
	*out_median = median;
	*out_mean = mean;
	*out_worst = worst;
}

extern "C" void
u_ls_ns_print_header(u_pp_delegate_t dg)
{
	//       "xxxxYYYYzzzzWWWW M'TTT'###.FFFms M'TTT'###.FFFms M'TTT'###.FFFms"
	u_pp(dg, "            name          median            mean           worst");
}

extern "C" void
u_ls_ns_print_and_reset(struct u_live_stats_ns *uls, u_pp_delegate_t dg)
{
	uint64_t median, mean, worst;
	u_ls_ns_get_and_reset(uls, &median, &mean, &worst);

	u_pp(dg, "%16s", uls->name);
	print_as_ms(dg, median);
	print_as_ms(dg, mean);
	print_as_ms(dg, worst);
}
