// Copyright 2025-2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Implementation of Oculus Rift sensor probing/initialization
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_rift_sensor
 */

#include "xrt/xrt_byte_order.h"

#include "util/u_debug.h"
#include "util/u_logging.h"
#include "util/u_var.h"
#include "util/u_linux.h"
#include "util/u_trace_marker.h"

#include "rift/rift_interface.h"

#include "uvc/uvc_interface.h"

#include "esp570.h"
#include "esp770u.h"
#include "ar0134.h"
#include "mt9v034.h"
#include "rift_sensor_internal.h"


DEBUG_GET_ONCE_LOG_OPTION(rift_sensor_log, "RIFT_SENSOR_LOG", U_LOGGING_WARN)

#define SENSOR_TRACE(d, ...) U_LOG_IFL_T(d->log_level, __VA_ARGS__)
#define SENSOR_DEBUG(d, ...) U_LOG_IFL_D(d->log_level, __VA_ARGS__)
#define SENSOR_INFO(d, ...) U_LOG_IFL_I(d->log_level, __VA_ARGS__)
#define SENSOR_WARN(d, ...) U_LOG_IFL_W(d->log_level, __VA_ARGS__)
#define SENSOR_ERROR(d, ...) U_LOG_IFL_E(d->log_level, __VA_ARGS__)

/*
 * Internal functions
 */

static void
rift_sensor_destroy(struct rift_sensor *sensor)
{
	if (sensor->hid_dev) {
		libusb_close(sensor->hid_dev);

		libusb_unref_device(libusb_get_device(sensor->hid_dev));
	}

	// @note uvc_fs_destroy is called by the xrt_frame_node destroy callback, so we don't need to call it here
}

static void
rift_sensor_context_destroy(struct rift_sensor_context *context)
{
	if (context->usb_thread.initialized) {
		os_thread_helper_destroy(&context->usb_thread);
	}

	u_var_remove_root(context);

	for (uint32_t i = 0; i < context->sensor_count; i++) {
		rift_sensor_destroy(&context->sensors[i]);
	}

	libusb_exit(context->usb_ctx);

	if (context->sensors != NULL) {
		free(context->sensors);
	}

	free(context);

	return;
}

static void
rift_sensor_context_node_break_apart(struct xrt_frame_node *node)
{
	struct rift_sensor_context *context = rift_sensor_context(node);

	SENSOR_DEBUG(context, "Breaking apart Rift sensor context node");

	if (context->usb_thread.initialized) {
		os_thread_helper_stop_and_wait(&context->usb_thread);
	}
}

static void
rift_sensor_context_node_destroy(struct xrt_frame_node *node)
{
	struct rift_sensor_context *context = rift_sensor_context(node);

	SENSOR_DEBUG(context, "Destroying Rift sensor context node, destroying sensors and freeing context.");

	rift_sensor_context_destroy(context);
}

