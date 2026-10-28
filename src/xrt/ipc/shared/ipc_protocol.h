// Copyright 2020-2024 Collabora, Ltd.
// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Common protocol definition.
 * @author Pete Black <pblack@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup ipc_shared
 */

#pragma once

#include "xrt/xrt_limits.h"
#include "xrt/xrt_compiler.h"
#include "xrt/xrt_compositor.h"
#include "xrt/xrt_results.h"
#include "xrt/xrt_defines.h"
#include "xrt/xrt_future.h"
#include "xrt/xrt_system.h"
#include "xrt/xrt_session.h"
#include "xrt/xrt_instance.h"
#include "xrt/xrt_compositor.h"
#include "xrt/xrt_device.h"
#include "xrt/xrt_space.h"
#include "xrt/xrt_tracking.h"
#include "xrt/xrt_config_build.h"

#include <assert.h>
#include <sys/types.h>


#define IPC_CRED_SIZE 1    // auth not implemented
#define IPC_BUF_SIZE 2048  // must be >= largest message length in bytes
#define IPC_MAX_VIEWS 8    // max views we will return configs for
#define IPC_MAX_FORMATS 32 // max formats our server-side compositor supports
#define IPC_MAX_DEVICES 8  // max number of devices we will map using shared mem
#define IPC_MAX_LAYERS XRT_MAX_LAYERS
#define IPC_MAX_SLOTS 128
#define IPC_MAX_CLIENTS 32
#define IPC_MAX_RAW_VIEWS 32 // Max views that we can get, artificial limit.
#define IPC_EVENT_QUEUE_SIZE 32


// example: v21.0.0-560-g586d33b5
#define IPC_VERSION_NAME_LEN 64

#if defined(XRT_OS_WINDOWS) && !defined(XRT_ENV_MINGW)
typedef int pid_t;
#endif

// Define the format to use to print a pid type
#if defined(XRT_ENV_MINGW)
// On Mingw64, this is an int64, aka a signed long long
#define PID_T_FMT "%lli"
#else
// On Linux and 'native' Windows, this is an int
#define PID_T_FMT "%d"
#endif

/*
 *
 * Shared memory structs.
 *
 */

/*!
 * Information about a device in the device list.
 *
 * @ingroup ipc
 */
struct ipc_tracking_origin_list_entry
{
	//! Tracking origin ID
	uint32_t id;
};

/*!
 * A list of the current tracking origins.
 *
 * @ingroup ipc
 */
struct ipc_tracking_origin_list
{
	//! Number of tracking origins.
	uint32_t origin_count;

	//! Compact list of tracking origins.
	struct ipc_tracking_origin_list_entry origins[XRT_SYSTEM_MAX_DEVICES];
};

/*!
 * A tracking in the shared memory area.
 *
 * @ingroup ipc
 */
struct ipc_tracking_origin_info
{
	//! For debugging.
	char name[XRT_TRACKING_NAME_LEN];

	//! What can the state tracker expect from this tracking system.
	enum xrt_tracking_type type;

	//! Initial offset of the tracking origin.
	struct xrt_pose offset;
};

/*!
 * Information about a device in the device list.
 *
 * @ingroup ipc
 */
struct ipc_device_list_entry
{
	//! Device ID
	uint32_t id;

	//! Device type
	enum xrt_device_type device_type;
};

/*!
 * List of devices available on the server.
 *
 * @ingroup ipc
 */
struct ipc_device_list
{
	//! Number of devices
	uint32_t device_count;

	//! Device entries
	struct ipc_device_list_entry devices[XRT_SYSTEM_MAX_DEVICES];
};

/*!
 * Device information sent over IPC.
 *
 * Followed by varlen data containing:
 * - An array of input_count * enum xrt_input_name
 * - An array of output_count * enum xrt_output_name
 *
 * @ingroup ipc
 */
struct ipc_device_info
{
	//! Enum identifier of the device.
	enum xrt_device_name name;
	enum xrt_device_type device_type;

	/*!
	 * The service-side @ref xrt_device_id, synchronized to IPC clients so that
	 * @ref xrt_device::id matches across all clients connected to the same
	 * service. This is distinct from the per-client-connection @c uint32_t
	 * slot index used to look up devices in IPC messages.
	 */
	uint64_t xrt_device_id_val;

	//! Which tracking system origin is this device attached to.
	XRT_ALIGNAS(8) uint32_t tracking_origin_id;

	//! A string describing the device.
	char str[XRT_DEVICE_NAME_LEN];

	//! A unique identifier. Persistent across configurations, if possible.
	char serial[XRT_DEVICE_NAME_LEN];

	//! Number of inputs.
	uint32_t input_count;

	//! Number of outputs.
	uint32_t output_count;

	//! The supported fields.
	struct xrt_device_supported supported;
};

/*!
 * Data for a single composition layer.
 *
 * Similar in function to @ref comp_layer
 *
 * @ingroup ipc
 */
