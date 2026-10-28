// Copyright 2026, Beyley Cardellio
// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Rift prober code.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup xrt_iface
 */

#include "xrt/xrt_config_build.h"
#include "xrt/xrt_config_drivers.h"
#include "xrt/xrt_prober.h"
#include "xrt/xrt_frameserver.h"
#include "xrt/xrt_system.h"

#include "tracking/t_constellation.h"

#include "constellation/t_rift_blobwatch.h"
#include "constellation/t_constellation_tracker.h"

#include "util/u_debug.h"
#include "util/u_misc.h"
#include "util/u_logging.h"
#include "util/u_trace_marker.h"
#include "util/u_var.h"
#include "util/u_sink.h"
#include "util/u_builder_search.h"

#include "target_builder_helpers.h"

#include "rift/rift_interface.h"

#ifdef XRT_BUILD_DRIVER_RIFT_SENSOR
#include "rift_sensor/rift_sensor_interface.h"
#endif

#ifdef XRT_BUILD_DRIVER_PSSENSE
#include "pssense/pssense_interface.h"
#endif

#ifdef XRT_BUILD_DRIVER_CONTACTGLOVE
#include "contactglove/contactglove_interface.h"
#endif


/*
 *
 * Internal structures
 *
 */

struct rift_builder
{
	struct t_builder base;

	enum u_logging_level log_level;

	struct rift_hmd *hmd;
	struct t_timing_event_source *timing_event_source;

#ifdef XRT_BUILD_DRIVER_RIFT_SENSOR
	struct rift_sensor_context *sensor_context;
	struct rift_sensor **sensors;
	size_t num_sensors;

	struct t_blobwatch **blobwatches;
	struct u_sink_debug *blobwatch_debug_sinks;

	struct t_constellation_tracker *constellation_tracker;
#endif

#ifdef XRT_BUILD_DRIVER_PSSENSE
	struct xrt_device *pssense_left;
	struct xrt_device *pssense_right;
#endif
};

static struct rift_builder *
rift_builder(struct xrt_builder *xb)
{
	return (struct rift_builder *)xb;
}

/*
 *
 * Misc stuff.
 *
 */

#ifdef XRT_BUILD_DRIVER_OHMD
#define DEFAULT_ENABLE false
#else
#define DEFAULT_ENABLE true
#endif

DEBUG_GET_ONCE_LOG_OPTION(rift_log, "RIFT_LOG", U_LOGGING_WARN)
DEBUG_GET_ONCE_BOOL_OPTION(rift_prober_enable, "RIFT_PROBER_ENABLE", DEFAULT_ENABLE)
DEBUG_GET_ONCE_BOOL_OPTION(rift_prober_constellation_tracking, "RIFT_PROBER_CONSTELLATION_TRACKING", false)

#undef DEFAULT_ENABLE

#define RIFT_ERROR(p, ...) U_LOG_IFL_E(p->log_level, __VA_ARGS__)
#define RIFT_WARN(p, ...) U_LOG_IFL_W(p->log_level, __VA_ARGS__)
#define RIFT_DEBUG(p, ...) U_LOG_IFL_D(p->log_level, __VA_ARGS__)

static const char *driver_list[] = {
    "rift",
#ifdef XRT_BUILD_DRIVER_PSSENSE
    "pssense",
#endif
#ifdef XRT_BUILD_DRIVER_CONTACTGLOVE
    "contactglove",
#endif
};

static bool
rift_is_oculus(struct xrt_prober *xp, struct xrt_prober_device *dev)
{
	unsigned char manufacturer[128] = {0};
	int result = xrt_prober_get_string_descriptor(xp, dev, XRT_PROBER_STRING_MANUFACTURER, manufacturer,
	                                              sizeof(manufacturer));
	if (result < 0) {
		return false;
	}

	// Some non-oculus devices (VR-Tek HMDs) reuse the same USB IDs as the oculus headsets, so we should check the
	// manufacturer
	if (strncmp((const char *)manufacturer, "Oculus VR, Inc.", sizeof(manufacturer)) != 0) {
		return false;
	}

	return true;
}

/*
 *
 * Member functions.
 *
 */

static xrt_result_t
rift_estimate_system(struct xrt_builder *xb,
                     cJSON *config,
                     struct xrt_prober *xp,
                     struct xrt_builder_estimate *estimate)
{
	struct rift_builder *rb = rift_builder(xb);

	struct xrt_prober_device **xpdevs = NULL;
	size_t xpdev_count = 0;
	xrt_result_t xret = XRT_SUCCESS;

