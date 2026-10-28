// Copyright 2020-2021, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Vive json implementation
 * @author Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * @author Moshi Turner <moshiturner@protonmail.com>
 * @ingroup aux_vive
 */

#include "math/m_api.h"
#include "math/m_mathinclude.h"

#include "util/u_misc.h"
#include "util/u_json.h"
#include "util/u_debug.h"
#include "util/u_distortion_mesh.h"

#include "tracking/t_tracking.h"

#include "vive_config.h"
#include "vive_tweaks.h"

#include <stdio.h>


/*
 *
 * Defines.
 *
 */

#define VIVE_TRACE(d, ...) U_LOG_IFL_T(d->log_level, __VA_ARGS__)
#define VIVE_DEBUG(d, ...) U_LOG_IFL_D(d->log_level, __VA_ARGS__)
#define VIVE_INFO(d, ...) U_LOG_IFL_I(d->log_level, __VA_ARGS__)
#define VIVE_WARN(d, ...) U_LOG_IFL_W(d->log_level, __VA_ARGS__)
#define VIVE_ERROR(d, ...) U_LOG_IFL_E(d->log_level, __VA_ARGS__)

#define JSON_INT(a, b, c) u_json_get_int(u_json_get(a, b), c)
#define JSON_FLOAT(a, b, c) u_json_get_float(u_json_get(a, b), c)
#define JSON_DOUBLE(a, b, c) u_json_get_double(u_json_get(a, b), c)
#define JSON_VEC3(a, b, c) u_json_get_vec3_array(u_json_get(a, b), c)
#define JSON_MATRIX_3X3(a, b, c) u_json_get_matrix_3x3(u_json_get(a, b), c)
#define JSON_STRING(a, b, c) u_json_get_string_into_array(u_json_get(a, b), c, sizeof(c))


/*
 *
 * Printing helpers.
 *
 */

static void
_print_vec3(const char *title, struct xrt_vec3 *vec)
{
	U_LOG_D("%s = %f %f %f", title, (double)vec->x, (double)vec->y, (double)vec->z);
}


/*
 *
 * Loading helpers.
 *
 */

static void
_get_color_coeffs(struct u_vive_values *values, const cJSON *coeffs, uint8_t eye, uint8_t channel)
{
	// For Vive this is 8 with only 3 populated.
	// For Index this is 4 with all values populated.
	const cJSON *item = NULL;
	size_t i = 0;
	cJSON_ArrayForEach(item, coeffs)
	{
		values->coefficients[channel][i] = (float)item->valuedouble;
		++i;
		if (i == 4) {
			break;
		}
	}
}

static void
_get_pose_from_pos_x_z(const cJSON *obj, struct xrt_pose *pose)
{
	struct xrt_vec3 plus_x;
	struct xrt_vec3 plus_z;
	JSON_VEC3(obj, "plus_x", &plus_x);
	JSON_VEC3(obj, "plus_z", &plus_z);
	JSON_VEC3(obj, "position", &pose->position);

	math_quat_from_plus_x_z(&plus_x, &plus_z, &pose->orientation);
}

static void
_get_distortion_properties(struct vive_config *d, const cJSON *eye_transform_json, uint8_t eye)
{
	const cJSON *eye_json = cJSON_GetArrayItem(eye_transform_json, eye);
	if (eye_json == NULL) {
		return;
	}

	struct xrt_matrix_3x3 rot = {0};
	if (JSON_MATRIX_3X3(eye_json, "eye_to_head", &rot)) {
		math_quat_from_matrix_3x3(&rot, &d->display.rot[eye]);
	}

	JSON_FLOAT(eye_json, "grow_for_undistort", &d->distortion.values[eye].grow_for_undistort);
	JSON_FLOAT(eye_json, "undistort_r2_cutoff", &d->distortion.values[eye].undistort_r2_cutoff);

	// The values are extracted from the matrix based on the pinhole model
	// https://docs.opencv.org/2.4/modules/calib3d/doc/camera_calibration_and_3d_reconstruction.html
	// [ fx,  0, cx,
	//    0, fy, cy,
	//    0,  0, -1 ]
	const cJSON *intrinsics = u_json_get(eye_json, "intrinsics");
	struct xrt_matrix_3x3 intrinsics_matrix;
	u_json_get_matrix_3x3(intrinsics, &intrinsics_matrix);
	d->distortion.values[eye].intrinsic_focus.x = intrinsics_matrix.v[0];
	d->distortion.values[eye].intrinsic_focus.y = intrinsics_matrix.v[4];
	d->distortion.values[eye].intrinsic_center.x = intrinsics_matrix.v[2];
	d->distortion.values[eye].intrinsic_center.y = intrinsics_matrix.v[5];

	const char *names[3] = {
	    "distortion_red",
	    "distortion",
	    "distortion_blue",
	};

	for (int i = 0; i < 3; i++) {
		const cJSON *distortion = u_json_get(eye_json, names[i]);
		if (distortion == NULL) {
			continue;
		}

		JSON_FLOAT(distortion, "center_x", &d->distortion.values[eye].center[i].x);
		JSON_FLOAT(distortion, "center_y", &d->distortion.values[eye].center[i].y);

		const cJSON *coeffs = u_json_get(distortion, "coeffs");
		if (coeffs != NULL) {
			_get_color_coeffs(&d->distortion.values[eye], coeffs, eye, i);
		}
	}
}

