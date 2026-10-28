// Copyright 2025, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  PSVR2 HMD eye tracking implementation
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_psvr2
 */

#include "math/m_space.h"

#include "util/u_trace_marker.h"
#include "util/u_file.h"
#include "util/u_linux.h"

#include "psvr2.h"


static void
process_gaze_packet(struct psvr2_hmd *hmd, uint8_t *buf, size_t bytes_read)
{
	struct pkt_gaze_state gaze_state;

	if (bytes_read < sizeof(gaze_state)) {
		PSVR2_WARN(hmd, "Gaze packet too small: %zu bytes", bytes_read);
		return;
	}

	memcpy(&gaze_state, buf, sizeof(gaze_state));

	if (memcmp(gaze_state.header, "GS", 2) != 0) {
		PSVR2_WARN(hmd, "Got gaze with bad header %x%x", gaze_state.header[0], gaze_state.header[1]);
		return;
	}

	hmd->et_data.processed_sample_packet = true;

	uint32_t remote_sample_timestamp_us = __le32_to_cpu(gaze_state.packet_data.combined.timestamp);

	// wrap-around intentional and A-OK, given these are unsigned
	uint32_t remote_sample_timestamp_delta_us =
	    remote_sample_timestamp_us - hmd->et_data.last_remote_report_sample_time_us;

	hmd->et_data.last_remote_report_sample_time_us = remote_sample_timestamp_us;

	timepoint_ns last_timestamp_ns = hmd->et_data.last_remote_report_sample_time_ns;
	timepoint_ns timestamp_ns = hmd->et_data.last_remote_report_sample_time_ns +
	                            ((int64_t)remote_sample_timestamp_delta_us * U_TIME_1US_IN_NS);
	hmd->et_data.last_remote_report_sample_time_ns = timestamp_ns;

	os_mutex_lock(&hmd->et_data.data_mutex);

	for (int i = 0; i < 2; i++) {
		struct pkt_eye_gaze *eye_gaze_data =
		    i == 0 ? &gaze_state.packet_data.left : &gaze_state.packet_data.right;

		struct xrt_vec3 gaze_point = __levec3_to_cpu(eye_gaze_data->gaze_point_mm);
		math_vec3_scalar_mul(1.0 / 1000.0, &gaze_point); // to meters
		gaze_point.x *= -1;
		gaze_point.z *= -1;

		struct xrt_vec3 gaze_direction = __levec3_to_cpu(eye_gaze_data->gaze_direction);
		// flip to correct coordinate space
		gaze_direction.x *= -1;
		gaze_direction.z *= -1;

		struct psvr2_et_eye_data *eye_data = &hmd->et_data.eyes[i];

		if (eye_gaze_data->blink_valid && eye_data->blink != eye_data->blink_interp) {
			// amount of time needed to blink, this is technically higher what it should be (100ms on the
			// low end for *whole* blink, closed and open, and this value is for reaching one of those
			// extremes), but we want this to feel "smoothed" out for users, since we only get binary blink
			// data from HMD, and apps won't necessarily smooth it out for us, and this number "feels good"
			const timepoint_ns blink_time_ns = U_TIME_1MS_IN_NS * 100LLU;

			// amount of blink movement occurred since last tick
			double blink_delta = (double)(timestamp_ns - last_timestamp_ns) / (double)blink_time_ns;

			// direction interp is moving
			float dir = eye_gaze_data->blink ? 1 : -1;

			eye_data->blink_interp += dir * blink_delta;
			eye_data->blink_interp = CLAMP(eye_data->blink_interp, 0, 1);
		}

		if (eye_gaze_data->gaze_direction_valid) {
			m_filter_euro_vec3_run(&eye_data->gaze_direction_filter, timestamp_ns, &gaze_direction,
			                       &eye_data->filtered_gaze_direction);
			math_vec3_normalize(&eye_data->filtered_gaze_direction);
		}

		eye_data->blink = eye_gaze_data->blink;
		eye_data->blink_valid = eye_gaze_data->blink_valid;
		eye_data->gaze_direction = gaze_direction;
		eye_data->gaze_direction_valid = eye_gaze_data->gaze_direction_valid;
		eye_data->gaze_point = gaze_point;
		eye_data->gaze_point_valid = eye_gaze_data->gaze_point_mm_valid;
		eye_data->pupil_diameter = __lef32_to_cpu(eye_gaze_data->pupil_diameter_mm) / 1000.0; // to m
		eye_data->pupil_diameter_valid = eye_gaze_data->pupil_diameter_valid;
		eye_data->unk_float_2_valid = eye_gaze_data->unk_bool_2;
		eye_data->unk_float_2 = __levec2_to_cpu(eye_gaze_data->unk_float_2);
		eye_data->unk_float_4_valid = eye_gaze_data->unk_bool_3;
		eye_data->unk_float_4 = __levec2_to_cpu(eye_gaze_data->unk_float_4);
	}

	{
		struct pkt_gaze_combined *combined = &gaze_state.packet_data.combined;

		struct xrt_vec3 gaze_direction = __levec3_to_cpu(combined->normalized_gaze);
		// flip to correct coordinate space
		gaze_direction.x *= -1;
		gaze_direction.z *= -1;

		struct xrt_vec3 gaze_point = __levec3_to_cpu(combined->gaze_point_3d);
		math_vec3_scalar_mul(1.0 / 1000.0, &gaze_point); // to meters
		// flip to correct coordinate space
		gaze_point.x *= -1;
		gaze_point.z *= -1;

		struct psvr2_et_combined_data *eye_data = &hmd->et_data.combined;

		if (combined->normalized_gaze_valid) {
			m_filter_euro_vec3_run(&eye_data->gaze_direction_filter, timestamp_ns, &gaze_direction,
			                       &eye_data->filtered_gaze_direction);
			math_vec3_normalize(&eye_data->filtered_gaze_direction);
		}

		eye_data->gaze_direction_valid = combined->normalized_gaze_valid;
		eye_data->gaze_direction = gaze_direction;
		eye_data->gaze_point_valid = combined->gaze_point_valid;
		eye_data->gaze_point = gaze_point;
		eye_data->is_valid = combined->is_valid;
		eye_data->unk_float_8_valid = combined->unk_bool_7;
		eye_data->unk_float_8 = __lef32_to_cpu(combined->unk_float_8);
		eye_data->unk_float3_pair_valid = combined->unk_bool_9;
		eye_data->unk_float_12 = __levec3_to_cpu(combined->unk_float_12);
		eye_data->unk_float_15 = __levec3_to_cpu(combined->unk_float_15);
		eye_data->unk_float_18 = __levec3_to_cpu(combined->unk_float_18);
	}

	hmd->et_data.unk_float_4_valid = gaze_state.packet_data.unk_bool_9;
	hmd->et_data.unk_float_4 = __lef32_to_cpu(gaze_state.packet_data.unk_float_4);

	hmd->et_data.unk_float_5_valid = gaze_state.packet_data.unk_bool_10;
	hmd->et_data.unk_float_5 = __lef32_to_cpu(gaze_state.packet_data.unk_float_5);

	// update the gaze direction
	float look_x_dir = atanf(hmd->et_data.combined.filtered_gaze_direction.x);
	float look_y_dir = atanf(hmd->et_data.combined.filtered_gaze_direction.y);

	struct xrt_space_relation gaze_relation = {0};
	math_quat_from_euler_angles(&(struct xrt_vec3){.x = look_y_dir, .y = -look_x_dir},
	                            &gaze_relation.pose.orientation);
	gaze_relation.pose.position = (struct xrt_vec3){0};

	if (hmd->et_data.combined.gaze_direction_valid) {
		gaze_relation.relation_flags =
		    XRT_SPACE_RELATION_POSITION_VALID_BIT | XRT_SPACE_RELATION_POSITION_TRACKED_BIT |
		    XRT_SPACE_RELATION_ORIENTATION_VALID_BIT | XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT;
	}

	os_mutex_unlock(&hmd->et_data.data_mutex);

	m_relation_history_push(hmd->et_data.gaze_relation_history, &gaze_relation, timestamp_ns);
}