static bool
rift_sensor_setup_stream_parameters_callback(uint16_t vid,
                                             uint16_t pid,
                                             bool is_usb2,
                                             libusb_device_handle *devh,
                                             struct uvc_probe_commit_control *control,
                                             struct uvc_stream_parameters *parameters,
                                             size_t *packet_size,
                                             int *alt_setting,
                                             void *user_data)
{
	int ret;

	struct rift_sensor_context *context = user_data;

	// unk vid
	if (vid != OCULUS_VR_VID) {
		return false;
	}

	switch (pid) {
	case OCULUS_DK2_SENSOR_PID:
		control->dwFrameInterval = __cpu_to_le32(166666);
		control->dwMaxVideoFrameSize = __cpu_to_le32(752 * 480);
		control->dwMaxPayloadTransferSize = __cpu_to_le32(3000);

		parameters->stride = 752;
		parameters->width = 752;
		parameters->height = 480;
		parameters->format = XRT_FORMAT_L8;

		*packet_size = 3060;
		*alt_setting = 7;

		esp570_setup_unknown_3(devh);

		return true;
	case OCULUS_CV1_SENSOR_PID:
		if (is_usb2) {
			// JPEG mode for USB 2 connection
			control->bFormatIndex = 2;
			control->bFrameIndex = 2;
			*packet_size = 2048;
			*alt_setting = 2;
			parameters->format = XRT_FORMAT_MJPEG;
		} else {
			control->bFrameIndex = 4;
			*packet_size = 16384;
			*alt_setting = 2;
			parameters->format = XRT_FORMAT_L8;
		}

		control->dwFrameInterval = __cpu_to_le32(192000);
		control->dwMaxVideoFrameSize = __cpu_to_le32(RIFT_SENSOR_FRAME_SIZE);
		control->dwMaxPayloadTransferSize = __cpu_to_le32(*packet_size);
		control->dwClockFrequency = __cpu_to_le32(RIFT_SENSOR_CLOCK_FREQ);

		parameters->stride = RIFT_SENSOR_WIDTH;
		parameters->width = RIFT_SENSOR_WIDTH;
		parameters->height = RIFT_SENSOR_HEIGHT;

		ret = rift_sensor_esp770u_init_regs(devh);
		if (ret < 0) {
			SENSOR_ERROR(context, "Failed to init CV1 sensor, reason %d", ret);
			return false;
		}

		return true;
	default: break;
	}

	return false;
}

static bool
rift_sensor_post_init_callback(uint16_t vid, uint16_t pid, bool is_usb2, libusb_device_handle *devh, void *user_data)
{
	int ret;

	struct rift_sensor_context *context = user_data;

	// In JPEG mode, we have some extra init packets to send
	if (pid == OCULUS_CV1_SENSOR_PID && is_usb2) {
		ret = rift_sensor_esp770u_init_jpeg(devh);
		if (ret < 0) {
			SENSOR_ERROR(context, "Failed to init CV1 sensor in JPEG mode");
			return false;
		}
	}

	return true;
}

static bool
rift_sensor_decode_camera_calib_stream(struct rift_sensor_context *context,
                                       __lef32 *stream,
                                       struct t_camera_calibration *calib)
{
	int marker = (int)__lef32_to_cpu(stream[0]);
	if (marker != 1) {
		SENSOR_ERROR(context, "Invalid camera calibration stream marker: %d", marker);
		return false;
	}

	int lens_type = (int)__lef32_to_cpu(stream[1]);
	if (lens_type == RIFT_SENSOR_CV1_LENS_TYPE_OLD_BA_REJECTED) {
		SENSOR_ERROR(context, "Invalid lens type in camera calibration stream: %d", lens_type);
		return false;
	}

	int height = (int)__lef32_to_cpu(stream[2]);
	int width = (int)__lef32_to_cpu(stream[3]);
	float focal = __lef32_to_cpu(stream[4]);
	float cx = __lef32_to_cpu(stream[5]);
	float cy = __lef32_to_cpu(stream[6]);
	float fov_distorted_deg = __lef32_to_cpu(stream[7]);
	if (lens_type == RIFT_SENSOR_CV1_LENS_TYPE_OPAQUE_BLOB) {
		// @todo parse this correctly, for now we don't need this, so don't handle it
		return false;
	}
	float fov_pinhole_deg = __lef32_to_cpu(stream[8]);
	if (fov_distorted_deg == 0.0f || fov_pinhole_deg == 0.0f) {
		// @todo manually compute this as the official runtime does
	}

	float radial[4] = {0};
	float affine[4] = {0};
	float spline_ratio[32] = {0};
	(void)spline_ratio; // @note: We aren't using this yet, the other algorithms aren't implemented as of now.

	int radial_count = 0;
	int affine_count = 0;
	int spline_count = 0;

