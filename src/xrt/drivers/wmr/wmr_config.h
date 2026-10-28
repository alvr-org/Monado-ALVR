/* Copyright 2021 Jan Schmidt
 * SPDX-License-Identifier: BSL-1.0
 */
/*!
 * @file
 * @brief	WMR and MS HoloLens configuration structures
 * @author Jan Schmidt <jan@centricular.com>
 * @ingroup drv_wmr
 */

#pragma once

#include "math/m_vec2.h"
#include "math/m_vec3.h"

#include "util/u_logging.h"
#include "util/u_distortion_mesh.h"
#include "xrt/xrt_tracking.h"

#define WMR_MAX_CAMERAS 4
#define WMR_HMD_IMU_INDEX 0
#define WMR_LEFT_CTRL_IMU_INDEX 1
#define WMR_RIGHT_CTRL_IMU_INDEX 2

/* Increase this number if anyone releases a controller with
 * more tracking LEDs */
#define WMR_MAX_LEDS 40

#ifdef __cplusplus
extern "C" {
#endif

enum wmr_distortion_model
{
	WMR_DISTORTION_MODEL_UNKNOWN = 0,
	WMR_DISTORTION_MODEL_POLYNOMIAL_3K,
	WMR_DISTORTION_MODEL_POLYNOMIAL_6KT,
};

/* Location is used as camera_id for setting gain */
enum wmr_camera_location
{
	WMR_CAMERA_LOCATION_HT0 = 0,
	WMR_CAMERA_LOCATION_HT1 = 1,
	WMR_CAMERA_LOCATION_DO0 = 2,
	WMR_CAMERA_LOCATION_DO1 = 3,
	WMR_CAMERA_LOCATION_HT2 = 4,
	WMR_CAMERA_LOCATION_HT3 = 5,
};

enum wmr_camera_purpose
{
	WMR_CAMERA_PURPOSE_HEAD_TRACKING,
	WMR_CAMERA_PURPOSE_DISPLAY_OBSERVER,
};

struct wmr_distortion_6KT
{
	enum wmr_distortion_model model;

	/* The config provides 15 float values: */
	union {
		struct
		{
			float cx, cy;         /* Principal point */
			float fx, fy;         /* Focal length */
			float k[6];           /* Radial distortion coefficients */
			float dist_x, dist_y; /* Center of distortion */
			float p2, p1;         /* Tangential distortion parameters */
			float metric_radius;  /* Metric radius */
		} params;
		float v[15];
	};
};

struct wmr_distortion_eye_config
{
	struct xrt_vec3 translation;    //!< Raw translation (to HT0)
	struct xrt_matrix_3x3 rotation; //!< Raw rotation (to HT0), row major
	struct xrt_pose pose;           //!< Pose from `translation` and `rotation`

	/* Radius of the (undistorted) visible area from the center (pixels) (I think) */
	double visible_radius;

	/* Width, Height (pixels) of the full display */
	struct xrt_vec2 display_size;
	/* Center for the eye viewport visibility (pixels) */
	struct xrt_vec2 visible_center;

	struct u_poly_3k_eye_values poly_3k; //!< Distortion parameters for each channel
};

struct wmr_camera_config
{
	enum wmr_camera_location location;
	enum wmr_camera_purpose purpose;

	struct xrt_rect roi;

	struct xrt_vec3 translation;    //!< Raw translation (to HT0)
	struct xrt_matrix_3x3 rotation; //!< Raw rotation (to HT0), row major
	struct xrt_pose pose;           //!< Pose from `translation` and `rotation`

	struct wmr_distortion_6KT distortion6KT;
};

/* Configuration for a single inertial sensor */
struct wmr_inertial_sensor_config
{
	struct xrt_vec3 translation;    //!< Raw translation (to HT0). Usually non-zero only on accelerometers.
	struct xrt_matrix_3x3 rotation; //!< Raw rotation (to HT0), row major
	struct xrt_pose pose;           //!< Pose from `translation` and `rotation`

	/* Current bias and mix matrix extracted from
	 * the configuration, which provides coefficients
	 * for temperature adjustments - but they're always 0,
	 * so we just take the constant coefficient */
	struct xrt_vec3 bias_offsets;
	struct xrt_matrix_3x3 mix_matrix;

	//! Bias random walk variance. @see slam_tracker::inertial_calibration.
	struct xrt_vec3 bias_var;

	//! Measurement noise standard deviation. @see slam_tracker::inertial_calibration.
	struct xrt_vec3 noise_std;
};

/* Precomputed transforms to convert between OpenXR and WMR coordinate systems */
struct wmr_sensor_transforms_config
{
	struct xrt_pose P_oxr_acc; //!< Converts accel samples into OpenXR coordinates
	struct xrt_pose P_oxr_gyr; //!< Converts gyro samples into OpenXR coordinates
	struct xrt_pose P_ht0_me;  //!< ME="middle of the eyes". HT0-to-ME transform but in OpenXR coordinates
	struct xrt_pose P_imu_me;  //!< IMU=accel. IMU-to-ME transform but in OpenXR coordinates
};

/* Configuration for the set of inertial sensors */
struct wmr_inertial_sensors_config
{
	struct wmr_inertial_sensor_config accel;
	struct wmr_inertial_sensor_config gyro;
	struct wmr_inertial_sensor_config mag;

	struct wmr_sensor_transforms_config transforms;
};

struct wmr_led_config
{
	struct xrt_vec3 pos;
	struct xrt_vec3 norm;
};

struct wmr_hmd_config
{
	/* Left and Right eye mapping and distortion params */
	struct wmr_distortion_eye_config eye_params[2];

	struct wmr_inertial_sensors_config sensors;

	int cam_count;
	struct wmr_camera_config cams[WMR_MAX_CAMERAS];

	struct wmr_camera_config *tcams[WMR_MAX_CAMERAS]; //!< Pointers to tracking cameras in `cameras`
	int tcam_count;                                   //!< Number of tracking cameras
	int slam_cam_count;                               //!< Number of tracking cameras to use for SLAM
	int sinks_count;                                  //!< 2x @ref tcam_count to split SLAM and camera streams
};

bool
wmr_hmd_config_parse(struct wmr_hmd_config *c, char *json_string, enum u_logging_level log_level);


struct wmr_controller_config
{
	struct wmr_inertial_sensors_config sensors;

	int led_count;
	struct wmr_led_config leds[WMR_MAX_LEDS];
};

bool
wmr_controller_config_parse(struct wmr_controller_config *c, char *json_string, enum u_logging_level log_level);

void
wmr_config_precompute_transforms(struct wmr_inertial_sensors_config *sensors,
                                 struct wmr_distortion_eye_config *eye_params);

#ifdef __cplusplus
}
#endif
