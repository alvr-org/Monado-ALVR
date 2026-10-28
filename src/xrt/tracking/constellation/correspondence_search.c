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

#include "math/m_api.h"
#include "math/m_vec3.h"

#include "os/os_time.h"

#include "constellation/pose_metrics.h"

#include "lambdatwist/lambdatwist_p3p.h"
#include "correspondence_search.h"
#include "led_search_model.h"
#include "image_point_sort.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>


/*
 * This file implements a brute-force correspondence search between LED models and observed IR LED blobs.
 *
 * The goal is to iterate over sets of LED and blob candidates, and test them as correspondences. Groups of 4
 * LEDs and Blobs are collected. The first 3 points are used with the Lambdatwist P3P to calculate a hypothesis
 * pose of the object. If the 4th point matches the hypothesis, it proceeds to a full pose check to evaluate
 * how many other LED/blob points match (inliers), and how closely (reprojection error).
 *
 * To speed up the search, the LED and blobs are iterated using nearest neighbour lists, as blobs that are nearby
 * are much more likely to match LEDs that are near each other.
 *
 * LED models provide a set of 3D LED points and their normals. The constellation_search_model helper takes the set
 * of LED points (constellation_led_model) and for each one calculates an search candidate list
 * of neighbour LEDs (LEDs with normals within 90° of the anchor LED) sorted by euclidean distance.
 *
 * Before each search, the caller provides the set of 2D blobs to match against, which are given similar
 * treatment - for each blob the set of neighbours is calculated, sorted by increasing distance.
 *
 * If available the vertical alignment of the device is used as a check on valid poses. Since the IMU can generally
 * extract the gravity vector of each device with some variance, poses that don't match the required gravity direction
 * are rejected, avoiding the full-pose check.
 *
 * Further, if CS_FLAGS_STOP_FOR_STRONG_MATCH is passed and a 'strong' pose match is found, the search is stopped
 * and the result returned immediately.
 * A strong match is defined in the pose-helper, as 7 or more LEDs matching observed blobs within 1.5 pixels/led
 * reprojection error.
 *
 * A full search of all possible LED and blob combinations is prohibitive, so the search is limited to matching
 * neighbours up to MAX_LED_SEARCH_DEPTH deep for LEDs - ie, for MAX_LED_SEARCH_DEPTH=6, we check each LED and
 * combinations of 3 from the 6 nearest LEDs. For blobs, we check MAX_BLOB_SEARCH_DEPTH nearest neighbours, but filter
 * the list to ignore blobs that are already assigned to another device (unless CS_FLAG_MATCH_ALL_BLOBS is set).
 *
 * In general, there are many more possible LEDs than there are visible blobs, and most of the LEDs will not be
 * visible in any given pose. blobs, on the other hand are (by definition) visible - if we find the right LEDs, they'll
 * match. As such, it's better to loop over some 1-2 depth combinations of blobs and test them against nearest LEDs,
 * before proceeding to any deeper search depths. If CS_FLAG_SHALLOW_SEARCH is provided, only depth 1/2 is checked.
 */

#define CS_TRACE(cs, ...) U_LOG_IFL_T((*((cs)->ct_log_level)), __VA_ARGS__)
#define CS_DEBUG(cs, ...) U_LOG_IFL_D((*((cs)->ct_log_level)), __VA_ARGS__)
#define CS_INFO(cs, ...) U_LOG_IFL_I((*((cs)->ct_log_level)), __VA_ARGS__)
#define CS_WARN(cs, ...) U_LOG_IFL_W((*((cs)->ct_log_level)), __VA_ARGS__)
#define CS_ERROR(cs, ...) U_LOG_IFL_E((*((cs)->ct_log_level)), __VA_ARGS__)

// @note These should never be upstreamed while set to 1, these are purely for very verbose debugging.
#define DUMP_SCENE 0
#define DUMP_BLOBS 0
#define DUMP_FULL_DEBUG 0
#define DUMP_FULL_LOG 0
#define DUMP_TIMING 0
#define CHECK_ALL_PROJECTIONS 0

#if DUMP_BLOBS
#define CS_DUMP_BLOBS(cs, ...) CS_DEBUG(cs, __VA_ARGS__)
#else
#define CS_DUMP_BLOBS(cs, ...)
#endif

#if DUMP_FULL_DEBUG
#define CS_FULL_DEBUG(cs, ...) CS_DEBUG(cs, __VA_ARGS__)
#else
#define CS_FULL_DEBUG(cs, ...)
#endif

#if DUMP_FULL_LOG
#define CS_FULL_LOG(cs, ...) CS_DEBUG(cs, __VA_ARGS__)
#else
#define CS_FULL_LOG(cs, ...)
#endif

#if DUMP_TIMING
#define CS_TIMING(cs, ...) CS_DEBUG(cs, __VA_ARGS__)
#else
#define CS_TIMING(cs, ...)
#endif

#define MAX_LED_SEARCH_DEPTH 8

#if CHECK_ALL_PROJECTIONS
static void
project_led_point(struct xrt_vec3 *led_pos,
                  struct xrt_pose *pose,
                  struct camera_model *calib,
                  struct xrt_vec2 *out_point)
{
	struct xrt_vec3 tmp;
	math_pose_transform_point(pose, led_pos, &tmp);
	t_camera_models_project(&calib->calib_pinhole, tmp.x, tmp.y, tmp.z, &out_point->x, &out_point->y);
}
#endif

