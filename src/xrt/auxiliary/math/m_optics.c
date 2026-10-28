// Copyright 2019, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Functions related to field-of-view.
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @ingroup aux_math
 */

#include "math/m_mathinclude.h"
#include "math/m_api.h"
#include "util/u_debug.h"

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <assert.h>


DEBUG_GET_ONCE_BOOL_OPTION(views, "MATH_DEBUG_VIEWS", false)

/*!
 * Perform some of the computations from
 * "Computing Half-Fields-Of-View from Simpler Display Models",
 * to solve for the half-angles for a triangle where we know the center and
 * total angle but not the "distance".
 *
 * In the diagram below, the top angle is theta_total, the length of the bottom
 * is w_total, and the distance between the vertical line and the left corner is
 * w_1.
 * out_theta_1 is the angle at the top of the left-most right triangle,
 * out_theta_2 is the angle at the top of the right-most right triangle,
 * and out_d is the length of that center vertical line, a logical "distance".
 *
 * Any outparams that are NULL will simply not be set.
 *
 * The triangle need not be symmetrical, despite how the diagram looks.
 *
 * ```
 *               theta_total
 *                    *
 *       theta_1 -> / |  \ <- theta_2
 *                 /  |   \
 *                /   |d   \
 *               /    |     \
 *              -------------
 *              [ w_1 ][ w_2 ]
 *
 *              [ --- w  --- ]
 * ```
 *
 * Distances are in arbitrary but consistent units. Angles are in radians.
 *
 * @return true if successful.
 */
static bool
math_solve_triangle(
    double w_total, double w_1, double theta_total, double *out_theta_1, double *out_theta_2, double *out_d)
{
	/* should have at least one out-variable */
	assert(out_theta_1 || out_theta_2 || out_d);
	const double w_2 = w_total - w_1;

	const double u = w_2 / w_1;
	const double v = tan(theta_total);

	/* Parts of the quadratic formula solution */
	const double b = u + 1.0;
	const double root = sqrt(b * b + 4 * u * v * v);
	const double two_a = 2 * v;

	/* The two possible solutions. */
	const double tan_theta_2_plus = (-b + root) / two_a;
	const double tan_theta_2_minus = (-b - root) / two_a;
	const double theta_2_plus = atan(tan_theta_2_plus);
	const double theta_2_minus = atan(tan_theta_2_minus);

	/* Pick the solution that is in the right range. */
	double tan_theta_2 = 0;
	double theta_2 = 0;
	if (theta_2_plus > 0.f && theta_2_plus < theta_total) {
		// OH_DEBUG(ohd, "Using the + solution to the quadratic.");
		tan_theta_2 = tan_theta_2_plus;
		theta_2 = theta_2_plus;
	} else if (theta_2_minus > 0.f && theta_2_minus < theta_total) {
		// OH_DEBUG(ohd, "Using the - solution to the quadratic.");
		tan_theta_2 = tan_theta_2_minus;
		theta_2 = theta_2_minus;
	} else {
		// OH_ERROR(ohd, "NEITHER QUADRATIC SOLUTION APPLIES!");
		return false;
	}
#define METERS_FORMAT "%0.4fm"
#define DEG_FORMAT "%0.1f deg"
	if (debug_get_bool_option_views()) {
		const double rad_to_deg = M_1_PI * 180.0;
		// comments are to force wrapping
		U_LOG_D("w=" METERS_FORMAT " theta=" DEG_FORMAT "    w1=" METERS_FORMAT " theta1=" DEG_FORMAT
		        "    w2=" METERS_FORMAT " theta2=" DEG_FORMAT "    d=" METERS_FORMAT,
		        w_total, theta_total * rad_to_deg,         //
		        w_1, (theta_total - theta_2) * rad_to_deg, //
		        w_2, theta_2 * rad_to_deg,                 //
		        w_2 / tan_theta_2);
	}
	if (out_theta_2) {
		*out_theta_2 = theta_2;
	}

	if (out_theta_1) {
		*out_theta_1 = theta_total - theta_2;
	}
	if (out_d) {
		*out_d = w_2 / tan_theta_2;
	}
	return true;
}

bool
math_compute_fovs(double w_total,
                  double w_1,
                  double horizfov_total,
                  double h_total,
                  double h_1,
                  double vertfov_total,
                  struct xrt_fov *fov)
{
	double d = 0;
	double theta_1 = 0;
	double theta_2 = 0;
	if (!math_solve_triangle(w_total, w_1, horizfov_total, &theta_1, &theta_2, &d)) {
		/* failure is contagious */
		return false;
	}

	fov->angle_left = (float)-theta_1;
	fov->angle_right = (float)theta_2;

	double phi_1 = 0;
	double phi_2 = 0;
	if (vertfov_total == 0) {
		phi_1 = atan(h_1 / d);

		/* h_2 is "up".
		 * so the corresponding phi_2 is naturally positive.
		 */
		const double h_2 = h_total - h_1;
		phi_2 = atan(h_2 / d);
	} else {
		/* Run the same algorithm again for vertical. */
		if (!math_solve_triangle(h_total, h_1, vertfov_total, &phi_1, &phi_2, NULL)) {
			/* failure is contagious */
			return false;
		}
	}