static void LIBUSB_CALL
gaze_xfer_cb(struct libusb_transfer *xfer)
{
	DRV_TRACE_MARKER();

	if (!psvr2_usb_xfer_continue(xfer, "Gaze")) {
		return;
	}

	// Handle gaze packet
	struct psvr2_hmd *hmd = xfer->user_data;
	if ((size_t)xfer->actual_length >= sizeof(struct pkt_gaze_state)) {
		PSVR2_TRACE(hmd, "Gaze - %d bytes", xfer->actual_length);
		PSVR2_TRACE_HEX(hmd, xfer->buffer, xfer->actual_length);

		process_gaze_packet(hmd, xfer->buffer, xfer->actual_length);
	} else {
		// hm?
		PSVR2_TRACE(hmd, "bad gaze - %d bytes", xfer->actual_length);
	}

	libusb_submit_transfer(xfer);
}

static void *
psvr2_eye_tracking_control_thread(void *usrptr)
{
	int result = 0;
	bool success;
	struct psvr2_hmd *hmd = (struct psvr2_hmd *)usrptr;

	const char *thread_name = "PSVR2 Eye Tracking Control";

	U_TRACE_SET_THREAD_NAME(thread_name);
	os_thread_name_self(thread_name);

#ifdef XRT_OS_LINUX
	// Try to raise priority of this thread.
	u_linux_try_to_set_realtime_priority_on_thread(hmd->log_level, thread_name);
#endif

	os_thread_helper_lock(&hmd->et_data.eye_tracking_thread);

	// uncomment this to be able to see if things are actually progressing as expected in a debugger, without having
	// to count yourself
	// #define TICK_DEBUG

#ifdef TICK_DEBUG
	int ticks = 0;
#endif

	while (os_thread_helper_is_running_locked(&hmd->et_data.eye_tracking_thread) && result >= 0) {
		os_thread_helper_unlock(&hmd->et_data.eye_tracking_thread);

		bool enable = hmd->et_data.want_enabled || hmd->et_data.force_enable;

		enum psvr2_gaze_stream_subcommand subcmd =
		    enable ? PSVR2_GAZE_STREAM_SUBCMD_ENABLE : PSVR2_GAZE_STREAM_SUBCMD_DISABLE;

		// send keepalive for et, lasts some amount of seconds, we send it regularly to keep it on
		success = send_psvr2_control(hmd, PSVR2_REPORT_ID_SET_GAZE_STREAM, subcmd, NULL, 0);

		if (!success) {
			PSVR2_ERROR(hmd, "Failed to send gaze keepalive");
			return NULL;
		}

		hmd->et_data.enabled = enable;

		os_nanosleep(U_TIME_1S_IN_NS);

		os_thread_helper_lock(&hmd->et_data.eye_tracking_thread);
#ifdef TICK_DEBUG
		ticks += 1;
#endif
	}

	os_thread_helper_unlock(&hmd->et_data.eye_tracking_thread);

	return NULL;
}

