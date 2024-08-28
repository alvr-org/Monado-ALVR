// Copyright 2019-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header defining xrt space and space overseer.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup xrt_iface
 */

#pragma once

#include "xrt/xrt_defines.h"

#ifdef __cplusplus
extern "C" {
#endif

#define XRT_MAX_CLIENT_SPACES 128
struct xrt_device;
struct xrt_tracking_origin;

/*!
 * A space very similar to a OpenXR XrSpace but not a full one-to-one mapping,
 * but used to power XrSpace.
 *
 * @see @ref xrt_space_overseer
 * @see @ref design-spaces
 * @ingroup xrt_iface
 */
struct xrt_space
{
	/*!
	 * Reference helper.
	 */
	struct xrt_reference reference;

	/*!
	 * Destroy function.
	 */
	void (*destroy)(struct xrt_space *xs);
};

/*!
 * Update the reference counts on space(s).
 *
 * @param[in,out] dst Pointer to a object reference: if the object reference is
 *                    non-null will decrement its counter. The reference that
 *                    @p dst points to will be set to @p src.
 * @param[in] src New object for @p dst to refer to (may be null).
 *                If non-null, will have its refcount increased.
 * @ingroup xrt_iface
 * @relates xrt_space
 */
static inline void
xrt_space_reference(struct xrt_space **dst, struct xrt_space *src)
{
	struct xrt_space *old_dst = *dst;

	if (old_dst == src) {
		return;
	}

	if (src) {
		xrt_reference_inc(&src->reference);
	}

	*dst = src;

	if (old_dst) {
		if (xrt_reference_dec_and_is_zero(&old_dst->reference)) {
			old_dst->destroy(old_dst);
		}
	}
}

/*!
 * Object that oversees and manages spaces, one created for each XR system.
 *
 * The space overseer is used by the state tracker to query the poses of spaces
 * and devices in that space system. While the default implementation
 * @ref u_space_overseer implements the spaces as a graph of relatable spaces,
 * that is a implementation detail (the interface also lends itself to that
 * since bases have parents). As such the graph is not exposed in this interface
 * and spaces are technically free floating.
 *
 * One advantage of the free floating nature is that an overseer implementation
 * has much greater flexibility in configuring the graph to fit the current XR
 * system the best, it also have freedom to reconfigure the graph at runtime
 * should that be needed. Since any potential graph isn't exposed there is no
 * need to synchronise it across the app process and the service process.
 *
 * @see @ref design-spaces
 * @ingroup xrt_iface
 */
struct xrt_space_overseer
{
	struct
	{
		struct xrt_space *root;        //!< Root space, always available
		struct xrt_space *view;        //!< View space, may be null (in very rare cases).
		struct xrt_space *local;       //!< Local space, may be null (in very rare cases).
		struct xrt_space *local_floor; //!< Local floor space, may be null.
		struct xrt_space *stage;       //!< Stage space, may be null.
		struct xrt_space *unbounded;   //!< Unbounded space, only here for slam trackers.

		/*!
		 * Semantic spaces to be mapped to OpenXR spaces.
		 */
	} semantic;

	//! Ptrs to the localspace
	struct xrt_space *localspace[XRT_MAX_CLIENT_SPACES];
	//! Ptrs to the localfloorspace
	struct xrt_space *localfloorspace[XRT_MAX_CLIENT_SPACES];

	/*!
	 * Create a space with a fixed offset to the parent space.
	 *
	 * @param[in] xso        Owning space overseer.
	 * @param[in] parent     The parent space for the new space.
	 * @param[in] offset     Offset to the space.
	 * @param[out] out_space The newly created space.
	 */
	xrt_result_t (*create_offset_space)(struct xrt_space_overseer *xso,
	                                    struct xrt_space *parent,
	                                    const struct xrt_pose *offset,
	                                    struct xrt_space **out_space);

	/*!
	 * Create a space that wraps the @p xdev input pose described by input
	 * @p name, implicitly make the device's tracking space the parent of
	 * the created space. The name pose_space was chosen because while most
	 * input poses are part of the device, they may also be things tracked
	 * by the device. The important part is that the space is following the
	 * pose, that it happens to be attached to device is coincidental.
	 *
	 * @param[in] xso        Owning space overseer.
	 * @param[in] xdev       Device to get the pose from.
	 * @param[in] name       Name of the pose input.
	 * @param[out] out_space The newly created space.
	 */
	xrt_result_t (*create_pose_space)(struct xrt_space_overseer *xso,
	                                  struct xrt_device *xdev,
	                                  enum xrt_input_name name,
	                                  struct xrt_space **out_space);

	/*!
	 * Locate a space in the base space.
	 *
	 * @see xrt_device::get_tracked_pose.
	 *
	 * @param[in] xso             Owning space overseer.
	 * @param[in] base_space      The space that we want the pose in.
	 * @param[in] base_offset     Offset if any to the base space.
	 * @param[in] at_timestamp_ns At which time.
	 * @param[in] space           The space to be located.
	 * @param[in] offset          Offset if any to the located space.
	 * @param[out] out_relation   Resulting pose.
	 */
	xrt_result_t (*locate_space)(struct xrt_space_overseer *xso,
	                             struct xrt_space *base_space,
	                             const struct xrt_pose *base_offset,
	                             int64_t at_timestamp_ns,
	                             struct xrt_space *space,
	                             const struct xrt_pose *offset,
	                             struct xrt_space_relation *out_relation);

	/*!
	 * Locate spaces in the base space.
	 *
	 * @see xrt_device::get_tracked_pose.
	 *
	 * @param[in] xso             Owning space overseer.
	 * @param[in] base_space      The space that we want the pose in.
	 * @param[in] base_offset     Offset if any to the base space.
	 * @param[in] at_timestamp_ns At which time.
	 * @param[in] spaces          The array of pointers to spaces to be located.
	 * @param[in] space_count     The number of spaces to locate.
	 * @param[in] offsets         Array of offset if any to the located spaces.
	 * @param[out] out_relations  Array of resulting poses.
	 */
	xrt_result_t (*locate_spaces)(struct xrt_space_overseer *xso,
	                              struct xrt_space *base_space,
	                              const struct xrt_pose *base_offset,
	                              int64_t at_timestamp_ns,
	                              struct xrt_space **spaces,
	                              uint32_t space_count,
	                              const struct xrt_pose *offsets,
	                              struct xrt_space_relation *out_relations);

	/*!
	 * Locate a the origin of the tracking space of a device, this is not
	 * the same as the device position. In other words, what is the position
	 * of the space that the device is in, and which it returns its poses
	 * in. Needed to use @ref xrt_device::get_view_poses and
	 * @ref xrt_device::get_hand_tracking.
	 *
	 * @see xrt_device::get_tracked_pose.
	 *
	 * @param[in] xso             Owning space overseer.
	 * @param[in] base_space      The space that we want the pose in.
	 * @param[in] base_offset     Offset if any to the base space.
	 * @param[in] at_timestamp_ns At which time.
	 * @param[in] xdev            Device to get the pose from.
	 * @param[out] out_relation   Resulting pose.
	 */
	xrt_result_t (*locate_device)(struct xrt_space_overseer *xso,
	                              struct xrt_space *base_space,
	                              const struct xrt_pose *base_offset,
	                              int64_t at_timestamp_ns,
	                              struct xrt_device *xdev,
	                              struct xrt_space_relation *out_relation);

	/*!
	 * Increment the usage count of a reference space (aka semantic space).
	 * This lets the overseer know when different spaces are being used,
	 * allowing it to triggering a recenter if the local space is used for
	 * the first time, or stopping calculating where the stage space is if
	 * it is not used by the current application.
	 *
	 * @param[in] xso   Owning space overseer.
	 * @param[in] type  Which reference space is being referenced.
	 */
	xrt_result_t (*ref_space_inc)(struct xrt_space_overseer *xso, enum xrt_reference_space_type type);

	/*!
	 * Decrement the usage count of a reference space (aka semantic space).
	 * See @ref xrt_space_overseer::ref_space_inc.
	 *
	 * @param[in] xso   Owning space overseer.
	 * @param[in] type  Which reference space is being referenced.
	 */
	xrt_result_t (*ref_space_dec)(struct xrt_space_overseer *xso, enum xrt_reference_space_type type);

	/*!
	 * Trigger a re-center of the local and local_floor spaces, not all
	 * implementations of @ref xrt_space_overseer may support recenter. The
	 * recenter operation will normally mean that the local and local_floor
	 * will move to the where the view space currently is.
	 *
	 * @param[in] xso The space overseer.
	 */
	xrt_result_t (*recenter_local_spaces)(struct xrt_space_overseer *xso);

	/*!
	 * Read the offset from a tracking origin, not all
	 * implementations of @ref xrt_space_overseer may support this.
	 * Outputs are only valid if XRT_SUCCESS is returned.
	 *
	 * @param[in] xso The space overseer.
	 * @param[in] xto The tracking origin.
	 * @param[out] out_offset Pointer to an xrt_pose to write the offset to
	 */
	xrt_result_t (*get_tracking_origin_offset)(struct xrt_space_overseer *xso,
	                                           struct xrt_tracking_origin *xto,
	                                           struct xrt_pose *out_offset);

	/*!
	 * Apply an offset to a tracking origin, not all
	 * implementations of @ref xrt_space_overseer may support this.
	 *
	 * @param[in] xso The space overseer.
	 * @param[in] xto The tracking origin.
	 * @param[in] offset The offset to apply.
	 */
	xrt_result_t (*set_tracking_origin_offset)(struct xrt_space_overseer *xso,
	                                           struct xrt_tracking_origin *xto,
	                                           const struct xrt_pose *offset);

	/*!
	 * Read the offset from the given reference space, not all
	 * implementations of @ref xrt_space_overseer may support this.
	 * Outputs are only valid if XRT_SUCCESS is returned.
	 *
	 * @param[in] xso The space overseer.
	 * @param[in] type The reference space.
	 * @param[out] out_offset Pointer to write the offset to.
	 */
	xrt_result_t (*get_reference_space_offset)(struct xrt_space_overseer *xso,
	                                           enum xrt_reference_space_type type,
	                                           struct xrt_pose *out_offset);

	/*!
	 * Apply an offset to the given reference space, not all
	 * implementations of @ref xrt_space_overseer may support this.
	 *
	 * @param[in] xso The space overseer.
	 * @param[in] type The reference space.
	 * @param[in] offset The offset to apply.
	 */
	xrt_result_t (*set_reference_space_offset)(struct xrt_space_overseer *xso,
	                                           enum xrt_reference_space_type type,
	                                           const struct xrt_pose *offset);

	/*!
	 * Create a localspace and a localfloorspace.
	 *
	 * @param[in] xso        Owning space overseer.
	 * @param[out] out_local_space The newly created localspace.
	 * @param[out] out_local_floor_space The newly created localfloorspace.
	 */
	xrt_result_t (*create_local_space)(struct xrt_space_overseer *xso,
	                                   struct xrt_space **out_local_space,
	                                   struct xrt_space **out_local_floor_space);

	/*!
	 * Destroy function.
	 *
	 * @param xso The space overseer.
	 */
	void (*destroy)(struct xrt_space_overseer *xs);
};

/*!
 * @copydoc xrt_space_overseer::create_offset_space
 *
 * Helper for calling through the function pointer.
 *
 * @public @memberof xrt_space_overseer
 */
static inline xrt_result_t
xrt_space_overseer_create_offset_space(struct xrt_space_overseer *xso,
                                       struct xrt_space *parent,
                                       const struct xrt_pose *offset,
                                       struct xrt_space **out_space)
{
	return xso->create_offset_space(xso, parent, offset, out_space);
}

/*!
 * @copydoc xrt_space_overseer::create_pose_space
 *
 * Helper for calling through the function pointer.
 *
 * @public @memberof xrt_space_overseer
 */
static inline xrt_result_t
xrt_space_overseer_create_pose_space(struct xrt_space_overseer *xso,
                                     struct xrt_device *xdev,
                                     enum xrt_input_name name,
                                     struct xrt_space **out_space)
{
	return xso->create_pose_space(xso, xdev, name, out_space);
}

/*!
 * @copydoc xrt_space_overseer::locate_space
 *
 * Helper for calling through the function pointer.
 *
 * @public @memberof xrt_space_overseer
 */
static inline xrt_result_t
xrt_space_overseer_locate_space(struct xrt_space_overseer *xso,
                                struct xrt_space *base_space,
                                const struct xrt_pose *base_offset,
                                int64_t at_timestamp_ns,
                                struct xrt_space *space,
                                const struct xrt_pose *offset,
                                struct xrt_space_relation *out_relation)
{
	return xso->locate_space(xso, base_space, base_offset, at_timestamp_ns, space, offset, out_relation);
}

/*!
 * @copydoc xrt_space_overseer::locate_spaces
 *
 * Helper for calling through the function pointer.
 *
 * @public @memberof xrt_space_overseer
 */
static inline xrt_result_t
xrt_space_overseer_locate_spaces(struct xrt_space_overseer *xso,
                                 struct xrt_space *base_space,
                                 const struct xrt_pose *base_offset,
                                 int64_t at_timestamp_ns,
                                 struct xrt_space **spaces,
                                 uint32_t space_count,
                                 const struct xrt_pose *offsets,
                                 struct xrt_space_relation *out_relations)
{
	return xso->locate_spaces(xso, base_space, base_offset, at_timestamp_ns, spaces, space_count, offsets,
	                          out_relations);
}

/*!
 * @copydoc xrt_space_overseer::locate_device
 *
 * Helper for calling through the function pointer.
 *
 * @public @memberof xrt_space_overseer
 */
static inline xrt_result_t
xrt_space_overseer_locate_device(struct xrt_space_overseer *xso,
                                 struct xrt_space *base_space,
                                 const struct xrt_pose *base_offset,
                                 int64_t at_timestamp_ns,
                                 struct xrt_device *xdev,
                                 struct xrt_space_relation *out_relation)
{
	return xso->locate_device(xso, base_space, base_offset, at_timestamp_ns, xdev, out_relation);
}

/*!
 * @copydoc xrt_space_overseer::ref_space_inc
 *
 * Helper for calling through the function pointer.
 *
 * @public @memberof xrt_space_overseer
 */
static inline xrt_result_t
xrt_space_overseer_ref_space_inc(struct xrt_space_overseer *xso, enum xrt_reference_space_type type)
{
	return xso->ref_space_inc(xso, type);
}

/*!
 * @copydoc xrt_space_overseer::ref_space_dec
 *
 * Helper for calling through the function pointer.
 *
 * @public @memberof xrt_space_overseer
 */
static inline xrt_result_t
xrt_space_overseer_ref_space_dec(struct xrt_space_overseer *xso, enum xrt_reference_space_type type)
{
	return xso->ref_space_dec(xso, type);
}

/*!
 * @copydoc xrt_space_overseer::recenter_local_spaces
 *
 * Helper for calling through the function pointer.
 *
 * @public @memberof xrt_space_overseer
 */
static inline xrt_result_t
xrt_space_overseer_recenter_local_spaces(struct xrt_space_overseer *xso)
{
	return xso->recenter_local_spaces(xso);
}

/*!
 * @copydoc xrt_space_overseer::get_tracking_origin_offset
 *
 * Helper for calling through the function pointer.
 *
 * @public @memberof xrt_space_overseer
 */
static inline xrt_result_t
xrt_space_overseer_get_tracking_origin_offset(struct xrt_space_overseer *xso,
                                              struct xrt_tracking_origin *xto,
                                              struct xrt_pose *out_offset)
{
	return xso->get_tracking_origin_offset(xso, xto, out_offset);
}

/*!
 * @copydoc xrt_space_overseer::set_tracking_origin_offset
 *
 * Helper for calling through the function pointer.
 *
 * @public @memberof xrt_space_overseer
 */
static inline xrt_result_t
xrt_space_overseer_set_tracking_origin_offset(struct xrt_space_overseer *xso,
                                              struct xrt_tracking_origin *xt0,
                                              const struct xrt_pose *offset)
{
	return xso->set_tracking_origin_offset(xso, xt0, offset);
}

/*!
 * @copydoc xrt_space_overseer::get_reference_space_offset
 *
 * Helper for calling through the function pointer.
 *
 * @public @memberof xrt_space_overseer
 */
static inline xrt_result_t
xrt_space_overseer_get_reference_space_offset(struct xrt_space_overseer *xso,
                                              enum xrt_reference_space_type type,
                                              struct xrt_pose *out_offset)
{
	return xso->get_reference_space_offset(xso, type, out_offset);
}

/*!
 * @copydoc xrt_space_overseer::set_reference_space_offset
 *
 * Helper for calling through the function pointer.
 *
 * @public @memberof xrt_space_overseer
 */
static inline xrt_result_t
xrt_space_overseer_set_reference_space_offset(struct xrt_space_overseer *xso,
                                              enum xrt_reference_space_type type,
                                              const struct xrt_pose *offset)
{
	return xso->set_reference_space_offset(xso, type, offset);
}

/*!
 * @copydoc xrt_space_overseer::create_local_space
 *
 * Helper for calling through the function pointer.
 *
 * @public @memberof xrt_space_overseer
 */
static inline xrt_result_t
xrt_space_overseer_create_local_space(struct xrt_space_overseer *xso,
                                      struct xrt_space **out_local_space,
                                      struct xrt_space **out_local_floor_space)
{
	return xso->create_local_space(xso, out_local_space, out_local_floor_space);
}

/*!
 * Helper for calling through the function pointer: does a null check and sets
 * xc_ptr to null if freed.
 *
 * @see xrt_space_overseer::destroy
 * @public @memberof xrt_space_overseer
 */
static inline void
xrt_space_overseer_destroy(struct xrt_space_overseer **xso_ptr)
{
	struct xrt_space_overseer *xso = *xso_ptr;
	if (xso == NULL) {
		return;
	}

	xso->destroy(xso);
	*xso_ptr = NULL;
}


#ifdef __cplusplus
}
#endif
