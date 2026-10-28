// Copyright 2025, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Implementation of Blubur S1 HMD driver.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_blubur_s1
 */

#include "math/m_api.h"
#include "math/m_mathinclude.h"
#include "math/m_clock_tracking.h"

#include "util/u_device.h"
#include "util/u_distortion_mesh.h"
#include "util/u_misc.h"
#include "util/u_time.h"
#include "util/u_debug.h"
#include "util/u_trace_marker.h"
#include "util/u_linux.h"
#include "util/u_var.h"

#include "blubur_s1_interface.h"
#include "blubur_s1_internal.h"
#include "blubur_s1_protocol.h"


DEBUG_GET_ONCE_BOOL_OPTION(blubur_s1_test_distortion, "BLUBUR_S1_TEST_DISTORTION", false)
DEBUG_GET_ONCE_LOG_OPTION(blubur_s1_log, "BLUBUR_S1_LOG", U_LOGGING_INFO)

#define S1_TRACE(d, ...) U_LOG_XDEV_IFL_T(&d->base, d->log_level, __VA_ARGS__)
#define S1_DEBUG(d, ...) U_LOG_XDEV_IFL_D(&d->base, d->log_level, __VA_ARGS__)
#define S1_DEBUG_HEX(d, data, data_size) U_LOG_XDEV_IFL_D_HEX(&d->base, d->log_level, data, data_size)
#define S1_INFO(d, ...) U_LOG_XDEV_IFL_I(&d->base, d->log_level, __VA_ARGS__)
#define S1_WARN(d, ...) U_LOG_XDEV_IFL_W(&d->base, d->log_level, __VA_ARGS__)
#define S1_ERROR(d, ...) U_LOG_XDEV_IFL_E(&d->base, d->log_level, __VA_ARGS__)

#define VIEW_SIZE (1440)
#define PANEL_WIDTH (VIEW_SIZE * 2)

static struct blubur_s1_hmd *
blubur_s1_hmd(struct xrt_device *xdev)
{
	return (struct blubur_s1_hmd *)xdev;
}

static void
blubur_s1_hmd_destroy(struct xrt_device *xdev)
{
	struct blubur_s1_hmd *hmd = blubur_s1_hmd(xdev);

	if (hmd->thread.initialized) {
		os_thread_helper_destroy(&hmd->thread);
	}

	if (hmd->dev != NULL) {
		os_hid_destroy(hmd->dev);
		hmd->dev = NULL;
	}

	m_imu_3dof_close(&hmd->fusion_3dof);

	if (hmd->relation_history != NULL) {
		m_relation_history_destroy(&hmd->relation_history);
	}

	os_mutex_destroy(&hmd->input_mutex);

	u_var_remove_root(hmd);

	free(hmd);
}

static xrt_result_t
blubur_s1_hmd_compute_poly_3k_distortion(
    struct xrt_device *xdev, uint32_t view, float u, float v, struct xrt_uv_triplet *out_result)
{
	struct blubur_s1_hmd *hmd = blubur_s1_hmd(xdev);

	struct u_poly_3k_eye_values *values = &hmd->poly_3k_values[view];
	u_compute_distortion_poly_3k(values, view, u, v, out_result);

	return XRT_SUCCESS;
}

static xrt_result_t
blubur_s1_hmd_compute_test_distortion(
    struct xrt_device *xdev, uint32_t view, float u, float v, struct xrt_uv_triplet *out_result)
{
	float x = u * 2.0f - 1.0f;
	float y = v * 2.0f - 1.0f;

	float r2 = x * x + y * y;
	float r = sqrtf(r2);
	float r3 = r2 * r;
	float r4 = r2 * r2;
	float r5 = r4 * r;

	float radial = (0.5978f * r5) - (0.7257f * r4) + (0.504f * r3) - (0.0833f * r2) + (0.709f * r) - 0.00006f;

