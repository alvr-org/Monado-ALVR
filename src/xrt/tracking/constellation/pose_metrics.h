// Copyright 2020-2023 Jan Schmidt
// Copyright 2025-2026 Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Metrics for constellation tracking poses
 * @author Jan Schmidt <jan@centricular.com>
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup tracking
 */

#pragma once

#include "xrt/xrt_defines.h"

#include "tracking/t_constellation.h"

#include "camera_model.h"


#ifdef __cplusplus
extern "C" {
#endif

#define MAX_OBJECT_LEDS 64

#define WORST_REPROJECTION_ERROR 10.0
//! Device origin poses closer than this are probably invalid solves.
#define CLOSEST_ACCEPTABLE_POSE_M 0.05
//! Poses farther away than this are definitely invalid solves.
#define FARTHEST_ACCEPTABLE_POSE_M 15.0

struct pose_rect
{
	double left;
	double top;
	double right;
	double bottom;
};

XRT_MAYBE_UNUSED static bool
pose_rect_has_area(struct pose_rect *rect)
{
	return rect->left != rect->right && rect->top != rect->bottom;
}

enum pose_match_flags
{
	//! A reasonable pose match - most LEDs matched to within a few pixels error
	POSE_MATCH_GOOD = 1 << 0,
	//! A strong pose match is a match with very low error
	POSE_MATCH_STRONG = 1 << 1,
	//! The position of the pose matched the prior well
	POSE_MATCH_POSITION = 1 << 2,
	//! The orientation of the pose matched the prior well
	POSE_MATCH_ORIENT = 1 << 3,
	//! If a pose prior was supplied when calculating the score, then rot/trans_error are set
	POSE_HAD_PRIOR = 1 << 4,
	//! The LED IDs on the blobs all matched the LEDs we thought (or were unassigned)
	POSE_MATCH_LED_IDS = 1 << 5,
	/*!
	 * Set when the pose cannot explain the device entirely on it's own. One case is when too few blobs matched for
	 * the reprojection error to say anything about the fit, a P3P solve or fewer. Such a solve reproduces its own
	 * points by construction, so its error is near zero whether or not the pose is right.
	 *
	 * Another case this may be triggered is when the amount of blobs is too low for the LED model in question to be
	 * confident about a slow solve. See @ref t_constellation_tracker_led_model_match_parameters.
	 */
	POSE_MATCH_DEGENERATE = 1 << 6,
};

#define POSE_SET_FLAGS(score, f) ((score)->match_flags |= (f))
#define POSE_CLEAR_FLAGS(score, f) ((score)->match_flags &= ~(f))
#define POSE_HAS_FLAGS(score, f) (((score)->match_flags & (f)) == (f))

#define POSE_CLEAR_VALID(score) POSE_CLEAR_FLAGS(score, POSE_MATCH_GOOD | POSE_MATCH_STRONG)

struct pose_metrics
{
	enum pose_match_flags match_flags;

	uint32_t matched_blobs;
	uint32_t unmatched_blobs;
	uint32_t visible_leds;

	double reprojection_error;

	//! Rotation error (compared to a prior)
	struct xrt_vec3 orient_error;
	//! Translation error (compared to a prior)
	struct xrt_vec3 pos_error;
};

struct pose_metrics_visible_led_info
{
	struct t_constellation_tracker_led *led;
	double led_radius_px_undistorted;   //< Expected max size of the LED in undistorted pixels at that distance
	struct xrt_vec2 pos_px_undistorted; //< Projected position of the LED (undistorted pixels)
	struct xrt_vec3 pos_m;              //< Projected physical position of the LED (metres)
	double facing_dot;                  //< Dot product between LED and camera
	struct t_blob *matched_blob;
};

struct pose_metrics_blob_match_info
{
	struct pose_metrics_visible_led_info visible_leds[MAX_OBJECT_LEDS];
	int num_visible_leds;

	bool all_led_ids_matched;
	uint32_t matched_blobs;
	uint32_t unmatched_blobs;

	double reprojection_error;
	struct pose_rect bounds;

	bool degenerate_solution;
};

bool
pose_metrics_match_pose_to_blobs(const struct xrt_pose *pose,
                                 struct t_blob *blobs,
                                 int num_blobs,
                                 struct t_constellation_tracker_led_model *led_model,
                                 t_constellation_device_id_t device_id,
                                 struct camera_model *calib,
                                 struct pose_metrics_blob_match_info *match_info);

bool
pose_metrics_evaluate_pose(struct pose_metrics *score,
                           const struct xrt_pose *pose,
                           struct t_blob *blobs,
                           int num_blobs,
                           struct t_constellation_tracker_led_model *leds_model,
                           t_constellation_device_id_t device_id,
                           struct camera_model *calib,
                           struct pose_rect *out_bounds);

bool
pose_metrics_evaluate_pose_with_prior(struct pose_metrics *score,
                                      const struct xrt_pose *pose,
                                      bool prior_must_match,
                                      const struct xrt_pose *pose_prior,
                                      const struct xrt_vec3 *pos_error_thresh,
                                      const struct xrt_vec3 *rot_error_thresh,
                                      struct t_blob *blobs,
                                      int num_blobs,
                                      struct t_constellation_tracker_led_model *led_model,
                                      t_constellation_device_id_t device_id,
                                      struct camera_model *calib,
                                      struct pose_rect *out_bounds);

/*!
 * Compares whether new_score is a better pose than old_score.
 *
 * @param old_score The old score to compare against.
 * @param new_score The new score to compare against the old score.
 * @return true if the new score is a better pose than the old score, false otherwise.
 */
bool
pose_metrics_score_is_better_pose(struct pose_metrics *old_score, struct pose_metrics *new_score);

#ifdef __cplusplus
}
#endif
