// Copyright 2020-2021, Collabora, Ltd.
// Copyright 2023, Jan Schmidt
// Copyright 2024, Joel Valenciano
// Copyright 2025, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  PSVR2 HMD device
 *
 * @author Jan Schmidt <jan@centricular.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @author Joel Valenciano <joelv1907@gmail.com>
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_psvr2
 */

#include "xrt/xrt_device.h"
#include "xrt/xrt_prober.h"
#include "xrt/xrt_tracking.h"

#include "os/os_threading.h"
#include "os/os_time.h"

#include "math/m_api.h"
#include "math/m_clock_tracking.h"
#include "math/m_mathinclude.h"
#include "math/m_relation_history.h"
#include "math/m_vec3.h"
#include "math/m_space.h"

#include "tracking/t_dead_reckoning.h"

#include "util/u_misc.h"
#include "util/u_debug.h"
#include "util/u_device.h"
#include "util/u_distortion_mesh.h"
#include "util/u_frame.h"
#include "util/u_logging.h"
#include "util/u_sink.h"
#include "util/u_time.h"
#include "util/u_trace_marker.h"
#include "util/u_var.h"
#include "util/u_debug.h"

#include <stdio.h>
#include <assert.h>
#include <inttypes.h>
#include <libusb.h>

#include "psvr2.h"


/*
 *
 * Structs and defines.
 *
 */

// @todo Once clang format is updated in CI, remove this
// clang-format off
#define SLAM_POSE_CORRECTION {.orientation = {.x = 0, .y = 0, .z = sqrt(2) / 2, .w = sqrt(2) / 2}}
// clang-format on

DEBUG_GET_ONCE_FLOAT_OPTION(psvr2_default_brightness, "PSVR2_DEFAULT_BRIGHTNESS", 1.0f)

DEBUG_GET_ONCE_LOG_OPTION(psvr2_log, "PSVR2_LOG", U_LOGGING_WARN)

static void
psvr2_usb_stop(struct psvr2_hmd *hmd);

static void
psvr2_usb_destroy(struct psvr2_hmd *hmd);

static void
psvr2_hmd_destroy(struct xrt_device *xdev)
{
	struct psvr2_hmd *hmd = psvr2_hmd(xdev);

	psvr2_free_et_data(hmd);

	os_thread_helper_lock(&hmd->usb_thread);
	hmd->usb_complete = 1;
	os_thread_helper_unlock(&hmd->usb_thread);
	os_thread_helper_destroy(&hmd->usb_thread);

	psvr2_usb_destroy(hmd);

	// @note We appear to be hitting a bug in libusb, so this is commented out
	//       see: https://github.com/libusb/libusb/issues/1605
	// if (hmd->dev != NULL) {
	// 	libusb_close(hmd->dev);
	// }

	if (hmd->ctx != NULL) {
		libusb_exit(hmd->ctx);
	}

	// Remove the variable tracking.
	u_var_remove_root(hmd);

	m_ff_vec3_f32_free(&hmd->ff_gyro);
	m_relation_history_destroy(&hmd->slam_relation_history);
	os_mutex_destroy(&hmd->data_lock);
	u_device_free(&hmd->base);
}

static xrt_result_t
psvr2_compute_distortion(struct xrt_device *xdev, uint32_t view, float u, float v, struct xrt_uv_triplet *result)
{
	struct psvr2_hmd *hmd = psvr2_hmd(xdev);

	psvr2_compute_distortion_asymmetric(hmd->distortion_calibration, result, view, u, v);

	return XRT_SUCCESS;
}

static xrt_result_t
psvr2_hmd_update_inputs(struct xrt_device *xdev)
{
	struct psvr2_hmd *hmd = psvr2_hmd(xdev);

	timepoint_ns now = os_monotonic_get_ns();

	int32_t proximity = hmd->proximity_sensor;
	hmd->base.inputs[PSVR2_HMD_INPUT_HEAD_DETECT].value.boolean = proximity;
	hmd->base.inputs[PSVR2_HMD_INPUT_HEAD_DETECT].timestamp = now;

	os_mutex_lock(&hmd->data_lock);
	hmd->base.inputs[PSVR2_HMD_INPUT_FUNCTION_BUTTON].value.boolean = hmd->function_button;
	hmd->base.inputs[PSVR2_HMD_INPUT_FUNCTION_BUTTON].timestamp = now;
	os_mutex_unlock(&hmd->data_lock);

	return XRT_SUCCESS;
}

static void
hmd_get_raw_tracker_pose(struct psvr2_hmd *hmd, timepoint_ns at_timestamp_ns, struct xrt_space_relation *out_relation)
{
	struct xrt_space_relation latest_relation;
	timepoint_ns latest_relation_ts;
	if (!m_relation_history_get_latest(hmd->slam_relation_history, &latest_relation_ts, &latest_relation)) {
		*out_relation = (struct xrt_space_relation)XRT_SPACE_RELATION_ZERO;
		return;
	}

	if (at_timestamp_ns <= latest_relation_ts) {
		m_relation_history_get(hmd->slam_relation_history, at_timestamp_ns, out_relation);
		return;
	}

	// Predict forward using dead reckoning
	if (!t_apply_dead_reckoning( //
	        hmd->ff_gyro,        //
	        NULL,                //
	        NULL,                //
	        at_timestamp_ns,     //
	        &latest_relation,    //
	        latest_relation_ts,  //
	        out_relation)) {
		// If dead reckoning fails, return the latest SLAM pose
		*out_relation = latest_relation;
	}
}

static xrt_result_t
psvr2_hmd_get_tracked_pose(struct xrt_device *xdev,
                           enum xrt_input_name name,
                           int64_t at_timestamp_ns,
                           struct xrt_space_relation *out_relation)
{
	struct psvr2_hmd *hmd = psvr2_hmd(xdev);

	switch (name) {
	case XRT_INPUT_GENERIC_HEAD_POSE:
	case XRT_INPUT_GENERIC_EYE_GAZE_POSE: break;
	default: PSVR2_ERROR(hmd, "unknown input name"); return XRT_ERROR_INPUT_UNSUPPORTED;
	}

	os_mutex_lock(&hmd->data_lock);

	if (hmd->timestamp_samples < TIMESTAMP_SAMPLES) {
		os_mutex_unlock(&hmd->data_lock);
		*out_relation = (struct xrt_space_relation)XRT_SPACE_RELATION_ZERO;
		return XRT_SUCCESS;
	}

	timepoint_ns prediction_ns_hw = at_timestamp_ns - hmd->hw2mono_vts;

	struct xrt_relation_chain chain = {0};

	// Push the eye pose before the head pose if required, so that we're returning the gaze relative to the head
	if (name == XRT_INPUT_GENERIC_EYE_GAZE_POSE) {
		m_relation_history_get(hmd->et_data.gaze_relation_history, prediction_ns_hw,
		                       m_relation_chain_reserve(&chain));
	}

	// Push the SLAM->head offset
	m_relation_chain_push_pose(&chain, &hmd->T_imu_head);

	// Push the normal head pose
	hmd_get_raw_tracker_pose(hmd, prediction_ns_hw, m_relation_chain_reserve(&chain));

	os_mutex_unlock(&hmd->data_lock);

