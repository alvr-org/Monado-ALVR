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

#include "math/m_api.h"
#include "math/m_vec3.h"

#include "util/u_logging.h"

#include "tracking/t_camera_models.h"

#include "pose_metrics.h"

#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>


#define POSE_METRICS_SPEW 0

#if POSE_METRICS_SPEW
#define LOG_SPEW(...) U_LOG_D(__VA_ARGS__)
#else
#define LOG_SPEW(...)
#endif

static void
expand_rect(struct pose_rect *bounds, double x, double y, double w, double h)
{
	bounds->left = MIN(bounds->left, x);
	bounds->top = MIN(bounds->top, y);
	bounds->right = MAX(bounds->right, x + w);
	bounds->bottom = MAX(bounds->bottom, y + h);
}

static int
find_best_matching_led(struct pose_metrics_visible_led_info *led_points,
                       int num_leds,
                       struct t_blob *blob,
                       double *out_sqerror)
{
	double best_z;
	int best_led_index = -1;
	double best_sqerror = 1e20;
	int leds_within_range = 0;

	for (int i = 0; i < num_leds; i++) {
		struct pose_metrics_visible_led_info *led_info = led_points + i;
		struct xrt_vec2 *pos_px_undistorted = &led_info->pos_px_undistorted;
		double led_radius_px_undistorted = led_info->led_radius_px_undistorted;
		double dx = fabs((double)pos_px_undistorted->x - blob->center_undistorted.x);
		double dy = fabs((double)pos_px_undistorted->y - blob->center_undistorted.y);
		double sqerror = dx * dx + dy * dy;

		// If the blob is much larger than the LED in either dimension, don't match.
		// @note: We're technically comparing distorted size to undistorted size, but that's a fine enough
		// approximation.
		if (blob->size.x > led_info->led_radius_px_undistorted * 4 ||
		    blob->size.y > led_info->led_radius_px_undistorted * 4) {
			continue;
		}

		// Check if the LED falls within the bounding box is closer to the camera (smaller Z),
		// or is at least ed_radius closer to the blob center
		if (sqerror < (led_radius_px_undistorted * led_radius_px_undistorted)) {
			leds_within_range++;

			if (best_led_index < 0 || best_z > led_info->pos_m.z ||
			    (sqerror + led_radius_px_undistorted) < best_sqerror) {
				best_z = led_info->pos_m.z;
				best_led_index = i;
				best_sqerror = sqerror;
			}
		}
	}

	if (leds_within_range > 1 && u_log_get_global_level() >= U_LOGGING_TRACE) {
		U_LOG_T("Multiple LEDs match blob @ %f, %f. best_sqerror %f LED %d z %f", blob->center_undistorted.x,
		        blob->center_undistorted.y, best_sqerror, best_led_index, led_points[best_led_index].pos_m.z);

		for (int i = 0; i < num_leds; i++) {
			struct pose_metrics_visible_led_info *led_info = led_points + i;
			struct xrt_vec2 *pos_px_undistorted = &led_info->pos_px_undistorted;
			struct xrt_vec3 *pos_m_undistorted = &led_info->pos_m;
			double led_radius_px = led_info->led_radius_px_undistorted;
			double dx = fabs((double)pos_px_undistorted->x - blob->center_undistorted.x);
			double dy = fabs((double)pos_px_undistorted->y - blob->center_undistorted.y);
			double sqerror = dx * dx + dy * dy;

			// Check if the LED falls within the bounding box has smaller error distance,
			// or is closer to the camera (smaller Z)
			if (sqerror < (led_radius_px * led_radius_px)) {
				U_LOG_T("LED %d sqerror %f pos px %f %f radius %f metres %f %f %f", i, sqerror,
				        pos_px_undistorted->x, pos_px_undistorted->y, led_radius_px,
				        pos_m_undistorted->x, pos_m_undistorted->y, pos_m_undistorted->z);
			}
		}
	}

	if (out_sqerror) {
		*out_sqerror = best_sqerror;
	}
	return best_led_index;
}

