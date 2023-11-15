// Copyright 2023, Alex Badics
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Driver for the Rokid Air and Max devices.
 *
 *
 * @author Alex Badics <admin@stickman.hu>
 * @ingroup drv_rokid
 */

#include "rokid_interface.h"

#include "xrt/xrt_defines.h"
#include "xrt/xrt_device.h"
#include "xrt/xrt_prober.h"

#include "os/os_threading.h"
#include "os/os_time.h"

#include "math/m_api.h"
#include "math/m_imu_3dof.h"
#include "math/m_mathinclude.h"
#include "math/m_predict.h"

#include "util/u_debug.h"
#include "util/u_device.h"
#include "util/u_distortion_mesh.h"
#include "util/u_trace_marker.h"
#include "util/u_var.h"

#ifdef XRT_OS_LINUX
#include "util/u_linux.h"
#endif



#include <libusb.h>


/*
 *
 * Defines and "normal" structs
 *
 */

#define ROKID_USB_INTERFACE_NUM 2
#define ROKID_INTERRUPT_IN_ENDPOINT 0x82
#define ROKID_USB_BUFFER_LEN 0x40
#define ROKID_USB_TRANSFER_TIMEOUT_MS 1000

struct rokid_fusion
{
	struct os_mutex mutex;
	struct m_imu_3dof i3dof;
	struct xrt_space_relation last_relation;
	uint64_t last_update;
	struct xrt_vec3 last_gyro;
	struct xrt_vec3 last_accel;
	uint64_t gyro_ts_device;
	uint64_t accel_ts_device;

	bool initialized;
};

struct rokid_hmd
{
	struct xrt_device base;
	enum u_logging_level log_level;

	struct os_thread_helper usb_thread;

	libusb_context *usb_ctx;
	libusb_device_handle *usb_dev;

	struct rokid_fusion fusion;
};


/*
 *
 * Packed structs for USB communication
 *
 */

#if defined(__GNUC__)
#define ROKID_PACKED __attribute__((packed))
#else
#define ROKID_PACKED
#endif /* __GNUC__ */

#if defined(_MSC_VER)
#pragma pack(push, 1)
#endif

struct rokid_usb_packed_vec
{
	float x;
	float y;
	float z;
} ROKID_PACKED;

struct rokid_usb_pkt_combined
{
	uint8_t packet_type;
	uint64_t timestamp;
	struct rokid_usb_packed_vec accel;
	struct rokid_usb_packed_vec gyro;
	struct rokid_usb_packed_vec magnetometer;
	uint8_t keys_pressed;
	uint8_t proxy_sensor;
	uint8_t _unknown_0;
	uint64_t vsync_timestamp;
	uint8_t _unknown_1[3];
	uint8_t display_brightness;
	uint8_t volume;
	uint8_t _unknown_2[3];
} ROKID_PACKED;


struct rokid_usb_pkt_sensor
{
	uint8_t packet_type;
	uint8_t sensor_type;
	uint32_t seq;
	uint8_t _unknown_0[3];
	uint64_t timestamp;
	uint8_t _unknown_1[4];
	struct rokid_usb_packed_vec vector;
	uint8_t _unknown_2[31];
} ROKID_PACKED;

#if defined(_MSC_VER)
#pragma pack(pop)
#endif

/*
 *
 * Helper functions
 *
 */

DEBUG_GET_ONCE_LOG_OPTION(rokid_log, "ROKID_LOG", U_LOGGING_WARN)

#define ROKID_TRACE(p, ...) U_LOG_XDEV_IFL_T(&rokid->base, rokid->log_level, __VA_ARGS__)
#define ROKID_DEBUG(p, ...) U_LOG_XDEV_IFL_D(&rokid->base, rokid->log_level, __VA_ARGS__)
#define ROKID_INFO(p, ...) U_LOG_XDEV_IFL_I(&rokid->base, rokid->log_level, __VA_ARGS__)
#define ROKID_ERROR(p, ...) U_LOG_XDEV_IFL_E(&rokid->base, rokid->log_level, __VA_ARGS__)