void
psvr2_free_et_data(struct psvr2_hmd *hmd)
{
	u_var_remove_root(&hmd->et_data);

	// stop the ET thread
	os_thread_helper_destroy(&hmd->et_data.eye_tracking_thread);

	m_relation_history_destroy(&hmd->et_data.gaze_relation_history);

	if (hmd->et_data.data_mutex_created) {
		os_mutex_destroy(&hmd->et_data.data_mutex);
	}

	// @note - the gaze USB transfer should have already been cancelled and freed by now in the main free function,
	// so we don't do that here, even if we init it in this file
}

int
psvr2_start_gaze_tracking(struct psvr2_hmd *hmd)
{
	int ret;

	ret = os_mutex_init(&hmd->et_data.data_mutex);
	if (ret < 0) {
		PSVR2_ERROR(hmd, "Could not create mutex for eye tracking data");
		return ret;
	}
	hmd->et_data.data_mutex_created = true;

	// Gaze endpoint
	hmd->gaze_xfer = libusb_alloc_transfer(0);
	if (hmd->gaze_xfer == NULL) {
		PSVR2_ERROR(hmd, "Could not alloc USB transfer for gaze data");
		return -1;
	}

	uint8_t *gaze_buf = U_TYPED_ARRAY_CALLOC(uint8_t, USB_GAZE_XFER_SIZE);
	if (gaze_buf == NULL) {
		PSVR2_ERROR(hmd, "Could not alloc buffer for gaze USB transfer");
		return -1;
	}

	libusb_fill_bulk_transfer(hmd->gaze_xfer, hmd->dev, LIBUSB_ENDPOINT_IN | PSVR2_GAZE_ENDPOINT, gaze_buf,
	                          USB_GAZE_XFER_SIZE, gaze_xfer_cb, hmd, 0);
	hmd->gaze_xfer->flags |= LIBUSB_TRANSFER_FREE_BUFFER;

	ret = libusb_submit_transfer(hmd->gaze_xfer);
	if (ret < 0) {
		PSVR2_ERROR(hmd, "Could not submit USB transfer for gaze data");
		return ret;
	}
	hmd->usb_active_xfers++;

	m_relation_history_create(&hmd->et_data.gaze_relation_history);
	if (hmd->et_data.gaze_relation_history == NULL) {
		PSVR2_ERROR(hmd, "Could not create relation history");
		return -1;
	}

	ret = os_thread_helper_init(&hmd->et_data.eye_tracking_thread);
	if (ret < 0) {
		PSVR2_ERROR(hmd, "Could not create thread for eye tracking keepalive");
		return -1;
	}

	FILE *eye_calib_file = u_file_open_file_in_config_dir_subpath("psvr2", "eye_calibration.bin", "r");

	if (eye_calib_file) {
		size_t file_size;
		char *contents = u_file_read_content(eye_calib_file, &file_size);

		if (contents != NULL) {
			ret = libusb_bulk_transfer(hmd->dev, LIBUSB_ENDPOINT_OUT | PSVR2_GAZE_INTERFACE,
			                           (uint8_t *)contents, file_size, NULL, 1000);
			if (ret < 0) {
				PSVR2_ERROR(hmd, "Could not send gaze calibration");
				free(contents);
				fclose(eye_calib_file);
				return -1;
			}

			free(contents);
		}

		fclose(eye_calib_file);
	}

	ret = os_thread_helper_start(&hmd->et_data.eye_tracking_thread, psvr2_eye_tracking_control_thread, hmd);
	if (ret < 0) {
		PSVR2_ERROR(hmd, "Could not start gaze keepalive thread");
		return -1;
	}

	m_filter_euro_vec3_init(&hmd->et_data.combined.gaze_direction_filter, M_EURO_FILTER_EYE_TRACKING_FCMIN,
	                        M_EURO_FILTER_EYE_TRACKING_FCMIN_D, M_EURO_FILTER_EYE_TRACKING_BETA);

	u_var_add_root(&hmd->et_data, "PSVR2 Eye Tracker", true);

	u_var_add_bool(&hmd->et_data, &hmd->et_data.want_enabled, "Eye Tracking Wanted");
	u_var_add_bool(&hmd->et_data, &hmd->et_data.force_enable, "Force Enable Eye Tracking");
	u_var_add_bool(&hmd->et_data, &hmd->et_data.enabled, "Enabled");

	{
		u_var_add_gui_header(&hmd->et_data, NULL, "Eye Tracker Data");
		struct psvr2_et_data *et_data = &hmd->et_data;

		u_var_add_ro_i64_ns(et_data, &et_data->last_remote_report_sample_time_ns, "Timestamp");
		u_var_add_ro_u32(et_data, &et_data->last_remote_report_sample_time_us, "Raw Timestamp (us)");

		u_var_add_bool(et_data, &et_data->unk_float_4_valid, "unk_float_4 Valid");
		u_var_add_f32(et_data, &et_data->unk_float_4, "unk_float_4");

		u_var_add_bool(et_data, &et_data->unk_float_5_valid, "unk_float_5 Valid");
		u_var_add_f32(et_data, &et_data->unk_float_5, "unk_float_5");
	}

	for (size_t i = 0; i < ARRAY_SIZE(hmd->et_data.eyes); i++) {
		u_var_add_gui_header(&hmd->et_data, NULL, i == 0 ? "Left Eye" : "Right Eye");
		struct psvr2_et_eye_data *eye = &hmd->et_data.eyes[i];

		m_filter_euro_vec3_init(&eye->gaze_direction_filter, M_EURO_FILTER_EYE_TRACKING_FCMIN,
		                        M_EURO_FILTER_EYE_TRACKING_FCMIN_D, M_EURO_FILTER_EYE_TRACKING_BETA);


		u_var_add_bool(&hmd->et_data, &eye->blink_valid, "Blink Valid");
		u_var_add_bool(&hmd->et_data, &eye->blink, "Blink");

		u_var_add_bool(&hmd->et_data, &eye->pupil_diameter_valid, "Pupil Diameter Valid");
		u_var_add_f32(&hmd->et_data, &eye->pupil_diameter, "Pupil Diameter (meters)");

		u_var_add_bool(&hmd->et_data, &eye->gaze_direction_valid, "Gaze Direction Valid");
		u_var_add_vec3_f32(&hmd->et_data, &eye->gaze_direction, "Gaze Direction");
		u_var_add_vec3_f32(&hmd->et_data, &eye->filtered_gaze_direction, "Filtered Gaze Direction");

		u_var_add_bool(&hmd->et_data, &eye->gaze_point_valid, "Gaze Point Valid");
		u_var_add_vec3_f32(&hmd->et_data, &eye->gaze_point, "Gaze Point");

		u_var_add_bool(&hmd->et_data, &eye->unk_float_2_valid, "unk_float_2 Valid");
		u_var_add_ro_vec2_f32(&hmd->et_data, &eye->unk_float_2, "unk_float_2");

		u_var_add_bool(&hmd->et_data, &eye->unk_float_4_valid, "unk_float_4 Valid");
		u_var_add_ro_vec2_f32(&hmd->et_data, &eye->unk_float_4, "unk_float_4");
	}

	{
		u_var_add_gui_header(&hmd->et_data, NULL, "Combined Eye Data");
		struct psvr2_et_combined_data *combined = &hmd->et_data.combined;

		u_var_add_bool(&hmd->et_data, &combined->gaze_point_valid, "Gaze Direction Valid");
		u_var_add_vec3_f32(&hmd->et_data, &combined->gaze_direction, "Gaze Direction");
		u_var_add_vec3_f32(&hmd->et_data, &combined->filtered_gaze_direction, "Filtered Gaze Direction");

		u_var_add_bool(&hmd->et_data, &combined->gaze_point_valid, "Gaze Point Valid");
		u_var_add_vec3_f32(&hmd->et_data, &combined->gaze_point, "Gaze Point");

		u_var_add_bool(&hmd->et_data, &combined->is_valid, "Valid");

		u_var_add_bool(&hmd->et_data, &combined->unk_float_8_valid, "unk_float_8 Valid");
		u_var_add_f32(&hmd->et_data, &combined->unk_float_8, "unk_float_8");

		u_var_add_bool(&hmd->et_data, &combined->unk_float3_pair_valid, "unk_float3_pair Valid");
		u_var_add_vec3_f32(&hmd->et_data, &combined->unk_float_12, "unk_float_12");
		u_var_add_vec3_f32(&hmd->et_data, &combined->unk_float_15, "unk_float_15");
		u_var_add_vec3_f32(&hmd->et_data, &combined->unk_float_18, "unk_float_18");
	}

	return 0;
}

