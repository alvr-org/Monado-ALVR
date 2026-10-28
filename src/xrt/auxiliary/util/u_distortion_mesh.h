// Copyright 2019, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Code to generate disortion meshes.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Moshi Turner <moshiturner@protonmail.com>
 * @ingroup aux_distortion
 */

#pragma once

#include "xrt/xrt_device.h"
#include "xrt/xrt_defines.h"

#include "util/u_distortion.h"


#ifdef __cplusplus
extern "C" {
#endif


/*
 *
 * Panotools distortion
 *
 */

/*!
 * Values to create a distortion mesh from panotools values.
 *
 * @ingroup aux_distortion
 */
struct u_panotools_values
{
	//! Panotools universal distortion k (reverse order from OpenHMD).
	float distortion_k[5];
	//! Panotools post distortion scale, <r, g, b>.
	float aberration_k[3];
	//! Panotools warp scale.
	float scale;
	//! Center of the lens.
	struct xrt_vec2 lens_center;
	//! Viewport size.
	struct xrt_vec2 viewport_size;
};

/*!
 * Distortion correction implementation for Panotools distortion values.
 *
 * @ingroup aux_distortion
 */
void
u_compute_distortion_panotools(struct u_panotools_values *values, float u, float v, struct xrt_uv_triplet *result);


/*
 *
 * Vive, Vive Pro & Index distortion
 *
 */

/*!
 * Values to create a distortion mesh from Vive configuration values.
 *
 * @ingroup aux_distortion
 */
struct u_vive_values
{
	float aspect_x_over_y;
	float grow_for_undistort;
	struct xrt_vec2 intrinsic_focus;
	struct xrt_vec2 intrinsic_center;

	float undistort_r2_cutoff;

	//! r/g/b
	struct xrt_vec2 center[3];

	//! r/g/b, a/b/c/d
	float coefficients[3][4];
};

/*!
 * Distortion correction implementation for the Vive, Vive Pro, Valve Index
 * distortion values found in the HMD configuration.
 *
 * @ingroup aux_distortion
 */
void
u_compute_distortion_vive(struct u_vive_values *values, float u, float v, struct xrt_uv_triplet *result);


/*
 *
 * Cardboard mesh distortion parameters.
 *
 */

/*!
 * Distortion correction implementation for the Cardboard devices.
 *
 * @ingroup aux_distortion
 */
void
u_compute_distortion_cardboard(struct u_cardboard_distortion_values *values,
                               float u,
                               float v,
                               struct xrt_uv_triplet *result);


/*
 *
 * Values for North Star 2D/Polynomial distortion correction.
 *
 */

struct u_ns_p2d_values
{
	float x_coefficients_left[16];
	float x_coefficients_right[16];
	float y_coefficients_left[16];
	float y_coefficients_right[16];
	struct xrt_fov fov[2]; // left, right
	float ipd;
};

/*!
 * Distortion correction implementation for North Star 2D/Polynomial.
 *
 * @ingroup aux_distortion
 */
void
u_compute_distortion_ns_p2d(struct u_ns_p2d_values *values, int view, float u, float v, struct xrt_uv_triplet *result);

/*
 *
 * Values for Moshi Turner's North Star distortion correction.
 *
 */
struct u_ns_meshgrid_values
{
	int number_of_ipds;
	float *ipds;
	int num_grid_points_u;
	int num_grid_points_v;
	struct xrt_vec2 *grid[2];
	struct xrt_fov fov[2]; // left, right
	float ipd;
};

/*!
 * Moshi Turner's North Star distortion correction implementation
 *
 * @ingroup aux_distortion
 */
void
u_compute_distortion_ns_meshgrid(
    struct u_ns_meshgrid_values *values, int view, float u, float v, struct xrt_uv_triplet *result);

/*
 *
 * Windows Mixed Reality distortion
 *
 */

struct u_poly_3k_distortion_values
{
	struct xrt_vec2_i32 display_size;

	/* X/Y center of the distortion (pixels) */
	struct xrt_vec2 eye_center;

	/* k1,k2,k3 params for radial distortion as
	 * per the radial distortion model in
	 * https://docs.opencv.org/4.x/d9/d0c/group__calib3d.html */
	double k[3];
};

struct u_poly_3k_eye_values
{
	//! Inverse affine transform to move from (undistorted) pixels
	//! to image plane / normalised image coordinates
	struct xrt_matrix_3x3 inv_affine_xform;

	//! tan(angle) FoV min/max for X and Y in the input texture
	struct xrt_vec2 tex_x_range;
	struct xrt_vec2 tex_y_range;

	struct u_poly_3k_distortion_values channels[3];
};

void
u_compute_distortion_poly_3k(
    struct u_poly_3k_eye_values *values, uint32_t view, float u, float v, struct xrt_uv_triplet *result);

/*
 * Compute the visible area bounds by calculating the X/Y limits of a
 * crosshair through the distortion center, and back-project to the render FoV,
 */
void
u_compute_distortion_bounds_poly_3k(const struct xrt_matrix_3x3 *inv_affine_xform,
                                    struct u_poly_3k_distortion_values *values,
                                    int view,
                                    struct xrt_fov *out_fov,
                                    struct xrt_vec2 *out_tex_x_range,
                                    struct xrt_vec2 *out_tex_y_range);


/*
 *
 * Rift-style distortion
 *
 */

#define U_RIFT_CHROMATIC_ABBERATION_COUNT 4

struct u_rift_panel
{
	//! Size of the display, in meters.
	struct xrt_vec2 size_m;
	//! The gap in meters between the two viewports. Zero in all real hardware.
	float eye_gap_m;

	//! Size of the display, in pixels.
	struct xrt_vec2 size_px;

	//! The distance between the lens centers, in meters.
	float lens_center_separation_m;
	//! The distance from the top of the panel to lens centers, in meters.
	float lens_center_from_panel_top;

	//! The diameter of the lens
	float lens_diameter_m;
};

struct u_rift_eye_profile
{
	//! Distance from the lens surface
	float eye_relief_m;

	//! The catmull-rom coeffecients.
	const float *k;
	//! The amount of coeffecients in use.
	int k_count;

	//! Color fringing correction
	float chromatic_abberation[U_RIFT_CHROMATIC_ABBERATION_COUNT];
	//! The tan-angle radius the last entry of @ref k corresponds to.
	float max_r;
	//! How many metres across the panel one unit of tan-gent covers, measured at the centre of the lens.
	float meters_per_tan_angle;
};

/*!
 * Oculus Rift distortion implementation.
 */
void
u_compute_distortion_rift(const struct u_rift_panel *panel,
                          const struct u_rift_eye_profile *profile,
                          uint32_t view,
                          float u,
                          float v,
                          struct xrt_uv_triplet *result);


/*
 *
 * None distortion
 *
 */

/*!
 * Helper function for none distortion devices.
 *
 * @ingroup aux_distortion
 */
xrt_result_t
u_distortion_mesh_none(struct xrt_device *xdev, uint32_t view, float u, float v, struct xrt_uv_triplet *result);


/*
 *
 * Mesh generation functions.
 *
 */

/*!
 * Given a @ref xrt_device generates meshes by calling
 * xdev->compute_distortion(), populates `xdev->hmd_parts.distortion.mesh` &
 * `xdev->hmd_parts.distortion.models`.
 *
 * @relatesalso xrt_device
 * @ingroup aux_distortion
 */
void
u_distortion_mesh_fill_in_compute(struct xrt_device *xdev);

/*!
 * Given a @ref xrt_device generates a no distortion mesh, populates
 * `xdev->hmd_parts.distortion.mesh` & `xdev->hmd_parts.distortion.models`.
 *
 * @relatesalso xrt_device
 * @ingroup aux_distortion
 */
void
u_distortion_mesh_fill_in_none(struct xrt_device *xdev);

/*!
 * Given a @ref xrt_device generates a no distortion mesh, also sets
 * `xdev->compute_distortion()` and populates `xdev->hmd_parts.distortion.mesh`
 * & `xdev->hmd_parts.distortion.models`.
 *
 * @relatesalso xrt_device
 * @ingroup aux_distortion
 */
void
u_distortion_mesh_set_none(struct xrt_device *xdev);


#ifdef __cplusplus
}
#endif