static void
check_pose_prior(struct pose_metrics *score,
                 const struct xrt_pose *pose,
                 const struct xrt_pose *pose_prior,
                 const struct xrt_vec3 *pos_error_thresh,
                 const struct xrt_vec3 *rot_error_thresh)
{
	struct xrt_quat orient_diff;

	score->match_flags |= POSE_HAD_PRIOR;

	score->pos_error = m_vec3_sub(pose->position, pose_prior->position);

	math_quat_unrotate(&pose->orientation, &pose_prior->orientation, &orient_diff);
	math_quat_normalize(&orient_diff);
	math_quat_ln_so3(&orient_diff, &score->orient_error);

	// Check each component of position and rotation are within the passed error bound and
	// clear any return flag that's not set
	if (pos_error_thresh) {
		score->match_flags |= POSE_MATCH_POSITION;

		if (fabsf(score->pos_error.x) > pos_error_thresh->x ||
		    fabsf(score->pos_error.y) > pos_error_thresh->y ||
		    fabsf(score->pos_error.z) > pos_error_thresh->z) {
			score->match_flags &= ~POSE_MATCH_POSITION;
		}
	}

	if (rot_error_thresh) {
		score->match_flags |= POSE_MATCH_ORIENT;

		if (fabsf(score->orient_error.x) > rot_error_thresh->x ||
		    fabsf(score->orient_error.y) > rot_error_thresh->y ||
		    fabsf(score->orient_error.z) > rot_error_thresh->z) {
			score->match_flags &= ~POSE_MATCH_ORIENT;
		}
	}
}

static bool
project_led_points(struct t_constellation_tracker_led_model *led_model,
                   struct camera_model *calib,
                   const struct xrt_pose *pose,
                   struct xrt_vec3 *out_positions,
                   struct xrt_vec2 *out_points)
{
	for (size_t i = 0; i < led_model->led_count; i++) {
		struct xrt_vec3 *tmp = out_positions + i;

		math_pose_transform_point(pose, &led_model->leds[i].position, tmp);

		// @note We're using the pinhole camera model since we're later comparing them to the blob
		//       undistorted pixel coordinates
		if (!t_camera_models_project(&calib->calib_pinhole, tmp->x, tmp->y, tmp->z, &out_points[i].x,
		                             &out_points[i].y)) {
			return false;
		}
	}

	return true;
}

static bool
get_visible_leds(const struct xrt_pose *T_cam_obj,
                 struct t_constellation_tracker_led_model *led_model,
                 struct camera_model *calib,
                 struct pose_metrics_visible_led_info *visible_led_points,
                 int *num_visible_leds)
{
	*num_visible_leds = 0;

	struct xrt_vec3 led_out_positions[MAX_OBJECT_LEDS] = {0};
	struct xrt_vec2 led_out_points[MAX_OBJECT_LEDS] = {0};

	// Project LEDs into our image space
	if (!project_led_points(led_model, calib, T_cam_obj, led_out_positions, led_out_points)) {
		return false;
	}

	// Compute LED pixel size based on model distance below using the larger X/Y focal length and LED's Z value
	const double focal_length = MAX(calib->calib_pinhole.fx, calib->calib_pinhole.fy);

	struct xrt_pose T_obj_cam = {0};
	math_pose_invert(T_cam_obj, &T_obj_cam);

	struct t_constellation_tracker_led *leds = led_model->leds;
	const int num_leds = led_model->led_count;

	for (int i = 0; i < num_leds; i++) {
		struct xrt_vec2 *led_pos_px = &led_out_points[i];
		struct xrt_vec3 *led_pos_m = &led_out_positions[i];
		struct t_constellation_tracker_led *led = &leds[i];

		// LEDs behind the camera are not visible
		if (led_pos_m->z <= 0.0) {
			continue;
		}

		// Outside the visible screen space (the bounding box of the frame's undistorted border)
		if (led_pos_px->x < calib->left_pinhole || led_pos_px->x > calib->right_pinhole ||
		    led_pos_px->y < calib->top_pinhole || led_pos_px->y > calib->bottom_pinhole) {
			continue;
		}

		// Calculate the expected size of an LED at this distance
		const double led_radius_px = focal_length * led->radius_m / led_pos_m->z;

		LOG_SPEW("LED id %d led_radius_px %f = focal length %f led_radius %f Z = %f m", led_model->leds[i].id,
		         led_radius_px, focal_length, led->radius_m, led_pos_m->z);

		// Convert the position to a unit vector for dot product comparison
		struct xrt_vec3 view_vec = *led_pos_m;
		struct xrt_vec3 normal;

		math_vec3_normalize(&view_vec);
		math_quat_rotate_vec3(&T_cam_obj->orientation, &leds[i].normal, &normal);

		double facing_dot = m_vec3_dot(view_vec, normal);

		LOG_SPEW("LED %u pos %f,%f,%f -> %f,%f (pos %f,%f,%f metres) dir %f %f %f dot %f visible %d", i,
		         leds[i].position.x, leds[i].position.y, leds[i].position.z, led_pos_px->x, led_pos_px->y,
		         led_pos_m->x, led_pos_m->y, led_pos_m->z, normal.x, normal.y, normal.z, facing_dot,
		         facing_dot < cos(M_PI - led->visibility_angle));

		// The vector to the LED position points out from the camera to the LED, but the normal points toward
		// the camera, so we need to compare against 180 - visiblility_angle here
		if (facing_dot > cos(M_PI - led->visibility_angle)) {
			continue;
		}

		if (led_model->compute_led_visibility &&
		    !t_constellation_tracker_led_model_compute_led_visibility(led_model, i, T_obj_cam.position)) {
			continue;
		}

		struct pose_metrics_visible_led_info *led_info = &visible_led_points[(*num_visible_leds)++];
		led_info->led = leds + i;
		led_info->pos_px_undistorted = *led_pos_px;
		led_info->pos_m = *led_pos_m;
		led_info->led_radius_px_undistorted = led_radius_px;
		led_info->matched_blob = NULL;
		led_info->facing_dot = facing_dot;
	}

	return true;
}