	switch (lens_type) {
	case RIFT_SENSOR_CV1_LENS_TYPE_RADTAN:
	case RIFT_SENSOR_CV1_LENS_TYPE_RADTAN_RECIPROCAL: {
		radial_count = 3;
		radial[0] = __lef32_to_cpu(stream[9]);
		radial[1] = __lef32_to_cpu(stream[10]);
		radial[2] = __lef32_to_cpu(stream[11]);
		affine_count = 2;
		affine[0] = __lef32_to_cpu(stream[12]);
		affine[1] = __lef32_to_cpu(stream[13]);

		// ignore dead store error, the radial/affine count sets are intentionally dead for documentation.
		(void)radial_count;
		(void)affine_count;

		(*calib) = (struct t_camera_calibration){
		    .image_size_pixels = {width, height},
		    .intrinsics =
		        {
		            {focal, 0.0f, cx},
		            {0.0f, focal, cy},
		            {0.0f, 0.0f, 1.0f},
		        },
		    .rt5 = {.k1 = radial[0], .k2 = radial[1], .k3 = radial[2], .p1 = affine[0], .p2 = affine[1]},
		    .distortion_model = T_DISTORTION_OPENCV_RADTAN_5,
		};

		return true;
	}
	case RIFT_SENSOR_CV1_LENS_TYPE_KB_LEGACY:
	case RIFT_SENSOR_CV1_LENS_TYPE_KB_SPLINE: {
		__lef32 *cursor_kb;
		if (lens_type == RIFT_SENSOR_CV1_LENS_TYPE_KB_LEGACY) {
			affine_count = 2;
			affine[0] = __lef32_to_cpu(stream[9]);
			affine[1] = __lef32_to_cpu(stream[10]);
			cursor_kb = stream + 11;

			(void)affine_count; // ignore dead store, that dead store is documentation.
		} else {
			affine_count = (int)__lef32_to_cpu(stream[9]);
			if (affine_count > (int)ARRAY_SIZE(affine)) {
				SENSOR_ERROR(context, "Got bad affine count %d", affine_count);
				return false;
			}
			cursor_kb = stream + 10;
			for (int i = 0; i < affine_count; i++) {
				affine[i] = __lef32_to_cpu(*(cursor_kb++));
			}
		}
		spline_count = (int)__lef32_to_cpu(*cursor_kb);
		if (spline_count > (int)ARRAY_SIZE(spline_ratio)) {
			SENSOR_ERROR(context, "Got bad spline count %d", spline_count);
			return false;
		}
		__lef32 *cursor_spline = cursor_kb + 1;
		for (int i = 0; i < spline_count; i++) {
			spline_ratio[i] = __lef32_to_cpu(*(cursor_spline++));
		}
		float spline_end_slope = __lef32_to_cpu(*cursor_spline);
		(void)spline_end_slope; // @note we aren't able to utilize this format yet.

		// We auto-correct to the new format
		lens_type = RIFT_SENSOR_CV1_LENS_TYPE_KB_SPLINE;
		(void)lens_type; // ignore dead store warning, this is a TODO codepath.

		return false; // @todo implement this format
	}
	case RIFT_SENSOR_CV1_LENS_TYPE_CV1: {
		radial_count = (int)__lef32_to_cpu(stream[9]);
		if (radial_count > (int)ARRAY_SIZE(radial)) {
			SENSOR_ERROR(context, "Got bad radial count %d", radial_count);
			return false;
		}

		__lef32 *cursor = stream + 10;
		for (int i = 0; i < radial_count; i++) {
			radial[i] = __lef32_to_cpu(*(cursor++));
		}

		affine_count = (int)__lef32_to_cpu(*(cursor++));
		if (affine_count > (int)ARRAY_SIZE(affine)) {
			SENSOR_ERROR(context, "Got bad affine count %d", affine_count);
			return false;
		}
		for (int i = 0; i < affine_count; i++) {
			affine[i] = __lef32_to_cpu(*(cursor++));
		}

		(*calib) = (struct t_camera_calibration){
		    .image_size_pixels = {width, height},
		    .intrinsics =
		        {
		            {focal, 0.0f, cx},
		            {0.0f, focal, cy},
		            {0.0f, 0.0f, 1.0f},
		        },
		    .cv1 =
		        {
		            .k1 = radial[0],
		            .k2 = radial[1],
		            .k3 = radial[2],
		            .k4 = radial[3],
		            .p1 = affine[0],
		            .p2 = affine[1],
		            .g3 = affine[2],
		            .g4 = affine[3],
		        },
		    .distortion_model = T_DISTORTION_RIFT_CV1,
		};

		return true;
	}
	default: {
		SENSOR_ERROR(context, "Unknown lens type %d", lens_type);
		return false;
	}
	}

