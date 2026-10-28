// Copyright 2025-2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Code to parse and handle the modern non-LH HTC configuration data.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup aux_htc
 */

#include "util/u_json.hpp"
#include "util/u_logging.h"
#include "util/u_debug.h"

#include "math/m_api.h"

#include "htc_config.h"

#include <string.h>
#include <math.h>

#include <optional>


using xrt::auxiliary::util::json::JSONNode;

// Allows users to force disable the HTC-specific distortion. Useful when lens-modding headsets like Vive Pro 2.
DEBUG_GET_ONCE_BOOL_OPTION(force_disable_distortion, "HTC_FORCE_DISABLE_DISTORTION", false)

static const char *
htc_colour_name(int colour)
{
	switch (colour) {
	case 0: return "red";
	case 1: return "green";
	case 2: return "blue";
	default: throw std::runtime_error("Invalid colour index");
	}
}

static bool
htc_get_distortion_parameter_count(
    enum htc_distortion_model model, size_t &radial, size_t &tangential, size_t &prism, size_t &wvr, bool &modify)
{
	switch (model) {
	case HTC_DISTORTION_MODEL_TRADITIONAL_SIMPLE: {
		radial = 3;
		tangential = 0;
		prism = 0;
		wvr = 0;
		modify = false;
		return true;
	}
	case HTC_DISTORTION_MODEL_TRADITIONAL_WITH_TANGENTIAL: {
		radial = 3;
		tangential = 2;
		prism = 0;
		wvr = 0;
		modify = false;
		return true;
	}
	case HTC_DISTORTION_MODEL_RADIAL_TANGENTIAL_PRISM: {
		radial = 3;
		tangential = 2;
		prism = 2;
		wvr = 0;
		modify = false;
		return true;
	}
	case HTC_DISTORTION_MODEL_STRENGTHEN_HIGH_ORDER: {
		radial = 12;
		tangential = 6;
		prism = 4;
		wvr = 6;
		modify = true;
		return true;
	}
	default: return false;
	}
}

static void
htc_get_grow_for_undistort(htc_config &config, int eye, float grow_for_undistort[4])
{
	auto &distortion = config.lens_correction.eyes[eye];

	if (distortion.grow_for_undistort[2] == 0.0f) {
		for (int i = 0; i < 4; i++) {
			grow_for_undistort[i] = distortion.grow_for_undistort[0];
		}
	} else {
		// Right eye swaps the first two grow values.
		grow_for_undistort[0] = distortion.grow_for_undistort[eye == 0 ? 0 : 1];
		grow_for_undistort[1] = distortion.grow_for_undistort[eye == 0 ? 1 : 0];
		grow_for_undistort[2] = distortion.grow_for_undistort[2];
		grow_for_undistort[3] = distortion.grow_for_undistort[3];
	}
}