	// Resolve the final relation
	m_relation_chain_resolve(&chain, out_relation);

	return XRT_SUCCESS;
}

static xrt_result_t
psvr2_hmd_get_view_poses(struct xrt_device *xdev,
                         const struct xrt_vec3 *default_eye_relation,
                         int64_t at_timestamp_ns,
                         enum xrt_view_type view_type,
                         uint32_t view_count,
                         struct xrt_space_relation *out_head_relation,
                         struct xrt_fov *out_fovs,
                         struct xrt_pose *out_poses)
{
	struct psvr2_hmd *hmd = psvr2_hmd(xdev);

	os_mutex_lock(&hmd->data_lock);
	if (hmd->ipd_updated) {
		hmd->info.lens_horizontal_separation_meters = hmd->ipd_mm / 1000.0;
		PSVR2_DEBUG(hmd, "IPD updated to %u mm", hmd->ipd_mm);
		hmd->ipd_updated = false;
	}
	os_mutex_unlock(&hmd->data_lock);

	struct xrt_vec3 eye_relation = *default_eye_relation;
	eye_relation.x = hmd->info.lens_horizontal_separation_meters;

	return u_device_get_view_poses(xdev, &eye_relation, at_timestamp_ns, view_type, view_count, out_head_relation,
	                               out_fovs, out_poses);
}

void
process_imu_record(struct psvr2_hmd *hmd, size_t index, struct imu_usb_record *in, timepoint_ns estimated_sample_time)
{
	struct imu_record imu_data;

	imu_data.vts_us = __le32_to_cpu(in->vts_us);
	for (int i = 0; i < 3; i++) {
		imu_data.accel[i] = __le16_to_cpu(in->accel[i]);
		imu_data.gyro[i] = __le16_to_cpu(in->gyro[i]);
	}
	imu_data.dp_frame_cnt = __le16_to_cpu(in->dp_frame_cnt);
	imu_data.dp_line_cnt = __le16_to_cpu(in->dp_line_cnt);
	imu_data.imu_ts_us = __le16_to_cpu(in->imu_ts_us);
	imu_data.status = __le16_to_cpu(in->status);

	PSVR2_TRACE(hmd,
	            "Record #%d: TS %u vts %u "
	            "accel { %d, %d, %d } gyro { %d, %d, %d } "
	            "dp_frame_cnt %u dp_line_cnt %u status %u",
	            (int)index, imu_data.imu_ts_us, imu_data.vts_us, imu_data.accel[0], imu_data.accel[1],
	            imu_data.accel[2], imu_data.gyro[0], imu_data.gyro[1], imu_data.gyro[2], imu_data.dp_frame_cnt,
	            imu_data.dp_line_cnt, imu_data.status);

	uint32_t last_imu_vts_us = hmd->last_imu_vts_us;
	uint32_t last_imu_ts = hmd->last_imu_ts;

	hmd->last_imu_vts_us = imu_data.vts_us; /* Last VTS timestamp */
	hmd->last_imu_ts = imu_data.imu_ts_us;

	hmd->last_gyro.x = -DEG_TO_RAD(imu_data.gyro[1] * GYRO_SCALE);
	hmd->last_gyro.y = DEG_TO_RAD(imu_data.gyro[2] * GYRO_SCALE);
	hmd->last_gyro.z = -DEG_TO_RAD(imu_data.gyro[0] * GYRO_SCALE);

	hmd->last_accel.x = -imu_data.accel[1] * ACCEL_SCALE;
	hmd->last_accel.y = imu_data.accel[2] * ACCEL_SCALE;
	hmd->last_accel.z = -imu_data.accel[0] * ACCEL_SCALE;

	// @note Overflow expected and fine, since this is an unsigned subtraction
	uint32_t imu_vts_delta_us = imu_data.vts_us - last_imu_vts_us;
	uint16_t imu_delta_us = imu_data.imu_ts_us - last_imu_ts;

	hmd->last_imu_vts_ns += (timepoint_ns)imu_vts_delta_us * U_TIME_1US_IN_NS;
	hmd->last_imu_ns += (timepoint_ns)imu_delta_us * U_TIME_1US_IN_NS;

	const timepoint_ns now_vts = hmd->last_imu_vts_ns;
	const timepoint_ns now_imu = hmd->last_imu_ns;

	m_clock_offset_a2b(IMU_FREQ, now_vts, estimated_sample_time, &hmd->hw2mono_vts);
	m_clock_offset_a2b(IMU_FREQ, now_imu, estimated_sample_time, &hmd->hw2mono_imu);

	if (hmd->timestamp_samples < TIMESTAMP_SAMPLES) {
		hmd->timestamp_samples++;
	}

	struct xrt_imu_sample sample = {
	    .timestamp_ns = hmd->last_imu_vts_ns,
	    .accel_m_s2 = {hmd->last_accel.x, hmd->last_accel.y, hmd->last_accel.z},
	    .gyro_rad_secs = {hmd->last_gyro.x, hmd->last_gyro.y, hmd->last_gyro.z},
	};

	m_ff_vec3_f32_push(hmd->ff_gyro, &hmd->last_gyro, sample.timestamp_ns);
}

static void
process_status_report(struct psvr2_hmd *hmd, uint8_t *buf, int bytes_read, timepoint_ns received_ns)
{
	struct status_record_hdr *hdr = (struct status_record_hdr *)buf;

	hmd->dprx_status = hdr->dprx_status;
	hmd->proximity_sensor = hdr->prox_sensor_flag;
	hmd->function_button = hdr->function_button;

	hmd->ipd_updated |= (hmd->ipd_mm != hdr->ipd_dial_mm);
	hmd->ipd_mm = hdr->ipd_dial_mm;

	size_t i = 0;
	uint8_t *cur = buf + sizeof(struct status_record_hdr);
	uint8_t *end = buf + bytes_read;
	size_t num_imu_samples = (size_t)(end - cur) / sizeof(struct imu_usb_record);
	while (cur < end && i < num_imu_samples) {
		struct imu_usb_record imu;
		memcpy(&imu, cur, sizeof(struct imu_usb_record));

		process_imu_record(hmd, i, &imu, received_ns - (num_imu_samples - 1 - i) * IMU_PERIOD_NS);

		cur += sizeof(struct imu_usb_record);
		i++;
	}
}

bool
psvr2_usb_xfer_continue(struct libusb_transfer *xfer, const char *type)
{
	struct psvr2_hmd *hmd = xfer->user_data;

	switch (xfer->status) {
	case LIBUSB_TRANSFER_OVERFLOW:
		PSVR2_ERROR(hmd, "%s xfer returned overflow!", type);
		/* Fall through */
	case LIBUSB_TRANSFER_ERROR:
	case LIBUSB_TRANSFER_TIMED_OUT:
	case LIBUSB_TRANSFER_CANCELLED:
	case LIBUSB_TRANSFER_STALL:
	case LIBUSB_TRANSFER_NO_DEVICE:
		os_thread_helper_lock(&hmd->usb_thread);
		hmd->usb_active_xfers--;
		os_thread_helper_signal_locked(&hmd->usb_thread);
		os_thread_helper_unlock(&hmd->usb_thread);
		PSVR2_TRACE(hmd, "%s xfer is aborting with status %d", type, xfer->status);
		return false;

	case LIBUSB_TRANSFER_COMPLETED: break;
	}

	return true;
}

