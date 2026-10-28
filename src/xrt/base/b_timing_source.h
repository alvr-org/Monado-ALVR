// Copyright 2026, Beyley Cardellio
// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Base timing source code
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup base
 */

#pragma once

#include "xrt/xrt_frame.h"
#include "xrt/xrt_results.h"

#include "tracking/t_time_sync.h"


#ifdef __cplusplus
extern "C" {
#endif

xrt_result_t
b_timing_source_create(struct xrt_frame_context *xfctx,
                       struct t_timing_event_sink **out_sink,
                       struct t_timing_event_source **out_source);

#ifdef __cplusplus
}
#endif
