// Copyright 2020-2023, Collabora, Ltd.
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


#ifdef __cplusplus
extern "C" {
#endif

struct xrt_instance;
struct xrt_system_devices;
struct xrt_session;
struct xrt_compositor_native;
struct xrt_session_info;


/*
 *
 * System.
 *
 */

/*!
 * A system is a collection of devices, policies and optionally a compositor
 * that is organised into a chosive group that is usable by one user, most of
 * the functionality of a system is exposed through other objects, this is the
 * main object. It is from this you create sessions that is used to by apps to
 * interact with the "system".
 *
 * Sibling objects: @ref xrt_system_devices, @ref xrt_system_compositor and
 * @ref xrt_space_overseer.
 *
 * @ingroup xrt_iface
 */
struct xrt_system
{
	/*!
	 * Create a @ref xrt_session and optionally a @ref xrt_compositor_native
	 * for this system.
	 *
	 * param[in]  xsys    Pointer to self.
	 * param[in]  xsi     Session info.
	 * param[out] out_xs  Created session.
	 * param[out] out_xcn Native compositor for this session, optional.
	 */
	xrt_result_t (*create_session)(struct xrt_system *xsys,
	                               const struct xrt_session_info *xsi,
	                               struct xrt_session **out_xs,
	                               struct xrt_compositor_native **out_xcn);

	/*!
	 * Destroy the system, must be destroyed after system devices and system
	 * compositor has been destroyed.
	 *
	 * Code consuming this interface should use @ref xrt_system_destroy.
	 *
	 * @param xsys Pointer to self
	 */
	void (*destroy)(struct xrt_system *xsys);
};

/*!
 * @copydoc xrt_system::create_session
 *
 * Helper for calling through the function pointer.
 *
 * @public @memberof xrt_system
 */
static inline xrt_result_t
xrt_system_create_session(struct xrt_system *xsys,
                          const struct xrt_session_info *xsi,
                          struct xrt_session **out_xs,
                          struct xrt_compositor_native **out_xcn)
{
	return xsys->create_session(xsys, xsi, out_xs, out_xcn);
}

/*!
 * Destroy an xrt_system - helper function.
 *
 * @param[in,out] xsysd_ptr A pointer to the xrt_system struct pointer.
 *
 * Will destroy the system if `*xsys_ptr` is not NULL. Will then set
 * `*xsys_ptr` to NULL.
 *
 * @public @memberof xrt_system
 */
static inline void
xrt_system_destroy(struct xrt_system **xsys_ptr)
{
	struct xrt_system *xsys = *xsys_ptr;
	if (xsys == NULL) {
		return;
	}

	*xsys_ptr = NULL;
	xsys->destroy(xsys);
}


/*
 *
 * System devices.
 *
 */

/*!
 * Maximum number of devices simultaneously usable by an implementation of
 * @ref xrt_system_devices
 *
 * @ingroup xrt_iface
 */
#define XRT_SYSTEM_MAX_DEVICES (32)

/*!
 * Data associating a device index (in @ref xrt_system_devices::xdevs) with a
 * given "role" for dynamic role switching.
 *
 * For each of the named roles, a negative value means unpopulated/not available.
 *
 * Populated by a call from the @ref xrt_system_devices interface.
 *
 * When the caller of @ref xrt_system_devices_get_roles sees a change (based on
 * comparing @ref generation_id) the caller must do the needed actions to handle
 * device changes. For example, for the OpenXR state tracker this may include
 * rebinding, queuing a change to the current interaction profile, and queuing
 * the events associated with such a change.
 *
 * @see xrt_system_devices
 * @ingroup xrt_iface
 */
struct xrt_system_roles
{
	/*!
	 * Monotonically increasing generation counter for the association
	 * between role and index.
	 *
	 * Increment whenever the roles are changed.
	 *
	 * All valid values are greater then zero; this is to
	 * make init easier where any cache can start at zero and be guaranteed
	 * to be replaced with a new @ref xrt_system_roles.
	 */
	uint64_t generation_id;

	/*!
	 * Index in @ref xrt_system_devices::xdevs for the user's left
	 * controller/hand, or negative if none available.
	 */
	int32_t left;

	/*!
	 * Index in @ref xrt_system_devices::xdevs for the user's right
	 * controller/hand, or negative if none available.
	 */
	int32_t right;

	/*!
	 * Index in @ref xrt_system_devices::xdevs for the user's gamepad
	 * device, or negative if none available.
	 */
	int32_t gamepad;
};

/*!
 * Guaranteed invalid constant for @ref xrt_system_roles, not using designated
 * initializers due to C++.
 *
 * @ingroup xrt_iface
 * @relates xrt_system_roles
 */
#define XRT_SYSTEM_ROLES_INIT                                                                                          \
	{                                                                                                              \
		0, -1, -1, -1                                                                                          \
	}


/*!
 * A collection of @ref xrt_device, and an interface for identifying the roles
 * they have been assigned.
 *
 * @see xrt_device, xrt_instance.
 */
struct xrt_system_devices
{
	/*!
	 * All devices known in the system.
	 *
	 * This is conventionally considered the "owning" reference to the devices.
	 * Valid entries are contiguous.
	 */
	struct xrt_device *xdevs[XRT_SYSTEM_MAX_DEVICES];

	/*!
	 * The number of elements in @ref xdevs that are valid.
	 */
	size_t xdev_count;

	/*!
	 * Observing pointers for devices in some static (unchangeable) roles.
	 *
	 * All pointers in this struct must also exist in @ref xdevs. The
	 * association between a member of this struct and a given device
	 * cannot change during runtime.
	 */
	struct
	{
		/*!
		 * An observing pointer to the device serving as the "head"
		 * (and HMD).
		 *
		 * Required.
		 */
		struct xrt_device *head;

		/*!
		 * An observing pointer to the device providing eye tracking
		 * (optional).
		 */
		struct xrt_device *eyes;

		/*!
		 * Devices providing optical (or otherwise more directly
		 * measured than from controller estimation) hand tracking.
		 */
		struct
		{

			/*!
			 * An observing pointer to the device providing hand
			 * tracking for the left hand (optional).
			 *
			 * Currently this is used for both optical and
			 * controller driven hand-tracking.
			 */
			struct xrt_device *left;

			/*!
			 * An observing pointer to the device providing hand
			 * tracking for the right hand (optional).
			 *
			 * Currently this is used for both optical and
			 * controller driven hand-tracking.
			 */
			struct xrt_device *right;
		} hand_tracking;
	} static_roles;


	/*!
	 * Function to get the dynamic input device roles from this system
	 * devices, see @ref xrt_system_roles for more information.
	 *
	 * @param xsysd Pointer to self
	 * @param[out] out_roles Pointer to xrt_system_roles
	 */
	xrt_result_t (*get_roles)(struct xrt_system_devices *xsysd, struct xrt_system_roles *out_roles);

	/*!
	 * Destroy all the devices that are owned by this system devices.
	 *
	 * Code consuming this interface should use @ref xrt_system_devices_destroy.
	 *
	 * @param xsysd Pointer to self
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
 * Will destroy the system devices if `*xsysd_ptr` is not NULL. Will then set
 * `*xsysd_ptr` to NULL.
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


#ifdef __cplusplus
}
#endif