static bool
get_visible_leds_bounds(struct pose_metrics_visible_led_info *visible_led_points,
                        int num_visible_leds,
                        struct pose_rect *bounds)
{
	if (num_visible_leds == 0) {
		return false;
	}

	const struct pose_metrics_visible_led_info *led_info = &visible_led_points[0];
	(*bounds) = (struct pose_rect){
	    .left = led_info->pos_px_undistorted.x - led_info->led_radius_px_undistorted,
	    .top = led_info->pos_px_undistorted.y - led_info->led_radius_px_undistorted,
	    .right = led_info->pos_px_undistorted.x + 2 * led_info->led_radius_px_undistorted,
	    .bottom = led_info->pos_px_undistorted.y + 2 * led_info->led_radius_px_undistorted,
	};

	for (int i = 1; i < num_visible_leds; ++i) {
		const struct pose_metrics_visible_led_info *led_info = &visible_led_points[i];

		expand_rect(bounds, led_info->pos_px_undistorted.x - led_info->led_radius_px_undistorted,
		            led_info->pos_px_undistorted.y - led_info->led_radius_px_undistorted,
		            2 * led_info->led_radius_px_undistorted, 2 * led_info->led_radius_px_undistorted);
	}

	return true;
}

/*
 * Exported functions.
 */

bool
pose_metrics_match_pose_to_blobs(const struct xrt_pose *pose,
                                 struct t_blob *blobs,
                                 int num_blobs,
                                 struct t_constellation_tracker_led_model *led_model,
                                 t_constellation_device_id_t device_id,
                                 struct camera_model *calib,
                                 struct pose_metrics_blob_match_info *match_info)
{

	match_info->reprojection_error = 0.0;
	match_info->matched_blobs = 0;
	match_info->unmatched_blobs = 0;
	match_info->degenerate_solution = false;

	if (!get_visible_leds(pose, led_model, calib, match_info->visible_leds, &match_info->num_visible_leds)) {
		U_LOG_W("No visible leds found");
		return false;
	}

	if (!get_visible_leds_bounds(match_info->visible_leds, match_info->num_visible_leds, &match_info->bounds)) {
		U_LOG_E("Failed to get leds bounds");
		return false;
	}

	const struct pose_rect *bounds = &match_info->bounds;
	LOG_SPEW("Bounding box for pose is %f,%f -> %f,%f", bounds->left, bounds->top, bounds->right, bounds->bottom);

