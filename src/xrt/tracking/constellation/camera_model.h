// Copyright 2014-2015 Philipp Zabel
// Copyright 2019-2023 Jan Schmidt
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Constellation tracker's camera model storage.
 * @author Philipp Zabel <philipp.zabel@gmail.com>
 * @author Jan Schmidt <jan@centricular.com>
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup tracking
 */

#pragma once

#include "tracking/t_camera_models.h"

#include <math.h>


struct camera_model
{
	//! Distorted frame width
	int width_true;
	//! Distorted frame height
	int height_true;
	//! The extents of the true frame's undistorted field of view, in pinhole pixel coordinates.
	float left_pinhole;
	//! See @ref left_pinhole
	float right_pinhole;
	//! Valid pinhole y coordinates are [top_pinhole, bottom_pinhole].
	float top_pinhole;
	//! See @ref top_pinhole
	float bottom_pinhole;

	//! The true calibration of the camera
	struct t_camera_model_params calib_true;
	//! An identity calibration model
	struct t_camera_model_params calib_pinhole;
};

/*!
 * Creates a camera model, computing a pinhole model (with the same focal lengths as the true calibration) whose
 * image covers the whole undistorted field of view of the true image.
 */
static inline struct camera_model
camera_model_init(int width_true, int height_true, const struct t_camera_model_params *calib)
{
	// Sample the whole border of the true image (corners included, so nothing a blob can land on gets cropped), and
	// find the extents of the undistorted (normalized) points
	const float w = (float)width_true;
	const float h = (float)height_true;
	const int steps = 64;
	float min_x = INFINITY, min_y = INFINITY, max_x = -INFINITY, max_y = -INFINITY;
	for (int i = 0; i <= steps; i++) {
		const float tx = w * (float)i / (float)steps;
		const float ty = h * (float)i / (float)steps;
		const float border[4][2] = {{tx, 0.0f}, {tx, h}, {0.0f, ty}, {w, ty}};

		for (int j = 0; j < 4; j++) {
			float x, y;
			t_camera_models_undistort(calib, border[j][0], border[j][1], &x, &y);
			if (!isfinite(x) || !isfinite(y)) {
				continue;
			}
			min_x = fminf(min_x, x);
			max_x = fmaxf(max_x, x);
			min_y = fminf(min_y, y);
			max_y = fmaxf(max_y, y);
		}
	}

	// Fall back to the true frame if nothing undistorted cleanly
	float left_pinhole = 0.0f;
	float right_pinhole = w;
	float top_pinhole = 0.0f;
	float bottom_pinhole = h;
	struct t_camera_model_params calib_pinhole = *calib;
	calib_pinhole.model = T_DISTORTION_PINHOLE;

	// if we found valid stuff, map that in, will be bigger than the normal model cus distortion.
	if (min_x <= max_x && min_y <= max_y) {
		left_pinhole = min_x * calib->fx + calib->cx;
		right_pinhole = max_x * calib->fx + calib->cx;
		top_pinhole = min_y * calib->fy + calib->cy;
		bottom_pinhole = max_y * calib->fy + calib->cy;
	}

	struct camera_model model = {
	    .width_true = width_true,
	    .height_true = height_true,
	    .left_pinhole = left_pinhole,
	    .right_pinhole = right_pinhole,
	    .top_pinhole = top_pinhole,
	    .bottom_pinhole = bottom_pinhole,
	    .calib_true = *calib,
	    .calib_pinhole = calib_pinhole,
	};
	return model;
}