static void
dump_pose(const struct correspondence_search *cs,
          const struct t_constellation_search_model *model,
          const struct xrt_pose *pose,
          const struct cs_model_info *mi)
{
#if DUMP_SCENE
	struct t_constellation_tracker_led_model *leds = model->led_model;

	CS_DEBUG(cs, "pose_points[%i] = [", mi->id);
	for (size_t i = 0; i < leds->led_count; i++) {
		struct xrt_vec3 pos, normal;

		// Project HMD LED into the image (no distortion)
		math_quat_rotate_vec3(&pose->orientation, &leds->leds[i].position, &pos);
		math_vec3_accum(&pose->position, &pos);

		// Can't be behind the camera
		if (pos.z < 0) {
			continue;
		}

		math_vec3_scalar_mul(1.0 / pos.z, &pos);

		float x = pos.x;
		float y = pos.y;

		math_vec3_normalize(&pos);

		math_quat_rotate_vec3(&pose->orientation, &leds->leds[i].normal, &normal);
		math_vec3_normalize(&normal);

		double facing_dot;
		facing_dot = m_vec3_dot(pos, normal);

		if (facing_dot < 0) {
			CS_DEBUG(cs, "  (%f,%f),", x, y);
		}
	}
	CS_DEBUG(cs, "]");
#endif
}

static bool
correspondence_search_project_pose(struct correspondence_search *cs,
                                   const struct t_constellation_search_model *model,
                                   const struct xrt_pose *pose,
                                   struct cs_model_info *mi,
                                   int depth)
{
	// Given a pose, project each 3D LED point back to 2D and assign correspondences to blobs.
	// If enough match, print out the pose.
	struct t_constellation_tracker_led_model *leds = model->led_model;

	// Increment stats
	cs->num_pose_checks++;

	// Invalid position, out of range
	if (pose->position.z < CLOSEST_ACCEPTABLE_POSE_M || pose->position.z > FARTHEST_ACCEPTABLE_POSE_M) {
		CS_FULL_LOG(cs, "Pose out of range - Z @ %f metres", pose->position.z);
		return false;
	}

	// See if we need to make a gravity vector alignment check
	if (mi->search_flags & CS_FLAG_MATCH_GRAVITY) {
		struct xrt_quat pose_gravity_swing, pose_gravity_twist;

		math_quat_decompose_swing_twist(&pose->orientation, &mi->gravity_vector, &pose_gravity_swing,
		                                &pose_gravity_twist);

		// Calculate the difference between the amount of gravity swing, ignoring axis
		float pose_angle = fabsf(acosf(pose_gravity_swing.w)) - fabsf(acosf(mi->gravity_swing.w));
		if (pose_angle > mi->gravity_tolerance_rad) {
			CS_FULL_DEBUG(
			    cs,
			    "model %d failed pose match - orientation was not within tolerance (error %f deg > %f "
			    "deg) gravity vec %f %f %f pose %f %f %f %f swing %f %f %f %f prior swing %f %f %f %f",
			    mi->id, RAD_TO_DEG(pose_angle), RAD_TO_DEG(mi->gravity_tolerance_rad), mi->gravity_vector.x,
			    mi->gravity_vector.y, mi->gravity_vector.z, pose->orientation.x, pose->orientation.y,
			    pose->orientation.z, pose->orientation.w, pose_gravity_swing.x, pose_gravity_swing.y,
			    pose_gravity_swing.z, pose_gravity_swing.w, mi->gravity_swing.x, mi->gravity_swing.y,
			    mi->gravity_swing.z, mi->gravity_swing.w);
			return false;
		}
	}

	struct pose_metrics score;

	// Check how many LEDs have matching blobs in this pose, if there's enough we have a good match
	// @todo: It would be better to be able to pass a list of undistorted blob points
	if (mi->search_flags & CS_FLAG_HAVE_POSE_PRIOR) {
		pose_metrics_evaluate_pose_with_prior(&score, pose, false, &mi->pose_prior, mi->pos_error_thresh,
		                                      mi->rot_error_thresh, cs->blobs, cs->num_points, leds,
		                                      model->device_id, cs->calib, NULL);
	} else {
		pose_metrics_evaluate_pose(&score, pose, cs->blobs, cs->num_points, leds, model->device_id, cs->calib,
		                           NULL);
	}

	/*
	 * If this pose is any good, test it further.
	 * Block degenerate solves from being considered since those are always "good" or "strong" by construction.
	 */
	if (POSE_HAS_FLAGS(&score, POSE_MATCH_GOOD) && !POSE_HAS_FLAGS(&score, POSE_MATCH_DEGENERATE)) {
		if (pose_metrics_score_is_better_pose(&mi->best_score, &score)) {
			mi->best_score = score;
			mi->best_pose = *pose;
#if DUMP_TIMING
			mi->best_pose_found_time = os_monotonic_get_ns();
#endif
			mi->best_pose_blob_depth = depth;
			mi->best_pose_led_depth = mi->led_depth;
			mi->match_flags = mi->best_score.match_flags;

			if (mi->match_flags & POSE_MATCH_STRONG) {
				CS_TIMING(cs,
				          "# Found a strong match for model %d - %d points out of %d with error %f "
				          "pixels^2 after %u trials and %u pose checks",
				          mi->id, mi->best_score.matched_blobs, mi->best_score.visible_leds,
				          mi->best_score.reprojection_error, cs->num_trials, cs->num_pose_checks);
				CS_TIMING(
				    cs,
				    "# Found at LED depth %d blob depth %d after %f ms. Anchor indices: LED %d blob %d",
				    mi->best_pose_led_depth, mi->best_pose_blob_depth,
				    (float)(mi->best_pose_found_time - mi->search_start_time) / 1000000.0,
				    mi->led_index, mi->blob_index);
				CS_TIMING(cs, "# pose orient %f %f %f %f pos %f %f %f", mi->best_pose.orientation.x,
				          mi->best_pose.orientation.y, mi->best_pose.orientation.z,
				          mi->best_pose.orientation.w, mi->best_pose.position.x,
				          mi->best_pose.position.y, mi->best_pose.position.z);
			}

			float error_per_led =
			    score.visible_leds > 0 ? score.reprojection_error / score.visible_leds : -1;
			(void)error_per_led; // Silence unused variable warning when not logging

			CS_FULL_LOG(cs,
			            "model %d new best pose candidate orient %f %f %f %f pos %f %f %f has %u visible "
			            "LEDs, error %f (%f / LED) after %u trials and %u pose checks",
			            mi->id, pose->orientation.x, pose->orientation.y, pose->orientation.z,
			            pose->orientation.w, pose->position.x, pose->position.y, pose->position.z,
			            score.visible_leds, score.reprojection_error, error_per_led, cs->num_trials,
			            cs->num_pose_checks);

			CS_FULL_LOG(cs, "model %d matched %u blobs of %u", mi->id, score.matched_blobs,
			            score.visible_leds);

			dump_pose(cs, model, pose, mi);

			return true;
		} else {
			float error_per_led =
			    score.visible_leds > 0 ? score.reprojection_error / score.visible_leds : -1;
			(void)error_per_led; // Silence unused variable warning when not logging

			CS_FULL_DEBUG(
			    cs,
			    "pose candidate orient %f %f %f %f pos %f %f %f has %u visible LEDs, error %f (%f / LED)",
			    pose->orientation.x, pose->orientation.y, pose->orientation.z, pose->orientation.w,
			    pose->position.x, pose->position.y, pose->position.z, score.visible_leds,
			    score.reprojection_error, error_per_led);
			CS_FULL_DEBUG(cs, "matched %u blobs of %u", score.matched_blobs, score.visible_leds);
		}

		CS_FULL_LOG(cs, "Found good pose match for device %u - %u LEDs matched %u visible ones", mi->id,
		            score.matched_blobs, score.visible_leds);

		return true;
	}

	CS_FULL_LOG(cs, "Failed pose match - only %u blobs matched %u visible LEDs. %u unmatched blobs. Error %f",
	            score.matched_blobs, score.visible_leds, score.unmatched_blobs, score.reprojection_error);

	return false;
}

