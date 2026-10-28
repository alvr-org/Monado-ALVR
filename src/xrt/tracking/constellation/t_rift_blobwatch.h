// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header defining the Rift blobwatch parameters and creation function.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup xrt_iface
 */

#pragma once

#include "xrt/xrt_config_build.h"

#include "constellation/t_constellation_tracker.h"


#ifdef __cplusplus
extern "C" {
#endif

// On CV1, the camera tends to detect very little noise, so we can be much more lenient with what pixels are considered
// for being a blob.
#define RIFT_BLOBWATCH_PIXEL_THRESHOLD_CV1 0x24
// On DK2, the camera sees much more noise, so we need to be much stricter about what pixels get included as a blob.
#define RIFT_BLOBWATCH_PIXEL_THRESHOLD_DK2 0x7f
// Require a pixel of this brightness to be included in a blob at all, to help filter out general noise.
#define RIFT_BLOBWATCH_BLOB_REQUIRED_THRESHOLD 0x40
#define RIFT_BLOBWATCH_DEFAULT_MAX_MATCH_DIST 50.0f
// @todo Revisit this number.
#define RIFT_BLOBWATCH_DEFAULT_MAX_BLOB_WIDTH 50

struct t_rift_blobwatch_params
{
	//! Minimum pixel magnitude to be included in a blob at all
	uint8_t pixel_threshold;
	/*!
	 * Require at least 1 pixel over this threshold in a blob, allows for collecting fainter blobs, as long as they
	 * have a bright point somewhere, and helps to eliminate generally faint background noise
	 */
	uint8_t blob_required_threshold;
	/*!
	 * Maximum distance (in pixels) for matching a blob between frames.
	 * Matches beyond this distance are rejected, preventing newly appeared blobs
	 * from inheriting incorrect velocity from distant unrelated blobs.
	 */
	float max_match_dist;
	/*!
	 * The maximum width of a single blob in pixels. Blobs larger than this will be ignored. This is to prevent
	 * large areas of brightness, such as the sun or a window, from being considered as a single large blob.
	 */
	uint16_t max_blob_width;
	/*!
	 * The camera calibration of the camera this refers to.
	 */
	struct t_camera_calibration camera_calibration;
};

/*!
 * @public @memberof t_rift_blobwatch
 */
int
t_rift_blobwatch_create(const struct t_rift_blobwatch_params *params,
                        struct xrt_frame_context *xfctx,
                        struct t_blob_sink *blob_sink,
                        struct xrt_frame_sink **out_frame_sink,
                        struct t_blobwatch **out_blobwatch);

#ifdef XRT_FEATURE_RERUN
/*!
 * Sets data to allow the blobwatch to send data to Rerun.
 */
void
t_rift_blobwatch_set_rerun_data(struct t_blobwatch *tbw,
                                struct t_constellation_tracker *tracker,
                                uint32_t mosaic_index,
                                uint32_t camera_index);
#endif

#ifdef __cplusplus
}
#endif
