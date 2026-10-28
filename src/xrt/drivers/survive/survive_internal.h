// Copyright 2019-2023, Collabora, Ltd.
// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Adapter to Libsurvive.
 * @author Christoph Haag <christoph.haag@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Moshi Turner <moshiturner@protonmail.com>
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_survive
 */

#pragma once

#include "tracking/t_tracking.h"

#include "util/u_hand_tracking.h"
#include "util/u_var.h"

#include "os/os_threading.h"

#include "vp2/vp2_config.h"
#include "vp2/vp2_hid.h"

#include "survive_interface.h"
#include "survive_api.h"


#define SURVIVE_TRACE(d, ...) U_LOG_XDEV_IFL_T(&d->base, d->sys->log_level, __VA_ARGS__)
#define SURVIVE_DEBUG(d, ...) U_LOG_XDEV_IFL_D(&d->base, d->sys->log_level, __VA_ARGS__)
#define SURVIVE_INFO(d, ...) U_LOG_XDEV_IFL_I(&d->base, d->sys->log_level, __VA_ARGS__)
#define SURVIVE_WARN(d, ...) U_LOG_XDEV_IFL_W(&d->base, d->sys->log_level, __VA_ARGS__)
#define SURVIVE_ERROR(d, ...) U_LOG_XDEV_IFL_E(&d->base, d->sys->log_level, __VA_ARGS__)

//! excl HMD we support 16 devices (controllers, trackers, ...)
#define MAX_TRACKED_DEVICE_COUNT 16

enum input_index
{
	// common inputs
	VIVE_CONTROLLER_AIM_POSE = 0,
	VIVE_CONTROLLER_GRIP_POSE,
	VIVE_CONTROLLER_SYSTEM_CLICK,
	VIVE_CONTROLLER_TRIGGER_CLICK,
	VIVE_CONTROLLER_TRIGGER_VALUE,
	VIVE_CONTROLLER_TRACKPAD,
	VIVE_CONTROLLER_TRACKPAD_TOUCH,

	// Vive Wand specific inputs
	VIVE_CONTROLLER_SQUEEZE_CLICK,
	VIVE_CONTROLLER_MENU_CLICK,
	VIVE_CONTROLLER_TRACKPAD_CLICK,

	// Valve Index specific inputs
	VIVE_CONTROLLER_THUMBSTICK,
	VIVE_CONTROLLER_A_CLICK,
	VIVE_CONTROLLER_B_CLICK,
	VIVE_CONTROLLER_THUMBSTICK_CLICK,
	VIVE_CONTROLLER_THUMBSTICK_TOUCH,
	VIVE_CONTROLLER_SYSTEM_TOUCH,
	VIVE_CONTROLLER_A_TOUCH,
	VIVE_CONTROLLER_B_TOUCH,
	VIVE_CONTROLLER_SQUEEZE_VALUE,
	VIVE_CONTROLLER_SQUEEZE_FORCE,
	VIVE_CONTROLLER_TRIGGER_TOUCH,
	VIVE_CONTROLLER_TRACKPAD_FORCE,

	VIVE_CONTROLLER_HAND_TRACKING,

	VIVE_TRACKER_POSE,

	VIVE_CONTROLLER_MAX_INDEX,
};

enum DeviceType
{
	DEVICE_TYPE_HMD,
	DEVICE_TYPE_CONTROLLER
};

struct survive_system;

/*!
 * @implements xrt_device
 */
struct survive_device
{
	struct xrt_device base;
	struct survive_system *sys;
	const SurviveSimpleObject *survive_obj;

	struct m_relation_history *relation_hist;

	//! Number of inputs.
	size_t num_last_inputs;
	//! Array of input structs.
	struct xrt_input *last_inputs;

	enum DeviceType device_type;

	union {
		struct
		{
			float proximity; // [0,1]
			//! The current IPD given by the headset
			float ipd;
			//! The IPD to force, -1 if no override
			float ipd_override_mm;
			//! Whether to use the default eye relation, or the IPD given by the HMD
			bool use_default_ipd;

			struct vive_config config;

			struct vp2_hid *vp2_hid;
		} hmd;

		struct
		{
			float curl[XRT_FINGER_COUNT];
			uint64_t curl_ts[XRT_FINGER_COUNT];
			struct u_hand_tracking hand_tracking;

			struct vive_controller_config config;
		} ctrl;
	};
};

/*!
 * @extends xrt_tracking_origin
 */
struct survive_system
{
	struct xrt_tracking_origin base;
	SurviveSimpleContext *ctx;
	struct survive_device *hmd;
	struct survive_device *controllers[MAX_TRACKED_DEVICE_COUNT];
	enum u_logging_level log_level;
	struct xrt_prober *xp;

	float wait_timeout;
	struct u_var_draggable_f32 timecode_offset_ms;

	struct os_thread_helper event_thread;
	struct os_mutex lock;
};