static void
quat_from_rotation_matrix(struct xrt_quat *me, double R[9])
{
	float trace = R[0] + R[3 + 1] + R[6 + 2];
	if (trace > 0) {
		float s = 0.5f / sqrtf(trace + 1.0f);
		me->w = -0.25f / s;
		me->x = -(R[6 + 1] - R[3 + 2]) * s;
		me->y = -(R[2] - R[6]) * s;
		me->z = -(R[3] - R[1]) * s;
	} else {
		if (R[0] > R[4] && R[0] > R[8]) {
			float s = 2.0f * sqrtf(1.0f + R[0] - R[3 + 1] - R[6 + 2]);
			me->w = -(R[6 + 1] - R[3 + 2]) / s;
			me->x = -0.25f * s;
			me->y = -(R[1] + R[3]) / s;
			me->z = -(R[2] + R[6]) / s;
		} else if (R[3 + 1] > R[6 + 2]) {
			float s = 2.0f * sqrtf(1.0f + R[3 + 1] - R[0] - R[6 + 2]);
			me->w = -(R[2] - R[6]) / s;
			me->x = -(R[1] + R[3]) / s;
			me->y = -0.25f * s;
			me->z = -(R[3 + 2] + R[6 + 1]) / s;
		} else {
			float s = 2.0f * sqrtf(1.0f + R[6 + 2] - R[0] - R[3 + 1]);
			me->w = -(R[3] - R[1]) / s;
			me->x = -(R[2] + R[6]) / s;
			me->y = -(R[3 + 2] + R[6 + 1]) / s;
			me->z = -0.25f * s;
		}
	}
}