	return false;
}

static int
rift_sensor_read_calibration(struct rift_sensor_context *context,
                             struct rift_sensor *sensor,
                             const struct libusb_device_descriptor *desc)
{
	int ret;

	struct t_camera_calibration default_calib = {
	    .image_size_pixels = {RIFT_SENSOR_WIDTH, RIFT_SENSOR_HEIGHT},
	    .intrinsics =
	        {
	            {733.0, 0.0, RIFT_SENSOR_WIDTH / 2.0},
	            {0.0, 733.0, RIFT_SENSOR_HEIGHT / 2.0},
	            {0.0, 0.0, 1.0},
	        },
	    .rt5 = {.k1 = -0.2947, .k2 = 0.1154, .k3 = 0.0, .p1 = -0.024, .p2 = 0.0},
	    .distortion_model = T_DISTORTION_OPENCV_RADTAN_5,
	};

	switch (desc->idProduct) {
	case OCULUS_DK2_SENSOR_PID: {
		sensor->frame_interval = 16666 * U_TIME_1US_IN_NS;
		sensor->variant = RIFT_VARIANT_DK2;
		sensor->calibration.distortion_model = T_DISTORTION_OPENCV_RADTAN_5;
		struct t_camera_calibration_rt5_params calibration_params;

		struct rift_sensor_dk2_calib calib;

		// Read 4 32-byte blocks at EEPROM address 0x2000
		for (int i = 0; i < 128; i += 32) {
			ret = esp570_eeprom_read(sensor->hid_dev, 0x2000 + i, ((uint8_t *)&calib) + i, 32);
			if (ret < 0) {
				SENSOR_ERROR(context, "DK2 EEPROM read failed! reason %d", ret);
				return ret;
			}
		}

		double fx = __lef64_to_cpu(calib.fx);
		double fy = __lef64_to_cpu(calib.fy);
		double cx = __lef64_to_cpu(calib.cx);
		double cy = __lef64_to_cpu(calib.cy);

		calibration_params.k1 = __lef64_to_cpu(calib.k1);
		calibration_params.k2 = __lef64_to_cpu(calib.k2);
		calibration_params.p1 = __lef64_to_cpu(calib.p1);
		calibration_params.p2 = __lef64_to_cpu(calib.p2);
		calibration_params.k3 = __lef64_to_cpu(calib.k3);

		sensor->calibration.rt5 = calibration_params;

		// clang-format off
		memcpy(sensor->calibration.intrinsics[0], (double[3]){fx,   0.0f, cx}, sizeof(double) * 3);
		memcpy(sensor->calibration.intrinsics[1], (double[3]){0.0f, fy,   cy}, sizeof(double) * 3);
		memcpy(sensor->calibration.intrinsics[2], (double[3]){0.0,  0.0f, 1.0}, sizeof(double) * 3);
		// clang-format on

		break;
	}
	case OCULUS_CV1_SENSOR_PID: {
		sensor->frame_interval = 19200 * U_TIME_1US_IN_NS;
		sensor->variant = RIFT_VARIANT_CV1;

		// Read a the 160-byte calibration block at EEPROM address 0x1d000
		struct rift_sensor_cv1_calib calib;
		ret = rift_sensor_esp770u_flash_read(sensor->hid_dev, 0x1d000, (uint8_t *)&calib, sizeof(calib));
		if (ret < 0) {
			SENSOR_ERROR(context, "CV1 EEPROM read failed! reason %d", ret);
			return ret;
		}

		uint64_t magic = __le64_to_cpu(calib.magic);
		if (magic != CV1_CALIB_MAGIC) {
			SENSOR_ERROR(context,
			             "CV1 EEPROM read failed! Invalid magic 0x%016" PRIx64 ", expected 0x%" PRIx64,
			             magic, CV1_CALIB_MAGIC);
			sensor->calibration = default_calib;
			return 0;
		}

		uint64_t version = __le64_to_cpu(calib.version);
		if (version == 0) {
			SENSOR_ERROR(context, "Invalid sensor version %" PRIu64 ", expected > 0", version);
			return -1;
		}

		uint32_t crc = __le32_to_cpu(calib.crc);
		uint32_t payload_length = __le32_to_cpu(calib.payload_length);
		uint32_t crc_length = payload_length + 0x20;
		calib.crc = 0; // zero out before CRC
		(void)crc;     // @todo: implement CRC check with crc length
		(void)crc_length;

		if (!rift_sensor_decode_camera_calib_stream(context, (__lef32 *)&calib.body, &sensor->calibration)) {
			sensor->calibration = default_calib;
			SENSOR_ERROR(context, "Failed to decode camera calibration stream");
			return -1;
		}

		SENSOR_DEBUG(context, "CV1 sensor calibration: version %" PRIu64 ", payload length %u", version,
		             payload_length);

		break;
	}
	default: SENSOR_ERROR(context, "Unknown sensor PID %" PRIu16, desc->idProduct); return -1;
	}

	return 0;
}

