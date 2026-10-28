// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Rift-style lens distortion, as used by the Oculus Rift DK2 and CV1.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup aux_util
 */

#include "math/m_api.h"
#include "math/m_vec2.h"

#include "u_distortion_mesh.h"


/*
 *
 * Helper functions
 *
 */

struct u_rift_distortion_render_info
{
	struct xrt_vec2 lens_center;
	struct xrt_vec2 tan_scale;
	struct xrt_vec2 pixels_per_tan;
};

struct u_rift_distortion_tan_fov
{
	float angle_left;
	float angle_right;
	float angle_up;
	float angle_down;
};

//! Computes the tan angle of a NDC point, as seen from the eye.
static inline struct xrt_vec2
to_tan_angle(const struct u_rift_distortion_render_info *info, struct xrt_vec2 ndc)
{
	return m_vec2_mul(m_vec2_sub(ndc, info->lens_center), info->tan_scale);
}

//! Value of the curve at knot j.
static float
value_at(const float *k, int j)
{
	if (j == 0) {
		return 1.0f;
	}

	return k[j];
}

//! Slope the curve leaves knot j with.
static float
slope_after(const float *k, int n, int j)
{
	if (j == 0) {
		return k[1] - k[0];
	}
	if (j <= (n - 3)) {
		return 0.5f * (k[j + 1] - k[j - 1]);
	}
	if (j == n - 2) {
		return 0.5f * (k[n - 1] - k[n - 2]);
	}
	return k[n - 1] - k[n - 2];
}

/*!
 * Slope the curve arrives at knot j with.
 * Same as slopeAfter everywhere except knot n-2; see section 5.3.
 */
static float
slope_before(const float *k, int n, int j)
{
	if (j <= n - 3) {
		return slope_after(k, n, j);
	}
	if (j == n - 2) {
		return 0.5f * (k[n - 1] - k[n - 3]);
	}
	return k[n - 1] - k[n - 2];
}

/*!
 * Standard cubic Hermite: value p0 with slope m0 at t=0, value p1 with slope m1 at t=1.
 * Any equivalent arrangement of these terms will do.
 */
static float
hermite(float p0, float m0, float p1, float m1, float t)
{
	const float t2 = t * t;
	const float t3 = t2 * t;
	return (2 * t3 - 3 * t2 + 1) * p0 //
	       + (t3 - 2 * t2 + t) * m0   //
	       + (-2 * t3 + 3 * t2) * p1  //
	       + (t3 - t2) * m1;          //
}

//! Magnification at curve position s.
static float
curve(const float *k, int n, float s)
{
	const int j = CLAMP((int)floorf(s), 0, n - 1); // pick the segment
	const float t = s - j;                         // how far into it

	const float p0 = value_at(k, j);
	const float m0 = slope_after(k, n, j);

	float p1, m1;
	if (j == n - 1) {
		p1 = p0 + m0;
		m1 = m0;
	} else {
		p1 = value_at(k, j + 1);
		m1 = slope_before(k, n, j + 1);
	}

	return hermite(p0, m0, p1, m1, t);
}

//! The magnification for a squared tan-angle radius.
static float
magnification(const struct u_rift_eye_profile *profile, float rho)
{
	const int n = profile->k_count;
	const float s = (n - 1) * rho / (profile->max_r * profile->max_r);
	return curve(profile->k, n, s);
}

