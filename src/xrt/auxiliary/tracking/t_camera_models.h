// Copyright 2023, Collabora, Ltd.
// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Camera (un)projection C API for various camera models.
 * @author Moshi Turner <moshiturner@protonmail.com>
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup aux_tracking
 */

#pragma once

#include "tracking/t_tracking.h"


#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Floating point parameters for @ref T_DISTORTION_FISHEYE_KB4
 * @ingroup aux_tracking
 */
struct t_camera_calibration_kb4_params_float
{
	float k1, k2, k3, k4;
};

/*!
 * Floating point parameters for @ref T_DISTORTION_OPENCV_RADTAN_8, also including
 * @p metric_radius.
 * @ingroup aux_tracking
 */
struct t_camera_calibration_rt8_params_float
{
	float k1, k2, p1, p2, k3, k4, k5, k6, metric_radius;
};

/*!
 * Floating point parameters for @ref T_DISTORTION_RIFT_CV1.
 *
 * @p k1 - @p k4 are the radial polynomial coefficients (Oculus @p d1 - @p d4), @p p1 / @p p2 the
 * tangential decentering terms, and @p g3 / @p g4 the 4th-order affine gain applied to the
 * decentering delta.
 * @ingroup aux_tracking
 */
struct t_camera_calibration_cv1_params_float
{
	float k1, k2, k3, k4;
	float p1, p2;
	float g3, g4;
};

/*!
 * Floating point calibration data for a single calibrated camera.
 * @note This is basically @ref t_camera_calibration, just without some compatibility stuff and using single floats
 * instead of doubles.
 * @ingroup aux_tracking
 */
struct t_camera_model_params
{
	float fx, fy, cx, cy;
	union {
		struct t_camera_calibration_kb4_params_float fisheye;
		struct t_camera_calibration_rt8_params_float rt8;
		struct t_camera_calibration_cv1_params_float cv1;
	};
	// This model gets reinterpreted from values in the main t_camera_calibration struct to either
	// * T_DISTORTION_FISHEYE_KB4
	// * T_DISTORTION_OPENCV_RADTAN_8
	// * T_DISTORTION_RIFT_CV1
	enum t_camera_distortion_model model;
};

/*!
 * Takes a @ref t_camera_calibration through \p cc, and returns a @ref t_camera_model_params that shadows \p cc
 * 's parameters through \p out_params
 */
void
t_camera_model_params_from_t_camera_calibration(const struct t_camera_calibration *cc,
                                                struct t_camera_model_params *out_params);

/*!
 * Takes a 2D image-space point through \p x and \p y, unprojects it to a normalized 3D direction, and returns
 * the result through \p out_x, \p out_y and \p out_z
 */
bool
t_camera_models_unproject(
    const struct t_camera_model_params *dist, const float x, const float y, float *out_x, float *out_y, float *out_z);

/*!
 * Takes a 2D image-space point through \p x and \p y, unprojects it to a normalized 3D direction, flips its Y
 * and Z values (performing a coordinate space transform from +Z forward -Y up to -Z forward +Y up) and returns the
 * result through \p out_x, \p out_y and \p out_z
 */
bool
t_camera_models_unproject_and_flip(
    const struct t_camera_model_params *dist, const float x, const float y, float *out_x, float *out_y, float *out_z);

/*!
 * Takes a distorted 2D point through \p x and \p y and computes the undistorted point into
 * \p out_x and \p out_y
 */
void
t_camera_models_undistort(
    const struct t_camera_model_params *dist, const float x, const float y, float *out_x, float *out_y);

/*!
 * Takes a 3D point through \p x, \p y, and \p z, projects it into image space, and returns the result
 * through \p out_x and \p out_y
 */
bool
t_camera_models_project(const struct t_camera_model_params *dist, //
                        const float x,                            //
                        const float y,                            //
                        const float z,                            //
                        float *out_x,                             //
                        float *out_y);

/*!
 * Takes a 3D point through \p x, \p y, and \p z, flips its Y and Z values (performing a coordinate space
 * transform from -Z forward +Y up to +Z forward -Y up), projects it into image space, and returns the result through
 * \p out_x and \p out_y
 */
bool
t_camera_models_flip_and_project(const struct t_camera_model_params *dist, //
                                 const float x,                            //
                                 const float y,                            //
                                 const float z,                            //
                                 float *out_x,                             //
                                 float *out_y);

#ifdef __cplusplus
}
#endif