	struct xrt_vec2 result = {
	    .x = (x * radial) / 2.0f + 0.5f,
	    .y = (y * radial) / 2.0f + 0.5f,
	};
	out_result->r = result;
	out_result->g = result;
	out_result->b = result;

	return XRT_SUCCESS;
}

static xrt_result_t
blubur_s1_hmd_update_inputs(struct xrt_device *xdev)
{
	struct blubur_s1_hmd *hmd = blubur_s1_hmd(xdev);

	int64_t now = os_monotonic_get_ns();

	os_mutex_lock(&hmd->input_mutex);
	bool presence = hmd->input.status & BLUBUR_S1_STATUS_PRESENCE;
	bool menu = hmd->input.status & BLUBUR_S1_STATUS_BUTTON;
	os_mutex_unlock(&hmd->input_mutex);

	xdev->inputs[1].value = (union xrt_input_value){.boolean = presence};
	xdev->inputs[1].timestamp = now;

	xdev->inputs[2].value = (union xrt_input_value){.boolean = menu};
	xdev->inputs[2].timestamp = now;

	return XRT_SUCCESS;
}

static xrt_result_t
blubur_s1_hmd_get_tracked_pose(struct xrt_device *xdev,
                               enum xrt_input_name name,
                               int64_t at_timestamp_ns,
                               struct xrt_space_relation *out_relation)
{
	struct blubur_s1_hmd *hmd = blubur_s1_hmd(xdev);

	if (name != XRT_INPUT_GENERIC_HEAD_POSE) {
		return XRT_ERROR_INPUT_UNSUPPORTED;
	}

	m_relation_history_get(hmd->relation_history, at_timestamp_ns, out_relation);

	return XRT_SUCCESS;
}

static xrt_result_t
blubur_s1_hmd_get_view_poses(struct xrt_device *xdev,
                             const struct xrt_vec3 *default_eye_relation,
                             int64_t at_timestamp_ns,
                             enum xrt_view_type view_type,
                             uint32_t view_count,
                             struct xrt_space_relation *out_head_relation,
                             struct xrt_fov *out_fovs,
                             struct xrt_pose *out_poses)
{
	return u_device_get_view_poses( //
	    xdev,                       //
	    default_eye_relation,       //
	    at_timestamp_ns,            //
	    view_type,                  //
	    view_count,                 //
	    out_head_relation,          //
	    out_fovs,                   //
	    out_poses);                 //
}