// clang-format off
static htc_eye_distortion default_distortions[2] = {
    {
        .center = {.x = 66, .y = 0},
        .model = HTC_DISTORTION_MODEL_RADIAL_TANGENTIAL_PRISM,
        .coeffecients =
            {
                {
                    .k = {1.0, 4.4488459, -68.805397, 1920.9266, -18288.236, 83074.07},
                    .p = {0.03657493, -0.011930554},
                    .s = {0.019099342, -0.017643826},
                },
                {
                    .k = {1.0, 3.4377847, -40.218407, 1579.1036, -16746.615, 79700.078},
                    .p = {0.035353571, -0.01024563},
                    .s = {0.014051946, -0.014563916},
                },
                {
                    .k = {1.0, 4.4488459, -68.805397, 1920.9266, -18288.236, 83074.07},
                    .p = {0.03657493, -0.011930554},
                    .s = {0.019099342, -0.017643826},
                },
            },
        .wvr = {},
        .modify =
            {
                .k = {0},
                .theta = 0.0,
            },
        .enlarge_ratio = 1.399999976158142,
        .grow_for_undistort = {0.4000000059604645, 0, 0, 0},
        .intrinsics =
            {
                {.v =
                     {
                         2.5617285, 0.0, -0.17111111, //
                         0.0, 2.5617285, 0.0,         //
                         0.0, 0.0, 1.0,               //
                     }},
                {.v =
                     {
                         1, 0, 0, //
                         0, 1, 0, //
                         0, 0, 1, //
                     }},
            },
        .resolution = {.x = 1080, .y = 1080},
        .scale = 1.033994913101196,
        .scale_ratio = 1.0,
        .normalized_radius = 1860.478393554688,
        .version = {.major = 1, .minor = 5},
        .warp =
            {
                .post = {0},
                .pre = {0},
                .max_radius = -1.0f,
            },
    },
    {
        .center = {.x = -69, .y = -5},
        .model = HTC_DISTORTION_MODEL_RADIAL_TANGENTIAL_PRISM,
        .coeffecients =
            {
                {
                    .k = {1.0, 3.4478931, -20.322369, 566.95569, -2448.3857, 5911.0508},
                    .p = {0.023906298, 0.017085155},
                    .s = {-0.028220069, -0.03382346},
                },
                {
                    .k = {1.0, 4.1760125, -47.955959, 1033.1934, -5843.1089, 15168.1},
                    .p = {0.024068261, 0.017389692},
                    .s = {-0.026223581, -0.034241389},
                },
                {
                    .k = {1.0, 5.2303009, -81.742905, 1496.7262, -8402.2266, 20153.543},
                    .p = {0.024962809, 0.016746229},
                    .s = {-0.024056688, -0.035858303},
                },
            },
        .wvr = {},
        .modify =
            {
                .k = {0},
                .theta = 0.0,
            },
        .enlarge_ratio = 1.399999976158142,
        .grow_for_undistort = {0.4000000059604645, 0, 0, 0},
        .intrinsics =
            {
                {.v =
                     {
                         2.5617285, 0.0, 0.1788889,     //
                         0.0, 2.5617285, -0.0032407406, //
                         0.0, 0.0, 1.0,                 //
                     }},
                {.v =
                     {
                         1, 0, 0, //
                         0, 1, 0, //
                         0, 0, 1, //
                     }},
            },
        .resolution = {.x = 1080, .y = 1080},
        .scale = 1.147185206413269,
        .scale_ratio = 1.0,
        .normalized_radius = 1860.478393554688,
        .version = {.major = 1, .minor = 5},
        .warp =
            {
                .post = {0},
                .pre = {0},
                .max_radius = -1.0f,
            },
    },
};
// clang-format on

static void
htc_eye_default_params(htc_eye_distortion &eye, bool left)
{
	if (left) {
		eye = default_distortions[0];
	} else {
		eye = default_distortions[1];
	}
}