	U_ZERO(estimate);

	if (!debug_get_bool_option_rift_prober_enable()) {
		return XRT_SUCCESS;
	}

	xret = xrt_prober_lock_list(xp, &xpdevs, &xpdev_count);
	U_LOG_CHK_AND_RET(rb->log_level, xret, "xrt_prober_lock_list");

	struct xrt_prober_device *dev =
	    u_builder_find_prober_device(xpdevs, xpdev_count, OCULUS_VR_VID, OCULUS_CV1_PID, XRT_BUS_TYPE_USB);
	if (dev != NULL && rift_is_oculus(xp, dev)) {
		estimate->certain.head = true;

		// We *might* have controllers
		estimate->maybe.left = true;
		estimate->maybe.right = true;

		// We *might* have a tracker and a remote
		estimate->maybe.extra_device_count = 2;
	}

	dev = u_builder_find_prober_device(xpdevs, xpdev_count, OCULUS_VR_VID, OCULUS_DK2_PID, XRT_BUS_TYPE_USB);
	if (dev != NULL && rift_is_oculus(xp, dev)) {
		estimate->certain.head = true;
	}

#ifdef XRT_BUILD_DRIVER_PSSENSE
	struct xrt_prober_device *dev_controller_left =
	    u_builder_find_prober_device(xpdevs, xpdev_count, PSSENSE_VID, PSSENSE_PID_LEFT, XRT_BUS_TYPE_ANY);
	if (dev_controller_left != NULL) {
		estimate->certain.left = true;
	}

	struct xrt_prober_device *dev_controller_right =
	    u_builder_find_prober_device(xpdevs, xpdev_count, PSSENSE_VID, PSSENSE_PID_RIGHT, XRT_BUS_TYPE_ANY);
	if (dev_controller_right != NULL) {
		estimate->certain.right = true;
	}
#endif

#ifdef XRT_BUILD_DRIVER_CONTACTGLOVE
	dev = u_builder_find_prober_device(xpdevs, xpdev_count, CONTACTGLOVE2_VID, CONTACTGLOVE2_PID, XRT_BUS_TYPE_USB);
	// All dongles can create two hand devices
	if (dev != NULL) {
		estimate->certain.left = true;
		estimate->certain.right = true;
	}
#endif

	RIFT_DEBUG(rb, "Rift builder estimate: head %d, left %d, right %d, extra %d", estimate->certain.head,
	           estimate->maybe.left, estimate->maybe.right, estimate->maybe.extra_device_count);

	xret = xrt_prober_unlock_list(xp, &xpdevs);
	U_LOG_CHK_AND_RET(rb->log_level, xret, "xrt_prober_unlock_list");

	return xret;
}

void
rift_open_pssense(struct rift_builder *rb,
                  struct xrt_frame_context *xfctx,
                  struct xrt_system_devices *xsysd,
                  struct xrt_prober *xp,
                  struct xrt_prober_device **xpdevs,
                  size_t xpdev_count,
                  struct t_builder_options *tbo)
{
#ifdef XRT_BUILD_DRIVER_PSSENSE
	struct xrt_prober_device *left_xpdev = u_builder_find_prober_device( //
	    xpdevs,                                                          //
	    xpdev_count,                                                     //
	    PSSENSE_VID,                                                     //
	    PSSENSE_PID_LEFT,                                                //
	    XRT_BUS_TYPE_ANY);

	if (left_xpdev != NULL) {
		struct t_timing_event_sink *timing_sink;
		struct xrt_device *left_xdev = pssense_create(xp, left_xpdev, xfctx, &timing_sink);
		if (left_xdev == NULL) {
			RIFT_ERROR(rb, "PS Sense left controller device creation failed.");
		} else {
			xsysd->static_xdevs[xsysd->static_xdev_count++] = left_xdev;
			tbo->left = left_xdev;

			if (rb->timing_event_source != NULL) {
				t_timing_event_source_add_sink(rb->timing_event_source, timing_sink);
			}

			rb->pssense_left = left_xdev;
		}
	}

	struct xrt_prober_device *right_xpdev = u_builder_find_prober_device( //
	    xpdevs,                                                           //
	    xpdev_count,                                                      //
	    PSSENSE_VID,                                                      //
	    PSSENSE_PID_RIGHT,                                                //
	    XRT_BUS_TYPE_ANY);

