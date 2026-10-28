// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Rerun recorder logic for blobwatches.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup tracking
 */

#pragma once

#include "constellation/t_constellation_tracker.h"


#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Pushes a camera frame from an image-based blobwatch to the constellation tracker.
 */
void
constellation_tracker_rerun_blobwatch_push_frame(struct t_constellation_tracker *tracker,
                                                 uint32_t mosaic_index,
                                                 uint32_t camera_index,
                                                 struct xrt_frame *frame);

#ifdef __cplusplus
}
#endif