static bool
htc_eye_parse(JSONNode &node, htc_eye_distortion &eye, bool left)
{
	const htc_eye_distortion &default_distortion = default_distortions[left ? 0 : 1];

	const char *eye_name = left ? "left" : "right";

	// Parse version
	std::string version_string = node["version"].asString();
	size_t dot_index = version_string.find('.');
	if (dot_index == std::string::npos || dot_index == 0 || dot_index == version_string.length() - 1) {
		U_LOG_E("Invalid version string '%s' in '%s' eye.", version_string.c_str(), eye_name);
		return false;
	}

	try {
		eye.version.major = std::stoi(version_string.substr(0, dot_index));
		eye.version.minor = std::stoi(version_string.substr(dot_index + 1));
	} catch (const std::exception &e) {
		U_LOG_E("Failed to parse version string '%s' in '%s' eye: %s", version_string.c_str(), eye_name,
		        e.what());
		return false;
	}

	if (eye.version.major != 1) {
		U_LOG_E("Unsupported major version %d in '%s' eye.", eye.version.major, eye_name);
		return false;
	}

	// Versions before 1.5 just get completely ignored... so we have to fill it in with default data
	if (eye.version.minor < 5) {
		htc_eye_default_params(eye, left);
		return true;
	}

	bool needs_halved_fx_fy = eye.version.minor == 5;

	bool is_tgn_dvt_1_1 = node["hmd_model_name"].asString() == "tgn_C10_2.88";

	// @todo - handle TGN DVT 1.1 special case
	// - set left center to 90,0
	// - copy distortion_coeff right to left
	// - copy distortion_coeff_WVR right to left
	// - copy enlarge_ratio right to left
	// - copy grow_for_undistort right to left
	// - copy grow_for_undistort2 right to left
	// - copy grow_for_undistort3 right to left
	// - copy grow_for_undistort4 right to left
	// - copy intrinsics right to left
	// - copy scale right to left
	if (is_tgn_dvt_1_1 && left) {
		U_LOG_W("TGN DVT 1.1 left eye special case not yet handled.");
		return false;
	}

	eye.center.x = node["center"][eye_name]["x"].asDouble();
	eye.center.y = node["center"][eye_name]["y"].asDouble();

	eye.model = htc_string_to_distortion_model(node["model_type"].asString().c_str());

	size_t radial_count, tangential_count, prism_count, wvr_count;
	bool has_distortion_coeff_modify;
	if (!htc_get_distortion_parameter_count(eye.model, radial_count, tangential_count, prism_count, wvr_count,
	                                        has_distortion_coeff_modify)) {
		U_LOG_E("Unsupported distortion model %d in '%s' eye.", eye.model, eye_name);
		return false;
	}

	JSONNode coeffs = node["distortion_coeff"][eye_name];
	if (coeffs.isInvalid()) {
		U_LOG_E("Missing 'distortion_coeff' section in '%s' eye.", eye_name);
		return false;
	}

	std::optional<JSONNode> coeffs_wvr{};
	if (wvr_count > 0) {
		coeffs_wvr = node["distortion_coeff_WVR"][eye_name];

		if (coeffs_wvr->isInvalid()) {
			U_LOG_E("Missing 'distortion_coeff_WVR' section in '%s' eye.", eye_name);
			return false;
		}
	}

	char buf[32] = {0};
	for (int colour = 0; colour < 3; colour++) {
		const char *colour_name = htc_colour_name(colour);

		// k0 is always 1.0, but isn't included in cosmos, so let's just hardcode it
		eye.coeffecients[colour].k[0] = 1.0;
		eye.wvr[colour].k[0] = 1.0;

		// Main coefficients
		for (size_t coeff = 0; coeff < radial_count; coeff++) {
			snprintf(buf, sizeof(buf), "k%zu", coeff + 1);

			eye.coeffecients[colour].k[coeff + 1] = coeffs[colour_name][buf].asDouble();
		}

		for (size_t coeff = 0; coeff < tangential_count; coeff++) {
			snprintf(buf, sizeof(buf), "p%zu", coeff + 1);
			eye.coeffecients[colour].p[coeff] = coeffs[colour_name][buf].asDouble();
		}

		for (size_t coeff = 0; coeff < prism_count; coeff++) {
			snprintf(buf, sizeof(buf), "s%zu", coeff + 1);
			eye.coeffecients[colour].s[coeff] = coeffs[colour_name][buf].asDouble();
		}

		// WVR coefficients
		for (size_t coeff = 0; coeff < wvr_count; coeff++) {
			assert(coeffs_wvr.has_value());

			snprintf(buf, sizeof(buf), "k%zu", coeff + 1);
			eye.wvr[colour].k[coeff + 1] = (*coeffs_wvr)[colour_name][buf].asDouble();
		}
	}

	if (has_distortion_coeff_modify) {
		JSONNode coeffs_modify = node["distortion_coeff_modify"][eye_name];
		if (coeffs_modify.isInvalid()) {
			U_LOG_E("Missing 'distortion_coeff_modify' section in '%s' eye.", eye_name);
			return false;
		}

		// Modify coefficients (only for blue)
		for (int coeff = 0; coeff < 11; coeff++) {
			snprintf(buf, sizeof(buf), "k%d", coeff);
			eye.modify.k[coeff] = coeffs_modify["blue"][buf].asDouble();
		}
		eye.modify.theta = coeffs_modify["blue"]["theta"].asDouble();
	}

	// Enlarge ratio
	JSONNode enlarge_ration_node = node["enlarge_ratio"];
	// Parsing nonsense, unknown which version made this change, somewhere between 1.5 and 1.10
	if (enlarge_ration_node.isDouble()) {
		eye.enlarge_ratio = enlarge_ration_node.asDouble();
	} else {
		eye.enlarge_ratio = enlarge_ration_node[eye_name].asDouble();
	}

	// Grow for undistort
	for (int i = 0; i < 4; i++) {
		snprintf(buf, sizeof(buf), "grow_for_undistort%d", i + 1);
		if (i == 0) {
			eye.grow_for_undistort[i] = node["grow_for_undistort"][eye_name].asDouble();
		} else {
			// These fields were added in version 1.8; before that, they should be treated as 0
			if (eye.version.minor < 8) {
				eye.grow_for_undistort[i] = 0;
			} else {
				eye.grow_for_undistort[i] = node[buf][eye_name].asDouble();
			}
		}
	}

	// Intrinsics
	for (int i = 0; i < 2; i++) {
		const char *name = (i == 0) ? "intrinsics" : "intrinsics2";

		JSONNode intrinsics = node[name][eye_name];
		if (intrinsics.isInvalid() || !intrinsics.isArray()) {
			U_LOG_E("Missing '%s' section in '%s' eye.", name, eye_name);
			return false;
		}

		struct xrt_matrix_3x3 &mat = eye.intrinsics[i];

		for (int row = 0; row < 3; row++) {
			JSONNode row_node = intrinsics[row];
			if (row_node.isInvalid() || !row_node.isArray()) {
				U_LOG_E("Missing row %d in '%s' section in '%s' eye.", row, name, eye_name);
				return false;
			}

			for (int col = 0; col < 3; col++) {
				double val = row_node[col].asDouble();

				// In version 1.5, fx and fy in the intrinsic matrix are doubled for some reason
				if (needs_halved_fx_fy && (row == col) && (row < 2)) {
					val *= 0.5;
				}

				mat.v[row * 3 + col] = val;
			}
		}
	}

	const double too_small_threshold = 0.000001;
	if (eye.intrinsics[0].v[0] <= too_small_threshold || eye.intrinsics[0].v[4] <= too_small_threshold ||
	    eye.grow_for_undistort[0] <= too_small_threshold) {
		eye.intrinsics[0].v[0] = default_distortion.intrinsics[0].v[0];
		eye.intrinsics[0].v[4] = default_distortion.intrinsics[0].v[4];
		eye.grow_for_undistort[0] = default_distortion.grow_for_undistort[0];
	}

	// Resolution
	eye.resolution.x = node["resolution"]["width"].asInt();
	eye.resolution.y = node["resolution"]["height"].asInt();

	eye.scale = node["scale"][eye_name].asDouble();

	// In version 8, we need to do some special loading logic for fx and fy, for some reason.
	if (eye.version.minor == 8) {
		float unk_transform = (left ? 14882.253 : 14861.041) / eye.scale;

		float new_focal = unk_transform / 21.14999961853027 / (eye.resolution.x / 2.0);

		eye.intrinsics[0].v[0] = new_focal;
		eye.intrinsics[0].v[4] = new_focal;
	}

	{ // We need to calculate cx, cy ourselves, and apply scale ratio onto it.
		// Load and apply the scale ratio
		eye.scale_ratio = 1.0;
		if (eye.version.minor >= 7) {
			eye.scale_ratio = node["scale_ratio"].asDouble();
			if (eye.scale_ratio == 0) {
				eye.scale_ratio = 1.0;
			}
		}

		// cx
		eye.intrinsics[0].v[2] = eye.center.x / ((eye.resolution.x / eye.scale_ratio) * 0.5);
		// cy
		eye.intrinsics[0].v[5] = ((float)-eye.center.y) / ((eye.resolution.y / eye.scale_ratio) * 0.5);
	}

	eye.normalized_radius = node["normalizedRadius"].asDouble();

	return true;
}