	if (right_xpdev != NULL) {
		struct t_timing_event_sink *timing_sink;
		struct xrt_device *right_xdev = pssense_create(xp, right_xpdev, xfctx, &timing_sink);
		if (right_xdev == NULL) {
			RIFT_ERROR(rb, "PS Sense right controller device creation failed.");
		} else {
			xsysd->static_xdevs[xsysd->static_xdev_count++] = right_xdev;
			tbo->right = right_xdev;

			if (rb->timing_event_source != NULL) {
				t_timing_event_source_add_sink(rb->timing_event_source, timing_sink);
			}

			rb->pssense_right = right_xdev;
		}
	}
#endif
}

void
rift_open_contactglove(struct rift_builder *rb,
                       struct xrt_system_devices *xsysd,
                       struct xrt_prober *xp,
                       struct xrt_prober_device **xpdevs,
                       size_t xpdev_count,
                       struct t_builder_options *tbo)
{
#ifdef XRT_BUILD_DRIVER_CONTACTGLOVE
	xrt_result_t ret;

	struct xrt_prober_device *dongle_xpdev = u_builder_find_prober_device( //
	    xpdevs,                                                            //
	    xpdev_count,                                                       //
	    CONTACTGLOVE2_VID,                                                 //
	    CONTACTGLOVE2_PID,                                                 //
	    XRT_BUS_TYPE_USB);

	if (dongle_xpdev == NULL) {
		return;
	}

	unsigned char serial_number[64] = {0};
	ret = xrt_prober_get_string_descriptor(xp, dongle_xpdev, XRT_PROBER_STRING_SERIAL_NUMBER, serial_number,
	                                       sizeof(serial_number));
	if (ret < 0) {
		RIFT_WARN(rb, "Failed to get ContactGlove dongle serial number with code %d. Setting default.", ret);
		snprintf((char *)serial_number, sizeof(serial_number), "contactglove_serial");
	}

	struct os_serial_device *contactglove_dongle_serial;
	ret = xrt_prober_open_serial_device(xp, dongle_xpdev, &CONTACTGLOVE2_SERIAL_PARAMETERS,
	                                    &contactglove_dongle_serial);
	if (ret == 0) {
		struct contactglove_dongle *dongle;

		ret = contactglove_create(CONTACTGLOVE_TYPE_CONTACTGLOVE2, (char *)serial_number,
		                          contactglove_dongle_serial, &dongle,
		                          xsysd->static_xdevs + xsysd->static_xdev_count);

		if (ret > 0) {
			xsysd->static_xdev_count += ret;

			// Fill in the devices into the static xdev array.
			tbo->left = xsysd->static_xdevs[xsysd->static_xdev_count - 2];
			tbo->right = xsysd->static_xdevs[xsysd->static_xdev_count - 1];

			// Set the hand tracking roles to the gloves
			tbo->hand_tracking.unobstructed.left = tbo->left;
			tbo->hand_tracking.unobstructed.right = tbo->right;
			RIFT_DEBUG(rb, "Created ContactGlove devices for serial number %s", serial_number);
		} else {
			RIFT_ERROR(rb, "Failed to create ContactGlove devices with code %d.", ret);
			os_serial_destroy(contactglove_dongle_serial);
		}
	} else {
		RIFT_ERROR(rb, "Failed to open ContactGlove dongle serial device with code %d.", ret);
	}
#endif
}

static void
add_devices_to_constellation_tracker(struct rift_builder *rb)
{
	int ret = rift_add_to_constellation_tracker(rb->hmd, rb->constellation_tracker);
	if (ret != 0) {
		RIFT_ERROR(rb, "Failed to add Rift HMD to constellation tracker with code %d", ret);
	}

#ifdef XRT_BUILD_DRIVER_PSSENSE
	if (rb->pssense_left != NULL) {
		ret = pssense_add_to_constellation_tracker(rb->pssense_left, rb->constellation_tracker);
		if (ret != 0) {
			RIFT_ERROR(rb, "Failed to add PS Sense left controller to constellation tracker with code %d",
			           ret);
		}
	}

	if (rb->pssense_right != NULL) {
		ret = pssense_add_to_constellation_tracker(rb->pssense_right, rb->constellation_tracker);
		if (ret != 0) {
			RIFT_ERROR(rb, "Failed to add PS Sense right controller to constellation tracker with code %d",
			           ret);
		}
	}
#endif
}