static struct xrt_vec3
rokid_convert_vector(struct rokid_usb_packed_vec *v)
{
	struct xrt_vec3 result = {
	    .x = v->x,
	    .y = v->y,
	    .z = v->z,
	};
	return result;
}

// Casting helper function
static inline struct rokid_hmd *
rokid_hmd(struct xrt_device *xdev)
{
	return (struct rokid_hmd *)xdev;
}

/*
 *
 * Fusion-related functions
 *
 */

static void
rokid_fusion_create(struct rokid_fusion *fusion)
{
	os_mutex_init(&fusion->mutex);
	m_imu_3dof_init(&fusion->i3dof, M_IMU_3DOF_USE_GRAVITY_DUR_300MS);
	struct xrt_space_relation zero_relation = XRT_SPACE_RELATION_ZERO;
	fusion->last_relation = zero_relation;
	fusion->initialized = true;
}

static void
rokid_fusion_parse_usb_packet(struct rokid_fusion *fusion, unsigned char usb_buffer[ROKID_USB_BUFFER_LEN])
{
	switch (usb_buffer[0]) {
	case 4: {
		// Old-style packet, where we get one packet for each sensor
		// Order is usually the same, but not guaranteed, because packet losses.
		struct rokid_usb_pkt_sensor *packet = (struct rokid_usb_pkt_sensor *)usb_buffer;
		switch (packet->sensor_type) {
		case 1: {
			fusion->last_accel = rokid_convert_vector(&packet->vector);
			fusion->accel_ts_device = packet->timestamp;
			break;
		}
		case 2: {
			fusion->last_gyro = rokid_convert_vector(&packet->vector);
			fusion->gyro_ts_device = packet->timestamp;
			break;
		}
		}
		break;
	}
	case 17: {
		// New-style combined packet
		struct rokid_usb_pkt_combined *packet = (struct rokid_usb_pkt_combined *)usb_buffer;
		fusion->last_gyro = rokid_convert_vector(&packet->gyro);
		fusion->last_accel = rokid_convert_vector(&packet->accel);
		fusion->gyro_ts_device = packet->timestamp;
		fusion->accel_ts_device = packet->timestamp;
		break;
	}
	}

	// Only update fusion once we have data from both sensors for this timestamp
	if (fusion->gyro_ts_device == fusion->accel_ts_device) {
		uint64_t now = os_monotonic_get_ns();
		m_imu_3dof_update(&fusion->i3dof, now, &fusion->last_accel, &fusion->last_gyro);
		struct xrt_vec3 angular_velocity_ws;
		math_quat_rotate_vec3(&fusion->i3dof.rot, &fusion->last_gyro, &angular_velocity_ws);
		fusion->last_relation.relation_flags = XRT_SPACE_RELATION_ORIENTATION_VALID_BIT |
		                                       XRT_SPACE_RELATION_ANGULAR_VELOCITY_VALID_BIT |
		                                       XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT;
		fusion->last_relation.pose.orientation = fusion->i3dof.rot;
		fusion->last_relation.angular_velocity = angular_velocity_ws;
		fusion->last_update = now;
	}
}


static void
rokid_fusion_get_pose(struct rokid_fusion *fusion, uint64_t at_timestamp_ns, struct xrt_space_relation *out_relation)
{
	if (at_timestamp_ns > fusion->last_update) {
		double time_diff = time_ns_to_s(at_timestamp_ns - fusion->last_update);
		if (time_diff > 0.1) {
			time_diff = 0.1;
		}
		m_predict_relation(&fusion->last_relation, time_diff, out_relation);
	} else {
		*out_relation = fusion->last_relation;
	}
}

static void
rokid_fusion_destroy(struct rokid_fusion *fusion)
{
	os_mutex_destroy(&fusion->mutex);
	m_imu_3dof_close(&fusion->i3dof);

	fusion->initialized = false;
}

static void
rokid_fusion_add_vars(struct rokid_fusion *fusion, void *root)
{
	m_imu_3dof_add_vars(&fusion->i3dof, root, "fusion.");
	u_var_add_pose(root, &fusion->last_relation.pose, "last_pose");
	u_var_add_ro_vec3_f32(root, &fusion->last_gyro, "gyro");
	u_var_add_ro_vec3_f32(root, &fusion->last_accel, "accel");
	u_var_add_ro_u64(root, &fusion->last_update, "timestamp");
}