static void LIBUSB_CALL
status_xfer_cb(struct libusb_transfer *xfer)
{
	DRV_TRACE_MARKER();

	if (!psvr2_usb_xfer_continue(xfer, "Status")) {
		return;
	}

	timepoint_ns received_ns = os_monotonic_get_ns();

	/* handle status packet */
	struct psvr2_hmd *hmd = xfer->user_data;
	os_mutex_lock(&hmd->data_lock);
	if ((size_t)xfer->actual_length >= sizeof(struct status_record_hdr)) {
		PSVR2_TRACE(hmd, "Status - %d bytes", xfer->actual_length);
		PSVR2_TRACE_HEX(hmd, xfer->buffer, xfer->actual_length);

		process_status_report(hmd, xfer->buffer, xfer->actual_length, received_ns);
	}

	libusb_submit_transfer(xfer);
	os_mutex_unlock(&hmd->data_lock);
}

static void LIBUSB_CALL
img_xfer_cb(struct libusb_transfer *xfer)
{
	DRV_TRACE_MARKER();

	if (!psvr2_usb_xfer_continue(xfer, "Camera frame")) {
		return;
	}

	struct psvr2_hmd *hmd = xfer->user_data;
	if (xfer->actual_length > 0) {
		PSVR2_TRACE(hmd, "Camera frame - %d bytes", xfer->actual_length);
		PSVR2_TRACE_HEX(hmd, xfer->buffer, MIN(256, xfer->actual_length));

		if (xfer->actual_length == USB_CAM_MODE10_XFER_SIZE) {
			for (int d = 0; d < 3; d++) {
				if (u_sink_debug_is_active(&hmd->debug_sinks[d])) {
					struct xrt_frame *xf = NULL;

					int w = 254, h = 508, stride = 256, offset, size_pp;
					if (d == 0) {
						offset = d;
						size_pp = 2;
						u_frame_create_one_off(XRT_FORMAT_L8, stride * 2, h, &xf);
					} else if (d == 1 || d == 2) {
						offset = (d == 1) ? 2 : 5;
						size_pp = 3;
						u_frame_create_one_off(XRT_FORMAT_R8G8B8, stride, h, &xf);
					}

					uint8_t *src = xfer->buffer + 256;
					uint8_t *dest = xf->data;
					for (int y = 0; y < h; y++) {
						int x;

						for (x = 0; x < w; x++) {
							for (int i = 0; i < size_pp; i++) {
								*dest++ = src[offset + i];
							}
							src += 8;
						}
						src += 16; /* Skip 16-bytes at the end of each line */
						           /* Skip output padding pixels */
						while (x++ < stride) {
							for (int i = 0; i < size_pp; i++) {
								*dest++ = 0;
							}
						}
					}
					xf->timestamp = os_monotonic_get_ns();
					u_sink_debug_push_frame(&hmd->debug_sinks[d], xf);
					xrt_frame_reference(&xf, NULL);
				}
			}
		} else if (xfer->actual_length == USB_CAM_MODE1_XFER_SIZE) {
			if (u_sink_debug_is_active(&hmd->debug_sinks[3])) {

				struct xrt_frame *xf = NULL;
				u_frame_create_one_off(XRT_FORMAT_L8, 1280, 640, &xf);

				uint8_t *src = xfer->buffer + 256;
				uint8_t *dest = xf->data;
				memcpy(dest, src, 640 * 1280);
				xf->timestamp = os_monotonic_get_ns();
				u_sink_debug_push_frame(&hmd->debug_sinks[3], xf);
				xrt_frame_reference(&xf, NULL);
			}
		}
	}

	os_mutex_lock(&hmd->data_lock);
	libusb_submit_transfer(xfer);
	os_mutex_unlock(&hmd->data_lock);
}

static void
process_slam_record(struct psvr2_hmd *hmd, uint8_t *buf, int bytes_read)
{
	struct slam_usb_record slam;
	assert(bytes_read >= (int)sizeof(struct slam_usb_record));
	memcpy(&slam, buf, sizeof(struct slam_usb_record));

	if (slam.unknown1 != 3) {
		PSVR2_TRACE(hmd, "SLAM - unknown1 field was not 3, it was %d", slam.unknown1);
	}
	// assert(usb_data->unknown1 == 3 || usb_data->unknown1 == 0);

	os_mutex_lock(&hmd->data_lock);

	const struct xrt_quat old_pose_orientation = hmd->last_slam_pose.orientation;

	uint32_t last_slam_vts_us = hmd->last_slam_vts_us;
	hmd->last_slam_vts_us = __le32_to_cpu(slam.vts_ts_us);
	timepoint_ns vts_ns = hmd->last_slam_vts_ns +=
	    (timepoint_ns)(hmd->last_slam_vts_us - last_slam_vts_us) * U_TIME_1US_IN_NS;

	//@todo: Manual axis correction should come from calibration somewhere I think
	hmd->last_slam_pose.position.x = __lef32_to_cpu(slam.pos[2]);
	hmd->last_slam_pose.position.y = __lef32_to_cpu(slam.pos[1]);
	hmd->last_slam_pose.position.z = -__lef32_to_cpu(slam.pos[0]);
	hmd->last_slam_pose.orientation.w = __lef32_to_cpu(slam.orient[0]);
	hmd->last_slam_pose.orientation.x = -__lef32_to_cpu(slam.orient[2]);
	hmd->last_slam_pose.orientation.y = -__lef32_to_cpu(slam.orient[1]);
	hmd->last_slam_pose.orientation.z = __lef32_to_cpu(slam.orient[3]);

	// Always choose nearest quaternion to last slam pose. This prevents the motion estimation from thinking the
	// device has rotated nearly 360 degrees when the SLAM tracker gives us the 2nd poled quaternion for the
	// orientation, since it does not appear to do any kind of continuity checking itself.
	if (math_quat_dot(&old_pose_orientation, &hmd->last_slam_pose.orientation) < 0.0f) {
		hmd->last_slam_pose.orientation.w = -hmd->last_slam_pose.orientation.w;
		hmd->last_slam_pose.orientation.x = -hmd->last_slam_pose.orientation.x;
		hmd->last_slam_pose.orientation.y = -hmd->last_slam_pose.orientation.y;
		hmd->last_slam_pose.orientation.z = -hmd->last_slam_pose.orientation.z;
	}

	struct xrt_pose tmp = hmd->slam_correction_pose;
	math_quat_normalize(&tmp.orientation);
	math_quat_rotate(&tmp.orientation, &hmd->last_slam_pose.orientation, &hmd->pose.orientation);
	hmd->pose.position = hmd->last_slam_pose.position;
	math_vec3_accum(&tmp.position, &hmd->pose.position);
	os_mutex_unlock(&hmd->data_lock);

	PSVR2_TRACE(hmd, "SLAM - %d leftover bytes", (int)sizeof(slam.remainder));
	PSVR2_TRACE_HEX(hmd, slam.remainder, sizeof(slam.remainder));

	struct xrt_pose_sample pose_sample = {
	    .timestamp_ns = vts_ns,
	    .pose = hmd->pose,
	};

	struct xrt_space_relation relation = {
	    .pose = pose_sample.pose,
	    .relation_flags = (enum xrt_space_relation_flags)(
	        XRT_SPACE_RELATION_ORIENTATION_VALID_BIT | XRT_SPACE_RELATION_POSITION_VALID_BIT |
	        XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT | XRT_SPACE_RELATION_POSITION_TRACKED_BIT),
	};

	m_relation_history_push_with_motion_estimation(hmd->slam_relation_history, &relation, pose_sample.timestamp_ns);
}