static void
check_led_against_model_subset(struct correspondence_search *cs,
                               struct cs_model_info *mi,
                               struct cs_image_point **blobs,
                               struct t_constellation_tracker_led *model_leds[4],
                               int depth)
{
	const struct t_constellation_search_model *model = mi->model;
	double x[3][3];
	struct xrt_vec3 *xcheck;
	double *y1, *y2, *y3;
	double Rs[4][9], Ts[4][3];

	y1 = blobs[0]->point_homog;
	y2 = blobs[1]->point_homog;
	y3 = blobs[2]->point_homog;

	cs->num_trials++;

#if 0 // enable this code to be able to trigger a log if a checked pose contains a set of wanted LED->Blob associations.
	struct
	{
		uint32_t blob_id;
		t_constellation_led_id_it led_id;
	} matches[] = {
	    {.blob_id = 177664, .led_id = 3}, {.blob_id = 173642, .led_id = 2}, {.blob_id = 183320, .led_id = 1},
	    {.blob_id = 176870, .led_id = 0}, {.blob_id = 183198, .led_id = 6}, /* {.blob_id = 183198, .led_id = 7}, */
	};

	uint32_t matched = 0;
	for (int i = 0; i < ARRAY_SIZE(matches); i++) {
		for (int j = 0; j < 4; j++) {
			if (matches[i].blob_id == blobs[j]->blob->blob_id && matches[i].led_id == model_leds[j]->id) {
				matched++;
				break;
			}
		}
	}

	if (matched >= 4) {
		CS_FULL_DEBUG(cs, "Hit triggered!!! model %d hit check for LED %d %d %d (%d) and blob %d %d %d (%d)", mi->id,
		              model_leds[0]->id, model_leds[1]->id, model_leds[2]->id, model_leds[3]->id,
		              blobs[0]->blob->blob_id, blobs[1]->blob->blob_id, blobs[2]->blob->blob_id,
		              blobs[3]->blob->blob_id);
	}
#endif

	for (int i = 0; i < 3; i++) {
		x[i][0] = model_leds[i]->position.x;
		x[i][1] = model_leds[i]->position.y;
		x[i][2] = model_leds[i]->position.z;
	}
	xcheck = &model_leds[3]->position;

	// @todo It would be better if this spat out quaternions, then we wouldn't need to convert below
	int valid = lambdatwist_p3p(y1, y2, y3, x[0], x[1], x[2], Rs, Ts);

	if (!valid) {
		CS_FULL_DEBUG(
		    cs, "model %d failed to find a valid P3P solution for LED %d %d %d (%d) and blob %d %d %d (%d)",
		    mi->id, model_leds[0]->id, model_leds[1]->id, model_leds[2]->id, model_leds[3]->id,
		    blobs[0]->blob->blob_id, blobs[1]->blob->blob_id, blobs[2]->blob->blob_id, blobs[3]->blob->blob_id);
		return;
	}

	for (int i = 0; i < valid; i++) {
		struct xrt_pose pose;
		struct xrt_vec3 checkpos, checkdir;

		// Construct quat and trans for this pose
		quat_from_rotation_matrix(&pose.orientation, Rs[i]);

		pose.position.x = Ts[i][0];
		pose.position.y = Ts[i][1];
		pose.position.z = Ts[i][2];

		if (pose.position.z < CLOSEST_ACCEPTABLE_POSE_M || pose.position.z > FARTHEST_ACCEPTABLE_POSE_M) {
			CS_FULL_DEBUG(cs,
			              "model %d failed to find a valid P3P solution for LED %d %d %d (%d) and blob %d "
			              "%d %d (%d) - pose out of range Z @ %f metres",
			              mi->id, model_leds[0]->id, model_leds[1]->id, model_leds[2]->id,
			              model_leds[3]->id, blobs[0]->blob->blob_id, blobs[1]->blob->blob_id,
			              blobs[2]->blob->blob_id, blobs[3]->blob->blob_id, pose.position.z);

			// The object is unlikely to be < 5cm or > 15m from the camera
			continue;
		}

#if 1
		math_quat_normalize(&pose.orientation);
#else
		// The quaternion produced should already be normalised, but sometimes it's not!
		l = math_quat_len(&pose.orientation);
		assert(l > 0.9999 && l < 1.0001);
#endif

		bool checks_failed = false;
		struct xrt_vec3 tmp;

		for (int p = 0; p < 3; p++) {
			struct xrt_vec3 tmpblob;

			// This pose must yield a projection of the anchor point, or something is really wrong
			math_quat_rotate_vec3(&pose.orientation, &model_leds[p]->position, &checkpos);
			math_vec3_accum(&pose.position, &checkpos);

			// And should be camera facing in this pose
			math_quat_rotate_vec3(&pose.orientation, &model_leds[p]->normal, &checkdir);
			math_vec3_normalize(&checkpos);

			double facing_dot = m_vec3_dot(checkpos, checkdir);

			// Require only that the anchor LED not be actively facing away from the camera
			// (that it's at worst perpendicular). Controller LEDs can be visible at that angle
			if (facing_dot > 0.0) {
				CS_FULL_DEBUG(cs,
				              "model %d failed to find a valid P3P solution for LED %d %d %d (%d) and "
				              "blob %d %d %d (%d) - LED not facing camera (dot %f)",
				              mi->id, model_leds[0]->id, model_leds[1]->id, model_leds[2]->id,
				              model_leds[3]->id, blobs[0]->blob->blob_id, blobs[1]->blob->blob_id,
				              blobs[2]->blob->blob_id, blobs[3]->blob->blob_id, facing_dot);

				// LED not facing the camera -> invalid pose
				checks_failed = true;
				break;
			}

			tmpblob.x = blobs[p]->point_homog[0];
			tmpblob.y = blobs[p]->point_homog[1];
			tmpblob.z = blobs[p]->point_homog[2];

			// Calculate the image plane projection of the anchor LED position and check it's within 2.5mm
			// of where it should be, to catch spurious failures in lambdatwist
			math_vec3_scalar_mul(1.0 / checkpos.z, &checkpos);
			tmp = m_vec3_sub(checkpos, tmpblob);
			float l_sq = m_vec3_len_sqrd(tmp);
			if (l_sq > ANCHOR_LED_MAX_DISTANCE_M_SQ) {
				CS_FULL_DEBUG(cs,
				              "Error pose candidate orient %f %f %f %f pos %f %f %f "
				              "LED %d @ %f %f %f projected to %f %f %f (err %f)",
				              pose.orientation.x, pose.orientation.y, pose.orientation.z,
				              pose.orientation.w, pose.position.x, pose.position.y, pose.position.z, p,
				              tmpblob.x, tmpblob.y, tmpblob.z, checkpos.x, checkpos.y, checkpos.z, l);
				checks_failed = true;
				break; // @todo: Figure out why this happened
			}

#if CHECK_ALL_PROJECTIONS
			struct xrt_vec2 reprojected;

			project_led_point(&model_leds[p]->position, &pose, cs->calib, &reprojected);
			double xdiff = reprojected.x - blobs[p]->blob->center_undistorted.x;
			double ydiff = reprojected.y - blobs[p]->blob->center_undistorted.y;

			CS_DEBUG(cs, "Blob %d @ %f,%f should match LED %d projection %f %f (err %f)", p,
			         blobs[p]->blob->center_undistorted.x, blobs[p]->blob->center_undistorted.y,
			         model_leds[p]->id, reprojected.x, reprojected.y, sqrt(xdiff * xdiff + ydiff * ydiff));
#else
			break;
#endif
		}

		if (checks_failed) {
			continue;
		}

		// Check against the 4th point to check the proposed P3P solution
		math_quat_rotate_vec3(&pose.orientation, xcheck, &checkpos);
		math_vec3_accum(&pose.position, &checkpos);
		math_vec3_scalar_mul(1.0 / checkpos.z, &checkpos);

		// Subtract projected point from undistorted 4th reference point to check error
		struct xrt_vec3 checkblob;
		checkblob.x = blobs[3]->point_homog[0];
		checkblob.y = blobs[3]->point_homog[1];
		checkblob.z = blobs[3]->point_homog[2];

		tmp = m_vec3_sub(checkpos, checkblob);
		float distance = m_vec3_len(tmp);

#if 0
		// Convert back to pixels for the debug output
		math_vec3_scalar_mul(cs->calib->calib.fx, &checkpos);
		math_vec3_scalar_mul(cs->calib->calib.fx, &checkblob);

		CS_DEBUG(cs, "model %u pose candidate orient %f %f %f %f pos %f %f %f", mi->id, pose.orientation.x,
		         pose.orientation.y, pose.orientation.z, pose.orientation.w, pose.position.x, pose.position.y,
		         pose.position.z);
		CS_DEBUG(cs, "4th point @ %f %f offset %f (undistorted) pixels from %f %f (blob_size %f)", checkpos.x,
		         checkpos.y, distance * cs->calib->calib.fx, checkblob.x, checkblob.y,
		         blobs[3]->max_dist * cs->calib->calib.fx);

#endif
		// Check that the 4th point projected to within its blob
		if (distance <= blobs[3]->max_dist) {
			CS_FULL_DEBUG(
			    cs,
			    "model %d found a valid P3P solution for LED %d %d %d (%d) and blob %d %d %d (%d) - "
			    "4th point projected within blob (err %f <= %f)",
			    mi->id, model_leds[0]->id, model_leds[1]->id, model_leds[2]->id, model_leds[3]->id,
			    blobs[0]->blob->blob_id, blobs[1]->blob->blob_id, blobs[2]->blob->blob_id,
			    blobs[3]->blob->blob_id, distance, blobs[3]->max_dist);

			if (correspondence_search_project_pose(cs, model, &pose, mi, depth) || 1) {
#if 0
				CS_DEBUG(cs,
				         "  P4P points %f,%f,%f -> %f %f"
				         "         %f,%f,%f -> %f %f"
				         "         %f,%f,%f -> %f %f"
				         "         %f,%f,%f -> %f %f",
				         x[0][0], x[0][1], x[0][2], y1[0], y1[1], x[1][0], x[1][1], x[1][2], y2[0],
				         y2[1], x[2][0], x[2][1], x[2][2], y3[0], y3[1], xcheck->x, xcheck->y,
				         xcheck->z, checkblob.x, checkblob.y);
#endif
			}
		} else {
			CS_FULL_DEBUG(cs,
			              "model %d failed to find a valid P3P solution for LED %d %d %d (%d) and blob %d "
			              "%d %d (%d) - 4th point projected outside blob (err %f > %f),",
			              mi->id, model_leds[0]->id, model_leds[1]->id, model_leds[2]->id,
			              model_leds[3]->id, blobs[0]->blob->blob_id, blobs[1]->blob->blob_id,
			              blobs[2]->blob->blob_id, blobs[3]->blob->blob_id, distance, blobs[3]->max_dist);
		}
	}
}