static void
htc_eye_compute_warp_parameters(htc_eye_distortion &eye, xrt_vec2_i32 &resolution)
{
	const struct xrt_vec2 panel_center = {.x = resolution.x / 2.0f, .y = resolution.y / 2.0f};

	float cx = panel_center.x + eye.center.x;
	float cy = panel_center.y + eye.center.y;

	float r = eye.normalized_radius / (eye.scale_ratio * eye.scale);

	eye.warp.post = {
	    r, 0, cx, //
	    0, r, cy, //
	    0, 0, 1,  //
	};

	math_matrix_3x3_inverse(&eye.warp.post, &eye.warp.pre);

	eye.warp.max_radius = -1;
}

static void
htc_to_lens_pixels(htc_config *config, int eye, const xrt_vec2 *in_uv, xrt_vec2 *out_pixels)
{
	out_pixels->x = in_uv->x * (config->lens_correction.resolution.x - 1);
	out_pixels->y = (1.0 - in_uv->y) * (config->lens_correction.resolution.y - 1);
}

static void
htc_grow_remap(htc_eye_distortion &dist, int eye, xrt_vec2 *inout_uv)
{
	assert(eye < 2);

	xrt_matrix_3x3 &intrinsics = dist.intrinsics[0];

	float fx = intrinsics.v[0];
	float fy = intrinsics.v[4];
	float cx = intrinsics.v[2];
	float cy = intrinsics.v[5];

	float first_grow = dist.grow_for_undistort[0];

	float grow1;
	float grow2;
	if (eye == 1) {
		grow1 = dist.grow_for_undistort[0];
		grow2 = dist.grow_for_undistort[1];
	} else {
		grow1 = dist.grow_for_undistort[1];
		grow2 = dist.grow_for_undistort[0];
	}
	float grow3 = dist.grow_for_undistort[2];
	float grow4 = dist.grow_for_undistort[3];

	// TODO: actually figure out what this is doing instead of just following the original code whole-sale
	// spooky magic, yes a real human wrote these variable names you can trust me
	float first_grow_plus_one = first_grow + 1.0;
	float v14 = (grow2 + 1.0) * (-1.0 - cx);
	float inverse_cy = 1.0 - cy;
	float v16 = (grow4 + 1.0) * (-1.0 - cy);
	float v17 = ((first_grow + 1.0) * (-1.0 - cy)) / fy;
	float v18 = ((first_grow + 1.0) * (-1.0 - cx)) / fx;
	float x_normalized = (inout_uv->x - 0.5) * 2.0;
	float y_normalized = (inout_uv->y - 0.5) * 2.0;

	inout_uv->x = ((((((v18 + ((first_grow_plus_one * (1.0 - cx)) / fx)) /
	                   (((first_grow_plus_one * (1.0 - cx)) / fx) - v18)) +
	                  x_normalized) *
	                 ((((first_grow_plus_one * (1.0 - cx)) / fx) - v18) /
	                  ((((grow1 + 1.0) * (1.0 - cx)) / fx) - (v14 / fx)))) -
	                (((v14 / fx) + (((grow1 + 1.0) * (1.0 - cx)) / fx)) /
	                 ((((grow1 + 1.0) * (1.0 - cx)) / fx) - (v14 / fx)))) *
	               0.5) +
	              0.5;

	inout_uv->y = ((((((v17 + ((first_grow_plus_one * inverse_cy) / fy)) /
	                   (((first_grow_plus_one * inverse_cy) / fy) - v17)) +
	                  y_normalized) *
	                 ((((first_grow_plus_one * inverse_cy) / fy) - v17) /
	                  ((((grow3 + 1.0) * inverse_cy) / fy) - (v16 / fy)))) -
	                (((v16 / fy) + (((grow3 + 1.0) * inverse_cy) / fy)) /
	                 ((((grow3 + 1.0) * inverse_cy) / fy) - (v16 / fy)))) *
	               0.5) +
	              0.5;
}