static void
blubur_s1_hmd_fill_in_poly_3k(struct blubur_s1_hmd *hmd)
{
	// clang-format off
	hmd->poly_3k_values[0] = (struct u_poly_3k_eye_values){
	    .channels =
	        {
	            {
	                .display_size = {PANEL_WIDTH, VIEW_SIZE},
	                .eye_center = {711.37015431841485f, 702.64004980572099f},
	                .k =
	                    {
	                        2.4622190410034843e-007f,
	                        1.0691119647014047e-012f,
	                        6.9872433537257567e-019f,
	                    },
	            },
	            {
	                .display_size = {PANEL_WIDTH, VIEW_SIZE},
	                .eye_center = {710.34756994635097f, 702.30352808724865f},
	                .k =
	                    {
	                        3.3081468849915169e-007f,
	                        6.6872723393907828e-013f,
	                        1.5518253834715642e-018f,
	                    },
	            },
	            {
	                .display_size = {PANEL_WIDTH, VIEW_SIZE},
	                .eye_center = {709.19922270098721f, 702.42895617576141f},
	                .k =
	                    {
	                        4.6306924021839207e-007f,
	                        1.5032174824131911e-013f,
	                        2.6240474534705725e-018f,
	                    },
	            },
	        },
	};
	hmd->poly_3k_values[1] = (struct u_poly_3k_eye_values){
	    .channels =
	        {
	            {
	                .display_size = {PANEL_WIDTH, VIEW_SIZE},
	                .eye_center = {2166.0195141711984f,
	                               693.80762487779759f },
	                .k =
	                    {
	                        1.6848296693566205e-007f,
	                        1.1446999540490656e-012f,
	                        1.8794325973106313e-019f,
	                    },
	            },
	            {
	                .display_size = {PANEL_WIDTH, VIEW_SIZE},
	                .eye_center = {2164.9567320272263f,
	                               693.8666328641682f },
	                .k =
	                    {
	                        2.2979021408214227e-007f,
	                        9.2094643470416607e-013f,
	                        6.8614927296300735e-019f,
	                    },
	            },
	            {
	                .display_size = {PANEL_WIDTH, VIEW_SIZE},
	                .eye_center = {2164.0315727658904f,
	                               693.45351818980896f },
	                .k =
	                    {
	                        3.1993667496208384e-007f,
	                        6.1930456677642785e-013f,
	                        1.2848584929803272e-018f,
	                    },
	            },
	        },
	};

	struct xrt_matrix_3x3 affine_xform[2] = {
	    {
	        .v =
	            {
	                886.745f, 0.205964f, 710.326f, //
	                0, 886.899f, 706.657f,         //
	                0, 0, 1,                       //
	            },
	    },
	    {
	        .v =
	            {
	                880.317f, 0.277553f, 2163.58f, //
	                0, 879.669f, 698.35f,          //
	                0, 0, 1,                       //
	            },
	    },
	};
	// clang-format on

	for (int i = 0; i < 2; i++) {
		math_matrix_3x3_inverse(&affine_xform[i], &hmd->poly_3k_values[i].inv_affine_xform);

		struct xrt_fov *fov = &hmd->base.hmd->distortion.fov[i];

		u_compute_distortion_bounds_poly_3k(
		    &hmd->poly_3k_values[i].inv_affine_xform, hmd->poly_3k_values[i].channels, i, fov,
		    &hmd->poly_3k_values[i].tex_x_range, &hmd->poly_3k_values[i].tex_y_range);

		S1_INFO(hmd, "FoV eye %d angles left %f right %f down %f up %f", i, fov->angle_left, fov->angle_right,
		        fov->angle_down, fov->angle_up);
	}
}

struct read_context
{
	uint8_t *data;
	size_t size;
	int read;
};

static void
read_into(struct read_context *ctx, void *dest, size_t size)
{
	if (ctx->read + size > ctx->size) {
		size = ctx->size - ctx->read;
	}

	memcpy(dest, ctx->data + ctx->read, size);
	ctx->read += size;
}

#define READ_STRUCTURE(ctx, dst) read_into(ctx, dst, sizeof(*dst))

//! Sign-extend a 21-bit signed integer stored in a 32-bit unsigned integer
static int32_t
sign21(uint32_t v)
{
	v &= 0x1FFFFF;           // keep 21 bits
	if (v & 0x100000)        // sign bit
		v |= 0xFFE00000; // extend sign to 32 bits
	return (int32_t)v;
}

//! Decode 3×21-bit ints from 8 bytes (little-endian packing)
static void
blubur_s1_decode_triad(const uint8_t b[8], int32_t out_xyz[3])
{
	uint32_t x = ((uint32_t)(b[2] >> 3)) | (((uint32_t)b[1] | ((uint32_t)b[0] << 8)) << 5);

	uint32_t y = ((uint32_t)(b[5] >> 6)) |
	             (((uint32_t)b[4] | (((uint32_t)b[3] | (((uint32_t)b[2] & 0x07) << 8)) << 8)) << 2);

	uint32_t z = ((uint32_t)(b[7] >> 1)) | (((uint32_t)b[6] | (((uint32_t)b[5] & 0x3F) << 8)) << 7);

	out_xyz[0] = sign21(x);
	out_xyz[1] = sign21(y);
	out_xyz[2] = sign21(z);
}

struct blubur_s1_decoded_sample
{
	int32_t accel[3]; // X,Y,Z
	int32_t gyro[3];  // X,Y,Z
};