/*!
 * Select k entries from the n provided in candidate_list into
 * output_list, then call check_led_match() with the result_list
 */
static void
select_k_blobs_from_n(struct correspondence_search *cs,
                      struct cs_model_info *mi,
                      struct t_constellation_tracker_led **model_leds,
                      struct cs_image_point **result_list,
                      struct cs_image_point **output_list,
                      struct cs_image_point **candidate_list,
                      int k,
                      int n,
                      int depth)
{
	if (k == 1) {
		output_list[0] = candidate_list[0];
		check_led_against_model_subset(cs, mi, result_list, model_leds, depth);

		// Short circuit if we found a strong pose match already
		if ((mi->match_flags & POSE_MATCH_STRONG) && (mi->search_flags & CS_FLAG_STOP_FOR_STRONG_MATCH)) {
			return;
		}

		if (n > 1) {
			select_k_blobs_from_n(cs, mi, model_leds, result_list, output_list, candidate_list + 1, 1,
			                      n - 1, depth + 1);
		}

		return;
	}

	/*
	 * 2 branches here:
	 *   take the first entry, then select k-1 from n-1
	 *   don't take the first entry, select k from n-1 if n > k
	 */
	assert(k > 1);
	assert(n > 1);
	output_list[0] = candidate_list[0];
	select_k_blobs_from_n(cs, mi, model_leds, result_list, output_list + 1, candidate_list + 1, k - 1, n - 1,
	                      depth);

	// Short circuit if we found a strong pose match already
	if ((mi->match_flags & POSE_MATCH_STRONG) && (mi->search_flags & CS_FLAG_STOP_FOR_STRONG_MATCH)) {
		return;
	}

	if (n > k) {
		select_k_blobs_from_n(cs, mi, model_leds, result_list, output_list, candidate_list + 1, k, n - 1,
		                      depth + 1);
	}
}