xrt_result_t
psvr2_get_face_tracking(struct xrt_device *xdev,
                        enum xrt_input_name facial_expression_type,
                        int64_t at_timestamp_ns,
                        struct xrt_facial_expression_set *out_value)
{
	xrt_result_t result = XRT_SUCCESS;

	struct psvr2_hmd *hmd = psvr2_hmd(xdev);

	os_mutex_lock(&hmd->et_data.data_mutex);

	// @todo: store a history of facial sample data to be able to interpolate/extrapolate to at_timestamp_ns
	//        for now, let's just always use the latest data

	timepoint_ns latest_local_sample_time_ns = hmd->et_data.last_remote_report_sample_time_ns + hmd->hw2mono_vts;

	bool gaze_directions_valid = true;
	float confidence[2];
	float blink[2];
	struct xrt_vec3 gaze_directions[2];

	for (int i = 0; i < 2; i++) {
		struct psvr2_et_eye_data *eye = &hmd->et_data.eyes[i];

		confidence[i] = 1.0f;
		blink[i] = (float)eye->blink_interp;
		gaze_directions[i] = eye->filtered_gaze_direction;

		if (!eye->blink_valid) {
			confidence[i] *= 0.25f;
		}

		if (!eye->gaze_direction_valid) {
			confidence[i] *= 0.66f;

			// if the per-eye gaze dir isn't valid, pull from the combined data
			gaze_directions[i] = hmd->et_data.combined.filtered_gaze_direction;

			// no per eye or combined gaze data :c
			// we'll still set it based on the combined gaze data, since that *more frequently* has
			// more up to date info (since it will still work with only one eye open)
			if (!hmd->et_data.combined.gaze_direction_valid) {
				confidence[i] *= 0.66f;
				gaze_directions_valid = false;
			}
		}
	}