static xrt_result_t
rift_open_system_impl(struct xrt_builder *xb,
                      cJSON *config,
                      struct xrt_prober *xp,
                      struct xrt_tracking_origin *origin,
                      struct xrt_system_devices *xsysd,
                      struct xrt_frame_context *xfctx,
                      struct t_builder_options *tbo)
{
	struct rift_builder *rb = rift_builder(xb);

	struct xrt_prober_device **xpdevs = NULL;
	size_t xpdev_count = 0;
	xrt_result_t xret = XRT_SUCCESS;
	int ret;
	(void)ret; // Avoid unused variable warning when sensors are disabled.

	DRV_TRACE_MARKER();

	xret = xrt_prober_lock_list(xp, &xpdevs, &xpdev_count);
	if (xret != XRT_SUCCESS) {
		goto unlock_and_fail;
	}

	enum rift_variant variant = RIFT_VARIANT_CV1;

	struct xrt_prober_device *head_xpdev =
	    u_builder_find_prober_device(xpdevs, xpdev_count, OCULUS_VR_VID, OCULUS_CV1_PID, XRT_BUS_TYPE_USB);

	// If there's no CV1, search for a DK2
	if (head_xpdev == NULL) {
		head_xpdev =
		    u_builder_find_prober_device(xpdevs, xpdev_count, OCULUS_VR_VID, OCULUS_DK2_PID, XRT_BUS_TYPE_USB);

		if (head_xpdev != NULL) {
			variant = RIFT_VARIANT_DK2;
		}
	}

	if (head_xpdev != NULL && rift_is_oculus(xp, head_xpdev)) {
		unsigned char serial_number[21] = {0};
		int result = xrt_prober_get_string_descriptor(xp, head_xpdev, XRT_PROBER_STRING_SERIAL_NUMBER,
		                                              serial_number, sizeof(serial_number));
		if (result < 0) {
			return -1;
		}

		struct os_hid_device *hmd_hid_dev = NULL;
		result = xrt_prober_open_hid_interface(xp, head_xpdev, 0, &hmd_hid_dev);
		if (result != 0) {
			return -1;
		}

		struct os_hid_device *radio_hid_dev = NULL;
		if (variant == RIFT_VARIANT_CV1) {
			result = xrt_prober_open_hid_interface(xp, head_xpdev, 1, &radio_hid_dev);
			if (result != 0) {
				return -1;
			}
		}

		struct xrt_device *xdevs[XRT_SYSTEM_MAX_DEVICES] = {0};
		int created_devices = rift_devices_create( //
		    hmd_hid_dev,                           //
		    radio_hid_dev,                         //
		    variant,                               //
		    (char *)serial_number,                 //
		    xfctx,                                 //
		    &rb->hmd,                              //
		    xdevs);                                //
		if (rb->hmd == NULL) {
			RIFT_ERROR(rb, "Rift HMD device creation failed");
			goto unlock_and_fail;
		}

		if (created_devices < 0) {
			RIFT_ERROR(rb, "Rift HMD device creation failed with code %d", created_devices);
			goto unlock_and_fail;
		}

		rb->timing_event_source = rift_hmd_get_timing_event_source(rb->hmd);

		// Just clamp instead of overflowing the buffer
		if (created_devices + (int)xsysd->static_xdev_count > XRT_SYSTEM_MAX_DEVICES) {
			created_devices = XRT_SYSTEM_MAX_DEVICES - (int)xsysd->static_xdev_count;
		}

		memcpy(xsysd->static_xdevs + xsysd->static_xdev_count, xdevs,
		       sizeof(struct xrt_device *) * created_devices);
		xsysd->static_xdev_count += (size_t)created_devices;

		for (int i = 0; i < created_devices; i++) {
			struct xrt_device *xdev = xdevs[i];
			switch (xdev->device_type) {
			case XRT_DEVICE_TYPE_HMD: tbo->head = xdev; break;
			case XRT_DEVICE_TYPE_LEFT_HAND_CONTROLLER: tbo->left = xdev; break;
			case XRT_DEVICE_TYPE_RIGHT_HAND_CONTROLLER: tbo->right = xdev; break;
			case XRT_DEVICE_TYPE_GAMEPAD: tbo->gamepad = xdev; break;
			default: break;
			}
		}
	}

	rift_open_pssense(rb, xfctx, xsysd, xp, xpdevs, xpdev_count, tbo);
	rift_open_contactglove(rb, xsysd, xp, xpdevs, xpdev_count, tbo);