/*
 *
 * USB handling boilerplate
 *
 */

static void *
rokid_usb_thread(void *ptr)
{

	U_TRACE_SET_THREAD_NAME("Rokid USB thread");
	struct rokid_hmd *rokid = ptr;

#ifdef XRT_OS_LINUX
	// Try to raise priority of this thread, so we don't miss packets under load
	u_linux_try_to_set_realtime_priority_on_thread(U_LOGGING_INFO, "Rokid USB thread");
#endif

	int last_libusb_result = LIBUSB_SUCCESS;

	os_thread_helper_lock(&rokid->usb_thread);
	while (os_thread_helper_is_running_locked(&rokid->usb_thread) && last_libusb_result == LIBUSB_SUCCESS) {
		os_thread_helper_unlock(&rokid->usb_thread);

		unsigned char usb_buffer[ROKID_USB_BUFFER_LEN] = {0};
		int read_length = 0;

		int last_libusb_result =
		    libusb_interrupt_transfer(rokid->usb_dev, ROKID_INTERRUPT_IN_ENDPOINT, usb_buffer,
		                              sizeof(usb_buffer), &read_length, ROKID_USB_TRANSFER_TIMEOUT_MS);
		if (last_libusb_result == LIBUSB_SUCCESS) {
			os_mutex_lock(&rokid->fusion.mutex);
			rokid_fusion_parse_usb_packet(&rokid->fusion, usb_buffer);
			os_mutex_unlock(&rokid->fusion.mutex);
		}

		os_thread_helper_lock(&rokid->usb_thread);
	}
	os_thread_helper_unlock(&rokid->usb_thread);
	if (last_libusb_result == LIBUSB_SUCCESS) {
		ROKID_INFO(rokid, "Usb thread exiting normally");
	} else {
		ROKID_ERROR(rokid, "Exiting on libusb error %s", libusb_strerror(last_libusb_result));
	}

	return NULL;
}

// Returns 0-255 for valid display modes (0 is 2D, 1 is 3D, others are more special modes),
// and -1 in case of error
static int
rokid_hmd_get_display_mode(struct rokid_hmd *rokid)
{
	uint8_t data[ROKID_USB_BUFFER_LEN] = {0};
	int res = libusb_control_transfer(rokid->usb_dev,
	                                  LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
	                                  0x81, // request type = get display mode,
	                                  0,    // wValue
	                                  0x1,  // wIndex
	                                  data, // data is mandatory for some reason,
	                                  sizeof(data), ROKID_USB_TRANSFER_TIMEOUT_MS);
	if (res < 0) {
		ROKID_ERROR(rokid, "Failed to set glasses to SBS mode");
		return -1;
	}
	return data[1];
}

static bool
rokid_hmd_set_display_mode(struct rokid_hmd *rokid, uint16_t mode)
{
	uint8_t data[1] = {1};
	int res = libusb_control_transfer(rokid->usb_dev,
	                                  LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
	                                  0x1,  // request type = set display mode,
	                                  mode, // display type
	                                  0x1,  // wIndex is fixed
	                                  data, // data is mandatory for some reason,
	                                  sizeof(data), ROKID_USB_TRANSFER_TIMEOUT_MS);
	if (res < 0) {
		ROKID_ERROR(rokid, "Failed to set glasses to SBS mode");
		return false;
	}
	return true;
}