static void LIBUSB_CALL
slam_xfer_cb(struct libusb_transfer *xfer)
{
	DRV_TRACE_MARKER();

	if (!psvr2_usb_xfer_continue(xfer, "SLAM frame")) {
		return;
	}

	struct psvr2_hmd *hmd = xfer->user_data;
	if (xfer->actual_length == sizeof(struct slam_usb_record)) {
		process_slam_record(hmd, xfer->buffer, xfer->actual_length);
	}

	os_mutex_lock(&hmd->data_lock);
	libusb_submit_transfer(xfer);
	os_mutex_unlock(&hmd->data_lock);
}

static void LIBUSB_CALL
dump_xfer_cb(struct libusb_transfer *xfer)
{
	DRV_TRACE_MARKER();
	struct psvr2_hmd *hmd = xfer->user_data;
	const char *name = NULL;

	if (xfer == hmd->led_detector_xfer)
		name = "LED Detector";
	else if (xfer == hmd->relocalizer_xfer)
		name = "RP";
	else if (xfer == hmd->vd_xfer)
		name = "VD";
	assert(name != NULL);

	if (!psvr2_usb_xfer_continue(xfer, name)) {
		return;
	}

	PSVR2_TRACE(hmd, "%s xfer size %u", name, xfer->actual_length);
	PSVR2_TRACE_HEX(hmd, xfer->buffer, xfer->actual_length);

	os_mutex_lock(&hmd->data_lock);
	libusb_submit_transfer(xfer);
	os_mutex_unlock(&hmd->data_lock);
}

static void *
psvr2_usb_thread(void *ptr)
{
	U_TRACE_SET_THREAD_NAME("PSVR2: USB communication");

	struct psvr2_hmd *hmd = ptr;

	os_thread_helper_lock(&hmd->usb_thread);
	while (os_thread_helper_is_running_locked(&hmd->usb_thread) && !hmd->usb_complete) {
		os_thread_helper_unlock(&hmd->usb_thread);

		libusb_handle_events_completed(hmd->ctx, &hmd->usb_complete);

		os_thread_helper_lock(&hmd->usb_thread);
	}

	os_thread_helper_unlock(&hmd->usb_thread);

	// Shut down USB communication
	psvr2_usb_stop(hmd);

	libusb_handle_events(hmd->ctx);

	return NULL;
}

struct psvr2_interface_info
{
	int interface_no;
	int altmode;
	const char *name;
};

struct psvr2_interface_info interface_list[] = {
    {.interface_no = PSVR2_STATUS_INTERFACE, .altmode = 1, .name = "status"},
    {.interface_no = PSVR2_SLAM_INTERFACE, .altmode = 0, .name = "SLAM"},
    {.interface_no = PSVR2_GAZE_INTERFACE, .altmode = 0, .name = "Gaze"},
    {.interface_no = PSVR2_CAMERA_INTERFACE, .altmode = 0, .name = "Camera"},
    {.interface_no = PSVR2_LD_INTERFACE, .altmode = 0, .name = "LED Detector"},
    {.interface_no = PSVR2_RP_INTERFACE, .altmode = 0, .name = "Relocalizer"},
    {.interface_no = PSVR2_VD_INTERFACE, .altmode = 0, .name = "VD"},
};

static bool
psvr2_usb_open(struct psvr2_hmd *hmd, struct xrt_prober_device *xpdev)
{
	int res;

	res = libusb_init(&hmd->ctx);
	if (res < 0) {
		PSVR2_ERROR(hmd, "Failed to init USB");
		return false;
	}

	hmd->dev = libusb_open_device_with_vid_pid(hmd->ctx, xpdev->vendor_id, xpdev->product_id);
	if (hmd->dev == NULL) {
		PSVR2_ERROR(hmd, "Failed to open USB device");
		return false;
	}

	for (size_t i = 0; i < sizeof(interface_list) / sizeof(interface_list[0]); i++) {
		int intf_no = interface_list[i].interface_no;
		int altmode = interface_list[i].altmode;
		const char *name = interface_list[i].name;

		res = libusb_claim_interface(hmd->dev, intf_no);
		if (res < 0) {
			PSVR2_ERROR(hmd, "Failed to claim USB %s interface", name);
			return false;
		}
		res = libusb_set_interface_alt_setting(hmd->dev, intf_no, altmode);
		if (res < 0) {
			PSVR2_ERROR(hmd, "Failed to set USB %s interface alt %d", name, altmode);
			return false;
		}
	}

	return true;
}

bool
get_psvr2_control(struct psvr2_hmd *hmd, uint16_t report_id, uint8_t subcmd, uint8_t *out_data, uint32_t buf_size)
{
	struct sie_ctrl_pkt pkt = {0};
	int ret;

	assert(buf_size <= sizeof(pkt.data));

	pkt.report_id = __cpu_to_le16(report_id);
	pkt.subcmd = __cpu_to_le16(subcmd);
	pkt.len = __cpu_to_le32(buf_size);

	ret = libusb_control_transfer(hmd->dev, LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_ENDPOINT | 0x80, 0x1,
	                              report_id, 0x0, (unsigned char *)&pkt, buf_size + 8, 100);
	if (ret < 0) {
		PSVR2_ERROR(hmd, "Failed to get report id %u subcmd %u, reason %d", report_id, subcmd, ret);
		return false;
	}

	memcpy(out_data, pkt.data, buf_size);

	return true;
}

bool
send_psvr2_control(struct psvr2_hmd *hmd, uint16_t report_id, uint8_t subcmd, uint8_t *pkt_data, uint32_t pkt_len)
{
	struct sie_ctrl_pkt pkt;
	int ret;

	assert(pkt_len <= sizeof(pkt.data));

	pkt.report_id = __cpu_to_le16(report_id);
	pkt.subcmd = __cpu_to_le16(subcmd);
	pkt.len = __cpu_to_le32(pkt_len);
	memcpy(pkt.data, pkt_data, pkt_len);

	ret = libusb_control_transfer(hmd->dev, LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_ENDPOINT, 0x9, report_id,
	                              0x0, (unsigned char *)&pkt, pkt_len + 8, 100);
	if (ret < 0) {
		PSVR2_ERROR(hmd, "Failed to send report id %u subcmd %u", report_id, subcmd);
		return false;
	}

	return true;
}

bool
set_camera_mode(struct psvr2_hmd *hmd, enum psvr2_camera_mode mode)
{
	struct camera_cmd
	{
		__le32 data[2];
	} cmd;

	cmd.data[0] = __cpu_to_le32(0x1);
	cmd.data[1] = __cpu_to_le32(mode);

	PSVR2_DEBUG(hmd, "Setting camera mode to 0x%x", mode);

	return send_psvr2_control(hmd, PSVR2_REPORT_ID_SET_CAMERA_MODE, 0x1, (uint8_t *)(&cmd), sizeof(cmd));
}

