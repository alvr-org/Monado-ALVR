// Copyright 2022-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Builder for Lighthouse-tracked devices (vive, index, tundra trackers, etc.)
 * @author Moses Turner <moses@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup xrt_iface
 */

#include "tracking/t_hand_tracking.h"
#include "tracking/t_tracking.h"

#include "xrt/xrt_config_drivers.h"
#include "xrt/xrt_device.h"
#include "xrt/xrt_prober.h"

#include "util/u_builders.h"
#include "util/u_config_json.h"
#include "util/u_debug.h"
#include "util/u_device.h"
#include "util/u_sink.h"
#include "util/u_system_helpers.h"

#include "target_builder_interface.h"

#include "vive/vive_common.h"
#include "vive/vive_config.h"
#include "vive/vive_calibration.h"
#include "vive/vive_builder.h"
#include "vive/vive_device.h"
#include "vive/vive_source.h"
#include "v4l2/v4l2_interface.h"

#include "xrt/xrt_frameserver.h"
#include "xrt/xrt_results.h"
#include "xrt/xrt_tracking.h"


#include <assert.h>

#ifdef XRT_BUILD_DRIVER_VIVE
#include "vive/vive_prober.h"
#endif

#ifdef XRT_BUILD_DRIVER_SURVIVE
#include "survive/survive_interface.h"
#endif

#ifdef XRT_BUILD_DRIVER_HANDTRACKING
#include "ht/ht_interface.h"
#include "ht_ctrl_emu/ht_ctrl_emu_interface.h"
#include "multi_wrapper/multi.h"
#include "../../tracking/hand/mercury/hg_interface.h"
#endif

#ifdef XRT_BUILD_DRIVER_OPENGLOVES
#include "opengloves/opengloves_interface.h"
#endif

#if defined(XRT_BUILD_DRIVER_SURVIVE)
#define DEFAULT_DRIVER "survive"
#else
#define DEFAULT_DRIVER "vive"
#endif

DEBUG_GET_ONCE_LOG_OPTION(lh_log, "LH_LOG", U_LOGGING_WARN)
DEBUG_GET_ONCE_OPTION(lh_impl, "LH_DRIVER", DEFAULT_DRIVER)
DEBUG_GET_ONCE_BOOL_OPTION(vive_slam, "VIVE_SLAM", false)
DEBUG_GET_ONCE_TRISTATE_OPTION(lh_handtracking, "LH_HANDTRACKING")