static void
apply_pre_warp(htc_eye_distortion &dist, float x, float y, float &out_x, float &out_y)
{
	// Run the pre-warp to get the homogeneous coordinates
	xrt_vec3 in_homogeneous = {x, y, 1.0f};
	math_matrix_3x3_transform_vec3(&dist.warp.pre, &in_homogeneous, &in_homogeneous);

	out_x = in_homogeneous.x / in_homogeneous.z;
	out_y = in_homogeneous.y / in_homogeneous.z;
}

static void
apply_traditional(htc_eye_distortion &dist, int channel, int parameters, float x, float y, float &out_x, float &out_y)
{
	double r2 = x * x + y * y;

	double *k = dist.coeffecients[channel].k;

	// double radial_distortion = ((r6 * k[3]) + (r4 * k[2]) + (r2 * k[1]) + k[0]);
	double radial_distortion = k[0];
	double ri = 1;
	for (int i = 0; i < parameters; i++) {
		ri *= r2;
		radial_distortion += (ri * k[i + 1]);
	}

	out_x = x * radial_distortion;
	out_y = y * radial_distortion;
}

static void
apply_tangential(htc_eye_distortion &dist, int channel, float &x, float &y)
{
	double r2 = x * x + y * y;

	double p1 = dist.coeffecients[channel].p[0];
	double p2 = dist.coeffecients[channel].p[1];

	x += (p2 * (2.0 * x * x + r2) + 2.0 * p1 * x * y);
	y += (p1 * (2.0 * y * y + r2) + 2.0 * p2 * x * y);
}

static void
apply_prism(htc_eye_distortion &dist, int channel, float &x, float &y)
{
	double r2 = x * x + y * y;

	double s1 = dist.coeffecients[channel].s[0];
	double s2 = dist.coeffecients[channel].s[1];

	x += r2 * s1;
	x += r2 * s2;
}

static void
apply_post_warp(htc_eye_distortion &dist, float x, float y, float &out_x, float &out_y)
{
	xrt_vec3 in_homogeneous = {x, y, 1.0f};
	math_matrix_3x3_transform_vec3(&dist.warp.post, &in_homogeneous, &in_homogeneous);

	out_x = in_homogeneous.x / in_homogeneous.z;
	out_y = in_homogeneous.y / in_homogeneous.z;
}