struct ipc_layer_entry
{
	//! @todo what is this used for?
	uint32_t xdev_id;

	/*!
	 * Up to two indices of swapchains to use.
	 *
	 * How many are actually used depends on the value of @p data.type
	 */
	uint32_t swapchain_ids[XRT_MAX_VIEWS * 2];

	/*!
	 * All basic (trivially-serializable) data associated with a layer,
	 * aside from which swapchain(s) are used.
	 */
	struct xrt_layer_data data;
};

/*!
 * Render state for a single client, including all layers.
 *
 * @ingroup ipc
 */
struct ipc_layer_slot
{
	struct xrt_layer_frame_data data;
	uint32_t layer_count;
	struct ipc_layer_entry layers[IPC_MAX_LAYERS];
};

/*!
 * A big struct that contains all data that is shared to a client, no pointers
 * allowed in this. To get the inputs of a device you go:
 *
 * ```C++
 * struct xrt_input *
 * helper(struct ipc_shared_memory *ism, uint32_t device_id, uint32_t input)
 * {
 * 	uint32_t index = ism->isdevs[device_id]->first_input_index + input;
 * 	return &ism->inputs[index];
 * }
 * ```
 *
 * @ingroup ipc
 */
struct ipc_shared_memory
{
	/*!
	 * The git revision of the service, used by clients to detect version mismatches.
	 */
	char u_git_tag[IPC_VERSION_NAME_LEN];

	/*!
	 * Various roles for the devices.
	 */
	struct
	{
		int32_t head;
		int32_t eyes;
		int32_t face;
		int32_t body;

		struct
		{
			struct
			{
				int32_t left;
				int32_t right;
			} unobstructed;

			struct
			{
				int32_t left;
				int32_t right;
			} conforming;
		} hand_tracking;
	} roles;

	struct
	{
		struct
		{
			/*!
			 * Pixel properties of this display, not in absolute
			 * screen coordinates that the compositor sees. So
			 * before any rotation is applied by xrt_view::rot.
			 *
			 * The xrt_view::display::w_pixels &
			 * xrt_view::display::h_pixels become the recommended
			 * image size for this view.
			 *
			 * @todo doesn't account for overfill for timewarp or
			 * distortion?
			 */
			struct
			{
				uint32_t w_pixels;
				uint32_t h_pixels;
			} display;
		} views[2];
		// view count
		uint32_t view_count;
		enum xrt_blend_mode blend_modes[XRT_MAX_DEVICE_BLEND_MODES];
		uint32_t blend_mode_count;
	} hmd;

	struct ipc_layer_slot slots[IPC_MAX_SLOTS];

	uint64_t startup_timestamp;
	struct xrt_plane_detector_begin_info_ext plane_begin_info_ext;
};

/*!
 * Pointer-free arguments for creating an @ref xrt_hand_tracker over IPC.
 */
struct ipc_hand_tracker_create_info
{
	enum xrt_hand hand;
	enum xrt_input_name requested_sources[2];
	uint32_t requested_source_count;
	bool has_locked_xdev;
	uint32_t locked_xdev_id;
};

/*!
 * Pointer-free arguments for creating an @ref xrt_body_tracker over IPC.
 */
struct ipc_body_tracker_create_info
{
	enum xrt_input_name body_tracking_type;
	bool has_locked_xdev;
	uint32_t locked_xdev_id;
};

/*!
 * Initial info from a client when it connects.
 */
struct ipc_client_description
{
	pid_t pid;
	struct xrt_application_info info;
};

struct ipc_client_list
{
	uint32_t ids[IPC_MAX_CLIENTS];
	uint32_t id_count;
};

/*!
 * Which types of IO to block for a client.
 *
 * @ingroup ipc
 */
struct ipc_client_io_blocks
{
	bool block_poses;
	bool block_hand_tracking;
	bool block_inputs;
	bool block_outputs;
};

/*!
 * State for a connected application.
 *
 * @ingroup ipc
 */
struct ipc_app_state
{
	// Stable and unique ID of the client, only unique within this instance.
	uint32_t id;

	bool primary_application;
	bool session_active;
	bool session_visible;
	bool session_focused;
	bool session_overlay;
	struct ipc_client_io_blocks io_blocks;
	uint32_t z_order;
	pid_t pid;
	struct xrt_application_info info;
};

/*!
 * Arguments for creating swapchains from native images.
 */
struct ipc_arg_swapchain_from_native
{
	uint32_t sizes[XRT_MAX_SWAPCHAIN_IMAGES];
};

/*!
 * Arguments for xrt_device::get_view_poses with two views.
 */
struct ipc_info_get_view_poses_2
{
	struct xrt_fov fovs[XRT_MAX_VIEWS];
	struct xrt_pose poses[XRT_MAX_VIEWS];
	struct xrt_space_relation head_relation;
};

struct ipc_pcm_haptic_buffer
{
	uint32_t num_samples;
	float sample_rate;
	bool append;
};