static void
toggle_camera_enable(struct psvr2_hmd *hmd)
{
	hmd->camera_enable = !hmd->camera_enable;

	struct u_var_button *btn = &hmd->camera_enable_btn;
	snprintf(btn->label, sizeof(btn->label),
	         hmd->camera_enable ? "Disable camera streams" : "Enable camera streams");

	if (hmd->camera_enable) {
		set_camera_mode(hmd, hmd->camera_mode);
	} else {
		set_camera_mode(hmd, PSVR2_CAMERA_MODE_OFF);
	}
}

bool
set_brightness(struct psvr2_hmd *hmd, float brightness)
{
	uint8_t brightness_byte = CLAMP(brightness * 31, 0, 31);

	return send_psvr2_control(hmd, PSVR2_REPORT_ID_SET_BRIGHTNESS, 1, &brightness_byte, sizeof(brightness_byte));
}

bool
get_firmware_info(struct psvr2_hmd *hmd, struct pkt_firmware_info *out_firmware_info)
{
	if (!get_psvr2_control(hmd, 0x81, 0x1, (uint8_t *)out_firmware_info, sizeof(*out_firmware_info))) {
		PSVR2_ERROR(hmd, "Failed to get device firmware info packet.");
		return false;
	}

	return true;
}

static xrt_result_t
psvr2_get_brightness(struct xrt_device *xdev, float *brightness)
{
	struct psvr2_hmd *hmd = psvr2_hmd(xdev);

	os_mutex_lock(&hmd->data_lock);
	*brightness = hmd->brightness;
	os_mutex_unlock(&hmd->data_lock);

	return XRT_SUCCESS;
}

static xrt_result_t
psvr2_set_brightness(struct xrt_device *xdev, float brightness, bool relative)
{
	struct psvr2_hmd *hmd = psvr2_hmd(xdev);

	// Handle relative brightness adjustment
	brightness = relative ? hmd->brightness + brightness : brightness;

	if (!set_brightness(hmd, brightness)) {
		PSVR2_ERROR(hmd, "Failed to set brightness to %.2f", brightness);
		return XRT_ERROR_OUTPUT_REQUEST_FAILURE;
	}

	os_mutex_lock(&hmd->data_lock);
	hmd->brightness = brightness;
	os_mutex_unlock(&hmd->data_lock);

	return XRT_SUCCESS;
}

static xrt_result_t
psvr2_hmd_set_output(struct xrt_device *xdev, enum xrt_output_name name, const struct xrt_output_value *value)
{
	switch (name) {
	case XRT_OUTPUT_NAME_PSVR2_HAPTIC: {
		struct xrt_output_value_vibration vibration = value->vibration;
		(void)vibration;

		// @todo: Implement headset haptics.

		break;
	}
	default: return XRT_ERROR_OUTPUT_UNSUPPORTED;
	}

	return XRT_SUCCESS;
}

static xrt_result_t
psvr2_hmd_get_compositor_info(struct xrt_device *xdev,
                              const struct xrt_device_compositor_mode *mode,
                              struct xrt_device_compositor_info *out_info)
{
	// @note The scanout duration is the same for both 90hz and 120hz VR modes
	const double scanout_duration = 2040.0 / 2200.0;

	*out_info = (struct xrt_device_compositor_info){
	    .panel_refresh_type = XRT_PANEL_REFRESH_TYPE_ROLLING,
	    .scanout_direction = XRT_SCANOUT_DIRECTION_TOP_TO_BOTTOM,
	    .scanout_time_ns = mode->frame_interval_ns * scanout_duration,
	};

	return XRT_SUCCESS;
}

static void
cycle_camera_mode(struct psvr2_hmd *hmd)
{
	struct u_var_button *btn = &hmd->camera_mode_btn;

	switch (hmd->camera_mode) {
	default:
		hmd->camera_mode++;
		snprintf(btn->label, sizeof(btn->label), "Camera Mode 0x%x", hmd->camera_mode);
		break;
	case PSVR2_CAMERA_MODE_BOTTOM_SBS_BC4:
		hmd->camera_mode = PSVR2_CAMERA_MODE_BOTTOM_SBS_CROPPED;
		snprintf(btn->label, sizeof(btn->label), "Camera Mode 0x1");
		break;
	}

	if (hmd->camera_enable) {
		set_camera_mode(hmd, hmd->camera_mode);
	} else {
		set_camera_mode(hmd, PSVR2_CAMERA_MODE_OFF);
	}
}