static void
blubur_s1_decode_sample_pack(const uint8_t sample_pack[32], struct blubur_s1_decoded_sample out_samples[2])
{
	int32_t triad[3];

	// Triad 0: Accel (sample 0)
	blubur_s1_decode_triad(&sample_pack[0], triad);
	out_samples[0].accel[0] = triad[0];
	out_samples[0].accel[1] = triad[1];
	out_samples[0].accel[2] = triad[2];

	// Triad 1: Gyro (sample 0)
	blubur_s1_decode_triad(&sample_pack[8], triad);
	out_samples[0].gyro[0] = triad[0];
	out_samples[0].gyro[1] = triad[1];
	out_samples[0].gyro[2] = triad[2];

	// Triad 2: Accel (sample 1)
	blubur_s1_decode_triad(&sample_pack[16], triad);
	out_samples[1].accel[0] = triad[0];
	out_samples[1].accel[1] = triad[1];
	out_samples[1].accel[2] = triad[2];

	// Triad 3: Gyro (sample 1)
	blubur_s1_decode_triad(&sample_pack[24], triad);
	out_samples[1].gyro[0] = triad[0];
	out_samples[1].gyro[1] = triad[1];
	out_samples[1].gyro[2] = triad[2];
}

static void
blubur_s1_convert_accel_sample(const int32_t in[3], struct xrt_vec3 *out)
{
	const float scale = 9.80665f / 8192.0f;

	out->x = in[0] * scale;
	out->y = in[1] * scale;
	out->z = in[2] * scale;
}

static void
blubur_s1_convert_gyro_sample(const int32_t in[3], struct xrt_vec3 *out)
{
	// NOTE: this was done by hand, so may not be perfect
	const float scale = (float)M_PI / (180.0f * 90.0);

	out->x = in[0] * scale;
	out->y = in[1] * scale;
	out->z = in[2] * scale;
}

int
blubur_s1_hmd_tick(struct blubur_s1_hmd *hmd)
{
	int result;

	uint8_t data[64];
	result = os_hid_read(hmd->dev, data, sizeof data, 1000);
	if (result == 0) {
		// timeout
		S1_TRACE(hmd, "Read timeout");
		return 0;
	}

	if (result != sizeof(data)) {
		S1_ERROR(hmd, "Got unexpected read size %d!", result);
		return result;
	}

	struct read_context ctx = {
	    .data = data,
	    .size = sizeof(data),
	    .read = 0,
	};

	struct blubur_s1_report_header header;
	READ_STRUCTURE(&ctx, &header);

	if (header.id != 0x83) {
		S1_WARN(hmd, "Got unexpected report id 0x%02x!", header.id);
		return 0;
	}

	struct blubur_s1_report_0x83 body;
	READ_STRUCTURE(&ctx, &body);

	// enum blubur_s1_status_bits status = body.status;

	// this is unsigned, so wraparound is fine
	uint16_t timestamp_delta_ms = body.timestamp - hmd->last_remote_timestamp_ms;

	if (timestamp_delta_ms == 0) {
		// duplicate packet?
		return 0;
	}

	hmd->last_remote_timestamp_ns += (uint64_t)timestamp_delta_ms * U_TIME_1MS_IN_NS;
	hmd->last_remote_timestamp_ms = body.timestamp;

	timepoint_ns now = os_monotonic_get_ns();

	// if we don't have enough samples, just wait
	if (hmd->hw2mono_samples < 250) {
		m_clock_offset_a2b(1000, hmd->last_remote_timestamp_ns, now, &hmd->hw2mono);

		hmd->hw2mono_samples++;

		return 0;
	}

	timepoint_ns local_sample_time_ns = hmd->last_remote_timestamp_ns + hmd->hw2mono;

	struct blubur_s1_decoded_sample samples[2];
	blubur_s1_decode_sample_pack(body.sensor.data, samples);

	struct xrt_vec3 accel_m_s2[2];
	blubur_s1_convert_accel_sample(samples[0].accel, &accel_m_s2[0]);
	blubur_s1_convert_accel_sample(samples[1].accel, &accel_m_s2[1]);

	struct xrt_vec3 gyro_rad_s[2];
	blubur_s1_convert_gyro_sample(samples[0].gyro, &gyro_rad_s[0]);
	blubur_s1_convert_gyro_sample(samples[1].gyro, &gyro_rad_s[1]);

	m_imu_3dof_update(&hmd->fusion_3dof, local_sample_time_ns, &accel_m_s2[0], &gyro_rad_s[0]);
	m_imu_3dof_update(&hmd->fusion_3dof, local_sample_time_ns + (U_TIME_1MS_IN_NS / 2LLU), &accel_m_s2[1],
	                  &gyro_rad_s[1]);

	struct xrt_space_relation latest_3dof_relation = {
	    .angular_velocity = hmd->fusion_3dof.last.gyro,
	    .pose.orientation = hmd->fusion_3dof.rot,
	    .relation_flags = XRT_SPACE_RELATION_ORIENTATION_VALID_BIT | XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT |
	                      XRT_SPACE_RELATION_ANGULAR_VELOCITY_VALID_BIT,
	};
	m_relation_history_push(hmd->relation_history, &latest_3dof_relation, hmd->fusion_3dof.last.timestamp_ns);

	os_mutex_lock(&hmd->input_mutex);
	hmd->input.status = body.status;
	os_mutex_unlock(&hmd->input_mutex);

	// S1_DEBUG_HEX(hmd, data, result);

	return 0;
}