	// Iterate the blobs and see which ones are within the bounding box and have a matching LED
	bool all_led_ids_matched = true;

	for (int i = 0; i < num_blobs; i++) {
		struct t_blob *b = &blobs[i];
		t_constellation_device_id_t led_object_id = b->matched_device_id;

		// Skip blobs which already have an ID not belonging to this device
		if (led_object_id != XRT_CONSTELLATION_INVALID_DEVICE_ID && led_object_id != device_id) {
			continue;
		}

		// Ignore blobs that are outside the pose bounding box
		if (b->center_undistorted.x < bounds->left || b->center_undistorted.y < bounds->top ||
		    b->center_undistorted.x > bounds->right || b->center_undistorted.y > bounds->bottom) {
			continue;
		}

		double sqerror = 0.f;
		int match_led_index = find_best_matching_led(match_info->visible_leds,     //
		                                             match_info->num_visible_leds, //
		                                             b,                            //
		                                             &sqerror);

		if (match_led_index < 0) {
			match_info->unmatched_blobs++;
			continue;
		}

		match_info->reprojection_error += sqerror;
		match_info->matched_blobs++;

		struct pose_metrics_visible_led_info *led_info = match_info->visible_leds + match_led_index;
		led_info->matched_blob = b;

		if (b->matched_device_led_id != XRT_CONSTELLATION_INVALID_LED_ID) {
			struct t_constellation_tracker_led *match_led = led_info->led;
			t_constellation_led_id_it led_id = match_led->id;
			if (b->matched_device_led_id != led_id || b->matched_device_id != device_id) {
				LOG_SPEW("mismatched LED id %d blob %d (@ %f,%f) has %d/%d", led_id, i,
				         b->center_undistorted.x, b->center_undistorted.y, b->matched_device_id,
				         b->matched_device_led_id);

				all_led_ids_matched = false;
			}
		}
	}

	match_info->all_led_ids_matched = all_led_ids_matched;

	// This is a degenerate solution, we have no way to check if this solve is bad within some error.
	if (match_info->matched_blobs <= 3) {
		match_info->degenerate_solution = true;
	}

	return true;
}

bool
pose_metrics_evaluate_pose(struct pose_metrics *score,
                           const struct xrt_pose *pose,
                           struct t_blob *blobs,
                           int num_blobs,
                           struct t_constellation_tracker_led_model *led_model,
                           t_constellation_device_id_t device_id,
                           struct camera_model *calib,
                           struct pose_rect *out_bounds)
{
	return pose_metrics_evaluate_pose_with_prior(score, pose, false, NULL, NULL, NULL, blobs, num_blobs, led_model,
	                                             device_id, calib, out_bounds);
}

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
                                      struct pose_rect *out_bounds)
{
	/*
	 * 1. Project the LED points with the provided pose
	 * 2. Build a bounding box for the points
	 * 3. For blobs within the bounding box, see if they match a LED
	 * 4. Count up the matched LED<->Blob correspondences, and the reprojection error
	 */
	struct pose_metrics_blob_match_info blob_match_info = {0};

	if (!pose_metrics_match_pose_to_blobs(pose, blobs, num_blobs, led_model, device_id, calib, &blob_match_info)) {
		return false;
	}

	assert(led_model->led_count > 0);
	assert(num_blobs > 0);
	assert(score != NULL);

	score->match_flags = 0;
	score->reprojection_error = blob_match_info.reprojection_error;
	score->matched_blobs = blob_match_info.matched_blobs;
	score->unmatched_blobs = blob_match_info.unmatched_blobs;
	score->visible_leds = blob_match_info.num_visible_leds;

	if (blob_match_info.degenerate_solution) {
		POSE_SET_FLAGS(score, POSE_MATCH_DEGENERATE);
	}

	if (blob_match_info.all_led_ids_matched) {
		POSE_SET_FLAGS(score, POSE_MATCH_LED_IDS);
	}

	double error_per_led = score->reprojection_error / score->matched_blobs;