static bool
psvr2_usb_start(struct psvr2_hmd *hmd)
{
	bool result = false;
	int res;

	os_thread_helper_lock(&hmd->usb_thread);

	/* Status endpoint */
	hmd->status_xfer = libusb_alloc_transfer(0);
	if (hmd->status_xfer == NULL) {
		PSVR2_ERROR(hmd, "Could not alloc USB transfer for status reports");
		goto out;
	}
	uint8_t *status_buf = malloc(USB_STATUS_XFER_SIZE);
	libusb_fill_interrupt_transfer(hmd->status_xfer, hmd->dev, LIBUSB_ENDPOINT_IN | PSVR2_STATUS_ENDPOINT,
	                               status_buf, USB_STATUS_XFER_SIZE, status_xfer_cb, hmd, 0);
	hmd->status_xfer->flags |= LIBUSB_TRANSFER_FREE_BUFFER;

	res = libusb_submit_transfer(hmd->status_xfer);
	if (res < 0) {
		PSVR2_ERROR(hmd, "Could not submit USB transfer for status reports");
		goto out;
	}
	hmd->usb_active_xfers++;

	/* Camera data */
	hmd->camera_enable = true;
	hmd->camera_mode = PSVR2_CAMERA_MODE_10;
	set_camera_mode(hmd, hmd->camera_mode);

	for (int i = 0; i < NUM_CAM_XFERS; i++) {
		hmd->camera_xfers[i] = libusb_alloc_transfer(0);
		if (hmd->camera_xfers[i] == NULL) {
			PSVR2_ERROR(hmd, "Could not alloc USB transfer %d for camera data", i);
			goto out;
		}

		uint8_t *recv_buf = malloc(USB_CAM_MODE10_XFER_SIZE);

		libusb_fill_bulk_transfer(hmd->camera_xfers[i], hmd->dev, LIBUSB_ENDPOINT_IN | PSVR2_CAMERA_ENDPOINT,
		                          recv_buf, USB_CAM_MODE10_XFER_SIZE, img_xfer_cb, hmd, 0);
		hmd->camera_xfers[i]->flags |= LIBUSB_TRANSFER_FREE_BUFFER;

		res = libusb_submit_transfer(hmd->camera_xfers[i]);
		if (res < 0) {
			PSVR2_ERROR(hmd, "Could not submit USB transfer %d for camera data", i);
			goto out;
		}
		hmd->usb_active_xfers++;
	}

	/* SLAM endpoint */
	hmd->slam_xfer = libusb_alloc_transfer(0);
	if (hmd->slam_xfer == NULL) {
		PSVR2_ERROR(hmd, "Could not alloc USB transfer for SLAM data");
		goto out;
	}
	uint8_t *slam_buf = malloc(USB_SLAM_XFER_SIZE);
	libusb_fill_bulk_transfer(hmd->slam_xfer, hmd->dev, LIBUSB_ENDPOINT_IN | PSVR2_SLAM_ENDPOINT, slam_buf,
	                          USB_SLAM_XFER_SIZE, slam_xfer_cb, hmd, 0);
	hmd->slam_xfer->flags |= LIBUSB_TRANSFER_FREE_BUFFER;

	res = libusb_submit_transfer(hmd->slam_xfer);
	if (res < 0) {
		PSVR2_ERROR(hmd, "Could not submit USB transfer for SLAM data");
		goto out;
	}
	hmd->usb_active_xfers++;

	/* LD endpoint */
	hmd->led_detector_xfer = libusb_alloc_transfer(0);
	if (hmd->led_detector_xfer == NULL) {
		PSVR2_ERROR(hmd, "Could not alloc USB transfer for LED Detector data");
		goto out;
	}
	uint8_t *led_detector_buf = malloc(USB_LD_XFER_SIZE);
	libusb_fill_bulk_transfer(hmd->led_detector_xfer, hmd->dev, LIBUSB_ENDPOINT_IN | PSVR2_LD_ENDPOINT,
	                          led_detector_buf, USB_LD_XFER_SIZE, dump_xfer_cb, hmd, 0);
	hmd->led_detector_xfer->flags |= LIBUSB_TRANSFER_FREE_BUFFER;

	res = libusb_submit_transfer(hmd->led_detector_xfer);
	if (res < 0) {
		PSVR2_ERROR(hmd, "Could not submit USB transfer for LED Detector data");
		goto out;
	}
	hmd->usb_active_xfers++;

	/* RP endpoint */
	hmd->relocalizer_xfer = libusb_alloc_transfer(0);
	if (hmd->relocalizer_xfer == NULL) {
		PSVR2_ERROR(hmd, "Could not alloc USB transfer for RP data");
		goto out;
	}
	uint8_t *relocalizer_buf = malloc(USB_RP_XFER_SIZE);
	libusb_fill_bulk_transfer(hmd->relocalizer_xfer, hmd->dev, LIBUSB_ENDPOINT_IN | PSVR2_RP_ENDPOINT,
	                          relocalizer_buf, USB_RP_XFER_SIZE, dump_xfer_cb, hmd, 0);
	hmd->relocalizer_xfer->flags |= LIBUSB_TRANSFER_FREE_BUFFER;

	res = libusb_submit_transfer(hmd->relocalizer_xfer);
	if (res < 0) {
		PSVR2_ERROR(hmd, "Could not submit USB transfer for RP data");
		goto out;
	}
	hmd->usb_active_xfers++;

	/* VD endpoint */
	hmd->vd_xfer = libusb_alloc_transfer(0);
	if (hmd->vd_xfer == NULL) {
		PSVR2_ERROR(hmd, "Could not alloc USB transfer for VD data");
		goto out;
	}
	uint8_t *vd_buf = malloc(USB_VD_XFER_SIZE);
	libusb_fill_bulk_transfer(hmd->vd_xfer, hmd->dev, LIBUSB_ENDPOINT_IN | PSVR2_VD_ENDPOINT, vd_buf,
	                          USB_VD_XFER_SIZE, dump_xfer_cb, hmd, 0);
	hmd->vd_xfer->flags |= LIBUSB_TRANSFER_FREE_BUFFER;

	res = libusb_submit_transfer(hmd->vd_xfer);
	if (res < 0) {
		PSVR2_ERROR(hmd, "Could not submit USB transfer for VD data");
		goto out;
	}
	hmd->usb_active_xfers++;

	res = psvr2_start_gaze_tracking(hmd);
	if (res < 0) {
		PSVR2_ERROR(hmd, "Could not start gaze tracking");
		goto out;
	}

	result = true;

out:
	os_thread_helper_unlock(&hmd->usb_thread);
	return result;
}

static void
set_slam_correction(struct psvr2_hmd *hmd)
{
	os_mutex_lock(&hmd->data_lock);
	math_pose_invert(&hmd->last_slam_pose, &hmd->slam_correction_pose);
	os_mutex_unlock(&hmd->data_lock);
}

static void
reset_slam_correction(struct psvr2_hmd *hmd)
{
	os_mutex_lock(&hmd->data_lock);
	hmd->slam_correction_pose = (struct xrt_pose)SLAM_POSE_CORRECTION;
	os_mutex_unlock(&hmd->data_lock);
}

static void
update_brightness(struct psvr2_hmd *hmd)
{
	(void)set_brightness(hmd, hmd->brightness);
}

#define TRANSFERS_LIST(_, hmd)                                                                                         \
	for (int i = 0; i < NUM_CAM_XFERS; i++) {                                                                      \
		_(hmd->camera_xfers[i])                                                                                \
	}                                                                                                              \
	_(hmd->status_xfer)                                                                                            \
	_(hmd->slam_xfer)                                                                                              \
	_(hmd->led_detector_xfer)                                                                                      \
	_(hmd->relocalizer_xfer)                                                                                       \
	_(hmd->vd_xfer)                                                                                                \
	_(hmd->gaze_xfer)

static void
psvr2_usb_stop(struct psvr2_hmd *hmd)
{
#define X(xfer)                                                                                                        \
	if (xfer) {                                                                                                    \
		if (libusb_cancel_transfer(xfer) != 0) {                                                               \
			PSVR2_ERROR(hmd, "failed to cancel transfer");                                                 \
		}                                                                                                      \
	}

	os_mutex_lock(&hmd->data_lock);
	TRANSFERS_LIST(X, hmd);
	os_mutex_unlock(&hmd->data_lock);

#undef X
}

void
psvr2_usb_destroy(struct psvr2_hmd *hmd)
{
#define X(xfer)                                                                                                        \
	if (xfer) {                                                                                                    \
		libusb_free_transfer(xfer);                                                                            \
		xfer = NULL;                                                                                           \
	}

	TRANSFERS_LIST(X, hmd);

#undef X
}

struct distortion_calibration_block
{
	uint8_t version_unk;
	uint8_t unk[7];
	float distortion_params[32];
};

