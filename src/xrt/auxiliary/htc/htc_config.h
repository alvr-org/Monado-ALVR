// Copyright 2025-2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Code to parse and handle the modern non-LH HTC configuration data.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup aux_htc
 */

#pragma once

#include "xrt/xrt_defines.h"


#ifdef __cplusplus
extern "C" {
#endif

enum htc_distortion_model
{
	HTC_DISTORTION_MODEL_INVALID = 0,
	HTC_DISTORTION_MODEL_TRADITIONAL_SIMPLE = 1,
	HTC_DISTORTION_MODEL_TRADITIONAL_WITH_TANGENTIAL = 2,
	HTC_DISTORTION_MODEL_NON_MODEL_SVR = 3,
	HTC_DISTORTION_MODEL_RATIONAL = 4,
	HTC_DISTORTION_MODEL_SECTIONAL = 5,
	HTC_DISTORTION_MODEL_TANGENTIAL_WEIGHT = 6,
	HTC_DISTORTION_MODEL_RADIAL_TANGENTIAL_PRISM = 7,
	HTC_DISTORTION_MODEL_PRISM_WITH_PROGRESSIVE = 8,
	HTC_DISTORTION_MODEL_STRENGTHEN_RADIAL = 9,
	HTC_DISTORTION_MODEL_STRENGTHEN = 10,
	HTC_DISTORTION_MODEL_STRENGTHEN_HIGH_ORDER = 11,
	HTC_DISTORTION_MODEL_WVR_RADIAL = 12,
	HTC_DISTORTION_MODEL_RADIAL_ROTATE_MODIFY = 13,
};

struct htc_eye_coefficients
{
	double k[13]; // radial
	double p[6];  // tangential
	double s[4];  // prism
};

struct htc_wvr_coefficients
{
	double k[6];
};

struct htc_modify_coefficients
{
	double k[11];
	double theta;
};

struct htc_distortion_version
{
	uint8_t major;
	uint8_t minor;
};

struct htc_warp_parameters
{
	struct xrt_matrix_3x3 post;
	struct xrt_matrix_3x3 pre;
	float max_radius;
};

struct htc_eye_distortion
{
	struct xrt_vec2 center;
	enum htc_distortion_model model;

	struct htc_eye_coefficients coeffecients[3];

	struct htc_wvr_coefficients wvr[3];

	struct htc_modify_coefficients modify; // Only contains blue

	double enlarge_ratio;
	double grow_for_undistort[4];

	struct xrt_matrix_3x3 intrinsics[2];

	struct xrt_vec2_i32 resolution;
	double scale;
	double scale_ratio;

	double normalized_radius;

	struct htc_distortion_version version;

	struct htc_warp_parameters warp;
};

struct htc_config
{
	struct
	{
		uint32_t eye_target_width_in_pixels;
		uint32_t eye_target_height_in_pixels;
	} device;

	struct
	{
		struct htc_eye_distortion eyes[2];

		struct xrt_vec2_i32 resolution; // The resolution to use for distorting
	} lens_correction;
};

/*!
 * Converts a string to an HTC distortion model enum value.
 *
 * @param model_str The string representation of the distortion model.
 * @return The corresponding htc_distortion_model enum value, or HTC_DISTORTION_MODEL_INVALID if the string is
 * unrecognized.
 */
enum htc_distortion_model
htc_string_to_distortion_model(const char *model_str);

bool
htc_config_parse(const char *config_data, size_t config_size, struct htc_config *out_config);

bool
htc_config_compute_distortion(struct htc_config *config,
                              int eye,
                              const struct xrt_vec2 *in,
                              struct xrt_uv_triplet *out_result);

void
htc_config_get_fov(struct htc_config *config, int eye, struct xrt_fov *out_fov);

#ifdef __cplusplus
}
#endif