static void
_get_lighthouse(const cJSON *lh_config, struct vive_config *d)
{
	const cJSON *json_channels = u_json_get(lh_config, "channelMap");
	const cJSON *json_normals = u_json_get(lh_config, "modelNormals");
	const cJSON *json_points = u_json_get(lh_config, "modelPoints");

	if (json_channels == NULL || json_normals == NULL || json_points == NULL) {
		return;
	}

	uint8_t channels_size = cJSON_GetArraySize(json_channels);
	uint8_t normals_size = cJSON_GetArraySize(json_normals);
	uint8_t points_size = cJSON_GetArraySize(json_points);

	if (channels_size != normals_size || normals_size != points_size || channels_size <= 0) {
		return;
	}

	struct vive_lh_sensor *s = U_TYPED_ARRAY_CALLOC(struct vive_lh_sensor, channels_size);

	uint8_t i = 0;
	const cJSON *item = NULL;
	cJSON_ArrayForEach(item, json_channels)
	{
		// Build the channel
		// NOTE: Value can only be between 0 and 31
		u_json_get_int(item, (int *)&s[i++].channel);
	}

	i = 0;
	item = NULL;
	cJSON_ArrayForEach(item, json_normals)
	{
		// Store in channel map order.
		u_json_get_vec3_array(item, &s[i++].normal);
	}

	i = 0;
	item = NULL;
	cJSON_ArrayForEach(item, json_points)
	{
		// Store in channel map order.
		u_json_get_vec3_array(item, &s[i++].pos);
	}

	d->lh.sensors = s;
	d->lh.sensor_count = channels_size;


	// Transform the sensors into IMU space.
	struct xrt_pose trackref_to_imu = XRT_POSE_IDENTITY;
	math_pose_invert(&d->imu.trackref, &trackref_to_imu);

	for (i = 0; i < d->lh.sensor_count; i++) {
		struct xrt_vec3 point = d->lh.sensors[i].pos;
		struct xrt_vec3 normal = d->lh.sensors[i].normal;

		math_quat_rotate_vec3(&trackref_to_imu.orientation, &normal, &d->lh.sensors[i].normal);
		math_pose_transform_point(&trackref_to_imu, &point, &d->lh.sensors[i].pos);
	}
}

static void
_get_imu(cJSON *dev_json, struct vive_imu_properties *imu)
{
	const cJSON *json = u_json_get(dev_json, "imu");
	// Early devices, such as the OG Vive, had imu info in the root JSON
	if (!json) {
		JSON_VEC3(dev_json, "acc_bias", &imu->acc_bias);
		JSON_VEC3(dev_json, "acc_scale", &imu->acc_scale);
		JSON_VEC3(dev_json, "gyro_bias", &imu->gyro_bias);
		JSON_VEC3(dev_json, "gyro_scale", &imu->gyro_scale);

		return;
	}

	JSON_VEC3(json, "acc_bias", &imu->acc_bias);
	JSON_VEC3(json, "acc_scale", &imu->acc_scale);
	JSON_VEC3(json, "gyro_bias", &imu->gyro_bias);
	// NOTE: gyro_scale may not exist on all devices
	JSON_VEC3(json, "gyro_scale", &imu->gyro_scale);

	_get_pose_from_pos_x_z(json, &imu->trackref);
}