static void
htc_traditional(htc_eye_distortion &dist, int channel, xrt_vec2 *inout_pixels)
{
	assert(channel < 3);

	// Run the pre-warp to get the homogeneous coordinates
	float x, y;
	apply_pre_warp(dist, inout_pixels->x, inout_pixels->y, x, y);

	float pre_warp_final_u, pre_warp_final_v;
	apply_traditional(dist, channel, 3, x, y, pre_warp_final_u, pre_warp_final_v);

	apply_post_warp(dist, pre_warp_final_u, pre_warp_final_v, inout_pixels->x, inout_pixels->y);
}

static void
htc_traditional_tangential(htc_eye_distortion &dist, int channel, xrt_vec2 *inout_pixels)
{
	assert(channel < 3);

	float x, y;
	apply_pre_warp(dist, inout_pixels->x, inout_pixels->y, x, y);

	float pre_warp_final_u, pre_warp_final_v;
	apply_traditional(dist, channel, 3, x, y, pre_warp_final_u, pre_warp_final_v);
	apply_tangential(dist, channel, pre_warp_final_u, pre_warp_final_v);

	apply_post_warp(dist, pre_warp_final_u, pre_warp_final_v, inout_pixels->x, inout_pixels->y);
}

static void
htc_radial_tangential_prism(htc_eye_distortion &dist, int channel, xrt_vec2 *inout_pixels)
{
	assert(channel < 3);

	float x, y;
	apply_pre_warp(dist, inout_pixels->x, inout_pixels->y, x, y);

	float pre_warp_final_u, pre_warp_final_v;
	apply_traditional(dist, channel, 5, x, y, pre_warp_final_u, pre_warp_final_v);
	apply_tangential(dist, channel, pre_warp_final_u, pre_warp_final_v);
	apply_prism(dist, channel, pre_warp_final_u, pre_warp_final_v);

	apply_post_warp(dist, pre_warp_final_u, pre_warp_final_v, inout_pixels->x, inout_pixels->y);
}

static bool
htc_strengthen_high_order(htc_eye_distortion &dist, int channel, xrt_vec2 *inout_pixels)
{
	assert(channel < 3);

	// Run the pre-warp to get the homogeneous coordinates
	float x, y;
	apply_pre_warp(dist, inout_pixels->x, inout_pixels->y, x, y);

	double r2 = x * x + y * y;
	double r = sqrt(r2);
	double r4 = r2 * r2;
	double r6 = r4 * r2;
	double r8 = r6 * r2;
	double r10 = r8 * r2;

	double *k = dist.coeffecients[channel].k;

	double p1 = dist.coeffecients[channel].p[0];
	double p2 = dist.coeffecients[channel].p[1];
	double p3 = dist.coeffecients[channel].p[2];
	double p4 = dist.coeffecients[channel].p[3];
	double p5 = dist.coeffecients[channel].p[4];
	double p6 = dist.coeffecients[channel].p[5];

	double s1 = dist.coeffecients[channel].s[0];
	double s2 = dist.coeffecients[channel].s[1];
	double s3 = dist.coeffecients[channel].s[2];
	double s4 = dist.coeffecients[channel].s[3];

	double radial_series =
	    ((((((((((((r * k[1]) + k[0]) + ((r * r) * k[2])) + (((r * r) * r) * k[3])) + ((r2 * r2) * k[4])) +
	           (((r2 * r2) * r) * k[5])) +
	          (r6 * k[6])) +
	         ((r6 * r) * k[7])) +
	        (r8 * k[8])) +
	       ((r8 * r) * k[9])) +
	      (r10 * k[10])) +
	     ((r10 * r) * k[11])) +
	    ((r10 * (r * r)) * k[12]);

	// @todo add the "max radius" here and return "false"

	double xsq3 = (x * 3.0) * x;
	double xsq4_ysq = (((x * 4.0) * x) * y) * y;
	double xsq2 = (x + x) * x;
	double y_2x = y * (x + x);
	double polyXY = ((xsq2 * x) * y) + ((y * y_2x) * y);
	double y_2x_r4 = y_2x * r4;
	double x2 = x * x;
	double y2 = y * y;

	double x_tangential =
	    ((((((((xsq3 * x2) + xsq4_ysq) + (y2 * y2)) * p2) + ((xsq3 + y2) * p1)) + (((xsq2 * r4) + r6) * p3)) +
	      (y_2x * p4)) +
	     (p5 * polyXY)) +
	    (p6 * y_2x_r4);
	double y_tangential =
	    (((((((p1 + p1) * x) * y) + (p2 * polyXY)) + (p3 * y_2x_r4)) + ((((y * 3.0) * y) + x2) * p4)) +
	     ((((x2 * x2) + xsq4_ysq) + (y2 * (y2 * 3.0))) * p5)) +
	    (((((y + y) * y) * r4) + r6) * p6);

	double pre_warp_final_u = (((radial_series * x) + x_tangential) + (r2 * s1)) + (r4 * s2);
	double pre_warp_final_v = (((y * radial_series) + y_tangential) + (r2 * s3)) + (r4 * s4);

	apply_post_warp(dist, pre_warp_final_u, pre_warp_final_v, inout_pixels->x, inout_pixels->y);

	return true;
}