/*
 * Generate quadruples of neighbouring blobs and pass to check_led_match().
 *
 * We want to
 *   a) preserve the first blob in the list each iteration
 *   b) Select combinations of 3 from the next 'max_search_depth' points.
 *
 * Permutation checking is done in the model LED inner loop, so this inner selection
 * only needs to try combos.
 *
 * We can work in a tmp array so as not to destroy the master list.
 *
 * The caller guarantees that the blob_list has at least max_search_depth+1
 * entries.
 */
static void
check_leds_against_anchor(struct correspondence_search *cs,
                          struct cs_model_info *mi,
                          struct t_constellation_tracker_led **model_leds,
                          struct cs_image_point *anchor)
{
	struct cs_image_point *work_list[MAX_BLOB_SEARCH_DEPTH + 1];
	int max_blob_search_depth = MIN(anchor->num_neighbours, mi->max_blob_depth);

	if (max_blob_search_depth < 3) {
		return; // Not enough blobs to compare against
	}

	work_list[0] = anchor;
	select_k_blobs_from_n(cs, mi, model_leds, work_list, work_list + 1, anchor->neighbours, 3,
	                      max_blob_search_depth, 1);
}

// Called with 4 model_leds that need checking against blob combos
static void
check_led_match(struct correspondence_search *cs,
                struct cs_model_info *mi,
                struct t_constellation_tracker_led **model_leds,
                int depth)
{
	int b;

	mi->led_depth = depth;

	for (b = 0; b < cs->num_points; b++) {
		struct cs_image_point *anchor = cs->points + b;
		mi->blob_index = b;
		check_leds_against_anchor(cs, mi, model_leds, anchor);
	}
}

/*
 * Select k constellation_led entries from the n provided in candidate_list into output_list, then call
 * check_led_match() with the result_list.
 * */
static void
select_k_leds_from_n(struct correspondence_search *cs,
                     struct cs_model_info *mi,
                     struct t_constellation_tracker_led **result_list,
                     struct t_constellation_tracker_led **output_list,
                     struct t_constellation_tracker_led **candidate_list,
                     int k,
                     int n,
                     int depth)
{
	if (k == 1) {
		struct t_constellation_tracker_led *swap_list[4];

		output_list[0] = candidate_list[0];
		check_led_match(cs, mi, result_list, depth);

		// Short circuit if we found a strong pose match already
		if ((mi->match_flags & POSE_MATCH_STRONG) && (mi->search_flags & CS_FLAG_STOP_FOR_STRONG_MATCH)) {
			return;
		}

		// Check the other orientation of blob 2/3, without affecting result_list that needs to stay intact for
		// other recursive calls
		swap_list[0] = result_list[0];
		swap_list[1] = result_list[2];
		swap_list[2] = result_list[1];
		swap_list[3] = result_list[3];

		check_led_match(cs, mi, swap_list, depth);

		// Short circuit if we found a strong pose match already
		if ((mi->match_flags & POSE_MATCH_STRONG) && (mi->search_flags & CS_FLAG_STOP_FOR_STRONG_MATCH)) {
			return;
		}

		// Skip branch: try remaining candidates so all C(n,3) combinations are covered
		if (n > 1) {
			select_k_leds_from_n(cs, mi, result_list, output_list, candidate_list + 1, 1, n - 1, depth + 1);
		}

		return;
	}

	/*
	 * 2 branches here:
	 *   take the first entry, then select k-1 from n-1
	 *   don't take the first entry, select k from n-1 if n > k
	 */
	assert(k > 1);
	assert(n > 1);
	output_list[0] = candidate_list[0];
	select_k_leds_from_n(cs, mi, result_list, output_list + 1, candidate_list + 1, k - 1, n - 1, depth);

	// Short circuit if we found a strong pose match already
	if ((mi->match_flags & POSE_MATCH_STRONG) && (mi->search_flags & CS_FLAG_STOP_FOR_STRONG_MATCH)) {
		return;
	}

	if (n > k) {
		select_k_leds_from_n(cs, mi, result_list, output_list, candidate_list + 1, k, n - 1, depth + 1);
	}
}

/*
 * Generate quadruples of neighbouring leds and pass to check_led_match().
 * We want to
 *   a) preserve the first led in the list each iteration
 *   b) Select combinations of 3 from the 'max_search_depth' neighbouring LEDs.
 *   c) Try 2 permutations, [0,1,2,3] and [0,2,1,3]. In each case index 0 is
 *   the anchor and index 3 is a disambiguation check on the P3P result
 *
 * We can work in a tmp array so as not to destroy the master list.
 *
 * The caller guarantees that the LED neighbours list has at least max_search_depth
 * entries.
 */
static void
generate_led_match_candidates(struct correspondence_search *cs,
                              struct cs_model_info *mi,
                              struct t_constellation_search_led_candidate *c)
{
	struct t_constellation_tracker_led *work_list[MAX_LED_SEARCH_DEPTH + 1];

	int max_search_depth = MIN(mi->max_led_depth, c->num_neighbours) - mi->min_led_depth + 1;
	if (max_search_depth < 3) {
		return; // Not enough LEDs to compare against
	}