static bool
_get_camera(struct vive_index_camera *cam, const cJSON *cam_json)
{
	bool succeeded = true;
	const cJSON *extrinsics = u_json_get(cam_json, "extrinsics");
	_get_pose_from_pos_x_z(extrinsics, &cam->trackref);


	const cJSON *intrinsics = u_json_get(cam_json, "intrinsics");

	succeeded = succeeded && u_json_get_double_array(u_json_get(u_json_get(intrinsics, "distort"), "coeffs"),
	                                                 cam->intrinsics.distortion, 4);

	succeeded = succeeded && u_json_get_double(u_json_get(intrinsics, "center_x"), &cam->intrinsics.center_x);
	succeeded = succeeded && u_json_get_double(u_json_get(intrinsics, "center_y"), &cam->intrinsics.center_y);

	succeeded = succeeded && u_json_get_double(u_json_get(intrinsics, "focal_x"), &cam->intrinsics.focal_x);
	succeeded = succeeded && u_json_get_double(u_json_get(intrinsics, "focal_y"), &cam->intrinsics.focal_y);
	succeeded = succeeded && u_json_get_int(u_json_get(intrinsics, "height"), &cam->intrinsics.image_size_pixels.h);
	succeeded = succeeded && u_json_get_int(u_json_get(intrinsics, "width"), &cam->intrinsics.image_size_pixels.w);

	if (!succeeded) {
		return false;
	}
	return true;
}

static bool
_get_cameras(struct vive_config *d, const cJSON *cameras_json)
{
	const cJSON *cmr = NULL;

	bool found_camera_json = false;
	bool succeeded_parsing_json = false;

	cJSON_ArrayForEach(cmr, cameras_json)
	{
		found_camera_json = true;

		const cJSON *name_json = u_json_get(cmr, "name");
		const char *name = name_json->valuestring;
		bool is_left = !strcmp("left", name);
		bool is_right = !strcmp("right", name);

		if (!is_left && !is_right) {
			continue;
		}

		if (!_get_camera(&d->cameras.view[is_right], cmr)) {
			succeeded_parsing_json = false;
			break;
		}

		succeeded_parsing_json = true;
	}

	if (!found_camera_json) {
		U_LOG_W("HMD is Index, but no cameras in json file!");
		return false;
	}
	if (!succeeded_parsing_json) {
		U_LOG_E("Failed to parse Index camera calibration!");
		return false;
	}

	struct xrt_pose trackref_to_head;
	struct xrt_pose camera_to_head;
	math_pose_invert(&d->display.trackref, &trackref_to_head);

	math_pose_transform(&trackref_to_head, &d->cameras.view[0].trackref, &camera_to_head);
	d->cameras.view[0].headref = camera_to_head;

	math_pose_transform(&trackref_to_head, &d->cameras.view[1].trackref, &camera_to_head);
	d->cameras.view[1].headref = camera_to_head;

	// Calculate where in the right camera space the left camera is.
	struct xrt_pose invert;
	struct xrt_pose left_in_right;
	math_pose_invert(&d->cameras.view[1].headref, &invert);
	math_pose_transform(&d->cameras.view[0].headref, &invert, &left_in_right);
	d->cameras.left_in_right = left_in_right;

	// To turn it into OpenCV cameras coordinate system.
	struct xrt_pose opencv = left_in_right;
	opencv.orientation.x = -left_in_right.orientation.x;
	opencv.position.y = -left_in_right.position.y;
	opencv.position.z = -left_in_right.position.z;
	d->cameras.opencv = opencv;

	d->cameras.valid = true;

	return true;
}


/*
 *
 * General helpers.
 *
 */

static void
vive_init_defaults(struct vive_config *d)
{
	d->display.eye_target_width_in_pixels = 1080;
	d->display.eye_target_height_in_pixels = 1200;

	d->display.rot[0].w = 1.0f;
	d->display.rot[1].w = 1.0f;

	d->imu.gyro_range = 8.726646f;
	d->imu.acc_range = 39.226600f;

	d->imu.acc_scale.x = 1.0f;
	d->imu.acc_scale.y = 1.0f;
	d->imu.acc_scale.z = 1.0f;

	d->imu.gyro_scale.x = 1.0f;
	d->imu.gyro_scale.y = 1.0f;
	d->imu.gyro_scale.z = 1.0f;

	d->cameras.valid = false;

	for (int view = 0; view < 2; view++) {
		d->distortion.values[view].aspect_x_over_y = 0.89999997615814209f;
		d->distortion.values[view].grow_for_undistort = 0.5f;
		d->distortion.values[view].undistort_r2_cutoff = 1.0f;
	}
}