static inline bool
htc_distort_internal(htc_config *config, int eye, int channel, const xrt_vec2 *in, xrt_vec2 *out)
{
	assert(eye < 2);
	assert(channel < 3);

	htc_eye_distortion &dist = config->lens_correction.eyes[eye];

	xrt_vec2_i32 &resolution = config->lens_correction.resolution;

	float enlarge_ratio = dist.enlarge_ratio;

	xrt_vec2 distorted;
	htc_to_lens_pixels(config, eye, in, &distorted);

	switch (dist.model) {
	case HTC_DISTORTION_MODEL_TRADITIONAL_SIMPLE: htc_traditional(dist, channel, &distorted); break;
	case HTC_DISTORTION_MODEL_TRADITIONAL_WITH_TANGENTIAL:
		htc_traditional_tangential(dist, channel, &distorted);
		break;
	case HTC_DISTORTION_MODEL_RADIAL_TANGENTIAL_PRISM:
		htc_radial_tangential_prism(dist, channel, &distorted);
		break;
	case HTC_DISTORTION_MODEL_STRENGTHEN_HIGH_ORDER:
		if (!htc_strengthen_high_order(dist, channel, &distorted)) {
			distorted.x = -10000;
			distorted.y = -10000;
		}
		break;
	default: return false;
	}

	xrt_vec2 normalized_centers = {
	    (dist.center.x / resolution.x) + 0.5f,
	    (dist.center.y / resolution.y) + 0.5f,
	};

	out->x = ((normalized_centers.x * (enlarge_ratio - 1.0)) + (distorted.x / (resolution.x - 1))) / enlarge_ratio;
	out->y =
	    // 1.0 - //
	    (((normalized_centers.y * (enlarge_ratio - 1.0)) + (distorted.y / (resolution.y - 1))) / enlarge_ratio);

	if (dist.grow_for_undistort[2] != 0) {
		htc_grow_remap(dist, eye, out);
	}

	return true;
}

/*
 * Exported functions
 */

enum htc_distortion_model
htc_string_to_distortion_model(const char *model_str)
{
	std::string str(model_str);

	htc_distortion_model model = HTC_DISTORTION_MODEL_INVALID;

	if (str == "traditional_simple") {
		model = HTC_DISTORTION_MODEL_TRADITIONAL_SIMPLE;
	} else if (str == "traditional_with_tangential") {
		model = HTC_DISTORTION_MODEL_TRADITIONAL_WITH_TANGENTIAL;
	} else if (str == "non_model_svr") {
		model = HTC_DISTORTION_MODEL_NON_MODEL_SVR;
	} else if (str == "rational") {
		model = HTC_DISTORTION_MODEL_RATIONAL;
	} else if (str == "sectional") {
		model = HTC_DISTORTION_MODEL_SECTIONAL;
	} else if (str == "tangential_weight") {
		model = HTC_DISTORTION_MODEL_TANGENTIAL_WEIGHT;
	} else if (str == "radial_tangential_prism") {
		model = HTC_DISTORTION_MODEL_RADIAL_TANGENTIAL_PRISM;
	} else if (str == "prism_with_progressive") {
		model = HTC_DISTORTION_MODEL_PRISM_WITH_PROGRESSIVE;
	} else if (str == "strengthen_radial") {
		model = HTC_DISTORTION_MODEL_STRENGTHEN_RADIAL;
	} else if (str == "strengthen") {
		model = HTC_DISTORTION_MODEL_STRENGTHEN;
	} else if (str == "strengthen_high_order") {
		model = HTC_DISTORTION_MODEL_STRENGTHEN_HIGH_ORDER;
	} else if (str == "wvr_radial") {
		model = HTC_DISTORTION_MODEL_WVR_RADIAL;
	} else if (str == "radial_rotate_modify") {
		model = HTC_DISTORTION_MODEL_RADIAL_ROTATE_MODIFY;
	} else {
		U_LOG_W("Unknown distortion model string: %s", model_str);
	}

	return model;
}