static bool
rokid_hmd_usb_init(struct rokid_hmd *rokid, struct xrt_prober_device *prober_device)
{
	int res;

	res = libusb_init(&rokid->usb_ctx);
	if (res < 0) {
		ROKID_ERROR(rokid, "Failed to init USB");
		return false;
	}

	rokid->usb_dev =
	    libusb_open_device_with_vid_pid(rokid->usb_ctx, prober_device->vendor_id, prober_device->product_id);
	if (rokid->usb_dev == NULL) {
		ROKID_ERROR(rokid, "Failed to open USB device");
		return false;
	}

	struct libusb_device_descriptor usb_desc;
	res = libusb_get_device_descriptor(libusb_get_device(rokid->usb_dev), &usb_desc);
	if (res < 0) {
		ROKID_ERROR(rokid, "Failed to get descriptor");
		return false;
	}

	res = libusb_get_string_descriptor_ascii(rokid->usb_dev, usb_desc.iProduct, (unsigned char *)rokid->base.str,
	                                         XRT_DEVICE_NAME_LEN - 1);
	if (res < 0) {
		ROKID_ERROR(rokid, "Failed to get product name");
		return false;
	}
	res = libusb_get_string_descriptor_ascii(rokid->usb_dev, usb_desc.iSerialNumber,
	                                         (unsigned char *)rokid->base.serial, XRT_DEVICE_NAME_LEN - 1);
	if (res < 0) {
		ROKID_ERROR(rokid, "Failed to get serial");
		return false;
	}

	res = libusb_set_auto_detach_kernel_driver(rokid->usb_dev, 1);
	if (res < 0) {
		ROKID_ERROR(rokid, "Failed to set autodetach on USB device");
		return false;
	}

	res = libusb_claim_interface(rokid->usb_dev, ROKID_USB_INTERFACE_NUM);
	if (res < 0) {
		ROKID_ERROR(rokid, "Failed to claim USB status interface");
		return false;
	}

	return true;
}

/*
 *
 * HMD entry points
 *
 */

static void
rokid_hmd_destroy(struct xrt_device *xdev)
{
	// This function has to handle partial initializations,
	// as it can be called from the middle of the constructor.

	struct rokid_hmd *rokid = rokid_hmd(xdev);
	if (rokid->usb_thread.initialized) {
		os_thread_helper_destroy(&rokid->usb_thread);
	}

	if (rokid->fusion.initialized) {
		rokid_fusion_destroy(&rokid->fusion);
	}
	// Remove the variable tracking.
	u_var_remove_root(rokid);

	u_device_free(&rokid->base);
}

static void
rokid_hmd_get_tracked_pose(struct xrt_device *xdev,
                           enum xrt_input_name name,
                           uint64_t at_timestamp_ns,
                           struct xrt_space_relation *out_relation)
{
	struct rokid_hmd *rokid = rokid_hmd(xdev);

	if (name != XRT_INPUT_GENERIC_HEAD_POSE) {
		ROKID_ERROR(rokid, "unknown input name");
		return;
	}
	os_mutex_lock(&rokid->fusion.mutex);
	rokid_fusion_get_pose(&rokid->fusion, at_timestamp_ns, out_relation);
	os_mutex_unlock(&rokid->fusion.mutex);
}