static void *
blubur_s1_hmd_thread(void *ptr)
{
	U_TRACE_SET_THREAD_NAME("Blubur S1 HMD");

	struct blubur_s1_hmd *hmd = (struct blubur_s1_hmd *)ptr;

#ifdef XRT_OS_LINUX
	u_linux_try_to_set_realtime_priority_on_thread(hmd->log_level, "Blubur S1 HMD");
#endif // XRT_OS_LINUX

	int result = 0;

	os_thread_helper_lock(&hmd->thread);

	while (os_thread_helper_is_running_locked(&hmd->thread) && result >= 0) {
		os_thread_helper_unlock(&hmd->thread);

		result = blubur_s1_hmd_tick(hmd);

		os_thread_helper_lock(&hmd->thread);
	}

	os_thread_helper_unlock(&hmd->thread);

	return NULL;
}

struct blubur_s1_hmd *
blubur_s1_hmd_create(struct os_hid_device *dev, const char *serial)
{
	int ret;

	struct blubur_s1_hmd *hmd =
	    U_DEVICE_ALLOCATE(struct blubur_s1_hmd, U_DEVICE_ALLOC_HMD | U_DEVICE_ALLOC_TRACKING_NONE, 3, 0);
	if (hmd == NULL) {
		return NULL;
	}

	hmd->log_level = debug_get_log_option_blubur_s1_log();
	hmd->dev = dev;

	ret = os_mutex_init(&hmd->input_mutex);
	if (ret < 0) {
		S1_ERROR(hmd, "Failed to init mutex!");
		free(hmd);
		return NULL;
	}

	hmd->base.destroy = blubur_s1_hmd_destroy;
	hmd->base.name = XRT_DEVICE_BLUBUR_S1;
	hmd->base.device_type = XRT_DEVICE_TYPE_HMD;

	hmd->base.hmd->screens[0].w_pixels = PANEL_WIDTH;
	hmd->base.hmd->screens[0].h_pixels = VIEW_SIZE;
	hmd->base.hmd->screens[0].nominal_frame_interval_ns = 1000000000LLU / 120; // 120hz

	hmd->base.hmd->view_count = 2;
	hmd->base.hmd->views[0] = (struct xrt_view){
	    .viewport =
	        {
	            .x_pixels = 0,
	            .y_pixels = 0,
	            .w_pixels = VIEW_SIZE,
	            .h_pixels = VIEW_SIZE,
	        },
	    .display =
	        {
	            .w_pixels = VIEW_SIZE,
	            .h_pixels = VIEW_SIZE,
	        },
	    .rot = u_device_rotation_ident,
	};
	hmd->base.hmd->views[1] = (struct xrt_view){
	    .viewport =
	        {
	            .x_pixels = VIEW_SIZE,
	            .y_pixels = 0,
	            .w_pixels = VIEW_SIZE,
	            .h_pixels = VIEW_SIZE,
	        },
	    .display =
	        {
	            .w_pixels = VIEW_SIZE,
	            .h_pixels = VIEW_SIZE,
	        },
	    .rot = u_device_rotation_ident,
	};

	hmd->base.hmd->blend_modes[0] = XRT_BLEND_MODE_OPAQUE;
	hmd->base.hmd->blend_mode_count = 1;

	blubur_s1_hmd_fill_in_poly_3k(hmd);

	hmd->base.hmd->distortion.models = XRT_DISTORTION_MODEL_COMPUTE;
	hmd->base.hmd->distortion.preferred = XRT_DISTORTION_MODEL_COMPUTE;
	hmd->base.compute_distortion = debug_get_bool_option_blubur_s1_test_distortion()
	                                   ? blubur_s1_hmd_compute_test_distortion
	                                   : blubur_s1_hmd_compute_poly_3k_distortion;
	u_distortion_mesh_fill_in_compute(&hmd->base);

	snprintf(hmd->base.str, XRT_DEVICE_NAME_LEN, "Blubur S1");
	snprintf(hmd->base.serial, XRT_DEVICE_NAME_LEN, "%s", serial);

	hmd->base.supported.position_tracking = true;
	hmd->base.supported.presence = true;

	hmd->base.inputs[0].name = XRT_INPUT_GENERIC_HEAD_POSE;
	hmd->base.inputs[1].name = XRT_INPUT_GENERIC_HEAD_DETECT;
	hmd->base.inputs[2].name = XRT_INPUT_BLUBUR_S1_MENU_CLICK;

	hmd->base.update_inputs = blubur_s1_hmd_update_inputs;
	hmd->base.get_tracked_pose = blubur_s1_hmd_get_tracked_pose;
	hmd->base.get_view_poses = blubur_s1_hmd_get_view_poses;

	// Initialize IMU fusion for 3DOF tracking
	m_imu_3dof_init(&hmd->fusion_3dof, M_IMU_3DOF_USE_GRAVITY_DUR_20MS);

	m_relation_history_create(&hmd->relation_history);
	if (hmd->relation_history == NULL) {
		S1_ERROR(hmd, "Failed to create relation history!");
		blubur_s1_hmd_destroy(&hmd->base);
		return NULL;
	}

	ret = os_thread_helper_init(&hmd->thread);
	if (ret < 0) {
		S1_ERROR(hmd, "Failed to init thread helper!");
		blubur_s1_hmd_destroy(&hmd->base);
		return NULL;
	}

	ret = os_thread_helper_start(&hmd->thread, blubur_s1_hmd_thread, hmd);
	if (ret < 0) {
		S1_ERROR(hmd, "Failed to start thread!");
		blubur_s1_hmd_destroy(&hmd->base);
		return NULL;
	}

	u_var_add_root(hmd, "Blubur S1", true);
	u_var_add_log_level(hmd, &hmd->log_level, "Log Level");

	m_imu_3dof_add_vars(&hmd->fusion_3dof, hmd, "3dof IMU Fusion");

	u_var_add_ro_u16(hmd, &hmd->last_remote_timestamp_ms, "Last Remote Timestamp (ms)");
	u_var_add_ro_i64_ns(hmd, &hmd->last_remote_timestamp_ns, "Last Remote Timestamp (ns)");

	u_var_add_ro_i64_ns(hmd, &hmd->hw2mono, "HW to Monotonic (ns)");
	u_var_add_ro_i32(hmd, &hmd->hw2mono_samples, "HW to Monotonic Samples");

	u_var_add_ro_i32(hmd, (int32_t *)&hmd->input.status, "Status");

	return hmd;
}