static struct u_rift_distortion_render_info
compute_render_info(const struct u_rift_panel *panel, const struct u_rift_eye_profile *profile, uint32_t view)
{
	// One eye gets half the panel’s width but all of its height.
	const float visible_width = (panel->size_m.x - panel->eye_gap_m) / 2.0f;

	// How much tan-angle spans the full [-1, 1] of NDC, in each axis.
	const struct xrt_vec2 tan_scale = m_vec2_div(panel->size_m, //
	                                             XRT_C11_COMPOUND(struct xrt_vec2){
	                                                 .x = 4 * profile->meters_per_tan_angle,
	                                                 .y = 2 * profile->meters_per_tan_angle,
	                                             });

	// Where the lens sits, as a fraction of the eye's viewport, remapped from [0, 1] to [-1, 1].
	const float from_left = (panel->size_m.x - panel->lens_center_separation_m) / 2.0f;
	struct xrt_vec2 lens_center = {
	    .x = 2.0f * (from_left / visible_width) - 1.0f,
	    .y = 2.0f * (panel->lens_center_from_panel_top / panel->size_m.y) - 1.0f,
	};

	// The two halves of the panel are mirror images, so the second eye's lens centre is the first one flipped.
	if (view != 0) {
		lens_center.x *= -1.0f;
	}

	const struct xrt_vec2 pixels_per_tan =
	    m_vec2_div(m_vec2_mul_scalar(panel->size_px, profile->meters_per_tan_angle),
	               XRT_C11_COMPOUND(struct xrt_vec2){
	                   .x = panel->size_m.x - panel->eye_gap_m,
	                   .y = panel->size_m.y,
	               });

	return XRT_C11_COMPOUND(struct u_rift_distortion_render_info){
	    .lens_center = lens_center,
	    .tan_scale = tan_scale,
	    .pixels_per_tan = pixels_per_tan,
	};
}

/*!
 * Computes the tangent FOV for an eye from the given parameters.
 *
 * @param lens_diameter    The diameter of the lens.
 * @param eye_relief       The eye relief of the lens.
 * @param rot_rad          The rotation of the eye, in radians. Max 30 degrees.
 */
static struct u_rift_distortion_tan_fov
fov_from_eye(float lens_diameter, float eye_relief, float rot_rad)
{
	const float pupil_radius = 0.0135f; // meters, eyeball center to pupil

	rot_rad = CLAMP(rot_rad, 0.0f, DEG_TO_RAD(30.0f));

	const float half_lens = lens_diameter / 2.0f;
	const float straight_ahead = half_lens / eye_relief;

	// Rolling the eye makes the pupil swing sideways and backwards slightly. 1mm lateral slack?
	const float sideways = pupil_radius * sinf(rot_rad) //
	                       + 0.001f * (rot_rad / DEG_TO_RAD(30.0f));
	const float backwards = pupil_radius * (1.0f - cosf(rot_rad));
	const float rolled = (half_lens + sideways) / (eye_relief + backwards);

	float tan = MAX(straight_ahead, rolled);

	return XRT_C11_COMPOUND(struct u_rift_distortion_tan_fov){
	    .angle_left = tan,
	    .angle_right = tan,
	    .angle_up = tan,
	    .angle_down = tan,
	};
}

/*!
 * Walk from the lens centre to `edge`, and return how far the view
 * reaches in one direction. `axis` picks x or y; `sign` is -1 for the
 * left and up directions, which run towards negative tan-angle.
 */
static float
physical_fov_sweep(const struct u_rift_distortion_render_info *info,
                   const struct u_rift_eye_profile *profile,
                   const struct xrt_vec2 edge,
                   int axis,
                   float sign)
{
	const int steps = 10;

	float best = 0; // never report negative extent
	for (int i = 0; i < steps; i++) {
		const float f = (float)i / (steps - 1);
		const struct xrt_vec2 ndc = m_vec2_lerp(info->lens_center, edge, f);
		const struct xrt_vec2 v = to_tan_angle(info, ndc);
		const struct xrt_vec2 scaled = m_vec2_mul_scalar(v, magnification(profile, v.x * v.x + v.y * v.y));
		best = MAX(best, sign * (axis == 0 ? scaled.x : scaled.y));
	}
	return best;
}

static struct u_rift_distortion_tan_fov
physical_fov(const struct u_rift_distortion_render_info *info, const struct u_rift_eye_profile *profile)
{
	const struct xrt_vec2 c = info->lens_center;

	return XRT_C11_COMPOUND(struct u_rift_distortion_tan_fov){
	    .angle_left =
	        physical_fov_sweep(info, profile, XRT_C11_COMPOUND(struct xrt_vec2){.x = -1.0f, .y = c.y}, 0, -1.0f),
	    .angle_right =
	        physical_fov_sweep(info, profile, XRT_C11_COMPOUND(struct xrt_vec2){.x = 1.0f, .y = c.y}, 0, 1.0f),
	    .angle_up =
	        physical_fov_sweep(info, profile, XRT_C11_COMPOUND(struct xrt_vec2){.x = c.x, .y = -1.0f}, 1, -1.0f),
	    .angle_down =
	        physical_fov_sweep(info, profile, XRT_C11_COMPOUND(struct xrt_vec2){.x = c.x, .y = 1.0f}, 1, 1.0f),
	};
}