	/* phi_1 is "down" so we record this as negative. */
	fov->angle_down = (float)(-phi_1);
	fov->angle_up = (float)phi_2;

	return true;
}


void
math_compute_parallelized_fov(const struct xrt_fov *fov,
                              const struct xrt_quat *canted_view_orientation,
                              struct xrt_fov *out_parallelized_fov)
{
	/*
	 * The FOV angles are defined by looking directly at the view, no matter
	 * how the view is rotated.
	 *
	 * The FOV is defined by angles, therefore not bound to specific sizes.
	 * For ease of calculations, we assume a triangle where the distance from
	 * the "eye" to the view is 1 unit (e.g. meter). With the adjacent side
	 * being 1 unit, the tangent tan(angle) = opposite / adjacent is simplified
	 * to tan(angle) = opposite.
	 *
	 * So tan_? is the distance from the view axis and view intersection to the
	 * respective edge of the FOV.
	 */
	double tan_l = tanf(fov->angle_left);
	double tan_r = tanf(fov->angle_right);
	double tan_u = tanf(fov->angle_up);
	double tan_d = tanf(fov->angle_down);

	/*
	 * With tan_? being the distance to the FOV edge, define a frustum using
	 * the 4 corners of the FOV.
	 */
	struct xrt_vec3 frustum_corners[4] = {
	    {tan_l, tan_u, -1.0f}, // Top-Left
	    {tan_r, tan_u, -1.0f}, // Top-Right
	    {tan_r, tan_d, -1.0f}, // Bottom-Right
	    {tan_l, tan_d, -1.0f}, // Bottom-Left
	};

	/*
	 * Left and Down will grow towards negative, Right and Up will grow towards
	 * positive.
	 */
	double new_tan_l = INFINITY;
	double new_tan_r = -INFINITY;
	double new_tan_u = -INFINITY;
	double new_tan_d = INFINITY;

	for (int i = 0; i < 4; i++) {
		struct xrt_vec3 rotated_frustum_corner;

		/*
		 * Now that we have the FOV defined by corner points, which are also
		 * vectors to the corner points, we need to actually calculate the FOV
		 * for a canted view given a parallel view.
		 *
		 * Parallelized views can be imagined as two rectangles that ar
		 * sitting parallel on a wall that is 1 unit away from each "eye".
		 *
		 * The application will render content into those two rectangles.
		 * The compositor will later rotate the views direction away, such that
		 * the rectangles will no longer be sitting next to each other on the
		 * wall. However, the application will still have rendered for parallel
		 * views in any case, because this entire exercise is to cater to
		 * applications that can render only to parallel views.
		 *
		 * This means that we should tell the application to render to parallel
		 * rectangles on the wall, but adjust these rectangles such that the
		 * area covers the FOV *after* the view direction is oriented back to
		 * the canted orientation.
		 * We achieve this by rotating the corner vectors by the canting
		 * orientation.
		 */
		math_quat_rotate_vec3(canted_view_orientation, &frustum_corners[i], &rotated_frustum_corner);

		/*
		 * Don't divide by zero in the unlikely case that a corner is rotated so
		 * far, it is behind the "eye".
		 */
		double distance = -rotated_frustum_corner.z;
		if (distance < DBL_EPSILON) {
			distance = DBL_EPSILON;
		}

		/*
		 * The corner vectors are now pointing in the right direction, but now
		 * we want to go back to angles and need the triangle implied by the
		 * view axis from the "eye" perpendicular to the view, the distance to
		 * the edges on the view and from the edge to the "eye" to have a view
		 * axis of unit length 1 again.
		 *
		 * Scaling this vector by -rotated_frustum_corner.z gives us
		 * rotated_frustum_corner.z / -rotated_frustum_corner.z = -1 for the z
		 * axis, which is the distance we want.
		 * Scaling x and y by the same value does not change the direction of
		 * the vector, preserving the angles.
		 */
		double projected_frustum_corner_x = rotated_frustum_corner.x / distance;
		double projected_frustum_corner_y = rotated_frustum_corner.y / distance;

		/*
		 * Grow the respective edge of the frustum if applicable. Because we
		 * scaled to a view axis length of 1 unit, the distance on the view to
		 * the FOV edge / (view axis length = 1) is a tangent.
		 */
		new_tan_l = fmin(new_tan_l, projected_frustum_corner_x);
		new_tan_r = fmax(new_tan_r, projected_frustum_corner_x);
		new_tan_d = fmin(new_tan_d, projected_frustum_corner_y);
		new_tan_u = fmax(new_tan_u, projected_frustum_corner_y);
	}

	// Convert the tangent we calculated back to an angle
	*out_parallelized_fov = (struct xrt_fov){
	    .angle_left = atan(new_tan_l),
	    .angle_right = atan(new_tan_r),
	    .angle_up = atan(new_tan_u),
	    .angle_down = atan(new_tan_d),
	};
}