static void
_calculate_fov(struct vive_config *d)
{
	float width = d->display.eye_target_width_in_pixels;
	float height = d->display.eye_target_height_in_pixels;
	for (uint8_t eye = 0; eye < 2; eye++) {
		// All intrinsic values must be divided 1 plus the
		// grow_for_undistort value from the HMD config before other
		// values can be extracted from them.
		float scale = d->distortion.values[eye].grow_for_undistort + 1.f;

		float intrinsic_focus_x = d->distortion.values[eye].intrinsic_focus.x / scale;
		float intrinsic_focus_y = d->distortion.values[eye].intrinsic_focus.y / scale;
		float intrinsic_center_x = d->distortion.values[eye].intrinsic_center.x / scale;
		float intrinsic_center_y = d->distortion.values[eye].intrinsic_center.y / scale;

		// All values are in pixel units and derived from the matrix
		// featured in the intrinsics section of Valve's LH config docs.
		float focus_x = intrinsic_focus_x * width / 2.f;
		float focus_y = intrinsic_focus_y * height / 2.f;
		float center_x = (intrinsic_center_x - 1.f) * (-1.f) * width / 2.f;
		float center_y = (intrinsic_center_y + 1.f) * height / 2.f;

		/**
		 * This function calculates the angles from the center of the
		 * view to the edges of the display to obtain the 4 FOV values.
		 * The center and focus values form a right-angled triangle,
		 * where the focal distance (distance to display) is the
		 * adjacent, and distance between edge and center being the
		 * opposite, as shown by the diagram below.
		 *
		 * [------ s ------]
		 * [- c -]
		 * +-----+---------+ display
		 *  \    |        /
		 *   |   |       /
		 *   \   |f    /
		 *    \  |    /
		 *     | |  /
		 *      \| /
		 *       * eye
		 * s = screen size (width or height)
		 * c = center
		 * f = focus
		 *
		 * The 4 FOV angles can be calculated by taking the arctan of
		 * the opposite (distance between screen edge and center) over
		 * the adjacent (focal distance).
		 */
		d->distortion.fov[eye].angle_up = +atanf(center_y / focus_y);
		d->distortion.fov[eye].angle_down = -atanf((height - center_y) / focus_y);
		d->distortion.fov[eye].angle_left = -atanf(center_x / focus_x);
		d->distortion.fov[eye].angle_right = +atanf((width - center_x) / focus_x);
	}

	// Apply any tweaks to the FoV.
	vive_tweak_fov(d);
}


/*
 *
 * 'Exported' hmd functions.
 *
 */