static int
rift_sensor_create(struct rift_sensor_context *context,
                   struct xrt_frame_context *xfctx,
                   struct rift_sensor *sensor,
                   libusb_device *device,
                   const struct libusb_device_descriptor *desc)
{
	int ret;

	struct libusb_device_handle *device_handle;
	ret = libusb_open(device, &device_handle);
	if (ret < 0) {
		SENSOR_WARN(context, "Failed to open USB device, reason %d", ret);
		return ret;
	}

	sensor->hid_dev = device_handle;

	ret = uvc_fs_create(context->usb_ctx, device_handle, desc, rift_sensor_setup_stream_parameters_callback,
	                    rift_sensor_post_init_callback, context, xfctx, &sensor->frame_server);
	if (ret < 0) {
		libusb_close(device_handle);
		SENSOR_ERROR(context, "Failed to create UVC frameserver, reason %d", ret);
		return ret;
	}

	struct xrt_fs_mode *supported_modes;
	uint32_t mode_count = 0;
	xrt_result_t xret = xrt_fs_enumerate_modes(sensor->frame_server, &supported_modes, &mode_count);
	if (xret != XRT_SUCCESS || mode_count == 0) {
		libusb_close(device_handle);
		SENSOR_ERROR(context,
		             "Failed to enumerate UVC frameserver for sensor, mode count %d, skipping, got xret %d.",
		             mode_count, xret);

		return -1;
	}

	// Pick the mode we want (the first)
	struct xrt_fs_mode mode = *supported_modes;
	free(supported_modes);

	sensor->usb2 = (desc->bcdUSB < 0x300);
	sensor->calibration.image_size_pixels = (struct xrt_size){(int)mode.width, (int)mode.height};
	ret = rift_sensor_read_calibration(context, sensor, desc);
	if (ret < 0) {
		libusb_close(device_handle);
		SENSOR_ERROR(context, "Failed to read calibration for sensor, reason %d", ret);
		return ret;
	}

	return 0;
}