	work_list[0] = c->led;
	select_k_leds_from_n(cs, mi, work_list, work_list + 1, c->neighbours + mi->min_led_depth - 1, 3,
	                     max_search_depth, mi->min_led_depth);
}

static bool
search_pose_for_model(struct correspondence_search *cs, struct cs_model_info *mi)
{
	const struct t_constellation_search_model *model = mi->model;
	int b, l;

	// Clear the info for this model
	memset(&mi->best_score, 0, sizeof(struct pose_metrics));
	mi->match_flags = 0;

	// Clear stats
	cs->num_trials = cs->num_pose_checks = 0;

	// Configure search params from the flags
	if (mi->search_flags & CS_FLAG_SHALLOW_SEARCH) {
		mi->max_blob_depth = 4; // Only test nearest neighbours of blobs +1
		mi->max_led_depth = 4;  // Test LEDs plus 1 neighbour removed
		mi->min_led_depth = 1;  // Start with the nearest LED neighbour
	} else {
		mi->max_blob_depth = MAX_BLOB_SEARCH_DEPTH;
		mi->max_led_depth = MAX_LED_SEARCH_DEPTH;
		mi->min_led_depth = 3; // Start with next nearest LED neighbour that shallow search stopped at
	}

	if (mi->search_flags & CS_FLAG_DEEP_SEARCH) {
		mi->max_blob_depth = MAX_BLOB_SEARCH_DEPTH;
		mi->max_led_depth = MAX_LED_SEARCH_DEPTH;
	}

#if DUMP_TIMING
	// Set the search start time
	mi->search_start_time = os_monotonic_get_ns();
#endif

	// Filter the list of blobs to unknown, or belonging to this model
	for (b = 0; b < cs->num_points; b++) {
		struct cs_image_point **all_neighbours = cs->blob_neighbours[b];
		struct cs_image_point *anchor = cs->points + b;
		int out_index = 0, in_index;
		t_constellation_led_id_it led_id = anchor->blob->matched_device_led_id;
		t_constellation_device_id_t object_id = anchor->blob->matched_device_id;

		if ((mi->search_flags & CS_FLAG_MATCH_ALL_BLOBS) || led_id == XRT_CONSTELLATION_INVALID_LED_ID ||
		    object_id == mi->id) {
			for (in_index = 0; in_index < cs->num_points && out_index < MAX_BLOB_SEARCH_DEPTH; in_index++) {
				struct cs_image_point *p = all_neighbours[in_index];

				// Don't include the blob in its own neighbours
				if (anchor->blob == p->blob) {
					continue;
				}

				led_id = p->blob->matched_device_led_id;
				object_id = p->blob->matched_device_id;

				if ((mi->search_flags & CS_FLAG_MATCH_ALL_BLOBS) ||
				    led_id == XRT_CONSTELLATION_INVALID_LED_ID || object_id == mi->id) {
					anchor->neighbours[out_index++] = p;
				}
			}
		}
		anchor->num_neighbours = out_index;

		CS_FULL_LOG(cs, "Model %d, blob %d @ %f,%f neighbours %d Search list:", mi->id, b,
		            anchor->blob->center_undistorted.x, anchor->blob->center_undistorted.y,
		            anchor->num_neighbours);
		for (int i = 0; i < anchor->num_neighbours; i++) {
			struct cs_image_point *p1 = anchor->neighbours[i];
			double dist = (p1->blob->center_distorted.y - anchor->blob->center_distorted.y) *
			                  (p1->blob->center_distorted.y - anchor->blob->center_distorted.y) +
			              (p1->blob->center_distorted.x - anchor->blob->center_distorted.x) *
			                  (p1->blob->center_distorted.x - anchor->blob->center_distorted.x);
			(void)dist; // Silence unused variable warning when not logging
			CS_FULL_LOG(cs, "\tLED ID %u (%f,%f) @ %f,%f. Dist %f", p1->blob->matched_device_led_id,
			            p1->point_homog[0], p1->point_homog[1], p1->blob->center_distorted.x,
			            p1->blob->center_distorted.y, sqrt(dist));
		}
	}


	// Start correspondence search for this model.
	// At this stage, each image point has a list of the nearest neighbours filtered for this model
	for (l = 0; l < model->num_points; l++) {
		struct t_constellation_search_led_candidate *c = model->points[l];
		mi->led_index = l;

		generate_led_match_candidates(cs, mi, c);

		// Stop early with a strong match if requested
		if ((mi->match_flags & POSE_MATCH_STRONG) && (mi->search_flags & CS_FLAG_STOP_FOR_STRONG_MATCH)) {
			break;
		}
	}

	return (mi->match_flags & POSE_MATCH_GOOD);
}

/*
 * Exported functions
 */

struct correspondence_search *
correspondence_search_new(const enum u_logging_level *ct_log_level, struct camera_model *camera_calib)
{
	struct correspondence_search *cs = U_TYPED_CALLOC(struct correspondence_search);

	cs->ct_log_level = ct_log_level;
	cs->calib = camera_calib;

	CS_DEBUG(cs, "Created new correspondence search with camera model %dx%d", camera_calib->width_true,
	         camera_calib->height_true);

	return cs;
}

void
correspondence_search_free(struct correspondence_search *cs)
{
	CS_DEBUG(cs, "Freeing correspondence search");

	if (cs->points) {
		free(cs->points);
	}

	free(cs);
}

