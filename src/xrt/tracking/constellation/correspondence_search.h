// Copyright 2020 Jan Schmidt
// Copyright 2026 Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Ab-initio blob<->LED correspondence search
 * @author Jan Schmidt <jan@centricular.com>
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup tracking
 */

#pragma once

#include "tracking/t_constellation.h"

#include "pose_metrics.h"
#include "camera_model.h"


#ifdef __cplusplus
extern "C" {
#endif

#define MAX_BLOB_SEARCH_DEPTH 5

//! Lambdatwist can sometimes have spurious failures, the anchor LED must be within 2.5mm to be accepted
#define ANCHOR_LED_MAX_DISTANCE_M_SQ (0.025 * 0.025)

enum correspondence_search_flags
{
	CS_FLAG_NONE = 0x0,
	//! Do quick search @ depth 1-2 neighbour depth
	CS_FLAG_SHALLOW_SEARCH = 0x1,
	//! do deeper searches @ up to MAX_LED_SEARCH_DEPTH/MAX_BLOB_SEARCH_DEPTH
	CS_FLAG_DEEP_SEARCH = 0x2,
	//! Stop searching if a strong match is found, otherwise search all and return best match
	CS_FLAG_STOP_FOR_STRONG_MATCH = 0x4,
	//! Allow matching against all blobs, not just unlabelled ones or for the current device
	CS_FLAG_MATCH_ALL_BLOBS = 0x8,
	//! If the input obj_cam_pose contains a valid prior
	CS_FLAG_HAVE_POSE_PRIOR = 0x10,
	//! Use the provided gravity vector to check pose verticality. Depends on CS_FLAG_HAVE_POSE_PRIOR.
	CS_FLAG_MATCH_GRAVITY = 0x20,
};

struct cs_image_point
{
	struct t_blob *blob;

	double point_homog[3]; // Homogeneous version of the point
	double size[2];        // w/h of the blob, in homogeneous coordinates
	double max_dist;       // norm of (W/H) for distance checks

	/* List of the nearest blobs, filtered for the active model */
	int num_neighbours;
	struct cs_image_point *neighbours[MAX_BLOB_SEARCH_DEPTH];
};

struct cs_model_info
{
	t_constellation_device_id_t id;

	const struct t_constellation_search_model *model;

	double best_pose_found_time; /* Time (in secs) at which the best pose was found */
	int best_pose_blob_depth;    /* Blob neighbor depth the best pose is from */
	int best_pose_led_depth;     /* LED neighbour depth the best pose is from */
	struct xrt_pose best_pose;
	enum pose_match_flags match_flags;

	struct pose_metrics best_score;

	/* Search parameters */
	double search_start_time;
	int led_depth;
	int led_index;
	int blob_index;

	enum correspondence_search_flags search_flags;
	int min_led_depth, max_led_depth;
	int max_blob_depth;

	/* Valid when CS_FLAG_HAVE_POSE_PRIOR is set */
	struct xrt_pose pose_prior;
	const struct xrt_vec3 *pos_error_thresh;
	const struct xrt_vec3 *rot_error_thresh;

	/* Used when CS_FLAG_MATCH_GRAVITY is set */
	struct xrt_vec3 gravity_vector;
	struct xrt_quat gravity_swing;
	float gravity_tolerance_rad;
};

struct correspondence_search
{
	const enum u_logging_level *ct_log_level;

	int num_points;
	struct cs_image_point *points;
	struct t_blob *blobs; /* Original blobs structs [num_points] */

	unsigned int num_trials;
	unsigned int num_pose_checks;

	struct camera_model *calib;

	/* List of the nearest blobs for each blob */
	struct cs_image_point
	    *blob_neighbours[XRT_CONSTELLATION_MAX_BLOBS_PER_FRAME][XRT_CONSTELLATION_MAX_BLOBS_PER_FRAME];
};

struct correspondence_search *
correspondence_search_new(const enum u_logging_level *ct_log_level, struct camera_model *camera_calib);

void
correspondence_search_free(struct correspondence_search *cs);

void
correspondence_search_set_blobs(struct correspondence_search *cs, struct t_blob *blobs, int num_blobs);

/*!
 * Does a brute-force search for a device's pose.
 *
 * @note The `pose` and `score` parameters are only written on success.
 */
bool
correspondence_search_find_one_pose(struct correspondence_search *cs,
                                    const struct t_constellation_search_model *model,
                                    enum correspondence_search_flags search_flags,
                                    struct xrt_pose *inout_pose,
                                    const struct xrt_vec3 *pos_error_thresh,
                                    const struct xrt_vec3 *rot_error_thresh,
                                    const struct xrt_vec3 *gravity_vector,
                                    float gravity_tolerance_rad,
                                    struct pose_metrics *out_score);

#ifdef __cplusplus
}
#endif
