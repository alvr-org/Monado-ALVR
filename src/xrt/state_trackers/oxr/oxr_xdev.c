// Copyright 2019-2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Various helpers for accessing @ref xrt_device.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup oxr_main
 */

#include "os/os_time.h"

#include "math/m_api.h"
#include "math/m_space.h"

#include "util/u_time.h"
#include "util/u_misc.h"

#include "oxr_objects.h"
#include "oxr_logger.h"
#include "oxr_handle.h"
#include "xrt/xrt_defines.h"

#include <stdio.h>


/*
 *
 * Helper functions.
 *
 */

#ifdef OXR_HAVE_MNDX_xdev_space
static enum xrt_input_name
find_suitable_pose_name(struct xrt_device *xdev)
{
	//! @todo More complete set of poses / a system to enumerate all canonical device poses.
	for (uint32_t i = 0; i < xdev->input_count; i++) {
		enum xrt_input_name name = xdev->inputs[i].name;
		switch (name) {
		case XRT_INPUT_GENERIC_HEAD_POSE: return name;
		case XRT_INPUT_GENERIC_TRACKER_POSE: return name;
		case XRT_INPUT_INDEX_GRIP_POSE: return name;
		case XRT_INPUT_SIMPLE_GRIP_POSE: return name;
		case XRT_INPUT_VIVE_GRIP_POSE: return name;
		case XRT_INPUT_VIVE_COSMOS_GRIP_POSE: return name;
		case XRT_INPUT_VIVE_FOCUS3_GRIP_POSE: return name;
		case XRT_INPUT_VIVE_TRACKER_GRIP_POSE: return name;
		case XRT_INPUT_WMR_GRIP_POSE: return name;
		case XRT_INPUT_PSMV_GRIP_POSE: return name;
		case XRT_INPUT_G2_CONTROLLER_GRIP_POSE: return name;
		case XRT_INPUT_GO_GRIP_POSE: return name;
		case XRT_INPUT_ODYSSEY_CONTROLLER_GRIP_POSE: return name;
		case XRT_INPUT_TOUCH_GRIP_POSE: return name;
		case XRT_INPUT_TOUCH_PLUS_GRIP_POSE: return name;
		case XRT_INPUT_TOUCH_PRO_GRIP_POSE: return name;
		case XRT_INPUT_PICO_G3_GRIP_POSE: return name;
		case XRT_INPUT_PICO_NEO3_GRIP_POSE: return name;
		case XRT_INPUT_PICO4_GRIP_POSE: return name;
		case XRT_INPUT_OPPO_MR_GRIP_POSE: return name;
		default: break;
		}
	}

	return 0;
}
#endif


/*
 *
 * 'Exported' functions.
 *
 */

void
oxr_xdev_destroy(struct xrt_device **xdev_ptr)
{
	struct xrt_device *xdev = *xdev_ptr;

	if (xdev == NULL) {
		return;
	}

	xdev->destroy(xdev);
	*xdev_ptr = NULL;
}

void
oxr_xdev_update(struct xrt_device *xdev)
{
	if (xdev != NULL) {
		xrt_device_update_inputs(xdev);
	}
}

bool
oxr_xdev_find_input(struct xrt_device *xdev, enum xrt_input_name name, struct xrt_input **out_input)
{
	*out_input = NULL;
	if (xdev == NULL) {
		return false;
	}

	for (uint32_t i = 0; i < xdev->input_count; i++) {
		if (xdev->inputs[i].name != name) {
			continue;
		}

		*out_input = &xdev->inputs[i];
		return true;
	}
	return false;
}

bool
oxr_xdev_find_output(struct xrt_device *xdev, enum xrt_output_name name, struct xrt_output **out_output)
{
	if (xdev == NULL) {
		return false;
	}

	for (uint32_t i = 0; i < xdev->output_count; i++) {
		if (xdev->outputs[i].name != name) {
			continue;
		}

		*out_output = &xdev->outputs[i];
		return true;
	}
	return false;
}

void
oxr_xdev_get_hand_tracking_at(struct oxr_logger *log,
                              struct oxr_instance *inst,
                              struct xrt_device *xdev,
                              enum xrt_input_name name,
                              XrTime at_time,
                              struct xrt_hand_joint_set *out_value)
{
	//! Convert at_time to monotonic and give to device.
	int64_t at_timestamp_ns = time_state_ts_to_monotonic_ns(inst->timekeeping, at_time);