	// If we have a pose prior, calculate the rotation and translation error and match flags as needed
	if (pose_prior) {
		// We can't validate a prior without error bounds
		assert(pos_error_thresh != NULL);
		assert(rot_error_thresh != NULL);

		check_pose_prior(score, pose, pose_prior, pos_error_thresh, rot_error_thresh);

		if (POSE_HAS_FLAGS(score, POSE_MATCH_POSITION | POSE_MATCH_ORIENT)) {
			if (score->matched_blobs <
			    led_model->match_parameters.min_leds_for_correspondence_search_with_prior) {
				// The pose had too few LEDs for a with-prior match.
				POSE_SET_FLAGS(score, POSE_MATCH_DEGENERATE);
			} else {
				// The pose matched the prior, and had enough LEDs for a with-prior match.
			}
		} else if (score->matched_blobs <
		           led_model->match_parameters.min_leds_for_correspondence_search_without_prior) {
			// The pose had too few LEDs for a without-prior match
			POSE_SET_FLAGS(score, POSE_MATCH_DEGENERATE);
		}
	} else if (blob_match_info.matched_blobs <
	           led_model->match_parameters.min_leds_for_correspondence_search_without_prior) {
		// Pose had too few LEDs for a without-prior match
		POSE_SET_FLAGS(score, POSE_MATCH_DEGENERATE);
	}

	// Don't add GOOD/STRONG flags if matched fewer than 3 blobs
	if (score->matched_blobs < 3) {
		goto done;
	}

	// At this point, we have at least 3 LEDs and their blobs matching
	if (POSE_HAS_FLAGS(score, POSE_MATCH_POSITION | POSE_MATCH_ORIENT)) {
		if (error_per_led < 2.0 && (score->unmatched_blobs * 4 <= score->matched_blobs ||
		                            (2 * score->visible_leds <= 3 * score->matched_blobs))) {
			LOG_SPEW("Got good prior match within pos (%f, %f, %f) rot (%f, %f, %f)", pos_error_thresh->x,
			         pos_error_thresh->y, pos_error_thresh->z, rot_error_thresh->x, rot_error_thresh->y,
			         rot_error_thresh->z);

			score->match_flags |= POSE_MATCH_GOOD;

			if (error_per_led < 1.5) {
				score->match_flags |= POSE_MATCH_STRONG;
			}
		} else {
			LOG_SPEW("Failed prior match within pos (%f, %f, %f) rot (%f, %f, %f)", pos_error_thresh->x,
			         pos_error_thresh->y, pos_error_thresh->z, rot_error_thresh->x, rot_error_thresh->y,
			         rot_error_thresh->z);
		}
	} else if (prior_must_match) {
		LOG_SPEW("Pose doesn't match prior, but we require it to do so");

		// If we must match the prior and failed, bail out
		goto done;
	} else if (score->visible_leds > 4 && score->matched_blobs > 4 && error_per_led < 3.0 &&
	           (score->unmatched_blobs * 4 <= score->matched_blobs ||
	            (2 * score->visible_leds <= 3 * score->matched_blobs))) {
		/*
		 * If we matched all the blobs in the pose bounding box (allowing 25% noise / overlapping blobs)
		 * or if we matched a large proportion (2/3) of the LEDs we expect to be visible, then consider this a
		 * good pose match
		 */
		score->match_flags |= POSE_MATCH_GOOD;

		// If we had no pose prior, but a close reprojection error, allow a STRONG match
		// If we had a pose prior and got here, the pose is out of tolerance, so only permit a "GOOD" match
		if (pose_prior == NULL && error_per_led < 1.5) {
			score->match_flags |= POSE_MATCH_STRONG;
		}

		LOG_SPEW("Got good match with %d visible LEDs, %d matched blobs, %d unmatched blobs, error per LED %f",
		         score->visible_leds, score->matched_blobs, score->unmatched_blobs, error_per_led);
	} else {
		LOG_SPEW(
		    "Failed to get good match with %d visible LEDs, %d matched blobs, %d unmatched blobs, error per "
		    "LED %f",
		    score->visible_leds, score->matched_blobs, score->unmatched_blobs, error_per_led);
	}

done:
	LOG_SPEW("score for pose is %u matched %u unmatched %u visible %f error", score->matched_blobs,
	         score->unmatched_blobs, score->visible_leds, score->reprojection_error);

	if (out_bounds) {
		*out_bounds = blob_match_info.bounds;
	}