static struct xrt_device *
rokid_hmd_create(struct xrt_prober_device *prober_device)
{
	// This indicates you won't be using Monado's built-in tracking algorithms.
	enum u_device_alloc_flags flags =
	    (enum u_device_alloc_flags)(U_DEVICE_ALLOC_HMD | U_DEVICE_ALLOC_TRACKING_NONE);

	struct rokid_hmd *rokid = U_DEVICE_ALLOCATE(struct rokid_hmd, flags, 1, 0);
	rokid->log_level = debug_get_log_option_rokid_log();

	ROKID_DEBUG(rokid, "Starting Rokid driver instance");

	rokid_fusion_create(&rokid->fusion);

	if (os_thread_helper_init(&rokid->usb_thread) != 0) {
		ROKID_ERROR(hmd, "Failed to init USB thread");
		goto cleanup;
	}
	os_thread_helper_name(&rokid->usb_thread, "Rokid USB thread");

	// This also sets base.str used below.
	if (!rokid_hmd_usb_init(rokid, prober_device)) {
		goto cleanup;
	}
	bool is_rokid_max = strstr(rokid->base.str, "Max");
	ROKID_INFO(rokid, "Rokid model: %s", (is_rokid_max ? "Max" : "Air"));

	// This list should be ordered, most preferred first.
	size_t idx = 0;
	rokid->base.hmd->blend_modes[idx++] = XRT_BLEND_MODE_OPAQUE;
	rokid->base.hmd->blend_mode_count = idx;

	rokid->base.update_inputs = u_device_noop_update_inputs;
	rokid->base.get_tracked_pose = rokid_hmd_get_tracked_pose;
	rokid->base.get_view_poses = u_device_get_view_poses;
	rokid->base.destroy = rokid_hmd_destroy;

	// Setup input.
	rokid->base.name = XRT_DEVICE_GENERIC_HMD;
	rokid->base.device_type = XRT_DEVICE_TYPE_HMD;
	rokid->base.inputs[0].name = XRT_INPUT_GENERIC_HEAD_POSE;
	rokid->base.orientation_tracking_supported = true;
	rokid->base.position_tracking_supported = false;

	// Set up display details
	// refresh rate
	rokid->base.hmd->screens[0].nominal_frame_interval_ns = time_s_to_ns(1.0f / 60.0f);

	const float quarter_vFOV = 0.25f * (is_rokid_max ? 46.0f : 40.0f) * ((float)M_PI / 180.0f);
	const float quarter_hFOV = quarter_vFOV * 16.0f / 9.0f;
	const float display_tilt = (is_rokid_max ? 0.035f : 0.011f);
	struct xrt_fov fov = {
	    .angle_left = -quarter_hFOV,
	    .angle_right = quarter_hFOV,
	    .angle_up = quarter_vFOV + display_tilt,
	    .angle_down = -quarter_vFOV + display_tilt,
	};
	rokid->base.hmd->distortion.fov[1] = rokid->base.hmd->distortion.fov[0] = fov;

	const int panel_w = 1920;
	const int panel_h = 1080;

	// Single "screen" (always the case)
	rokid->base.hmd->screens[0].w_pixels = panel_w * 2;
	rokid->base.hmd->screens[0].h_pixels = panel_h;

	// Left, Right
	for (uint8_t eye = 0; eye < 2; ++eye) {
		rokid->base.hmd->views[eye].display.w_pixels = panel_w;
		rokid->base.hmd->views[eye].display.h_pixels = panel_h;
		rokid->base.hmd->views[eye].viewport.y_pixels = 0;
		rokid->base.hmd->views[eye].viewport.w_pixels = panel_w;
		rokid->base.hmd->views[eye].viewport.h_pixels = panel_h;
		rokid->base.hmd->views[eye].rot = u_device_rotation_ident;
	}
	// left eye starts at x=0, right eye starts at x=panel_width
	rokid->base.hmd->views[0].viewport.x_pixels = 0;
	rokid->base.hmd->views[1].viewport.x_pixels = panel_w;

	// Distortion information, fills in xdev->compute_distortion().
	u_distortion_mesh_set_none(&rokid->base);

	// Setup variable tracker: Optional but useful for debugging
	u_var_add_root(rokid, "Rokid", true);
	u_var_add_log_level(rokid, &rokid->log_level, "log_level");
	rokid_fusion_add_vars(&rokid->fusion, rokid);

	if (os_thread_helper_start(&rokid->usb_thread, rokid_usb_thread, rokid) != 0) {
		ROKID_ERROR(hmd, "Failed to start USB thread");
		goto cleanup;
	}

	int display_mode = rokid_hmd_get_display_mode(rokid);
	if (display_mode < 0) {
		ROKID_ERROR(hmd, "Failed to get display mode");
		goto cleanup;
	}
	if (display_mode != 1) {
		ROKID_INFO(rokid, "Setting Rokid display to SBS mode");
		if (!rokid_hmd_set_display_mode(rokid, 1)) {
			ROKID_ERROR(hmd, "Failed to get display mode");
			goto cleanup;
		}
		os_nanosleep((int64_t)3 * (int64_t)U_TIME_1S_IN_NS);
	}

	ROKID_INFO(rokid, "Started Rokid driver instance");

	return &rokid->base;

cleanup:
	rokid_hmd_destroy(&rokid->base);
	return NULL;
}


int
rokid_found(struct xrt_prober *xp,
            struct xrt_prober_device **devices,
            size_t device_count,
            size_t index,
            cJSON *attached_data,
            struct xrt_device **out_xdev)
{
	struct xrt_device *device = rokid_hmd_create(devices[index]);
	if (!device) {
		return 0;
	}
	out_xdev[0] = device;
	return 1;
}