static void *
rift_sensor_usb_thread_run(void *user_ptr)
{
	int result = 0;
	struct rift_sensor_context *context = (struct rift_sensor_context *)user_ptr;

	const char *thread_name = "Rift Sensor USB";

	U_TRACE_SET_THREAD_NAME(thread_name);
	os_thread_name_self(thread_name);

#ifdef XRT_OS_LINUX
	// Try to raise priority of this thread.
	u_linux_try_to_set_realtime_priority_on_thread(context->log_level, thread_name);
#endif

	os_thread_helper_lock(&context->usb_thread);

	// #define TICK_DEBUG
#ifdef TICK_DEBUG
	int ticks = 0;
#endif

	while (os_thread_helper_is_running_locked(&context->usb_thread) && result >= 0) {
		os_thread_helper_unlock(&context->usb_thread);

		result = libusb_handle_events_timeout_completed(context->usb_ctx,
		                                                &(struct timeval){.tv_sec = 0, .tv_usec = 1000}, NULL);

		// if timeout, ignore and just loop again to check if we should exit, otherwise handle the error
		if (result == LIBUSB_ERROR_TIMEOUT) {
			result = 0;
		}

		os_thread_helper_lock(&context->usb_thread);
#ifdef TICK_DEBUG
		ticks++;
#endif
	}

	os_thread_helper_unlock(&context->usb_thread);

	return NULL;
}

/*
 * Exported functions
 */

int
rift_sensor_context_create(struct rift_sensor_context **out_context, struct xrt_frame_context *xfctx)
{
	int ret;

	struct rift_sensor_context *context = U_TYPED_CALLOC(struct rift_sensor_context);
	context->node = (struct xrt_frame_node){
	    .next = NULL,
	    .break_apart = rift_sensor_context_node_break_apart,
	    .destroy = rift_sensor_context_node_destroy,
	};
	context->log_level = debug_get_log_option_rift_sensor_log();

	u_var_add_root(context, "Rift Sensors", false);
	u_var_add_log_level(context, &context->log_level, "Log Level");

	ret = libusb_init(&context->usb_ctx);
	if (ret < 0) {
		SENSOR_ERROR(context, "Failed to initialize libusb, reason %d", ret);
		goto fail;
	}

	if (!libusb_has_capability(LIBUSB_CAP_SUPPORTS_DETACH_KERNEL_DRIVER) ||
	    !libusb_has_capability(LIBUSB_CAP_HAS_HID_ACCESS)) {
		SENSOR_WARN(context,
		            "libusb does not have expected capabilities,"
		            "may not be able to access Rift sensors");
	}

	libusb_device **devices = NULL;
	ssize_t num_devices = libusb_get_device_list(context->usb_ctx, &devices);
	if (num_devices < 0) {
		SENSOR_ERROR(context, "Failed to get device list from libusb, reason %zd", num_devices);
		ret = -1;
		goto fail;
	}

	// Find how many sensors are connected
	uint32_t found_sensors = 0;
	for (ssize_t i = 0; i < num_devices; i++) {
		libusb_device *device = devices[i];

		struct libusb_device_descriptor desc;
		ret = libusb_get_device_descriptor(device, &desc);
		if (ret < 0) {
			SENSOR_WARN(context, "Failed to get device descriptor for device, reason %d, skipping.", ret);
			continue;
		}

		// skip oculus devices
		if (desc.idVendor != OCULUS_VR_VID) {
			continue;
		}

		switch (desc.idProduct) {
		case OCULUS_CV1_SENSOR_PID:
		case OCULUS_DK2_SENSOR_PID: {
			if (found_sensors >= UINT32_MAX) {
				SENSOR_WARN(
				    context,
				    "Way too many sensors connected, found at least %u, skipping additional sensors.",
				    found_sensors);

				break;
			}

			found_sensors++;
			break;
		}
		default: continue;
		}
	}

	context->sensors = U_TYPED_ARRAY_CALLOC(struct rift_sensor, found_sensors);

	for (ssize_t i = 0; i < num_devices; i++) {
		libusb_device *device = devices[i];

		struct libusb_device_descriptor desc;
		ret = libusb_get_device_descriptor(device, &desc);
		if (ret < 0) {
			SENSOR_WARN(context, "Failed to get device descriptor for device, reason %d, skipping.", ret);
			continue;
		}

		if (desc.idVendor != OCULUS_VR_VID) {
			continue;
		}

		switch (desc.idProduct) {
		case OCULUS_CV1_SENSOR_PID:
		case OCULUS_DK2_SENSOR_PID: {
			struct rift_sensor *sensor = &context->sensors[context->sensor_count];

			ret = rift_sensor_create(context, xfctx, sensor, device, &desc);
			if (ret < 0) {
				SENSOR_ERROR(context, "Failed to create sensor for device %x:%x, reason %d, skipping.",
				             desc.idVendor, desc.idProduct, ret);
				// skip...
				continue;
			}

			SENSOR_DEBUG(context, "Found Rift sensor: %04x:%04x, usb2: %d", desc.idVendor, desc.idProduct,
			             sensor->usb2);

			libusb_ref_device(device);

			context->sensor_count++;

			break;
		}
		}
	}

	libusb_free_device_list(devices, 1);

	xrt_frame_context_add(xfctx, &context->node);

	*out_context = context;
	return 0;

fail:
	rift_sensor_context_destroy(context);
	return ret;
}