	return true;
}

bool
pose_metrics_score_is_better_pose(struct pose_metrics *old_score, struct pose_metrics *new_score)
{
	/*
	 * If our previous best pose was "strong", only take better "strong" poses.
	 * However, don't take that into account with a P3P solve, a strong P3P solve is worse than a good P4P+ solve.
	 */
	if (!POSE_HAS_FLAGS(old_score, POSE_MATCH_DEGENERATE) && //
	    POSE_HAS_FLAGS(old_score, POSE_MATCH_STRONG) &&      //
	    !POSE_HAS_FLAGS(new_score, POSE_MATCH_STRONG)) {     //
		return false;
	}

	/*
	 * If the old score wasn't good, but the new one is, take the new one.
	 * However, don't allow a good P3P to override a non-good P4P+.
	 */
	if (!POSE_HAS_FLAGS(new_score, POSE_MATCH_DEGENERATE) && //
	    !POSE_HAS_FLAGS(old_score, POSE_MATCH_GOOD) &&       //
	    POSE_HAS_FLAGS(new_score, POSE_MATCH_GOOD)) {        //
		return true;
	}

	/*
	 * A score that matched nothing has a reprojection error of zero, which would otherwise read as a perfect fit
	 * rather than as the absence of one. Both forms start at the worst error so that only a score with something
	 * to say about the blobs can win a comparison below.
	 */
	double new_error = WORST_REPROJECTION_ERROR;
	double new_error_per_led = WORST_REPROJECTION_ERROR;
	if (new_score->matched_blobs > 0) {
		new_error = new_score->reprojection_error;
		new_error_per_led = new_score->reprojection_error / new_score->matched_blobs;
	}

	double old_error = WORST_REPROJECTION_ERROR;
	double old_error_per_led = WORST_REPROJECTION_ERROR;
	if (old_score->matched_blobs > 0) {
		old_error = old_score->reprojection_error;
		old_error_per_led = old_score->reprojection_error / old_score->matched_blobs;
	}

	/*
	 * If either score is a P3P solve, then the best error per LED is incomputable since a P3P solve inherently has
	 * mathematically almost no error. Penalize it so that zero reprojection error doesn't overwhelm the fact it's a
	 * P3P solve and more LEDs is better.
	 */
	if (POSE_HAS_FLAGS(new_score, POSE_MATCH_DEGENERATE)) {
		new_error_per_led = WORST_REPROJECTION_ERROR;
		new_error = WORST_REPROJECTION_ERROR;
	}
	if (POSE_HAS_FLAGS(old_score, POSE_MATCH_DEGENERATE)) {
		old_error_per_led = WORST_REPROJECTION_ERROR;
		old_error = WORST_REPROJECTION_ERROR;
	}

	// Prefer more matched blobs with tighter error/LED
	if (old_score->matched_blobs < new_score->matched_blobs && (new_error_per_led < old_error_per_led)) {
		return true;
	}

	/*
	 * Prefer at least 2 more matched blobs with slightly worse error/LED.
	 *
	 * This is also the only branch a degenerate solve can win against a score that matched nothing, since both
	 * sides are then the worst error and it is the 10% slack that separates them. Without it the first degenerate
	 * pose of a search would never be recorded as the best one at all.
	 */
	if (old_score->matched_blobs + 1 < new_score->matched_blobs && (new_error_per_led < old_error_per_led * 1.1)) {
		return true;
	}

	// Else, prefer closer reprojection with at least as many matches
	if (old_score->matched_blobs == new_score->matched_blobs && new_error < old_error) {
		return true;
	}

	/*
	 * If both scores have pose priors, prefer the one where the orientation better matches the prior
	 * BUT only if reprojection error is comparable (within 20%)
	 */
	if (POSE_HAS_FLAGS(old_score, POSE_HAD_PRIOR) && POSE_HAS_FLAGS(new_score, POSE_HAD_PRIOR)) {
		if (old_score->matched_blobs == new_score->matched_blobs && new_error < old_error * 1.2) {
			if (m_vec3_len(new_score->orient_error) < m_vec3_len(old_score->orient_error)) {
				return true;
			}
		}
	}

	return false;
}