static void
psvr2_setup_distortion_and_fovs(struct psvr2_hmd *hmd)
{
	/* Each eye has an X offset, a Y offset, and two scale
	 * factors (the main scale factor, and another that
	 * allows for tilting the view, set to 0 for no tilt).
	 * It seems to be stored like this:
	 * struct calibration_t {
	 *	float offsetx_left;
	 *	float offsety_left;
	 *	float offsetx_right;
	 *	float offsety_right;
	 *	float scale1_left;
	 *	float scale2_left;
	 *	float scale1_right;
	 *	float scale2_right;
	 * } calibration;
	 * */
	struct distortion_calibration_block calibration_block;

	uint8_t buf[0x100];
	get_psvr2_control(hmd, 0x8f, 1, buf, sizeof buf);
	memcpy(&calibration_block, buf, sizeof calibration_block);

	memset(hmd->distortion_calibration, 0, sizeof(hmd->distortion_calibration));
	if (calibration_block.version_unk < 4) {
		hmd->distortion_calibration[0] = -0.09919293;
		hmd->distortion_calibration[2] = 0.09919293;
	} else {
		float *p = calibration_block.distortion_params;

		hmd->distortion_calibration[0] = (((-p[0] - p[6]) * 29.9 + 14.95) / 1000.0 - 3.22) / 32.46199;
		hmd->distortion_calibration[1] = (((-p[1] * 29.9) + 14.95) / 1000.0) / 32.46199;

		hmd->distortion_calibration[2] = (((p[6] - p[2]) * 29.9 + 14.95) / 1000.0 + 3.22) / 32.46199;
		hmd->distortion_calibration[3] = (((-p[3] * 29.9) + 14.95) / 1000.0) / 32.46199;

		float left = -p[4] * M_PI / 180.0;
		hmd->distortion_calibration[4] = cosf(left);
		hmd->distortion_calibration[5] = sinf(left);

		float right = -p[5] * M_PI / 180.0;
		hmd->distortion_calibration[6] = cosf(right);
		hmd->distortion_calibration[7] = sinf(right);
	}

	struct xrt_fov *fovs = hmd->base.hmd->distortion.fov;
	fovs[0].angle_up = 53.0f * (M_PI / 180.0f);
	fovs[0].angle_down = -53.0f * (M_PI / 180.0f);
	fovs[0].angle_left = -61.5f * (M_PI / 180.0f);
	fovs[0].angle_right = 43.5f * (M_PI / 180.0f);

	fovs[1].angle_up = fovs[0].angle_up;
	fovs[1].angle_down = fovs[0].angle_down;
	fovs[1].angle_left = -fovs[0].angle_right;
	fovs[1].angle_right = -fovs[0].angle_left;
}

static xrt_result_t
psvr2_begin_feature(struct xrt_device *xdev, enum xrt_device_feature_type type)
{
	struct psvr2_hmd *hmd = psvr2_hmd(xdev);

	switch (type) {
	case XRT_DEVICE_FEATURE_EYE_TRACKING: hmd->eye_feature_enabled = true; break;
	case XRT_DEVICE_FEATURE_FACE_TRACKING: hmd->face_feature_enabled = true; break;
	default: return XRT_ERROR_FEATURE_NOT_SUPPORTED;
	}

	if (hmd->eye_feature_enabled || hmd->face_feature_enabled) {
		hmd->et_data.want_enabled = true;
	}

	return XRT_SUCCESS;
}

static xrt_result_t
psvr2_end_feature(struct xrt_device *xdev, enum xrt_device_feature_type type)
{
	struct psvr2_hmd *hmd = psvr2_hmd(xdev);

	switch (type) {
	case XRT_DEVICE_FEATURE_EYE_TRACKING: hmd->eye_feature_enabled = false; break;
	case XRT_DEVICE_FEATURE_FACE_TRACKING: hmd->face_feature_enabled = false; break;
	default: return XRT_ERROR_FEATURE_NOT_SUPPORTED;
	}

	if (!hmd->eye_feature_enabled && !hmd->face_feature_enabled) {
		hmd->et_data.want_enabled = false;
	}

	return XRT_SUCCESS;
}