int
rift_sensor_enable_exposure_sync(struct rift_sensor_context *context, struct rift_sensor *sensor, uint8_t radio_id[5])
{
	int result;

	switch (sensor->variant) {
	case RIFT_VARIANT_DK2: {
		result = mt9v034_setup(sensor->hid_dev);
		if (result < 0) {
			SENSOR_ERROR(context, "Failed to setup DK2 camera, reason %d", result);
			return result;
		}

		result = mt9v034_set_sync(sensor->hid_dev, true);
		if (result < 0) {
			SENSOR_ERROR(context, "Failed to turn on DK2 exposure sync, reason %d", result);
			return result;
		}

		break;
	}
	case RIFT_VARIANT_CV1: {
		result = rift_sensor_ar0134_init(sensor->hid_dev, sensor->usb2);
		if (result < 0) {
			SENSOR_ERROR(context, "Failed to setup CV1 camera, reason %d", result);
			return result;
		}

		result = rift_sensor_esp770u_setup_radio(sensor->hid_dev, radio_id);
		if (result < 0) {
			SENSOR_ERROR(context, "Failed to connect CV1 sensor to radio, reason %d", result);
			return result;
		}

		break;
	}
	default: break;
	}

	return 0;
}

int
rift_sensor_context_start(struct rift_sensor_context *context)
{
	int result;
	result = os_thread_helper_init(&context->usb_thread);
	if (result < 0)
		return result;

	result = os_thread_helper_start(&context->usb_thread, rift_sensor_usb_thread_run, context);
	if (result < 0)
		return result;

	return 0;
}

int
rift_sensor_context_get_sensors(struct rift_sensor_context *context,
                                struct rift_sensor ***out_sensors,
                                uint32_t *out_count)
{
	struct rift_sensor **sensors = U_TYPED_ARRAY_CALLOC(struct rift_sensor *, context->sensor_count);
	if (sensors == NULL) {
		return -1;
	}

	for (size_t i = 0; i < context->sensor_count; i++) {
		sensors[i] = &context->sensors[i];
	}
	*out_sensors = sensors;
	*out_count = context->sensor_count;

	return 0;
}

struct xrt_fs *
rift_sensor_get_frame_server(struct rift_sensor *sensor)
{
	return sensor->frame_server;
}

enum rift_variant
rift_sensor_get_variant(struct rift_sensor *sensor)
{
	return sensor->variant;
}

void
rift_sensor_get_calibration(struct rift_sensor *sensor, struct t_camera_calibration *out_calibration)
{
	*out_calibration = sensor->calibration;
}

void
rift_sensor_setup_frame_timestamp_callback(struct rift_sensor *sensor, struct rift_hmd *hmd)
{
	uvc_fs_set_source_timestamp_callback(sensor->frame_server, rift_hmd_frame_timestamp_callback, hmd);
}
