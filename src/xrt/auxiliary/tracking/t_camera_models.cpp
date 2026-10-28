// Copyright 2023, Collabora, Ltd.
// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Camera (un)projection functions for various camera models.
 * @author Moshi Turner <moshiturner@protonmail.com>
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup aux_tracking
 *
 * Some notes:
 * These functions should return exactly the same values as basalt-headers, down to floating point bits.
 *
 * They were mainly written as an expedient way to stop depending on OpenCV-based (un)projection code in Monado's hand
 * tracking code, and to encourage compiler optimizations through inlining.
 *
 * Current users:
 *
 * * Mercury hand tracking
 * * Constellation tracker
 */

#include "t_camera_models.hpp"

#include <assert.h>


using namespace xrt::auxiliary::tracking::camera_models;

/*
 * Misc functions.
 */

static inline void
interpret_as_rt8(const t_camera_calibration &cc, t_camera_model_params &out_params)
{
	// Make a temporary buffer that definitely has zeros in it.
	double distortion_tmp[XRT_DISTORTION_MAX_DIM] = {0};

	if (cc.distortion_model != T_DISTORTION_OPENCV_RADTAN_8) {
		U_LOG_W("Reinterpreting %s distortion as %s", t_stringify_camera_distortion_model(cc.distortion_model),
		        t_stringify_camera_distortion_model(T_DISTORTION_OPENCV_RADTAN_8));
	}

	size_t dist_num = t_num_params_from_distortion_model(cc.distortion_model);

	for (size_t i = 0; i < dist_num; i++) {
		// Copy only the valid values over. The high indices will be zero, which means that rt4 and rt5
		// calibrations will work correctly.
		distortion_tmp[i] = cc.distortion_parameters_as_array[i];
	}

	int acc_idx = 0;
	out_params.rt8.k1 = distortion_tmp[acc_idx++];
	out_params.rt8.k2 = distortion_tmp[acc_idx++];
	out_params.rt8.p1 = distortion_tmp[acc_idx++];
	out_params.rt8.p2 = distortion_tmp[acc_idx++];
	out_params.rt8.k3 = distortion_tmp[acc_idx++];
	out_params.rt8.k4 = distortion_tmp[acc_idx++];
	out_params.rt8.k5 = distortion_tmp[acc_idx++];
	out_params.rt8.k6 = distortion_tmp[acc_idx++];

	if (cc.distortion_model == T_DISTORTION_WMR) {
		out_params.rt8.metric_radius = cc.wmr.rpmax;
	} else {
		out_params.rt8.metric_radius = 0;
	}
	out_params.model = T_DISTORTION_OPENCV_RADTAN_8;
}

/*
 * Exported C functions.
 */

void
t_camera_model_params_from_t_camera_calibration(const t_camera_calibration *cc, t_camera_model_params *out_params)
{
	// Paranoia.
	U_ZERO(out_params);

	// First row, first column.
	out_params->fx = (float)cc->intrinsics[0][0];
	// Second row, second column.
	out_params->fy = (float)cc->intrinsics[1][1];
	// First row, third column.
	out_params->cx = (float)cc->intrinsics[0][2];
	// Second row, third column.
	out_params->cy = (float)cc->intrinsics[1][2];

	out_params->model = cc->distortion_model;

	switch (cc->distortion_model) {
	case T_DISTORTION_FISHEYE_KB4: {
		out_params->fisheye.k1 = (float)cc->kb4.k1;
		out_params->fisheye.k2 = (float)cc->kb4.k2;
		out_params->fisheye.k3 = (float)cc->kb4.k3;
		out_params->fisheye.k4 = (float)cc->kb4.k4;
	} break;
	case T_DISTORTION_OPENCV_RADTAN_14:
	case T_DISTORTION_OPENCV_RADTAN_5:
	case T_DISTORTION_OPENCV_RADTAN_8:
	case T_DISTORTION_WMR: interpret_as_rt8(*cc, *out_params); break;
	case T_DISTORTION_RIFT_CV1: {
		out_params->cv1.k1 = (float)cc->cv1.k1;
		out_params->cv1.k2 = (float)cc->cv1.k2;
		out_params->cv1.k3 = (float)cc->cv1.k3;
		out_params->cv1.k4 = (float)cc->cv1.k4;
		out_params->cv1.p1 = (float)cc->cv1.p1;
		out_params->cv1.p2 = (float)cc->cv1.p2;
		out_params->cv1.g3 = (float)cc->cv1.g3;
		out_params->cv1.g4 = (float)cc->cv1.g4;
	} break;
	case T_DISTORTION_PINHOLE: break; // no params
	default:
		U_LOG_E("t_camera_model_params_from_t_camera_calibration doesn't support camera model %s yet!",
		        t_stringify_camera_distortion_model(cc->distortion_model));
		break;
	}
}

bool
t_camera_models_unproject(
    const t_camera_model_params *dist, const float x, const float y, float *out_x, float *out_y, float *out_z)
{
	switch (dist->model) {
	case T_DISTORTION_PINHOLE: {
		return pinhole_unproject(*dist, x, y, *out_x, *out_y, *out_z);
	};
	case T_DISTORTION_OPENCV_RADTAN_8: {
		return rt8_unproject(*dist, x, y, *out_x, *out_y, *out_z);
	}; break;
	case T_DISTORTION_FISHEYE_KB4: {
		return kb4_unproject(*dist, x, y, *out_x, *out_y, *out_z);
	}; break;
	// Return false so we don't get warnings on Release builds.
	default: assert(false); return false;
	}
}

bool
t_camera_models_unproject_and_flip(
    const t_camera_model_params *dist, const float x, const float y, float *out_x, float *out_y, float *out_z)
{
	bool ret = t_camera_models_unproject(dist, x, y, out_x, out_y, out_z);

	*out_z *= -1;
	*out_y *= -1;
	return ret;
}

void
t_camera_models_undistort(const t_camera_model_params *dist, const float x, const float y, float *out_x, float *out_y)
{
	return undistort(*dist, x, y, *out_x, *out_y);
}

bool
t_camera_models_project(const t_camera_model_params *dist, //
                        const float x,                     //
                        const float y,                     //
                        const float z,                     //
                        float *out_x,                      //
                        float *out_y)
{
	return project(*dist, x, y, z, *out_x, *out_y);
}

bool
t_camera_models_flip_and_project(const t_camera_model_params *dist, //
                                 const float x,                     //
                                 const float y,                     //
                                 const float z,                     //
                                 float *out_x,                      //
                                 float *out_y)
{
	float _y = y * -1;
	float _z = z * -1;

	return t_camera_models_project(dist, x, _y, _z, out_x, out_y);
}