bool
vive_config_parse(struct vive_config *d, char *json_string, enum u_logging_level log_level)
{
	d->log_level = log_level;
	vive_init_defaults(d);


	VIVE_DEBUG(d, "JSON config:\n%s", json_string);

	cJSON *json = cJSON_Parse(json_string);
	if (!cJSON_IsObject(json)) {
		VIVE_ERROR(d, "Could not parse JSON data.");
		goto fail;
	}

	bool success = JSON_STRING(json, "model_number", d->firmware.model_number);
	if (!success) {
		success = JSON_STRING(json, "model_name", d->firmware.model_number);
	}
	if (!success) {
		VIVE_ERROR(d, "Could not find HMD model.");
		goto fail;
	}

	VIVE_DEBUG(d, "Parsing model number: %s", d->firmware.model_number);
	d->variant = vive_determine_variant(d->firmware.model_number);
	if (d->variant == VIVE_UNKNOWN) {
		VIVE_ERROR(d, "HMD variant is unknown.");
		goto fail;
	}


	_get_imu(json, &d->imu);
	const cJSON *head_json = u_json_get(json, "head");
	if (head_json) {
		_get_pose_from_pos_x_z(head_json, &d->display.trackref);

		struct xrt_pose trackref_to_head;
		struct xrt_pose imu_to_head;

		math_pose_invert(&d->display.trackref, &trackref_to_head);
		math_pose_transform(&trackref_to_head, &d->imu.trackref, &imu_to_head);

		d->display.imuref = imu_to_head;
	}


	const cJSON *lh_config = u_json_get(json, "lighthouse_config");
	if (lh_config) {
		_get_lighthouse(lh_config, d);
	}


	const cJSON *cameras_json = u_json_get(json, "tracked_cameras");
	if (cameras_json) {
		_get_cameras(d, cameras_json);
	}

	// Only exists on the OG Vive
	success = JSON_DOUBLE(json, "lens_separation", &d->display.lens_separation);
	// Modern devices with static IPDs, such as the Bigscreen Beyond
	if (!success) {
		const cJSON *ipd_json = u_json_get(json, "ipd");
		if (ipd_json) {
			int ipd;
			success = JSON_INT(ipd_json, "default_mm", &ipd);

			if (success) {
				d->display.lens_separation = (double)ipd / 1000.;
			}
		}
	}

	JSON_STRING(json, "device_serial_number", d->firmware.device_serial_number);
	// Only exists on Vive devices
	JSON_STRING(json, "mb_serial_number", d->firmware.mb_serial_number);

	const cJSON *device_json = u_json_get(json, "device");
	if (device_json) {
		JSON_INT(device_json, "eye_target_height_in_pixels", &d->display.eye_target_height_in_pixels);
		JSON_INT(device_json, "eye_target_width_in_pixels", &d->display.eye_target_width_in_pixels);

		// Only exists on Vive devices
		JSON_DOUBLE(device_json, "persistence", &d->display.persistence);
		success = JSON_FLOAT(device_json, "physical_aspect_x_over_y", &d->distortion.values[0].aspect_x_over_y);
		if (success) {
			d->distortion.values[1].aspect_x_over_y = d->distortion.values[0].aspect_x_over_y;
		}
	}

	const cJSON *eye_transform_json = u_json_get(json, "tracking_to_eye_transform");
	if (eye_transform_json) {
		for (uint8_t eye = 0; eye < 2; eye++) {
			_get_distortion_properties(d, eye_transform_json, eye);
		}
	}

	_calculate_fov(d);

	cJSON_Delete(json);

	VIVE_DEBUG(d, "= Vive configuration =");
	VIVE_DEBUG(d, "lens_separation: %f", d->display.lens_separation);
	VIVE_DEBUG(d, "persistence: %f", d->display.persistence);
	VIVE_DEBUG(d, "physical_aspect_x_over_y: %f", (double)d->distortion.values[0].aspect_x_over_y);

	VIVE_DEBUG(d, "model_number: %s", d->firmware.model_number);
	VIVE_DEBUG(d, "mb_serial_number: %s", d->firmware.mb_serial_number);
	VIVE_DEBUG(d, "device_serial_number: %s", d->firmware.device_serial_number);

	VIVE_DEBUG(d, "eye_target_height_in_pixels: %d", d->display.eye_target_height_in_pixels);
	VIVE_DEBUG(d, "eye_target_width_in_pixels: %d", d->display.eye_target_width_in_pixels);

	if (d->log_level <= U_LOGGING_DEBUG) {
		_print_vec3("acc_bias", &d->imu.acc_bias);
		_print_vec3("acc_scale", &d->imu.acc_scale);
		_print_vec3("gyro_bias", &d->imu.gyro_bias);
		_print_vec3("gyro_scale", &d->imu.gyro_scale);
	}

	VIVE_DEBUG(d, "grow_for_undistort: %f", (double)d->distortion.values[0].grow_for_undistort);

	VIVE_DEBUG(d, "undistort_r2_cutoff 0: %f", (double)d->distortion.values[0].undistort_r2_cutoff);
	VIVE_DEBUG(d, "undistort_r2_cutoff 1: %f", (double)d->distortion.values[1].undistort_r2_cutoff);

	return true;

fail:
	vive_config_teardown(d);
	return false;
}

void
vive_config_teardown(struct vive_config *config)
{
	if (config->lh.sensors != NULL) {
		free(config->lh.sensors);
		config->lh.sensors = NULL;
		config->lh.sensor_count = 0;
	}
}


/*
 *
 * 'Exported' controller functions.
 *
 */