#define LH_TRACE(...) U_LOG_IFL_T(debug_get_log_option_lh_log(), __VA_ARGS__)
#define LH_DEBUG(...) U_LOG_IFL_D(debug_get_log_option_lh_log(), __VA_ARGS__)
#define LH_INFO(...) U_LOG_IFL_I(debug_get_log_option_lh_log(), __VA_ARGS__)
#define LH_WARN(...) U_LOG_IFL_W(debug_get_log_option_lh_log(), __VA_ARGS__)
#define LH_ERROR(...) U_LOG_IFL_E(debug_get_log_option_lh_log(), __VA_ARGS__)
#define LH_ASSERT(predicate, ...)                                                                                      \
	do {                                                                                                           \
		bool p = predicate;                                                                                    \
		if (!p) {                                                                                              \
			U_LOG(U_LOGGING_ERROR, __VA_ARGS__);                                                           \
			assert(false && "LH_ASSERT failed: " #predicate);                                              \
			exit(EXIT_FAILURE);                                                                            \
		}                                                                                                      \
	} while (false);
#define LH_ASSERT_(predicate) LH_ASSERT(predicate, "Assertion failed " #predicate)

static const char *driver_list[] = {
#ifdef XRT_BUILD_DRIVER_SURVIVE
    "survive",
#endif

#ifdef XRT_BUILD_DRIVER_VIVE
    "vive",
#endif

#ifdef XRT_BUILD_DRIVER_OPENGLOVES
    "opengloves",
#endif
};

enum lighthouse_driver
{
	DRIVER_VIVE,
	DRIVER_SURVIVE,
	DRIVER_STEAMVR
};

struct lighthouse_system
{
	struct u_builder base;

	struct xrt_frame_context *xfctx;
	enum lighthouse_driver driver; //!< Which lighthouse implementation we are using
	bool is_valve_index; //!< Is our HMD a Valve Index? If so, try to set up hand-tracking and SLAM as needed
	struct vive_tracking_status vive_tstatus; //!< Visual tracking status for Index under Vive driver
	struct xrt_fs *xfs;                       //!< Frameserver for Valve Index camera, if we have one.
	struct vive_config *hmd_config;
	struct t_slam_calibration slam_calib; //!< Calibration data for SLAM
};


/*
 *
 * Helper tracking setup functions.
 *
 */

static uint32_t
get_selected_mode(struct xrt_fs *xfs)
{
	struct xrt_fs_mode *modes = NULL;
	uint32_t count = 0;
	xrt_fs_enumerate_modes(xfs, &modes, &count);

	LH_ASSERT(count != 0, "No stream modes found in Index camera");

	uint32_t selected_mode = 0;
	for (uint32_t i = 0; i < count; i++) {
		if (modes[i].format == XRT_FORMAT_YUYV422) {
			selected_mode = i;
			break;
		}
	}

	free(modes);
	return selected_mode;
}

static void
on_video_device(struct xrt_prober *xp,
                struct xrt_prober_device *pdev,
                const char *product,
                const char *manufacturer,
                const char *serial,
                void *ptr)
{
	struct lighthouse_system *lhs = (struct lighthouse_system *)ptr;

	// Hardcoded for the Index.
	if (product != NULL && manufacturer != NULL) {
		if ((strcmp(product, "3D Camera") == 0) && (strcmp(manufacturer, "Etron Technology, Inc.") == 0)) {
			xrt_prober_open_video_device(xp, pdev, lhs->xfctx, &lhs->xfs);
			return;
		}
	}
}

static struct xrt_slam_sinks *
valve_index_slam_track(struct vive_device *vive_head,
                       struct xrt_frame_context *xfctx,
                       struct t_slam_calibration *slam_calib)
{
	struct xrt_slam_sinks *sinks = NULL;

#ifdef XRT_FEATURE_SLAM
	struct t_slam_tracker_config config = {0};
	t_slam_fill_default_config(&config);
	config.cam_count = 2;
	config.slam_calib = slam_calib;

	int create_status = t_slam_create(xfctx, &config, &vive_head->tracking.slam, &sinks);
	if (create_status != 0) {
		return NULL;
	}

	int start_status = t_slam_start(vive_head->tracking.slam);
	if (start_status != 0) {
		return NULL;
	}

	LH_INFO("Lighthouse HMD SLAM tracker successfully started");
#endif

	return sinks;
}

static bool
valve_index_hand_track(struct lighthouse_system *lhs,
                       struct xrt_device *head,
                       struct xrt_frame_context *xfctx,
                       struct xrt_pose head_in_left_cam,
                       struct t_stereo_camera_calibration *stereo_calib,
                       struct xrt_slam_sinks **out_sinks,
                       struct xrt_device **out_devices)
{
#ifdef XRT_BUILD_DRIVER_HANDTRACKING
	struct xrt_device *two_hands[2] = {NULL};
	struct xrt_slam_sinks *sinks = NULL;

	LH_ASSERT_(stereo_calib != NULL);

	// zero-initialized out of paranoia
	struct t_camera_extra_info info = {0};

	info.views[0].camera_orientation = CAMERA_ORIENTATION_0;
	info.views[1].camera_orientation = CAMERA_ORIENTATION_0;

	info.views[0].boundary_type = HT_IMAGE_BOUNDARY_CIRCLE;
	info.views[1].boundary_type = HT_IMAGE_BOUNDARY_CIRCLE;


	//!@todo This changes by like 50ish pixels from device to device. For now, the solution is simple: just
	//! make the circle a bit bigger than we'd like.
	// Maybe later we can do vignette calibration? Write a tiny optimizer that tries to fit Index's
	// gradient? Unsure.
	info.views[0].boundary.circle.normalized_center.x = 0.5f;
	info.views[0].boundary.circle.normalized_center.y = 0.5f;

	info.views[1].boundary.circle.normalized_center.x = 0.5f;
	info.views[1].boundary.circle.normalized_center.y = 0.5f;

	info.views[0].boundary.circle.normalized_radius = 0.55;
	info.views[1].boundary.circle.normalized_radius = 0.55;

	struct xrt_device *ht_device = NULL;
	int create_status = ht_device_create( //
	    xfctx,                            //
	    stereo_calib,                     //
	    info,                             //
	    &sinks,                           //
	    &ht_device);
	if (create_status != 0) {
		LH_WARN("Failed to create hand tracking device\n");
		return false;
	}

	ht_device = multi_create_tracking_override( //
	    XRT_TRACKING_OVERRIDE_ATTACHED,         //
	    ht_device,                              //
	    head,                                   //
	    XRT_INPUT_GENERIC_HEAD_POSE,            //
	    &head_in_left_cam);                     //

	int created_devices = cemu_devices_create( //
	    head,                                  //
	    ht_device,                             //
	    two_hands);                            //
	if (created_devices != 2) {
		LH_WARN("Unexpected amount of hand devices created (%d)\n", create_status);
		xrt_device_destroy(&ht_device);
		return false;
	}

	LH_INFO("Hand tracker successfully created\n");

	*out_sinks = sinks;
	out_devices[0] = two_hands[0];
	out_devices[1] = two_hands[1];
	return true;
#endif

	return false;
}


/*
 *
 * Member functions.
 *
 */

static xrt_result_t
lighthouse_estimate_system(struct xrt_builder *xb,
                           cJSON *config,
                           struct xrt_prober *xp,
                           struct xrt_builder_estimate *estimate)
{
	struct lighthouse_system *lhs = (struct lighthouse_system *)xb;
#ifdef XRT_BUILD_DRIVER_VIVE
	bool have_vive_drv = true;
#else
	bool have_vive_drv = false;
#endif

#ifdef XRT_BUILD_DRIVER_SURVIVE
	bool have_survive_drv = true;
#else
	bool have_survive_drv = false;
#endif

	const char *drv = debug_get_option_lh_impl();
	if (strcmp(drv, "steamvr") == 0) {
		lhs->driver = DRIVER_STEAMVR;
	} else if (have_survive_drv && strcmp(drv, "survive") == 0) {
		lhs->driver = DRIVER_SURVIVE;
	} else if (have_vive_drv && strcmp(drv, "vive") == 0) {
		lhs->driver = DRIVER_VIVE;
	} else {
		const char *selected;
		if (have_survive_drv) {
			selected = "survive";
			lhs->driver = DRIVER_SURVIVE;
		} else if (have_vive_drv) {
			selected = "vive";
			lhs->driver = DRIVER_VIVE;
		} else {
			LH_ASSERT_(false);
		}
		LH_WARN("Requested driver %s was not available, so we went with %s instead", drv, selected);
	}

	// Error on wrong configuration.
	if (lhs->driver == DRIVER_STEAMVR) {
		LH_ERROR("Use new env variable STEAMVR_LH_ENABLE=true to enable SteamVR driver");
		return XRT_ERROR_PROBING_FAILED;
	}

#ifdef XRT_BUILD_DRIVER_HANDTRACKING
	bool have_hand_tracking = true;
#else
	bool have_hand_tracking = false;
#endif

	bool have_6dof = lhs->driver != DRIVER_VIVE;

	return vive_builder_estimate( //
	    xp,                       // xp
	    have_6dof,                // have_6dof
	    have_hand_tracking,       // have_hand_tracking
	    &lhs->is_valve_index,     // out_have_valve_index
	    estimate);                // out_estimate
}

// If the HMD is a Valve Index, decide if we want visual (HT/Slam) trackers, and if so set them up.
static bool
valve_index_setup_visual_trackers(struct lighthouse_system *lhs,
                                  struct xrt_device *head,
                                  struct vive_device *vive_head,
                                  struct xrt_frame_context *xfctx,
                                  struct xrt_prober *xp,
                                  struct xrt_slam_sinks *out_sinks,
                                  struct xrt_device **out_devices)
{
	bool slam_enabled = lhs->vive_tstatus.slam_enabled;
	bool hand_enabled = lhs->vive_tstatus.hand_enabled;

	// Hand tracking calibration
	struct t_stereo_camera_calibration *stereo_calib = NULL;
	struct xrt_pose head_in_left_cam;
	vive_get_stereo_camera_calibration(lhs->hmd_config, &stereo_calib, &head_in_left_cam);

	// SLAM calibration
	lhs->slam_calib.cam_count = 2;
	vive_get_slam_cams_calib(lhs->hmd_config, &lhs->slam_calib.cams[0], &lhs->slam_calib.cams[1]);
	vive_get_slam_imu_calibration(lhs->hmd_config, &lhs->slam_calib.imu);

	// Initialize SLAM tracker
	struct xrt_slam_sinks *slam_sinks = NULL;
	if (slam_enabled) {
		LH_ASSERT_(lhs->driver == DRIVER_VIVE);
		LH_ASSERT_(vive_head != NULL);

		slam_sinks = valve_index_slam_track(vive_head, xfctx, &lhs->slam_calib);
		if (slam_sinks == NULL) {
			lhs->vive_tstatus.slam_enabled = false;
			slam_enabled = false;
			LH_WARN("Unable to setup the SLAM tracker");
		}
	}

	// Initialize hand tracker
	struct xrt_slam_sinks *hand_sinks = NULL;
	struct xrt_device *hand_devices[2] = {NULL};
	if (hand_enabled) {
		bool success = valve_index_hand_track( //
		    lhs,                               //
		    head,                              //
		    xfctx,                             //
		    head_in_left_cam,                  //
		    stereo_calib,                      //
		    &hand_sinks,                       //
		    hand_devices);                     //
		if (!success) {
			lhs->vive_tstatus.hand_enabled = false;
			hand_enabled = false;
			LH_WARN("Unable to setup the hand tracker");
		}
	}

	t_stereo_camera_calibration_reference(&stereo_calib, NULL);

	if (lhs->driver == DRIVER_VIVE) { // Refresh trackers status in vive driver
		LH_ASSERT_(vive_head != NULL);
		vive_set_trackers_status(vive_head, lhs->vive_tstatus);
	}

	// Setup frame graph

	struct xrt_frame_sink *entry_left_sink = NULL;
	struct xrt_frame_sink *entry_right_sink = NULL;
	struct xrt_frame_sink *entry_sbs_sink = NULL;

	if (slam_enabled && hand_enabled) {
		u_sink_split_create(xfctx, slam_sinks->cams[0], hand_sinks->cams[0], &entry_left_sink);
		u_sink_split_create(xfctx, slam_sinks->cams[1], hand_sinks->cams[1], &entry_right_sink);
		u_sink_stereo_sbs_to_slam_sbs_create(xfctx, entry_left_sink, entry_right_sink, &entry_sbs_sink);
		u_sink_create_format_converter(xfctx, XRT_FORMAT_L8, entry_sbs_sink, &entry_sbs_sink);
	} else if (slam_enabled) {
		entry_left_sink = slam_sinks->cams[0];
		entry_right_sink = slam_sinks->cams[1];
		u_sink_stereo_sbs_to_slam_sbs_create(xfctx, entry_left_sink, entry_right_sink, &entry_sbs_sink);
		u_sink_create_format_converter(xfctx, XRT_FORMAT_L8, entry_sbs_sink, &entry_sbs_sink);
	} else if (hand_enabled) {
		entry_left_sink = hand_sinks->cams[0];
		entry_right_sink = hand_sinks->cams[1];
		u_sink_stereo_sbs_to_slam_sbs_create(xfctx, entry_left_sink, entry_right_sink, &entry_sbs_sink);
		u_sink_create_format_converter(xfctx, XRT_FORMAT_L8, entry_sbs_sink, &entry_sbs_sink);
	} else {
		LH_WARN("No visual trackers were set");
		return false;
	}
	//! @todo Using a single slot queue is wrong for SLAM
	u_sink_simple_queue_create(xfctx, entry_sbs_sink, &entry_sbs_sink);

	struct xrt_slam_sinks entry_sinks = {
	    .cam_count = 1,
	    .cams = {entry_sbs_sink},
	    .imu = slam_enabled ? slam_sinks->imu : NULL,
	    .gt = slam_enabled ? slam_sinks->gt : NULL,
	};

	*out_sinks = entry_sinks;
	if (hand_enabled) {
		out_devices[0] = hand_devices[0];
		out_devices[1] = hand_devices[1];
	}

	return true;
}



static bool
stream_data_sources(struct lighthouse_system *lhs,
                    struct vive_device *vive_head,
                    struct xrt_prober *xp,
                    struct xrt_slam_sinks sinks)
{
	// Open frame server
	xrt_prober_list_video_devices(xp, on_video_device, lhs);
	if (lhs->xfs == NULL) {
		LH_WARN("Couldn't find Index camera at all. Is it plugged in?");
		return false;
	}

	uint32_t mode = get_selected_mode(lhs->xfs);

	// If SLAM is enabled (only on vive driver) we intercept the data sink
	if (lhs->vive_tstatus.slam_enabled) {
		LH_ASSERT_(lhs->driver == DRIVER_VIVE);
		LH_ASSERT_(vive_head != NULL);
		LH_ASSERT_(vive_head->source != NULL);

		vive_source_hook_into_sinks(vive_head->source, &sinks);
	}

	bool bret = xrt_fs_stream_start(lhs->xfs, sinks.cams[0], XRT_FS_CAPTURE_TYPE_TRACKING, mode);
	if (!bret) {
		LH_ERROR("Unable to start data streaming");
		return false;
	}

	return true;
}

static void
try_add_opengloves(struct xrt_device *left,
                   struct xrt_device *right,
                   struct xrt_device **out_left_ht,
                   struct xrt_device **out_right_ht)
{
#ifdef XRT_BUILD_DRIVER_OPENGLOVES
	struct xrt_device *og_left = NULL, *og_right = NULL;
	opengloves_create_devices(left, right, &og_left, &og_right);

	// Overwrite the hand tracking roles with openglove ones.
	if (og_left != NULL) {
		*out_left_ht = og_left;
	}
	if (og_right != NULL) {
		*out_right_ht = og_right;
	}
#endif
}

static xrt_result_t
lighthouse_open_system_impl(struct xrt_builder *xb,
                            cJSON *config,
                            struct xrt_prober *xp,
                            struct xrt_tracking_origin *origin,
                            struct xrt_system_devices *xsysd,
                            struct xrt_frame_context *xfctx,
                            struct u_builder_roles_helper *ubrh)
{
	struct lighthouse_system *lhs = (struct lighthouse_system *)xb;
	xrt_result_t result = XRT_SUCCESS;

	// Needed when we probe for video devices.
	lhs->xfctx = xfctx;

	// Decide whether to initialize the SLAM tracker
	bool slam_wanted = debug_get_bool_option_vive_slam();
#ifdef XRT_FEATURE_SLAM
	bool slam_supported = lhs->driver == DRIVER_VIVE; // Only with vive driver
#else
	bool slam_supported = false;
#endif
	bool slam_enabled = slam_supported && slam_wanted;

	// Decide whether to initialize the hand tracker
#ifdef XRT_BUILD_DRIVER_HANDTRACKING
	bool hand_supported = true;
#else
	bool hand_supported = false;
#endif

	struct vive_tracking_status tstatus = {.slam_wanted = slam_wanted,
	                                       .slam_supported = slam_supported,
	                                       .slam_enabled = slam_enabled,
	                                       .controllers_found = false,
	                                       .hand_supported = hand_supported,
	                                       .hand_wanted = debug_get_tristate_option_lh_handtracking()};
	lhs->vive_tstatus = tstatus;

	switch (lhs->driver) {
	case DRIVER_STEAMVR: {
		assert(false);
		return XRT_ERROR_DEVICE_CREATION_FAILED;
	}
	case DRIVER_SURVIVE: {
#ifdef XRT_BUILD_DRIVER_SURVIVE
		xsysd->xdev_count += survive_get_devices(&xsysd->xdevs[xsysd->xdev_count], &lhs->hmd_config);
#endif
		break;
	}
	case DRIVER_VIVE: {
#ifdef XRT_BUILD_DRIVER_VIVE
		struct xrt_prober_device **xpdevs = NULL;
		size_t xpdev_count = 0;

		result = xrt_prober_lock_list(xp, &xpdevs, &xpdev_count);
		if (result != XRT_SUCCESS) {
			LH_ERROR("Unable to lock the prober dev list");
			goto end_err;
		}
		for (size_t i = 0; i < xpdev_count; i++) {
			struct xrt_prober_device *device = xpdevs[i];
			if (device->bus != XRT_BUS_TYPE_USB) {
				continue;
			}
			if (device->vendor_id != HTC_VID && device->vendor_id != VALVE_VID) {
				continue;
			}
			switch (device->product_id) {
			case VIVE_PID:
			case VIVE_PRO_MAINBOARD_PID:
			case VIVE_PRO2_MAINBOARD_PID:
			case VIVE_PRO_LHR_PID: {
				struct vive_source *vs = vive_source_create(xfctx);
				int num_devices = vive_found(          //
				    xp,                                //
				    xpdevs,                            //
				    xpdev_count,                       //
				    i,                                 //
				    NULL,                              //
				    lhs->vive_tstatus,                 //
				    vs,                                //
				    &lhs->hmd_config,                  //
				    &xsysd->xdevs[xsysd->xdev_count]); //
				xsysd->xdev_count += num_devices;
			} break;
			case VIVE_WATCHMAN_DONGLE:
			case VIVE_WATCHMAN_DONGLE_GEN2: {
				int num_devices = vive_controller_found( //
				    xp,                                  //
				    xpdevs,                              //
				    xpdev_count,                         //
				    i,                                   //
				    NULL,                                //
				    &xsysd->xdevs[xsysd->xdev_count]);   //
				xsysd->xdev_count += num_devices;
			} break;
			}
		}
		xrt_prober_unlock_list(xp, &xpdevs);
#endif
		break;
	}
	default: {
		LH_ASSERT(false, "Driver was not set to a known value");
		break;
	}
	}

	// Device indices.
	int head_idx = -1;
	int left_idx = -1;
	int right_idx = -1;

	u_device_assign_xdev_roles(xsysd->xdevs, xsysd->xdev_count, &head_idx, &left_idx, &right_idx);

	if (head_idx < 0) {
		LH_ERROR("Unable to find HMD");
		result = XRT_ERROR_DEVICE_CREATION_FAILED;
		goto end_err;
	}

	// Devices to populate.
	struct xrt_device *head = NULL;
	struct xrt_device *left = NULL, *right = NULL;
	struct xrt_device *left_ht = NULL, *right_ht = NULL;

	// Always have a head.
	head = xsysd->xdevs[head_idx];

	// It's okay if we didn't find controllers
	if (left_idx >= 0) {
		lhs->vive_tstatus.controllers_found = true;
		left = xsysd->xdevs[left_idx];
		left_ht = u_system_devices_get_ht_device_left(xsysd);
	}

	if (right_idx >= 0) {
		lhs->vive_tstatus.controllers_found = true;
		right = xsysd->xdevs[right_idx];
		right_ht = u_system_devices_get_ht_device_right(xsysd);
	}

	if (lhs->is_valve_index) {
		if (lhs->vive_tstatus.hand_wanted == DEBUG_TRISTATE_ON) {
			lhs->vive_tstatus.hand_enabled = true;
		} else if (lhs->vive_tstatus.hand_wanted == DEBUG_TRISTATE_AUTO) {
			if (lhs->vive_tstatus.controllers_found) {
				lhs->vive_tstatus.hand_enabled = false;
			} else {
				lhs->vive_tstatus.hand_enabled = true;
			}
		} else if (lhs->vive_tstatus.hand_wanted == DEBUG_TRISTATE_OFF) {
			lhs->vive_tstatus.hand_enabled = false;
		}



		bool success = true;

		if (lhs->hmd_config == NULL) {
			// This should NEVER happen, but we're not writing Rust.
			U_LOG_E("Didn't get a vive config? Not creating visual trackers.");
			goto end_valve_index;
		}
		if (!lhs->hmd_config->cameras.valid) {
			U_LOG_I(
			    "HMD didn't have cameras or didn't have a valid camera calibration. Not creating visual "
			    "trackers.");
			goto end_valve_index;
		}

		struct xrt_slam_sinks sinks = {0};
		struct xrt_device *hand_devices[2] = {NULL};
		struct vive_device *vive_head = NULL;

		// Only do cast if we are using the vive driver.
		if (lhs->driver == DRIVER_VIVE) {
			vive_head = (struct vive_device *)head;
		}

		success = valve_index_setup_visual_trackers( //
		    lhs,                                     //
		    head,                                    //
		    vive_head,                               //
		    xfctx,                                   //
		    xp,                                      //
		    &sinks,                                  //
		    hand_devices);                           //
		if (!success) {
			result = XRT_SUCCESS; // We won't have trackers, but creation was otherwise ok
			goto end_valve_index;
		}

		if (lhs->vive_tstatus.hand_enabled) {
			if (hand_devices[0] != NULL) {
				xsysd->xdevs[xsysd->xdev_count++] = hand_devices[0];
				left = hand_devices[0];
				left_ht = hand_devices[0];
			}

			if (hand_devices[1] != NULL) {
				xsysd->xdevs[xsysd->xdev_count++] = hand_devices[1];
				right = hand_devices[1];
				right_ht = hand_devices[1];
			}
		}

		success = stream_data_sources(lhs, vive_head, xp, sinks);
		if (!success) {
			result = XRT_SUCCESS; // We can continue after freeing trackers
			goto end_valve_index;
		}
	}
end_valve_index:

	// Check for error just in case.
	if (result != XRT_SUCCESS) {
		goto end_err;
	}

	// Should we use OpenGloves.
	if (!lhs->vive_tstatus.hand_enabled) {
		// We only want to try to add opengloves if we aren't optically tracking hands
		try_add_opengloves(left, right, &left_ht, &right_ht);
	}

	// Assign to role(s).
	ubrh->head = head;
	ubrh->left = left;
	ubrh->right = right;
	ubrh->hand_tracking.left = left_ht;
	ubrh->hand_tracking.right = right_ht;

	// Clean up after us.
	lhs->xfctx = NULL;

	return XRT_SUCCESS;


end_err:
	// Clean up after us.
	lhs->xfctx = NULL;

	return result;
}

static void
lighthouse_destroy(struct xrt_builder *xb)
{
	struct lighthouse_system *lhs = (struct lighthouse_system *)xb;
	free(lhs);
}


/*
 *
 * 'Exported' functions.
 *
 */

struct xrt_builder *
t_builder_lighthouse_create(void)
{
	struct lighthouse_system *lhs = U_TYPED_CALLOC(struct lighthouse_system);

	// xrt_builder fields.
	lhs->base.base.estimate_system = lighthouse_estimate_system;
	lhs->base.base.open_system = u_builder_open_system_static_roles;
	lhs->base.base.destroy = lighthouse_destroy;
	lhs->base.base.identifier = "lighthouse";
	lhs->base.base.name = "Lighthouse-tracked FLOSS (Vive, Index, Tundra trackers, etc.) devices builder";
	lhs->base.base.driver_identifiers = driver_list;
	lhs->base.base.driver_identifier_count = ARRAY_SIZE(driver_list);

	// u_builder fields.
	lhs->base.open_system_static_roles = lighthouse_open_system_impl;

	return &lhs->base.base;
}