	xret = xrt_prober_unlock_list(xp, &xpdevs);
	if (xret != XRT_SUCCESS) {
		goto fail;
	}

#ifdef XRT_BUILD_DRIVER_RIFT_SENSOR
	ret = rift_sensor_context_create(&rb->sensor_context, xfctx);
	if (ret != 0) {
		RIFT_WARN(rb, "Rift sensor context creation failed with code %d", ret);
	}

	uint32_t sensor_count = 0;
	bool have_applicable_sensor = false;
	if (rb->sensor_context) {
		ret = rift_sensor_context_get_sensors(rb->sensor_context, &rb->sensors, &sensor_count);
		if (ret < 0) {
			RIFT_WARN(rb, "Rift sensor context get sensors failed with code: %u", sensor_count);
		}

		// See if any sensors are applicable
		for (uint32_t i = 0; i < sensor_count; i++) {
			struct rift_sensor *sensor = rb->sensors[i];
			enum rift_variant sensor_variant = rift_sensor_get_variant(sensor);
			if (sensor_variant != variant) {
				continue;
			}

			have_applicable_sensor = true;
			break;
		}
	}

	uint8_t radio_id[5] = {0};
	if (rb->hmd) {
		rift_get_radio_id(rb->hmd, radio_id);
	}

	// Only do constellation tracking if we have applicable sensors.
	if (have_applicable_sensor && debug_get_bool_option_rift_prober_constellation_tracking()) {
		ret = rift_sensor_context_start(rb->sensor_context);
		if (ret != 0) {
			RIFT_WARN(rb, "Rift sensor context start failed with code %d", ret);
		}

		rb->blobwatches = U_TYPED_ARRAY_CALLOC(struct t_blobwatch *, sensor_count);
		rb->blobwatch_debug_sinks = U_TYPED_ARRAY_CALLOC(struct u_sink_debug, sensor_count);

		struct t_constellation_tracker_params constellation_tracker_params = {
		    .flags = T_CONSTELLATION_TRACKER_FLAGS_NONE,
		    .num_mosaics = 1,
		};

		struct t_constellation_tracker_camera_mosaic *mosaic = &constellation_tracker_params.mosaics[0];

		for (uint32_t i = 0; i < sensor_count && i < XRT_TRACKING_MAX_CAMS; i++) {
			struct rift_sensor *sensor = rb->sensors[i];

			enum rift_variant sensor_variant = rift_sensor_get_variant(sensor);
			if (sensor_variant != variant) {
				continue;
			}

			struct t_camera_calibration calibration;
			rift_sensor_get_calibration(sensor, &calibration);

			mosaic->cameras[mosaic->num_cameras++] = (struct t_constellation_tracker_camera){
			    .calibration = calibration,
			    .has_concrete_pose = false,
			    .pose_in_origin = XRT_POSE_IDENTITY,
			};

			RIFT_DEBUG(rb, "Rift sensor %u added to constellation tracker at index %zu", i,
			           mosaic->num_cameras - 1);
		}

		ret = t_constellation_tracker_create(xfctx, &constellation_tracker_params, &rb->constellation_tracker);
		if (ret != 0) {
			RIFT_ERROR(rb, "Failed to create Rift constellation tracker with code %d", ret);
			goto unlock_and_fail;
		}

		add_devices_to_constellation_tracker(rb);

		for (uint32_t i = 0; i < sensor_count && rb->num_sensors < mosaic->num_cameras; i++) {
			struct rift_sensor *sensor = rb->sensors[i];

			enum rift_variant sensor_variant = rift_sensor_get_variant(sensor);
			if (sensor_variant != variant) {
				continue;
			}

			rift_sensor_setup_frame_timestamp_callback(sensor, rb->hmd);

			struct u_sink_debug *debug_sink = &rb->blobwatch_debug_sinks[rb->num_sensors];
			struct t_blobwatch **blobwatch = &rb->blobwatches[rb->num_sensors];

			u_sink_debug_init(debug_sink);

			struct t_blob_sink *blob_sink = mosaic->cameras[rb->num_sensors].blob_sink;
			u_sink_blob_visualizer_create(xfctx, blob_sink, debug_sink, RIFT_SENSOR_WIDTH,
			                              RIFT_SENSOR_HEIGHT, &blob_sink);

			struct xrt_frame_sink *frame_sink;
			struct t_rift_blobwatch_params params = {
			    .pixel_threshold = variant == RIFT_VARIANT_CV1 ? RIFT_BLOBWATCH_PIXEL_THRESHOLD_CV1
			                                                   : RIFT_BLOBWATCH_PIXEL_THRESHOLD_DK2,
			    .blob_required_threshold = RIFT_BLOBWATCH_BLOB_REQUIRED_THRESHOLD,
			    .max_match_dist = RIFT_BLOBWATCH_DEFAULT_MAX_MATCH_DIST,
			    .max_blob_width = RIFT_BLOBWATCH_DEFAULT_MAX_BLOB_WIDTH,
			    .camera_calibration = {0},
			};
			rift_sensor_get_calibration(sensor, &params.camera_calibration);

			ret = t_rift_blobwatch_create(&params, xfctx, blob_sink, &frame_sink, blobwatch);
			if (ret != 0) {
				RIFT_WARN(rb, "Failed to create Rift blobwatch for sensor %u with code %d", i, ret);
				continue;
			}

#ifdef XRT_FEATURE_RERUN
			t_rift_blobwatch_set_rerun_data(*blobwatch, rb->constellation_tracker, 0, rb->num_sensors);
#endif

			u_sink_create_format_converter(xfctx, XRT_FORMAT_L8, frame_sink, &frame_sink);

			if (!u_sink_simple_queue_create(xfctx, frame_sink, &frame_sink)) {
				RIFT_WARN(rb, "Failed to create Rift blobwatch queue for sensor %u", i);
				continue;
			}

			struct xrt_fs *fs = rift_sensor_get_frame_server(sensor);

			xret = xrt_fs_stream_start(fs, frame_sink, XRT_FS_CAPTURE_TYPE_TRACKING, 0);
			if (xret != XRT_SUCCESS) {
				RIFT_WARN(rb, "Failed to start Rift sensor frame server stream for sensor %u, got %d",
				          i, xret);
				continue;
			}

			u_var_add_sink_debug(rb, debug_sink, "Sensor Blobwatch");
			RIFT_DEBUG(rb, "Rift sensor %u initialized and streaming at index %zu", i, rb->num_sensors);

			// Mark that we've added another sensor
			rb->num_sensors++;

			// @note radio_id may be null on DK2, but DK2 doesn't use the radio ID so this is fine.
			ret = rift_sensor_enable_exposure_sync(rb->sensor_context, sensor, radio_id);
			if (ret != 0) {
				RIFT_WARN(rb, "Rift sensor context exposure sync enable failed with code %d", ret);
			}
		}
	}
#endif