struct xrt_device *
psvr2_hmd_create(struct xrt_prober_device *xpdev)
{
	DRV_TRACE_MARKER();

	// This indicates you won't be using Monado's built-in tracking algorithms.
	enum u_device_alloc_flags flags =
	    (enum u_device_alloc_flags)(U_DEVICE_ALLOC_HMD | U_DEVICE_ALLOC_TRACKING_NONE);

	struct psvr2_hmd *hmd = U_DEVICE_ALLOCATE(struct psvr2_hmd, flags, PSVR2_HMD_INPUT_COUNT, 1);

	snprintf(hmd->base.tracking_origin->name, XRT_TRACKING_NAME_LEN, "PS VR2 Tracking");
	hmd->base.tracking_origin->type = XRT_TRACKING_TYPE_EXTERNAL_SLAM;
	hmd->base.tracking_origin->initial_offset = (struct xrt_pose)XRT_POSE_IDENTITY;

	if (os_mutex_init(&hmd->data_lock) != 0) {
		PSVR2_ERROR(hmd, "Failed to init data mutex!");
		goto cleanup;
	}

	if (os_thread_helper_init(&hmd->usb_thread) != 0) {
		PSVR2_ERROR(hmd, "Failed to initialise threading");
		goto cleanup;
	}

	m_relation_history_create(&hmd->slam_relation_history);

	if (!psvr2_usb_open(hmd, xpdev)) {
		goto cleanup;
	}

	// This list should be ordered, most preferred first.
	size_t idx = 0;
	hmd->base.hmd->blend_modes[idx++] = XRT_BLEND_MODE_OPAQUE;
	hmd->base.hmd->blend_mode_count = idx;

	u_device_populate_function_pointers(&hmd->base, psvr2_hmd_get_tracked_pose, psvr2_hmd_destroy);

	hmd->base.update_inputs = psvr2_hmd_update_inputs;
	hmd->base.get_view_poses = psvr2_hmd_get_view_poses;
	hmd->base.get_brightness = psvr2_get_brightness;
	hmd->base.set_brightness = psvr2_set_brightness;
	hmd->base.set_output = psvr2_hmd_set_output;
	hmd->base.get_compositor_info = psvr2_hmd_get_compositor_info;
	hmd->base.begin_feature = psvr2_begin_feature;
	hmd->base.end_feature = psvr2_end_feature;
	hmd->base.get_face_tracking = psvr2_get_face_tracking;

	hmd->pose = (struct xrt_pose)XRT_POSE_IDENTITY;
	hmd->log_level = debug_get_log_option_psvr2_log();
	hmd->T_imu_head = (struct xrt_pose){
	    .position = {.x = 0.000247f, .y = -0.000273f, .z = 0.104826f},
	    .orientation = XRT_QUAT_IDENTITY,
	};

	m_ff_vec3_f32_alloc(&hmd->ff_gyro, 1024);

	// Print name.
	snprintf(hmd->base.str, XRT_DEVICE_NAME_LEN, "PS VR2 HMD");
	snprintf(hmd->base.serial, XRT_DEVICE_NAME_LEN, "PS VR2 HMD S/N");

	// Setup input.
	hmd->base.name = XRT_DEVICE_PSVR2;
	hmd->base.device_type = XRT_DEVICE_TYPE_HMD;
	hmd->base.inputs[PSVR2_HMD_INPUT_HEAD_POSE].name = XRT_INPUT_GENERIC_HEAD_POSE;
	hmd->base.inputs[PSVR2_HMD_INPUT_HEAD_DETECT].name = XRT_INPUT_GENERIC_HEAD_DETECT;
	hmd->base.inputs[PSVR2_HMD_INPUT_FUNCTION_BUTTON].name = XRT_INPUT_PSVR2_SYSTEM_CLICK;
	hmd->base.inputs[PSVR2_HMD_INPUT_EYE_GAZE_POSE].name = XRT_INPUT_GENERIC_EYE_GAZE_POSE;
	hmd->base.inputs[PSVR2_HMD_INPUT_FB_FACE_TRACKING2_VISUAL].name = XRT_INPUT_FB_FACE_TRACKING2_VISUAL;
	hmd->base.inputs[PSVR2_HMD_INPUT_HTC_EYE_FACE_TRACKING].name = XRT_INPUT_HTC_EYE_FACE_TRACKING;
	hmd->base.inputs[PSVR2_HMD_INPUT_ANDROID_FACE_TRACKING].name = XRT_INPUT_ANDROID_FACE_TRACKING;

	hmd->base.outputs[0].name = XRT_OUTPUT_NAME_PSVR2_HAPTIC;

	hmd->base.supported.orientation_tracking = true;
	hmd->base.supported.position_tracking = true;
	hmd->base.supported.presence = true;
	hmd->base.supported.brightness_control = true;
	hmd->base.supported.compositor_info = true;
	hmd->base.supported.eye_gaze = true;
	hmd->base.supported.face_tracking = true;

	// Set up display details
	// refresh rate
	hmd->base.hmd->screens[0].nominal_frame_interval_ns = time_s_to_ns(1.0f / 120.0f);
	hmd->base.compute_distortion = psvr2_compute_distortion;

	struct xrt_hmd_parts *parts = hmd->base.hmd;
	parts->distortion.models = XRT_DISTORTION_MODEL_COMPUTE;
	parts->distortion.preferred = XRT_DISTORTION_MODEL_COMPUTE;

	// This default matches the default lens separation
	hmd->ipd_mm = 65;

	hmd->info.display.w_pixels = 4000;
	hmd->info.display.h_pixels = 2040;
	hmd->info.display.w_meters = 0.13f;
	hmd->info.display.h_meters = 0.07f;
	hmd->info.lens_horizontal_separation_meters = 0.13f / 2.0f;
	hmd->info.lens_vertical_position_meters = 0.07f / 2.0f;
	// These need to be set to avoid an error, but the fovs
	// computed further down are preferred.
	hmd->info.fov[0] = (float)(106.0 * (M_PI / 180.0));
	hmd->info.fov[1] = (float)(106.0 * (M_PI / 180.0));

	if (!u_device_setup_split_side_by_side(&hmd->base, &hmd->info)) {
		PSVR2_ERROR(hmd, "Failed to setup basic device info");
		goto cleanup;
	}

	psvr2_setup_distortion_and_fovs(hmd);

	u_distortion_mesh_fill_in_compute(&hmd->base);

	const struct xrt_pose slam_correction_pose = SLAM_POSE_CORRECTION;
	hmd->slam_correction_pose = slam_correction_pose;

	for (int i = 0; i < 4; i++) {
		u_sink_debug_init(&hmd->debug_sinks[i]);
	}

	u_var_add_root(hmd, "PS VR2 HMD", true);
	u_var_add_pose(hmd, &hmd->pose, "pose");
	u_var_add_pose(hmd, &hmd->slam_correction_pose, "SLAM correction pose");
	{
		hmd->slam_correction_set_btn.cb = (void (*)(void *))set_slam_correction;
		hmd->slam_correction_set_btn.ptr = hmd;
		u_var_add_button(hmd, &hmd->slam_correction_set_btn, "Set");
	}
	{
		hmd->slam_correction_reset_btn.cb = (void (*)(void *))reset_slam_correction;
		hmd->slam_correction_reset_btn.ptr = hmd;
		u_var_add_button(hmd, &hmd->slam_correction_reset_btn, "Reset");
	}

	u_var_add_gui_header(hmd, NULL, "Last IMU data");
	u_var_add_ro_u32(hmd, &hmd->last_imu_vts_us, "VTS Timestamp");
	u_var_add_ro_i64_ns(hmd, &hmd->last_imu_vts_ns, "VTS Timestamp (ns)");
	u_var_add_ro_i64_ns(hmd, &hmd->hw2mono_vts, "hw2mono_vts");
	u_var_add_u16(hmd, &hmd->last_imu_ts, "Timestamp");
	u_var_add_ro_vec3_f32(hmd, &hmd->last_accel, "accel");
	u_var_add_ro_vec3_f32(hmd, &hmd->last_gyro, "gyro");

	u_var_add_gui_header(hmd, NULL, "Last SLAM data");
	u_var_add_ro_u32(hmd, &hmd->last_slam_vts_us, "VTS Timestamp");
	u_var_add_ro_i64_ns(hmd, &hmd->last_slam_vts_ns, "VTS Timestamp (ns)");
	u_var_add_pose(hmd, &hmd->last_slam_pose, "Pose");

	u_var_add_gui_header(hmd, NULL, "Status");
	u_var_add_u8(hmd, &hmd->dprx_status, "HMD Display Port RX status");
	u_var_add_ro_i32(hmd, (int32_t *)&hmd->proximity_sensor, "HMD Proximity");
	u_var_add_bool(hmd, &hmd->function_button, "HMD Function button");
	u_var_add_u8(hmd, &hmd->ipd_mm, "HMD IPD (mm)");

	u_var_add_f32(hmd, &hmd->brightness, "Brightness");
	hmd->brightness_btn.cb = (void (*)(void *))update_brightness;
	hmd->brightness_btn.ptr = hmd;
	u_var_add_button(hmd, &hmd->brightness_btn, "Set Brightness");

	u_var_add_gui_header(hmd, NULL, "Camera data");
	{
		hmd->camera_enable_btn.cb = (void (*)(void *))toggle_camera_enable;
		hmd->camera_enable_btn.ptr = hmd;
		u_var_add_button(hmd, &hmd->camera_enable_btn, "Disable camera streams");

		hmd->camera_mode_btn.cb = (void (*)(void *))cycle_camera_mode;
		hmd->camera_mode_btn.ptr = hmd;
		u_var_add_button(hmd, &hmd->camera_mode_btn, "Camera Mode 0x10");
	}
	for (int i = 0; i < 3; i++) {
		char name[32];
		sprintf(name, "Substream %d", i);
		u_var_add_sink_debug(hmd, &hmd->debug_sinks[i], name);
	}
	u_var_add_sink_debug(hmd, &hmd->debug_sinks[3], "Mode 1 stream");

	u_var_add_gui_header(hmd, NULL, "Logging");
	u_var_add_log_level(hmd, &hmd->log_level, "log_level");

	float initial_brightness = debug_get_float_option_psvr2_default_brightness();
	if (!set_brightness(hmd, initial_brightness)) {
		PSVR2_WARN(hmd, "Failed to set initial brightness");
	}
	hmd->brightness = initial_brightness;

	struct pkt_firmware_info firmware_info;
	if (get_firmware_info(hmd, &firmware_info)) {
		snprintf(hmd->base.serial, XRT_DEVICE_NAME_LEN, "%.*s", (int)sizeof(firmware_info.pcb_id),
		         firmware_info.pcb_id);

		PSVR2_INFO(hmd, "Headset has firmware version of %08x", __le32_to_cpu(firmware_info.version));
	} else {
		PSVR2_WARN(hmd, "Failed to get device firmware info, serial number will not be correct.");
	}

	// Start USB communications
	hmd->usb_complete = 0;
	if (os_thread_helper_start(&hmd->usb_thread, psvr2_usb_thread, hmd) != 0) {
		PSVR2_ERROR(hmd, "Failed to start USB thread");
		goto cleanup;
	}

	if (!psvr2_usb_start(hmd)) {
		PSVR2_ERROR(hmd, "Failed to submit USB transfers");
		goto cleanup;
	}

	return &hmd->base;

cleanup:
	psvr2_hmd_destroy(&hmd->base);
	return NULL;
}