	struct xrt_hand_joint_set value;

	int64_t ignored;

	xrt_device_get_hand_tracking(xdev, name, at_timestamp_ns, &value, &ignored);

	*out_value = value;
}


/*
 *
 * XDev List
 *
 */

#ifdef OXR_HAVE_MNDX_xdev_space

static XrResult
oxr_xdev_list_destroy(struct oxr_logger *log, struct oxr_handle_base *hb)
{
	struct oxr_xdev_list *xdl = (struct oxr_xdev_list *)hb;

	free(xdl);

	return XR_SUCCESS;
}

XrResult
oxr_xdev_list_create(struct oxr_logger *log,
                     struct oxr_session *sess,
                     const XrCreateXDevListInfoMNDX *createInfo,
                     struct oxr_xdev_list **out_xdl)
{
	struct xrt_system_devices *xsysd = sess->sys->xsysd;
	uint32_t count = xsysd->xdev_count;

	struct oxr_xdev_list *xdl = NULL;
	OXR_ALLOCATE_HANDLE_OR_RETURN(log, xdl, OXR_XR_DEBUG_XDEVLIST, oxr_xdev_list_destroy, &sess->handle);

	/**
	 * @todo Should ids be explicitly unique per xdev list? Currently an id queried from xdev list 1 may or may not
	 * refer to the same xdev as an id queried from xdev list 2, which is error prone for app developers.
	 *
	 * On the other hand, it may be desirable to keep an id for an xdev fixed for the life time of the xdev. See
	 * also XR_ML_marker_understanding (This is NOT what the xdev_space code does now, this is only an example what
	 * other extensions do. xdev_space solves this with the xdev list generation_id): "Assuming the same set of
	 * markers are in view across several snapshots, the runtime should return the same set of atoms. An application
	 * can use the list of atoms as a simple test for if a particular marker has gone in or out of view."
	 */

	// The value of the assigned XrXDevIdMNDX atom.
	// Just to make them not start at 0 or 1.
	uint64_t id_gen = 42;

	for (uint32_t i = 0; i < count; i++) {
		struct xrt_device *xdev = xsysd->xdevs[i];
		enum xrt_input_name name = find_suitable_pose_name(xdev);

		xdl->ids[i] = id_gen++;
		xdl->xdevs[i] = xdev;
		xdl->names[i] = name;
	}

	xdl->device_count = count;
	xdl->sess = sess;

	//! @todo Always the first generation, Monado doesn't have hotplug (yet).
	xdl->generation_number = 1;

	*out_xdl = xdl;

	return XR_SUCCESS;
}

XrResult
oxr_xdev_list_get_properties(struct oxr_logger *log,
                             struct oxr_xdev_list *xdl,
                             uint32_t index,
                             XrXDevPropertiesMNDX *properties)
{
	if (index >= xdl->device_count) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "index %u > device_count %u", index, xdl->device_count);
	}

	struct xrt_device *xdev = xdl->xdevs[index];
	bool can_create_space = xdl->names[index] != 0;

	snprintf(properties->name, ARRAY_SIZE(properties->name), "%s", xdev->str);
	snprintf(properties->serial, ARRAY_SIZE(properties->serial), "%s", xdev->serial);
	properties->canCreateSpace = can_create_space;

	return XR_SUCCESS;
}

XrResult
oxr_xdev_list_space_create(struct oxr_logger *log,
                           struct oxr_xdev_list *xdl,
                           const XrCreateXDevSpaceInfoMNDX *createInfo,
                           uint32_t index,
                           struct oxr_space **out_space)
{
	if (index >= xdl->device_count) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE,
		                 "(createInfo->id == %" PRIu64 ") index %u > device_count %u", createInfo->id, index,
		                 xdl->device_count);
	}
	if (xdl->names[index] == 0) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE,
		                 "(createInfo->id == %" PRIu64 ") have no pose to create a space for", createInfo->id);
	}

	const struct xrt_pose *pose = (const struct xrt_pose *)&createInfo->offset;
	struct xrt_device *xdev = xdl->xdevs[index];
	enum xrt_input_name name = xdl->names[index];

	return oxr_space_xdev_pose_create(log, xdl->sess, xdev, name, pose, out_space);
}

#endif // OXR_HAVE_MNDX_xdev_space