	return XRT_SUCCESS;

unlock_and_fail:
	xret = xrt_prober_unlock_list(xp, &xpdevs);
	if (xret != XRT_SUCCESS) {
		return xret;
	}

	/* Fallthrough */
fail:
	return XRT_ERROR_DEVICE_CREATION_FAILED;
}

static void
rift_destroy(struct xrt_builder *xb)
{
	struct rift_builder *rb = rift_builder(xb);

#ifdef XRT_BUILD_DRIVER_RIFT_SENSOR
	if (rb->sensors) {
		free(rb->sensors);
	}

	if (rb->blobwatch_debug_sinks) {
		for (size_t i = 0; i < rb->num_sensors; i++) {
			u_sink_debug_destroy(&rb->blobwatch_debug_sinks[i]);
		}

		free(rb->blobwatch_debug_sinks);
		rb->blobwatch_debug_sinks = NULL;
	}

	if (rb->blobwatches) {
		// @note The blobwatches are freed when their frame nodes are destroyed, so we don't need to destroy
		// them here, just free the array.

		free(rb->blobwatches);
		rb->blobwatches = NULL;
	}
#endif

	u_var_remove_root(rb);

	free(xb);
}


/*
 *
 * 'Exported' functions.
 *
 */

struct xrt_builder *
rift_builder_create(void)
{
	struct rift_builder *rb = U_TYPED_CALLOC(struct rift_builder);

	rb->log_level = debug_get_log_option_rift_log();

	// xrt_builder fields.
	rb->base.base.estimate_system = rift_estimate_system;
	rb->base.base.open_system = t_builder_open_system_static_roles;
	rb->base.base.destroy = rift_destroy;
	rb->base.base.identifier = "rift";
	rb->base.base.name = "Oculus Rift";
	rb->base.base.driver_identifiers = driver_list;
	rb->base.base.driver_identifier_count = ARRAY_SIZE(driver_list);

	// t_builder fields.
	rb->base.open_system_static_roles = rift_open_system_impl;

	u_var_add_root(rb, "Rift Builder", false);
	u_var_add_log_level(rb, &rb->log_level, "Log Level");

	return &rb->base.base;
}