	switch (facial_expression_type) {
	case XRT_INPUT_ANDROID_FACE_TRACKING: {
		if (!hmd->et_data.processed_sample_packet || !hmd->et_data.enabled) {
			out_value->face_expression_set_android = (struct xrt_facial_expression_set_android){
			    .state = XRT_FACE_TRACKING_STATE_STOPPED_ANDROID,
			    .is_valid = false,
			};

			break;
		}

		out_value->face_expression_set_android = (struct xrt_facial_expression_set_android){
		    .parameters =
		        {
		            [XRT_FACE_PARAMETER_INDICES_EYES_CLOSED_L_ANDROID] = blink[0],
		            [XRT_FACE_PARAMETER_INDICES_EYES_CLOSED_R_ANDROID] = blink[1],
		            [XRT_FACE_PARAMETER_INDICES_EYES_LOOK_LEFT_L_ANDROID] = MAX(0, -gaze_directions[0].x),
		            [XRT_FACE_PARAMETER_INDICES_EYES_LOOK_LEFT_R_ANDROID] = MAX(0, -gaze_directions[1].x),
		            [XRT_FACE_PARAMETER_INDICES_EYES_LOOK_RIGHT_L_ANDROID] = MAX(0, gaze_directions[0].x),
		            [XRT_FACE_PARAMETER_INDICES_EYES_LOOK_RIGHT_R_ANDROID] = MAX(0, gaze_directions[1].x),
		            [XRT_FACE_PARAMETER_INDICES_EYES_LOOK_DOWN_L_ANDROID] = MAX(0, -gaze_directions[0].y),
		            [XRT_FACE_PARAMETER_INDICES_EYES_LOOK_DOWN_R_ANDROID] = MAX(0, -gaze_directions[1].y),
		            [XRT_FACE_PARAMETER_INDICES_EYES_LOOK_UP_L_ANDROID] = MAX(0, gaze_directions[0].y),
		            [XRT_FACE_PARAMETER_INDICES_EYES_LOOK_UP_R_ANDROID] = MAX(0, gaze_directions[1].y),
		        },
		    .region_confidences =
		        {
		            [XRT_FACE_CONFIDENCE_REGIONS_LOWER_ANDROID] = 0.0f,
		            [XRT_FACE_CONFIDENCE_REGIONS_LEFT_UPPER_ANDROID] = confidence[0],
		            [XRT_FACE_CONFIDENCE_REGIONS_RIGHT_UPPER_ANDROID] = confidence[1],
		        },
		    .state = hmd->et_data.enabled ? XRT_FACE_TRACKING_STATE_TRACKING_ANDROID
		                                  : XRT_FACE_TRACKING_STATE_STOPPED_ANDROID,
		    .sample_time_ns = latest_local_sample_time_ns,
		    .is_valid = true,
		};
		break;
	}
	case XRT_INPUT_FB_FACE_TRACKING2_VISUAL: {
		if (!hmd->et_data.processed_sample_packet || !hmd->et_data.enabled) {
			out_value->face_expression_set2_fb = (struct xrt_facial_expression_set2_fb){
			    .data_source = XRT_FACE_TRACKING_DATA_SOURCE2_VISUAL_FB,
			    .is_valid = false,
			};

			break;
		}

		out_value->face_expression_set2_fb = (struct xrt_facial_expression_set2_fb){
		    .weights =
		        {
		            [XRT_FACE_EXPRESSION2_EYES_CLOSED_L_FB] = blink[0],
		            [XRT_FACE_EXPRESSION2_EYES_CLOSED_R_FB] = blink[1],
		            [XRT_FACE_EXPRESSION2_EYES_LOOK_LEFT_L_FB] = MAX(0, -gaze_directions[0].x),
		            [XRT_FACE_EXPRESSION2_EYES_LOOK_LEFT_R_FB] = MAX(0, -gaze_directions[1].x),
		            [XRT_FACE_EXPRESSION2_EYES_LOOK_RIGHT_L_FB] = MAX(0, gaze_directions[0].x),
		            [XRT_FACE_EXPRESSION2_EYES_LOOK_RIGHT_R_FB] = MAX(0, gaze_directions[1].x),
		            [XRT_FACE_EXPRESSION2_EYES_LOOK_DOWN_L_FB] = MAX(0, -gaze_directions[0].y),
		            [XRT_FACE_EXPRESSION2_EYES_LOOK_DOWN_R_FB] = MAX(0, -gaze_directions[1].y),
		            [XRT_FACE_EXPRESSION2_EYES_LOOK_UP_L_FB] = MAX(0, gaze_directions[0].y),
		            [XRT_FACE_EXPRESSION2_EYES_LOOK_UP_R_FB] = MAX(0, gaze_directions[1].y),
		        },
		    .confidences =
		        {
		            [XRT_FACE_CONFIDENCE2_LOWER_FACE_FB] = 0.0f,
		            [XRT_FACE_CONFIDENCE2_UPPER_FACE_FB] = (confidence[0] + confidence[1]) / 2.0f,
		        },
		    .data_source = XRT_FACE_TRACKING_DATA_SOURCE2_VISUAL_FB,
		    .sample_time_ns = latest_local_sample_time_ns,
		    .is_valid = true,
		    .is_eye_following_blendshapes_valid = gaze_directions_valid,
		};
		break;
	}
	case XRT_INPUT_HTC_EYE_FACE_TRACKING: {
		if (!hmd->et_data.processed_sample_packet || !hmd->et_data.enabled) {
			out_value->eye_expression_set_htc = (struct xrt_facial_eye_expression_set_htc){
			    .base =
			        {
			            .is_active = false,
			        },
			};

			break;
		}

		out_value->eye_expression_set_htc = (struct xrt_facial_eye_expression_set_htc){
		    .base =
		        {
		            .is_active = true,
		            .sample_time_ns = latest_local_sample_time_ns,
		        },
		    .expression_weights =
		        {
		            [XRT_EYE_EXPRESSION_LEFT_BLINK_HTC] = blink[0],
		            [XRT_EYE_EXPRESSION_RIGHT_BLINK_HTC] = blink[1],
		        },
		};

		break;
	}
	default: result = XRT_ERROR_INPUT_UNSUPPORTED; break;
	}

	os_mutex_unlock(&hmd->et_data.data_mutex);

	return result;
}