void
correspondence_search_set_blobs(struct correspondence_search *cs, struct t_blob *blobs, int num_blobs)
{
	struct cs_image_point *blob_list[XRT_CONSTELLATION_MAX_BLOBS_PER_FRAME];

	assert(num_blobs <= XRT_CONSTELLATION_MAX_BLOBS_PER_FRAME);

	if (cs->points != NULL) {
		free(cs->points);
	}

	// @todo: Try to make this not allocate
	cs->points = U_TYPED_ARRAY_CALLOC(struct cs_image_point, num_blobs);
	cs->num_points = num_blobs;
	cs->blobs = blobs;

	CS_DUMP_BLOBS(cs, "Building blobs search array");
	for (int i = 0; i < num_blobs; i++) {
		struct cs_image_point *p = cs->points + i;
		struct t_blob *b = blobs + i;

		p->point_homog[0] = b->center_homogenized.x;
		p->point_homog[1] = b->center_homogenized.y;
		p->point_homog[2] = b->center_homogenized.z;

		p->size[0] = b->size.x / cs->calib->calib_pinhole.fx;
		p->size[1] = b->size.y / cs->calib->calib_pinhole.fy;
		p->max_dist = sqrt(p->size[0] * p->size[0] + p->size[1] * p->size[1]);

		p->blob = b;
		blob_list[i] = p;

		CS_DUMP_BLOBS(cs, "Blob %u = (%f,%f %fx%f) -> (%f, %f) %f x %f (homog (%f, %f) %f x %f) (LED id %d)", i,
		              b->center_undistorted.x, b->center_undistorted.y, b->size.x, b->size.y,
		              p->point_homog[0] * cs->calib->calib_pinhole.fx,
		              p->point_homog[1] * cs->calib->calib_pinhole.fy, p->size[0] * cs->calib->calib_pinhole.fx,
		              p->size[1] * cs->calib->calib_pinhole.fy, p->point_homog[0], p->point_homog[1],
		              p->size[0], p->size[1], b->matched_device_led_id);
	}

	// Now the blob_list is populated, loop over the blob list and for each,
	// and prepare a list of neighbours sorted by distance
	for (int i = 0; i < cs->num_points; i++) {
		struct cs_image_point *anchor = cs->points + i;

		// Sort the blobs by proximity to anchor blob
		sort_image_points(blob_list, cs->num_points, anchor);
		memcpy(cs->blob_neighbours[i], blob_list, cs->num_points * sizeof(struct cs_image_point *));
	}
}

bool
correspondence_search_find_one_pose(struct correspondence_search *cs,
                                    const struct t_constellation_search_model *model,
                                    enum correspondence_search_flags search_flags,
                                    struct xrt_pose *inout_pose,
                                    const struct xrt_vec3 *pos_error_thresh,
                                    const struct xrt_vec3 *rot_error_thresh,
                                    const struct xrt_vec3 *gravity_vector,
                                    float gravity_tolerance_rad,
                                    struct pose_metrics *out_score)
{
	assert(inout_pose != NULL);
	assert(out_score != NULL);

	// If neither deep nor shallow search was requested, do a full search
	if ((search_flags & (CS_FLAG_SHALLOW_SEARCH | CS_FLAG_DEEP_SEARCH)) == 0) {
		search_flags |= CS_FLAG_SHALLOW_SEARCH | CS_FLAG_DEEP_SEARCH;
	}

	struct cs_model_info mi = {
	    .id = model->device_id,
	    .model = model,
	    .best_pose = XRT_POSE_IDENTITY,
	    .match_flags = 0,
	    .search_flags = search_flags,
	};

	if (search_flags & CS_FLAG_HAVE_POSE_PRIOR) {
		assert(pos_error_thresh != NULL);
		assert(rot_error_thresh != NULL);

		mi.pose_prior = *inout_pose;
		mi.pos_error_thresh = pos_error_thresh;
		mi.rot_error_thresh = rot_error_thresh;
	}

	if (search_flags & CS_FLAG_MATCH_GRAVITY) {
		struct xrt_quat pose_gravity_twist;

		// We need a pose prior to extract the gravity swing to match
		assert((search_flags & CS_FLAG_HAVE_POSE_PRIOR) != 0);
		assert(gravity_vector != NULL);

		mi.gravity_vector = *gravity_vector;
		mi.gravity_tolerance_rad = gravity_tolerance_rad;

		math_quat_decompose_swing_twist(&inout_pose->orientation, gravity_vector, &mi.gravity_swing,
		                                &pose_gravity_twist);
	}

	if (search_pose_for_model(cs, &mi) && (mi.match_flags & POSE_MATCH_GOOD)) {
		*inout_pose = mi.best_pose;
		*out_score = mi.best_score;

		CS_TIMING(cs, "# Best %s match for model %d was %d points out of %d with error %f pixels^2",
		          (search_flags & CS_FLAG_MATCH_GRAVITY) ? "aligned" : "unaligned", mi.id,
		          mi.best_score.matched_blobs, mi.best_score.visible_leds, mi.best_score.reprojection_error);
		CS_TIMING(cs, "# Found at LED depth %d blob depth %d after %f ms of %f ms", mi.best_pose_led_depth,
		          mi.best_pose_blob_depth, (float)(mi.best_pose_found_time - mi.search_start_time) / 1000000.0,
		          (float)(os_monotonic_get_ns() - mi.search_start_time) / 1000000.0);
		CS_TIMING(cs, "# pose orient %f %f %f %f pos %f %f %f", mi.best_pose.orientation.x,
		          mi.best_pose.orientation.y, mi.best_pose.orientation.z, mi.best_pose.orientation.w,
		          mi.best_pose.position.x, mi.best_pose.position.y, mi.best_pose.position.z);
		return true;
	}

	return false;
}