bool
htc_config_parse(const char *config_data, size_t config_size, struct htc_config *out_config)
{
	// Zero-init to be in a good state
	memset(out_config, 0, sizeof(*out_config));

	JSONNode root{std::string{config_data, config_size}};
	if (root.isInvalid()) {
		U_LOG_E("Failed to parse JSON config data.");
		return false;
	}

	{ // Parse device
		JSONNode device = root["device"];
		if (device.isInvalid()) {
			U_LOG_E("Missing 'device' section in config.");
			return false;
		}

		out_config->device.eye_target_width_in_pixels = device["eye_target_width_in_pixels"].asInt();
		out_config->device.eye_target_height_in_pixels = device["eye_target_height_in_pixels"].asInt();
	}

	{ // Parse lens correction
		JSONNode inhouse_lens_correction = root["inhouse_lens_correction"];

		if (inhouse_lens_correction.isInvalid()) {
			U_LOG_E("Missing 'inhouse_lens_correction' section in config.");
			return false;
		}

		try {
			JSONNode left = inhouse_lens_correction["left"];
			if (left.isInvalid()) {
				U_LOG_E("Missing 'left' section in 'inhouse_lens_correction'.");
				return false;
			}

			if (!htc_eye_parse(left, out_config->lens_correction.eyes[0], true)) {
				U_LOG_E("Failed to parse left eye distortion.");
				return false;
			}

			JSONNode right = inhouse_lens_correction["right"];
			if (right.isInvalid()) {
				U_LOG_E("Missing 'right' section in 'inhouse_lens_correction'.");
				return false;
			}

			if (!htc_eye_parse(right, out_config->lens_correction.eyes[1], false)) {
				U_LOG_E("Failed to parse right eye distortion.");
				return false;
			}
		} catch (const std::exception &e) {
			U_LOG_E("Exception while parsing lens correction: %s", e.what());
			return false;
		}

		htc_eye_distortion &left_eye = out_config->lens_correction.eyes[0];

		// Calculate the lens correction resolution based on the physical display resolution and scale ratio
		out_config->lens_correction.resolution.x = (int32_t)(left_eye.resolution.x / left_eye.scale_ratio);
		out_config->lens_correction.resolution.y = (int32_t)(left_eye.resolution.y / left_eye.scale_ratio);

		htc_eye_compute_warp_parameters(out_config->lens_correction.eyes[0],
		                                out_config->lens_correction.resolution);
		htc_eye_compute_warp_parameters(out_config->lens_correction.eyes[1],
		                                out_config->lens_correction.resolution);
	}

	return true;
}

bool
htc_config_compute_distortion(struct htc_config *config,
                              int eye,
                              const struct xrt_vec2 *in,
                              struct xrt_uv_triplet *out_result)
{
	assert(eye < 2);

	if (debug_get_bool_option_force_disable_distortion()) {
		return false;
	}

	bool success = true;

	success &= htc_distort_internal(config, eye, 0, in, &out_result->r);
	success &= htc_distort_internal(config, eye, 1, in, &out_result->g);
	success &= htc_distort_internal(config, eye, 2, in, &out_result->b);

	return success;
}

void
htc_config_get_fov(htc_config *config, int eye, xrt_fov *out_fov)
{
	htc_eye_distortion &dist = config->lens_correction.eyes[eye];

	xrt_matrix_3x3 &intrinsics = dist.intrinsics[0];

	float grow[4];
	htc_get_grow_for_undistort(*config, eye, grow);

	grow[0] += 1.0f;
	grow[1] += 1.0f;
	grow[2] += 1.0f;
	grow[3] += 1.0f;

	out_fov->angle_left = atanf((-1.0 - intrinsics.v[2]) * grow[0] / intrinsics.v[0]);
	out_fov->angle_right = atanf((1.0 - intrinsics.v[2]) * grow[1] / intrinsics.v[0]);
	out_fov->angle_up = atanf((1.0 - intrinsics.v[4 + 1]) * grow[2] / intrinsics.v[4]);
	out_fov->angle_down = atanf((-1.0 - intrinsics.v[4 + 1]) * grow[3] / intrinsics.v[4]);
}