static struct u_rift_distortion_tan_fov
fov(const struct u_rift_panel *panel,
    const struct u_rift_eye_profile *profile,
    const struct u_rift_distortion_render_info *info)
{
	const struct u_rift_distortion_tan_fov from_eye =
	    fov_from_eye(panel->lens_diameter_m, profile->eye_relief_m, DEG_TO_RAD(30.0f));
	const struct u_rift_distortion_tan_fov from_panel = physical_fov(info, profile);

	return XRT_C11_COMPOUND(struct u_rift_distortion_tan_fov){
	    .angle_left = MIN(from_eye.angle_left, from_panel.angle_left),
	    .angle_right = MIN(from_eye.angle_right, from_panel.angle_right),
	    .angle_up = MIN(from_eye.angle_up, from_panel.angle_up),
	    .angle_down = MIN(from_eye.angle_down, from_panel.angle_down),
	};
}

struct u_rift_distortion_scale_offset
{
	struct xrt_vec2 scale;
	struct xrt_vec2 offset;
};

static struct u_rift_distortion_scale_offset
ndc_mapping(const struct u_rift_distortion_tan_fov *fov)
{
	return XRT_C11_COMPOUND(struct u_rift_distortion_scale_offset){
	    .scale =
	        {
	            .x = 2.0f / (fov->angle_left + fov->angle_right),
	            .y = 2.0f / (fov->angle_up + fov->angle_down),
	        },
	    .offset =
	        {
	            .x = (fov->angle_left - fov->angle_right) / (fov->angle_left + fov->angle_right),
	            .y = (fov->angle_up - fov->angle_down) / (fov->angle_up + fov->angle_down),
	        },
	};
}

static struct u_rift_distortion_scale_offset
uv_mapping(const struct u_rift_distortion_scale_offset ndc)
{
	return XRT_C11_COMPOUND(struct u_rift_distortion_scale_offset){
	    .scale = m_vec2_mul_scalar(ndc.scale, 0.5f),
	    .offset = m_vec2_add_scalar(m_vec2_mul_scalar(ndc.offset, 0.5f), 0.5f),
	};
}

static void
colour_fringing_scales(const struct u_rift_eye_profile *profile, float rho, float *scales)
{
	const float r = magnification(profile, rho);

	const float *c = profile->chromatic_abberation;

	scales[0] = r * (1.0f + c[0] + rho * c[1]);
	scales[1] = r;
	scales[2] = r * (1.0f + c[2] + rho * c[3]);
}

/*
 *
 * Exported functions.
 *
 */

void
u_compute_distortion_rift(const struct u_rift_panel *panel,
                          const struct u_rift_eye_profile *profile,
                          uint32_t view,
                          float u,
                          float v,
                          struct xrt_uv_triplet *result)
{
	assert(profile->k_count >= 4);

	const struct u_rift_distortion_render_info info = compute_render_info(panel, profile, 0);
	const struct u_rift_distortion_tan_fov port = fov(panel, profile, &info);
	const struct u_rift_distortion_scale_offset uv_map = uv_mapping(ndc_mapping(&port));

	struct xrt_vec2 ndc = m_vec2_sub_scalar(m_vec2_mul_scalar(
	                                            XRT_C11_COMPOUND(struct xrt_vec2){
	                                                .x = u,
	                                                .y = v,
	                                            },
	                                            2.0f),
	                                        1.0f);

	if (view == 0) {
		ndc.x *= -1.0f;
	}

	const struct xrt_vec2 tan = to_tan_angle(&info, ndc);
	const float rho = tan.x * tan.x + tan.y * tan.y;
	float scales[3];
	colour_fringing_scales(profile, rho, scales);

	struct xrt_vec2 *out[3] = {&result->r, &result->g, &result->b};
	for (int i = 0; i < 3; i++) {
		(*out[i]) = m_vec2_add(m_vec2_mul(m_vec2_mul_scalar(tan, scales[i]), uv_map.scale), uv_map.offset);

		if (view == 0) {
			out[i]->x = 1 - out[i]->x;
		}
	}
}
