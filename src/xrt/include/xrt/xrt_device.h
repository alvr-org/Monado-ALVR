// Copyright 2019-2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header defining an xrt display or controller device.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Moses Turner <mosesturner@protonmail.com>
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup xrt_iface
 */

#pragma once

#include "xrt/xrt_defines.h"
#include "xrt/xrt_visibility_mask.h"
#include "xrt/xrt_limits.h"

#ifdef __cplusplus
extern "C" {
#endif

struct xrt_tracking;

#define XRT_DEVICE_NAME_LEN 256


/*!
 * A per-lens/display view information.
 *
 * @ingroup xrt_iface
 */
struct xrt_view
{
	/*!
	 * @brief Viewport position on the screen.
	 *
	 * In absolute screen coordinates on an unrotated display, like the
	 * HMD presents it to the OS.
	 *
	 * This field is only used by @ref comp to setup the device rendering.
	 *
	 * If the view is being rotated by xrt_view.rot 90° right in the
	 * distortion shader then `display.w_pixels == viewport.h_pixels` and
	 * `display.h_pixels == viewport.w_pixels`.
	 */
	struct
	{
		uint32_t x_pixels;
		uint32_t y_pixels;
		uint32_t w_pixels;
		uint32_t h_pixels;
	} viewport;

	/*!
	 * @brief Physical properties of this display (or the part of a display
	 * that covers this view).
	 *
	 * Not in absolute screen coordinates but like the clients see them i.e.
	 * after rotation is applied by xrt_view::rot.
	 * This field is only used for the clients' swapchain setup.
	 *
	 * The xrt_view::display::w_pixels and xrt_view::display::h_pixels
	 * become the recommended image size for this view, after being scaled
	 * by the debug environment variable `XRT_COMPOSITOR_SCALE_PERCENTAGE`.
	 */
	struct
	{
		uint32_t w_pixels;
		uint32_t h_pixels;
	} display;

	/*!
	 * @brief Rotation 2d matrix used to rotate the position of the output
	 * of the distortion shaders onto the screen.
	 *
	 * If the distortion shader is based on a mesh, then this matrix rotates
	 * the vertex positions.
	 */
	struct xrt_matrix_2x2 rot;
};

/*!
 * All of the device components that deals with interfacing to a users head.
 *
 * HMD is probably a bad name for the future but for now will have to do.
 *
 * @ingroup xrt_iface
 */
struct xrt_hmd_parts
{
	/*!
	 * @brief The hmd screen as an unrotated display, like the HMD presents
	 * it to the OS.
	 *
	 * This field is used by @ref comp to setup the extended mode window.
	 */
	struct
	{
		int w_pixels;
		int h_pixels;
		//! Nominal frame interval
		uint64_t nominal_frame_interval_ns;
	} screens[1];

	/*!
	 * Display information.
	 *
	 * For now hardcoded display to two.
	 */
	struct xrt_view views[XRT_MAX_VIEWS];

	size_t view_count;
	/*!
	 * Array of supported blend modes.
	 */
	enum xrt_blend_mode blend_modes[XRT_MAX_DEVICE_BLEND_MODES];
	size_t blend_mode_count;

	/*!
	 * Distortion information.
	 */
	struct
	{
		//! Supported distortion models, a bitfield.
		enum xrt_distortion_model models;
		//! Preferred disortion model, single value.
		enum xrt_distortion_model preferred;

		struct
		{
			//! Data.
			float *vertices;
			//! Number of vertices.
			uint32_t vertex_count;
			//! Stride of vertices
			uint32_t stride;
			//! 1 or 3 for (chromatic aberration).
			uint32_t uv_channels_count;

			//! Indices, for triangle strip.
			int *indices;
			//! Number of indices for the triangle strips (one per view).
			uint32_t index_counts[XRT_MAX_VIEWS];
			//! Offsets for the indices (one offset per view).
			uint32_t index_offsets[XRT_MAX_VIEWS];
			//! Total number of elements in mesh::indices array.
			uint32_t index_count_total;
		} mesh;

		//! distortion is subject to the field of view
		struct xrt_fov fov[XRT_MAX_VIEWS];
	} distortion;
};

/*!
 * A single named input, that sits on a @ref xrt_device.
 *
 * @ingroup xrt_iface
 */
struct xrt_input
{
	//! Is this input active.
	bool active;

	int64_t timestamp;

	enum xrt_input_name name;

	union xrt_input_value value;
};

/*!
 * A single named output, that sits on a @ref xrt_device.
 *
 * @ingroup xrt_iface
 */
struct xrt_output
{
	enum xrt_output_name name;
};


/*!
 * A binding pair, going @p from a binding point to a @p device input.
 *
 * @ingroup xrt_iface
 */
struct xrt_binding_input_pair
{
	enum xrt_input_name from;   //!< From which name.
	enum xrt_input_name device; //!< To input on the device.
};

/*!
 * A binding pair, going @p from a binding point to a @p device output.
 *
 * @ingroup xrt_iface
 */
struct xrt_binding_output_pair
{
	enum xrt_output_name from;   //!< From which name.
	enum xrt_output_name device; //!< To output on the device.
};

/*!
 * A binding profile, has lists of binding pairs to goes from device in @p name
 * to the device it hangs off on.
 *
 * @ingroup xrt_iface
 */
struct xrt_binding_profile
{
	//! Device this binding emulates.
	enum xrt_device_name name;

	struct xrt_binding_input_pair *inputs;
	size_t input_count;
	struct xrt_binding_output_pair *outputs;
	size_t output_count;
};

/*!
 * @interface xrt_device
 *
 * A single HMD or input device.
 *
 * @ingroup xrt_iface
 */
struct xrt_device
{
	//! Enum identifier of the device.
	enum xrt_device_name name;
	enum xrt_device_type device_type;

	//! A string describing the device.
	char str[XRT_DEVICE_NAME_LEN];

	//! A unique identifier. Persistent across configurations, if possible.
	char serial[XRT_DEVICE_NAME_LEN];

	//! Null if this device does not interface with the users head.
	struct xrt_hmd_parts *hmd;

	//! Always set, pointing to the tracking system for this device.
	struct xrt_tracking_origin *tracking_origin;

	//! Number of bindings in xrt_device::binding_profiles
	size_t binding_profile_count;
	// Array of alternative binding profiles.
	struct xrt_binding_profile *binding_profiles;

	//! Number of inputs.
	size_t input_count;
	//! Array of input structs.
	struct xrt_input *inputs;

	//! Number of outputs.
	size_t output_count;
	//! Array of output structs.
	struct xrt_output *outputs;

	bool orientation_tracking_supported;
	bool position_tracking_supported;
	bool hand_tracking_supported;
	bool eye_gaze_supported;
	bool force_feedback_supported;
	bool ref_space_usage_supported;
	bool form_factor_check_supported;
	bool stage_supported;
	bool face_tracking_supported;
	bool body_tracking_supported;
	bool battery_status_supported;

	/*
	 *
	 * Functions.
	 *
	 */

	/*!
	 * Update any attached inputs.
	 *
	 * @param[in] xdev        The device.
	 */
	void (*update_inputs)(struct xrt_device *xdev);

	/*!
	 * @brief Get relationship of a tracked device to the tracking origin
	 * space as the base space.
	 *
	 * It is the responsibility of the device driver to do any prediction,
	 * there are helper functions available for this.
	 *
	 * The timestamps are system monotonic timestamps, such as returned by
	 * os_monotonic_get_ns().
	 *
	 * @param[in] xdev           The device.
	 * @param[in] name           Some devices may have multiple poses on
	 *                           them, select the one using this field. For
	 *                           HMDs use @p XRT_INPUT_GENERIC_HEAD_POSE.
	 *                           For Unbounded Reference Space you can use
	 *                           @p XRT_INPUT_GENERIC_UNBOUNDED_SPACE_POSE
	 *                           to get the origin of that space.
	 * @param[in] at_timestamp_ns If the device can predict or has a history
	 *                            of positions, this is when the caller
	 *                            wants the pose to be from.
	 * @param[out] out_relation The relation read from the device.
	 *
	 * @see xrt_input_name
	 */
	void (*get_tracked_pose)(struct xrt_device *xdev,
	                         enum xrt_input_name name,
	                         uint64_t at_timestamp_ns,
	                         struct xrt_space_relation *out_relation);

	/*!
	 * @brief Get relationship of hand joints to the tracking origin space as
	 * the base space.
	 *
	 * It is the responsibility of the device driver to either do prediction
	 * or return joints from a previous time and write that time out to
	 * @p out_timestamp_ns.
	 *
	 * The timestamps are system monotonic timestamps, such as returned by
	 * os_monotonic_get_ns().
	 *
	 * @param[in] xdev                 The device.
	 * @param[in] name                 Some devices may have multiple poses on
	 *                                 them, select the one using this field. For
	 *                                 hand tracking use @p XRT_INPUT_GENERIC_HAND_TRACKING_DEFAULT_SET.
	 * @param[in] desired_timestamp_ns If the device can predict or has a history
	 *                                 of positions, this is when the caller
	 *                                 wants the pose to be from.
	 * @param[out] out_value           The hand joint data read from the device.
	 * @param[out] out_timestamp_ns    The timestamp of the data being returned.
	 *
	 * @see xrt_input_name
	 */
	void (*get_hand_tracking)(struct xrt_device *xdev,
	                          enum xrt_input_name name,
	                          uint64_t desired_timestamp_ns,
	                          struct xrt_hand_joint_set *out_value,
	                          uint64_t *out_timestamp_ns);

	/*!
	 * @brief Get the requested blend shape properties & weights for a face tracker
	 *
	 * @param[in] xdev                    The device.
	 * @param[in] facial_expression_type  The facial expression data type (XR_FB_face_tracking,
	 * XR_HTC_facial_tracking, etc).
	 * @param[in] out_value               Set of requested expression weights & blend shape properties.
	 *
	 * @see xrt_input_name
	 */
	xrt_result_t (*get_face_tracking)(struct xrt_device *xdev,
	                                  enum xrt_input_name facial_expression_type,
	                                  struct xrt_facial_expression_set *out_value);

	/*!
	 * @brief Get the body skeleton in T-pose, used to query the skeleton hierarchy, scale, proportions etc
	 *
	 * @param[in] xdev                    The device.
	 * @param[in] body_tracking_type      The body joint set type (XR_FB_body_tracking,
	 * XR_META_body_tracking_full_body, etc).
	 * @param[in] out_value               The body skeleton hierarchy/properties.
	 *
	 * @see xrt_input_name
	 */
	xrt_result_t (*get_body_skeleton)(struct xrt_device *xdev,
	                                  enum xrt_input_name body_tracking_type,
	                                  struct xrt_body_skeleton *out_value);

	/*!
	 * @brief Get the joint locations for a body tracker
	 *
	 * @param[in] xdev                    The device.
	 * @param[in] body_tracking_type      The body joint set type (XR_FB_body_tracking,
	 * XR_META_body_tracking_full_body, etc).
	 * @param[in] desired_timestamp_ns    If the device can predict or has a history
	 *                                    of locations, this is when the caller
	 * @param[in] out_value               Set of body joint locations & properties.
	 *
	 * @see xrt_input_name
	 */
	xrt_result_t (*get_body_joints)(struct xrt_device *xdev,
	                                enum xrt_input_name body_tracking_type,
	                                uint64_t desired_timestamp_ns,
	                                struct xrt_body_joint_set *out_value);

	/*!
	 * Set a output value.
	 *
	 * @param[in] xdev           The device.
	 * @param[in] name           The output component name to set.
	 * @param[in] value          The value to set the output to.
	 * @see xrt_output_name
	 */
	void (*set_output)(struct xrt_device *xdev, enum xrt_output_name name, const union xrt_output_value *value);

	/*!
	 * @brief Get the per-view pose in relation to the view space.
	 *
	 * On most devices with coplanar displays and no built-in eye tracking
	 * or IPD sensing, this just calls a helper to process the provided
	 * eye relation, but this may also handle canted displays as well as
	 * eye tracking.
	 *
	 * Incorporates a call to xrt_device::get_tracked_pose or a wrapper for it
	 *
	 * @param[in] xdev         The device.
	 * @param[in] default_eye_relation
	 *                         The interpupillary relation as a 3D position.
	 *                         Most simple stereo devices would just want to
	 *                         set `out_pose->position.[x|y|z] = ipd.[x|y|z]
	 *                         / 2.0f` and adjust for left vs right view.
	 *                         Not to be confused with IPD that is absolute
	 *                         distance, this is a full 3D translation
	 *                         If a device has a more accurate/dynamic way of
	 *                         knowing the eye relation, it may ignore this
	 *                         input.
	 * @param[in] at_timestamp_ns This is when the caller wants the poses and FOVs to be from.
	 * @param[in] view_count   Number of views.
	 * @param[out] out_head_relation
	 *                         The head pose in the device tracking space.
	 *                         Combine with @p out_poses to get the views in
	 *                         device tracking space.
	 * @param[out] out_fovs    An array (of size @p view_count ) to populate
	 *                         with the device-suggested fields of view.
	 * @param[out] out_poses   An array (of size @p view_count ) to populate
	 *                         with view output poses in head space. When
	 *                         implementing, be sure to also set orientation:
	 *                         most likely identity orientation unless you
	 *                         have canted screens.
	 *                         (Caution: Even if you have eye tracking, you
	 *                         won't use eye orientation here!)
	 */
	void (*get_view_poses)(struct xrt_device *xdev,
	                       const struct xrt_vec3 *default_eye_relation,
	                       uint64_t at_timestamp_ns,
	                       uint32_t view_count,
	                       struct xrt_space_relation *out_head_relation,
	                       struct xrt_fov *out_fovs,
	                       struct xrt_pose *out_poses);

	/**
	 * Compute the distortion at a single point.
	 *
	 * The input is @p u @p v in screen/output space (that is, predistorted), you are to compute and return the u,v
	 * coordinates to sample the render texture. The compositor will step through a range of u,v parameters to build
	 * the lookup (vertex attribute or distortion texture) used to pre-distort the image as required by the device's
	 * optics.
	 *
	 * @param xdev            the device
	 * @param view            the view index
	 * @param u               horizontal texture coordinate
	 * @param v               vertical texture coordinate
	 * @param[out] out_result corresponding u,v pairs for all three color channels.
	 */
	bool (*compute_distortion)(
	    struct xrt_device *xdev, uint32_t view, float u, float v, struct xrt_uv_triplet *out_result);

	/*!
	 * Get the visibility mask for this device.
	 *
	 * @param[in] xdev       The device.
	 * @param[in] type       The type of visibility mask.
	 * @param[in] view_index The index of the view to get the mask for.
	 * @param[out] out_mask  Output mask, caller must free.
	 */
	xrt_result_t (*get_visibility_mask)(struct xrt_device *xdev,
	                                    enum xrt_visibility_mask_type type,
	                                    uint32_t view_index,
	                                    struct xrt_visibility_mask **out_mask);

	/*!
	 * Called by the @ref xrt_space_overseer when a reference space that is
	 * implemented by this device is first used, or when the last usage of
	 * the reference space stops.
	 *
	 * What is provided is both the @ref xrt_reference_space_type that
	 * triggered the usage change and the @ref xrt_input_name (if any) that
	 * is used to drive the space.
	 *
	 * @see xrt_space_overseer_ref_space_inc
	 * @see xrt_space_overseer_ref_space_dec
	 * @see xrt_input_name
	 * @see xrt_reference_space_type
	 */
	xrt_result_t (*ref_space_usage)(struct xrt_device *xdev,
	                                enum xrt_reference_space_type type,
	                                enum xrt_input_name name,
	                                bool used);

	/*!
	 * @brief Check if given form factor is available or not.
	 *
	 * This should only be used in HMD device, if the device driver supports form factor check.
	 *
	 * @param[in] xdev The device.
	 * @param[in] form_factor Form factor to check.
	 *
	 * @return true if given form factor is available; otherwise false.
	 */
	bool (*is_form_factor_available)(struct xrt_device *xdev, enum xrt_form_factor form_factor);

	/*!
	 * @brief Get battery status information.
	 *
	 * @param[in] xdev          The device.
	 * @param[out] out_present  Whether battery status information exist for this device.
	 * @param[out] out_charging Whether the device's battery is being charged.
	 * @param[out] out_charge   Battery charge as a value between 0 and 1.
	 */
	xrt_result_t (*get_battery_status)(struct xrt_device *xdev,
	                                   bool *out_present,
	                                   bool *out_charging,
	                                   float *out_charge);

	/*!
	 * Destroy device.
	 */
	void (*destroy)(struct xrt_device *xdev);

	// Add new functions above destroy.
};

/*!
 * Helper function for @ref xrt_device::update_inputs.
 *
 * @copydoc xrt_device::update_inputs
 *
 * @public @memberof xrt_device
 */
static inline void
xrt_device_update_inputs(struct xrt_device *xdev)
{
	xdev->update_inputs(xdev);
}

/*!
 * Helper function for @ref xrt_device::get_tracked_pose.
 *
 * @copydoc xrt_device::get_tracked_pose
 *
 * @public @memberof xrt_device
 */
static inline void
xrt_device_get_tracked_pose(struct xrt_device *xdev,
                            enum xrt_input_name name,
                            uint64_t at_timestamp_ns,
                            struct xrt_space_relation *out_relation)
{
	xdev->get_tracked_pose(xdev, name, at_timestamp_ns, out_relation);
}

/*!
 * Helper function for @ref xrt_device::get_hand_tracking.
 *
 * @copydoc xrt_device::get_hand_tracking
 *
 * @public @memberof xrt_device
 */
static inline void
xrt_device_get_hand_tracking(struct xrt_device *xdev,
                             enum xrt_input_name name,
                             uint64_t desired_timestamp_ns,
                             struct xrt_hand_joint_set *out_value,
                             uint64_t *out_timestamp_ns)
{
	xdev->get_hand_tracking(xdev, name, desired_timestamp_ns, out_value, out_timestamp_ns);
}

/*!
 * Helper function for @ref xrt_device::get_face_tracking.
 *
 * @copydoc xrt_device::get_face_tracking
 *
 * @public @memberof xrt_device
 */
static inline xrt_result_t
xrt_device_get_face_tracking(struct xrt_device *xdev,
                             enum xrt_input_name facial_expression_type,
                             struct xrt_facial_expression_set *out_value)
{
	return xdev->get_face_tracking(xdev, facial_expression_type, out_value);
}

/*!
 * Helper function for @ref xrt_device::get_body_skeleton.
 *
 * @copydoc xrt_device::get_body_skeleton
 *
 * @public @memberof xrt_device
 */
static inline xrt_result_t
xrt_device_get_body_skeleton(struct xrt_device *xdev,
                             enum xrt_input_name body_tracking_type,
                             struct xrt_body_skeleton *out_value)
{
	if (xdev->get_body_skeleton == NULL) {
		return XRT_ERROR_NOT_IMPLEMENTED;
	}
	return xdev->get_body_skeleton(xdev, body_tracking_type, out_value);
}

/*!
 * Helper function for @ref xrt_device::get_body_joints.
 *
 * @copydoc xrt_device::get_body_joints
 *
 * @public @memberof xrt_device
 */
static inline xrt_result_t
xrt_device_get_body_joints(struct xrt_device *xdev,
                           enum xrt_input_name body_tracking_type,
                           uint64_t desired_timestamp_ns,
                           struct xrt_body_joint_set *out_value)
{
	if (xdev->get_body_joints == NULL) {
		return XRT_ERROR_NOT_IMPLEMENTED;
	}
	return xdev->get_body_joints(xdev, body_tracking_type, desired_timestamp_ns, out_value);
}

/*!
 * Helper function for @ref xrt_device::set_output.
 *
 * @copydoc xrt_device::set_output
 *
 * @public @memberof xrt_device
 */
static inline void
xrt_device_set_output(struct xrt_device *xdev, enum xrt_output_name name, const union xrt_output_value *value)
{
	xdev->set_output(xdev, name, value);
}

/*!
 * Helper function for @ref xrt_device::get_view_poses.
 *
 * @copydoc xrt_device::get_view_poses
 * @public @memberof xrt_device
 */
static inline void
xrt_device_get_view_poses(struct xrt_device *xdev,
                          const struct xrt_vec3 *default_eye_relation,
                          uint64_t at_timestamp_ns,
                          uint32_t view_count,
                          struct xrt_space_relation *out_head_relation,
                          struct xrt_fov *out_fovs,
                          struct xrt_pose *out_poses)
{
	xdev->get_view_poses(xdev, default_eye_relation, at_timestamp_ns, view_count, out_head_relation, out_fovs,
	                     out_poses);
}

/*!
 * Helper function for @ref xrt_device::compute_distortion.
 *
 * @copydoc xrt_device::compute_distortion
 *
 * @public @memberof xrt_device
 */
static inline bool
xrt_device_compute_distortion(
    struct xrt_device *xdev, uint32_t view, float u, float v, struct xrt_uv_triplet *out_result)
{
	return xdev->compute_distortion(xdev, view, u, v, out_result);
}

/*!
 * Helper function for @ref xrt_device::get_visibility_mask.
 *
 * @copydoc xrt_device::get_visibility_mask
 *
 * @public @memberof xrt_device
 */
static inline xrt_result_t
xrt_device_get_visibility_mask(struct xrt_device *xdev,
                               enum xrt_visibility_mask_type type,
                               uint32_t view_index,
                               struct xrt_visibility_mask **out_mask)
{
	if (xdev->get_visibility_mask == NULL) {
		return XRT_ERROR_NOT_IMPLEMENTED;
	}
	return xdev->get_visibility_mask(xdev, type, view_index, out_mask);
}

/*!
 * Helper function for @ref xrt_device::ref_space_usage.
 *
 * @copydoc xrt_device::ref_space_usage
 *
 * @public @memberof xrt_device
 */
static inline xrt_result_t
xrt_device_ref_space_usage(struct xrt_device *xdev,
                           enum xrt_reference_space_type type,
                           enum xrt_input_name name,
                           bool used)
{
	return xdev->ref_space_usage(xdev, type, name, used);
}

/*!
 * Helper function for @ref xrt_device::is_form_factor_available.
 *
 * @copydoc xrt_device::is_form_factor_available
 *
 * @public @memberof xrt_device
 */
static inline bool
xrt_device_is_form_factor_available(struct xrt_device *xdev, enum xrt_form_factor form_factor)
{
	return xdev->is_form_factor_available(xdev, form_factor);
}

/*!
 * Helper function for @ref xrt_device::get_battery_status.
 *
 * @copydoc xrt_device::get_battery_status
 *
 * @public @memberof xrt_device
 */
static inline xrt_result_t
xrt_device_get_battery_status(struct xrt_device *xdev, bool *out_present, bool *out_charging, float *out_charge)
{
	if (xdev->get_battery_status == NULL) {
		return XRT_ERROR_NOT_IMPLEMENTED;
	}
	return xdev->get_battery_status(xdev, out_present, out_charging, out_charge);
}

/*!
 * Helper function for @ref xrt_device::destroy.
 *
 * Handles nulls, sets your pointer to null.
 *
 * @public @memberof xrt_device
 */
static inline void
xrt_device_destroy(struct xrt_device **xdev_ptr)
{
	struct xrt_device *xdev = *xdev_ptr;
	if (xdev == NULL) {
		return;
	}

	xdev->destroy(xdev);
	*xdev_ptr = NULL;
}


#ifdef __cplusplus
} // extern "C"
#endif
