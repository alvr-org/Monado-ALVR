// Copyright 2020-2022, Collabora, Ltd.
// Copyright 2023, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header for system objects.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup xrt_iface
 */

#pragma once

#include "xrt/xrt_compiler.h"
#include "xrt/xrt_defines.h"

struct xrt_system_devices;
struct xrt_instance;


#define XRT_SYSTEM_MAX_DEVICES (32)

/*!
 * Roles of the devices, negative index means unpopulated. When the devices
 * change the caller will do the needed actions to handle a device changes, for
 * the OpenXR state tracker this may include switching bound interaction profile
 * and generating the events for such a change.
 *
 * @see xrt_system_devices
 * @ingroup xrt_iface
 */
struct xrt_system_roles
{
	/*!
	 * Monotonically increasing generation counter, is increased when ever
	 * the roles is changed. Will always be greater then zero, this is to
	 * make init easier where any cache can start at zero and be guaranteed
	 * to be replaced with a new @ref xrt_system_roles.
	 */
	uint64_t generation_id;

	int32_t left;
	int32_t right;
	int32_t gamepad;
};

/*!
 * Guaranteed invalid content, not using designated initializers due to C++.
 *
 * @ingroup xrt_iface
 */
#define XRT_SYSTEM_ROLES_INIT                                                                                          \
	{                                                                                                              \
		0, -1, -1, -1                                                                                          \
	}


/*!
 * A collection of @ref xrt_device, and the roles they have been assigned.
 *
 * @see xrt_device, xrt_instance.
 */
struct xrt_system_devices
{
	struct xrt_device *xdevs[XRT_SYSTEM_MAX_DEVICES];
	size_t xdev_count;

	struct
	{
		struct xrt_device *head;
		struct xrt_device *eyes;
		struct
		{
			struct xrt_device *left;
			struct xrt_device *right;
		} hand_tracking;
	} static_roles;


	/*!
	 * Function to get the dynamic input device roles from this system
	 * devices, see @ref xrt_system_roles for more information.
	 *
	 * @param[in] xsysd Pointer to self
	 * @param[out] roles Pointer to xrt_system_roles
	 */
	xrt_result_t (*get_roles)(struct xrt_system_devices *xsysd, struct xrt_system_roles *out_roles);

	/*!
	 * Destroy all the devices that are owned by this system devices.
	 *
	 * Code consuming this interface should use xrt_system_devices_destroy.
	 */
	void (*destroy)(struct xrt_system_devices *xsysd);
};

/*!
 * @copydoc xrt_system_devices::get_roles
 *
 * Helper for calling through the function pointer.
 *
 * @public @memberof xrt_system_devices
 */
static inline xrt_result_t
xrt_system_devices_get_roles(struct xrt_system_devices *xsysd, struct xrt_system_roles *out_roles)
{
	return xsysd->get_roles(xsysd, out_roles);
}

/*!
 * Destroy an xrt_system_devices and owned devices - helper function.
 *
 * @param[in,out] xsysd_ptr A pointer to the xrt_system_devices struct pointer.
 *
 * Will destroy the system devices if *xsysd_ptr is not NULL. Will then set
 * *xsysd_ptr to NULL.
 *
 * @public @memberof xrt_system_devices
 */
static inline void
xrt_system_devices_destroy(struct xrt_system_devices **xsysd_ptr)
{
	struct xrt_system_devices *xsysd = *xsysd_ptr;
	if (xsysd == NULL) {
		return;
	}

	*xsysd_ptr = NULL;
	xsysd->destroy(xsysd);
}