bool
vive_config_parse_controller(struct vive_controller_config *d, char *json_string, enum u_logging_level log_level)
{
	d->log_level = log_level;
	VIVE_DEBUG(d, "JSON config:\n%s", json_string);

	cJSON *json = cJSON_Parse(json_string);
	if (!cJSON_IsObject(json)) {
		VIVE_ERROR(d, "Could not parse JSON data.");
		return false;
	}


	bool success = JSON_STRING(json, "model_number", d->firmware.model_number);
	if (!success) {
		success = JSON_STRING(json, "model_name", d->firmware.model_number);
	}
	if (!success) {
		VIVE_ERROR(d, "Could not find controller model.");
		return false;
	}


	VIVE_DEBUG(d, "Parsing model number: %s", d->firmware.model_number);

	if (strcmp(d->firmware.model_number, "Vive. Controller MV") == 0 ||
	    strcmp(d->firmware.model_number, "VIVE Controller Pro MV") == 0 ||
	    strcmp(d->firmware.model_number, "Vive Controller MV") == 0) {
		d->variant = CONTROLLER_VIVE_WAND;
		VIVE_DEBUG(d, "Found Vive Wand controller");
	} else if (strcmp(d->firmware.model_number, "Knuckles Right") == 0 ||
	           strcmp(d->firmware.model_number, "Knuckles EV3.0 Right") == 0) {
		d->variant = CONTROLLER_INDEX_RIGHT;
		VIVE_DEBUG(d, "Found Knuckles Right controller");
	} else if (strcmp(d->firmware.model_number, "Knuckles Left") == 0 ||
	           strcmp(d->firmware.model_number, "Knuckles EV3.0 Left") == 0) {
		d->variant = CONTROLLER_INDEX_LEFT;
		VIVE_DEBUG(d, "Found Knuckles Left controller");
	} else if (strcmp(d->firmware.model_number, "FlipVr Controller VC1B Left") == 0) {
		d->variant = CONTROLLER_FLIPVR_LEFT;
		VIVE_DEBUG(d, "Found Shiftall Inc. FlipVR VC1B Left controller");
	} else if (strcmp(d->firmware.model_number, "FlipVr Controller VC1B Right") == 0) {
		d->variant = CONTROLLER_FLIPVR_RIGHT;
		VIVE_DEBUG(d, "Found Shiftall Inc. FlipVR VC1B Right controller");
	} else if (strcmp(d->firmware.model_number, "Vive Tracker PVT") == 0 ||
	           strcmp(d->firmware.model_number, "Vive. Tracker MV") == 0 ||
	           strcmp(d->firmware.model_number, "Vive Tracker MV") == 0) {
		d->variant = CONTROLLER_TRACKER_GEN1;
		VIVE_DEBUG(d, "Found Gen 1 tracker.");
	} else if (strcmp(d->firmware.model_number, "VIVE Tracker Pro MV") == 0) {
		d->variant = CONTROLLER_TRACKER_GEN2;
		VIVE_DEBUG(d, "Found Gen 2 tracker.");
	} else if (strcmp(d->firmware.model_number, "VIVE Tracker 3.0 MV") == 0) {
		d->variant = CONTROLLER_TRACKER_GEN3;
		VIVE_DEBUG(d, "Found Gen 3 tracker.");
	} else if (strcmp(d->firmware.model_number, "Tundra Tracker") == 0) {
		d->variant = CONTROLLER_TRACKER_TUNDRA;
		VIVE_DEBUG(d, "Found Tundra tracker.");
	} else {
		VIVE_ERROR(d, "Failed to parse controller variant!\n\tfirmware.model_[number|name]: '%s'",
		           d->firmware.model_number);
	}

	_get_imu(json, &d->imu);

	JSON_STRING(json, "device_serial_number", d->firmware.device_serial_number);
	// Only exists on Vive devices
	JSON_STRING(json, "mb_serial_number", d->firmware.mb_serial_number);


	cJSON_Delete(json);

	VIVE_DEBUG(d, "= Vive controller configuration =");

	VIVE_DEBUG(d, "model_number: %s", d->firmware.model_number);
	VIVE_DEBUG(d, "mb_serial_number: %s", d->firmware.mb_serial_number);
	VIVE_DEBUG(d, "device_serial_number: %s", d->firmware.device_serial_number);

	if (d->log_level <= U_LOGGING_DEBUG) {
		_print_vec3("acc_bias", &d->imu.acc_bias);
		_print_vec3("acc_scale", &d->imu.acc_scale);
		_print_vec3("gyro_bias", &d->imu.gyro_bias);
		_print_vec3("gyro_scale", &d->imu.gyro_scale);
	}

	return true;
}
