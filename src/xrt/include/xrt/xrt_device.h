// Copyright 2019-2024, Collabora, Ltd.
// Copyright 2024-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header defining an xrt display or controller device.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Moshi Turner <moshiturner@protonmail.com>
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup xrt_iface
 */

#pragma once

#include "xrt/xrt_defines.h"
#include "xrt/xrt_plane_detector.h"
#include "xrt/xrt_visibility_mask.h"
#include "xrt/xrt_limits.h"

#ifdef __cplusplus
extern "C" {
#endif

struct xrt_tracking;

#define XRT_DEVICE_NAME_LEN 256


/*!
 * A unique identifier for an @ref xrt_device instance.
 *
 * Each device is assigned a 64-bit ID when it is created in the Monado service
 * process, or in the application process for in-process setups. For
 * out-of-process clients the ID is synchronized over the IPC layer, so every
 * client connected to the same service refers to a given device by the same
 * value.
 *
 * The ID is intended for hash maps, caches, and similar lookups. Unlike a
 * memory pointer it is never reused after the device is destroyed.
 *
 * A value of zero is invalid and means the device ID has not been assigned,
 * matching the zero-initialized state of the struct.
 *
 * @ingroup xrt_iface
 */
struct xrt_device_id
{
	uint64_t val;
};

/*!
 * Value of an @ref xrt_device_id that has not been assigned.
 *
 * @ingroup xrt_iface
 * @relates xrt_device_id
 */
// clang-format off
#define XRT_DEVICE_ID_INVALID XRT_C11_COMPOUND(struct xrt_device_id){0}
// clang-format on

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
 * @brief Compositor information for a device.
 */
struct xrt_device_compositor_info
{
	/*!
	 * @brief The type of panel refresh the screen uses.
	 *
	 * This affects how scanout poses are queried for rolling refresh
	 * compensation: if the mode is @ref XRT_PANEL_REFRESH_TYPE_GLOBAL,
	 * the start and end scanout world poses will be identical; otherwise,
	 * separate start and end poses are queried using different predicted
	 * timestamps derived from the other members of
	 * @ref xrt_device_compositor_info.
	 *
	 * The transmission direction (if known) should be specified via
	 * @ref scanout_direction. Some features may require this information
	 * even for global-refresh panels, e.g. beam racing.
	 */
	enum xrt_panel_refresh_type panel_refresh_type;

	//! The direction scanout on the display occurs.
	enum xrt_scanout_direction scanout_direction;
	/*!
	 * The amount of time it takes to scanout the display in nanoseconds, from the start of the scanout, to the end
	 * of the scanout.
	 *
	 * @note alignas for 32 bit client support, see @ref ipc-design
	 */
	XRT_ALIGNAS(8) int64_t scanout_time_ns;
};

/*!
 * @brief Compositor mode information for a device.
 */
struct xrt_device_compositor_mode
{
	//! The amount of nanoseconds between frames.
	int64_t frame_interval_ns;
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

	//! alignas for 32 bit client support, see @ref ipc-design
	XRT_ALIGNAS(8) int64_t timestamp;

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
 * Higher level features for devices.
 */
enum xrt_device_feature_type
{
	XRT_DEVICE_FEATURE_HAND_TRACKING_LEFT = 0,
	XRT_DEVICE_FEATURE_HAND_TRACKING_RIGHT,
	XRT_DEVICE_FEATURE_EYE_TRACKING,
	XRT_DEVICE_FEATURE_FACE_TRACKING,
	XRT_DEVICE_FEATURE_MAX_ENUM,
};

/*!
 * Output limits of a particular device
 */
struct xrt_output_limits
{
	//! The sample rate of the device's haptic PCM support, 0 if haptic PCM is not supported.
	float haptic_pcm_sample_rate;
};

/*!
 * Static data of supported features of the @ref xrt_device this struct sits on.
 *
 * This struct needs to always be a piece of data as it sits inside of the
 * shared memory area in the IPC layer, so no pointers please.
 *
 * @ingroup xrt_iface
 */
struct xrt_device_supported
{
	bool orientation_tracking;
	bool position_tracking;
	bool hand_tracking;
	bool eye_gaze;
	bool presence;
	bool force_feedback;
	bool ref_space_usage;
	bool form_factor_check;
	bool stage;
	bool face_tracking;
	bool face_tracking_calibration_state;
	bool body_tracking;
	bool body_tracking_calibration;
	bool body_tracking_fidelity;
	bool battery_status;
	bool brightness_control;
	bool compositor_info;
	bool notify_chirality;

	bool planes;
	enum xrt_plane_detection_capability_flags_ext plane_capability_flags;

	/*!
	 * The device itself only has 2 views but is aware of quad views, has
	 * better knowledge of which fovs to use for quad views to stereo views
	 * emulation. So it supports retrieving quad views from get_views.
	 */
	bool get_views_quad;
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
	//! Instance identifier, see @ref xrt_device_id.
	struct xrt_device_id id;

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

	//! Number of inputs.
	size_t input_count;
	//! Array of input structs.
	struct xrt_input *inputs;

	//! Number of outputs.
	size_t output_count;
	//! Array of output structs.
	struct xrt_output *outputs;

	//! What features/functions/things does this device supports?
	struct xrt_device_supported supported;


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
	xrt_result_t (*update_inputs)(struct xrt_device *xdev);

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
	xrt_result_t (*get_tracked_pose)(struct xrt_device *xdev,
	                                 enum xrt_input_name name,
	                                 int64_t at_timestamp_ns,
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
	xrt_result_t (*get_hand_tracking)(struct xrt_device *xdev,
	                                  enum xrt_input_name name,
	                                  int64_t desired_timestamp_ns,
	                                  struct xrt_hand_joint_set *out_value,
	                                  int64_t *out_timestamp_ns);

	/*!
	 * @brief Get the requested blend shape properties & weights for a face tracker
	 *
	 * @param[in] xdev                    The device.
	 * @param[in] facial_expression_type  The facial expression data type (XR_FB_face_tracking,
	 * XR_HTC_facial_tracking, etc).
	 * @param[in] at_timestamp_ns         Timestamp to be optionally used for prediction/history. For OXR extensions
	 * that do not pass a timestamp, the current timestamp is used.
	 * @param[in] out_value               Set of requested expression weights & blend shape properties.
	 *
	 * @see xrt_input_name
	 */
	xrt_result_t (*get_face_tracking)(struct xrt_device *xdev,
	                                  enum xrt_input_name facial_expression_type,
	                                  int64_t at_timestamp_ns,
	                                  struct xrt_facial_expression_set *out_value);

	/*!
	 * @brief Gets the face tracking calibration state
	 *
	 * @param[in] xdev                    The device.
	 * @param[in] out_face_is_calibrated  Is face tracking calibrated?
	 *
	 * @see xrt_input_name
	 */
	xrt_result_t (*get_face_calibration_state_android)(struct xrt_device *xdev, bool *out_face_is_calibrated);

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
	                                int64_t desired_timestamp_ns,
	                                struct xrt_body_joint_set *out_value);

	/*!
	 * @brief XR_META_body_tracking_calibration - body tracking extension to reset the body tracking calibration
	 *
	 * @param[in] xdev              The body tracking device.
	 */
	xrt_result_t (*reset_body_tracking_calibration_meta)(struct xrt_device *xdev);

	/*!
	 * @brief XR_META_body_tracking_calibration - body tracking extension to suggest a body tracking calibration
	 * override
	 *
	 * @param[in] xdev              The body tracking device.
	 * @param[in] new_body_height   The suggested new body height to override.
	 */
	xrt_result_t (*set_body_tracking_calibration_override_meta)(struct xrt_device *xdev, float new_body_height);

	/*!
	 * @brief XR_META_body_tracking_fidelity - body tracking extension for request changing the tracking fidelity
	 *
	 * @param[in] xdev              The body tracking device.
	 * @param[in] new_fidelity      The new tracking fidelity mode.
	 *
	 * @see xrt_body_tracking_fidelity_meta
	 */
	xrt_result_t (*set_body_tracking_fidelity_meta)(struct xrt_device *xdev,
	                                                enum xrt_body_tracking_fidelity_meta new_fidelity);

	/*!
	 * Set a output value.
	 *
	 * @param[in] xdev           The device.
	 * @param[in] name           The output component name to set.
	 * @param[in] value          The value to set the output to.
	 * @see xrt_output_name
	 */
	xrt_result_t (*set_output)(struct xrt_device *xdev,
	                           enum xrt_output_name name,
	                           const struct xrt_output_value *value);

	/*!
	 * Gets limits of this devices outputs.
	 *
	 * @param[in] xdev           The device.
	 * @param[out] limits        The returned limits.
	 */
	xrt_result_t (*get_output_limits)(struct xrt_device *xdev, struct xrt_output_limits *limits);

	/*!
	 * Begin a plane detection request
	 *
	 * @param[in] xdev               The device.
	 * @param[in] begin_info         The query specifying what type of planes are requested.
	 * @param[in] plane_detection_id  The id for a previous plane detection request to be replaced or 0.
	 * @param[out] out_plane_detection_id  The id of the new plane detection request generated by the xdev.
	 * @return generally XRT_SUCCESS, except for internal runtime failures.
	 */
	xrt_result_t (*begin_plane_detection_ext)(struct xrt_device *xdev,
	                                          const struct xrt_plane_detector_begin_info_ext *begin_info,
	                                          uint64_t plane_detection_id,
	                                          uint64_t *out_plane_detection_id);

	/*!
	 * Destroy internal resources associated with plane_detector_id.
	 *
	 * @param[in] xdev               The device.
	 * @param[in] plane_detection_id  An id generated by the xdev.
	 * @return generally XRT_SUCCESS, except for internal runtime failures.
	 */
	xrt_result_t (*destroy_plane_detection_ext)(struct xrt_device *xdev, uint64_t plane_detection_id);

	/*!
	 * Get the state of a plane detection request.
	 *
	 * @param[in] xdev               The device.
	 * @param[in] plane_detector_id  An id generated by the xdev.
	 * @param[out] out_state         The state of the plane detection.
	 * @return generally XRT_SUCCESS, except for internal runtime failures.
	 */
	xrt_result_t (*get_plane_detection_state_ext)(struct xrt_device *xdev,
	                                              uint64_t plane_detection_id,
	                                              enum xrt_plane_detector_state_ext *out_state);

	/*!
	 * Get results of a plane detection request.
	 *
	 * @param[in] xdev               The device.
	 * @param[in] plane_detector_id  An id generated by the xdev.
	 * @param[out] detections        The detected planes, if any.
	 * @return generally XRT_SUCCESS, except for internal runtime failures.
	 */
	xrt_result_t (*get_plane_detections_ext)(struct xrt_device *xdev,
	                                         uint64_t plane_detection_id,
	                                         struct xrt_plane_detections_ext *out_detections);

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
	 * @param[in] view_type    Type of view configuration (mono or stereo).
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
	xrt_result_t (*get_view_poses)(struct xrt_device *xdev,
	                               const struct xrt_vec3 *default_eye_relation,
	                               int64_t at_timestamp_ns,
	                               enum xrt_view_type view_type,
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
	xrt_result_t (*compute_distortion)(
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
	 * @param[out] out_available true if given form factor is available; otherwise false.
	 */
	xrt_result_t (*is_form_factor_available)(struct xrt_device *xdev,
	                                         enum xrt_form_factor form_factor,
	                                         bool *out_available);

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
	 * @brief Get the current display brightness.
	 *
	 * @param[in] xdev             The device.
	 * @param[out] out_brightness  Current display brightness. Usually between 0 and 1. Some devices may
	 *                             exceed 1 if the supported range exceeds 100%
	 */
	xrt_result_t (*get_brightness)(struct xrt_device *xdev, float *out_brightness);

	/*!
	 * @brief Set the display brightness.
	 *
	 * @param[in] xdev            The device.
	 * @param[in] brightness      Desired display brightness. Usually between 0 and 1. Some devices may
	 *                            allow exceeding 1 if the supported range exceeds 100%, but it will be clamped to
	 *                            the supported range.
	 * @param[in] relative        Whether to add \a brightness to the current brightness, instead of overwriting
	 *                            the current brightness.
	 */
	xrt_result_t (*set_brightness)(struct xrt_device *xdev, float brightness, bool relative);

	/*!
	 * @brief Gets the compositor info for a device for the given mode.
	 *
	 * This function should never block, and never wait on congested locks.
	 *
	 * @param[in] xdev            The device.
	 * @param[in] mode            The mode to query with.
	 * @param[out] out_info       Populated with the output data.
	 */
	xrt_result_t (*get_compositor_info)(struct xrt_device *xdev,
	                                    const struct xrt_device_compositor_mode *mode,
	                                    struct xrt_device_compositor_info *out_info);

	/*!
	 * Enable the feature for this device.
	 *
	 * @param[in] xdev        The device.
	 * @param[in] type        The type of device feature.
	 */
	xrt_result_t (*begin_feature)(struct xrt_device *xdev, enum xrt_device_feature_type type);

	/*!
	 * Disable the feature for this device.
	 *
	 * @param[in] xdev        The device.
	 * @param[in] type        The type of device feature.
	 */
	xrt_result_t (*end_feature)(struct xrt_device *xdev, enum xrt_device_feature_type type);

	/*!
	 * Notify the device of it's new chirality, or that it no longer has a set chirality.
	 *
	 * Devices are assumed to have no set chirality by default.
	 *
	 * @param[in] xdev          The device.
	 * @param[in] has_chirality Whether or not the device has a set chirality.
	 * @param[in] chirality     The device's current chirality (unset if `has_chirality` is false).
	 */
	xrt_result_t (*notify_chirality)(struct xrt_device *xdev, bool has_chirality, enum xrt_hand chirality);

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
XRT_NONNULL_ALL static inline xrt_result_t
xrt_device_update_inputs(struct xrt_device *xdev)
{
	return xdev->update_inputs(xdev);
}

/*!
 * Helper function for @ref xrt_device::get_tracked_pose.
 *
 * @copydoc xrt_device::get_tracked_pose
 *
 * @public @memberof xrt_device
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_device_get_tracked_pose(struct xrt_device *xdev,
                            enum xrt_input_name name,
                            int64_t at_timestamp_ns,
                            struct xrt_space_relation *out_relation)
{
	return xdev->get_tracked_pose(xdev, name, at_timestamp_ns, out_relation);
}

/*!
 * Helper function for @ref xrt_device::get_hand_tracking.
 *
 * @copydoc xrt_device::get_hand_tracking
 *
 * @public @memberof xrt_device
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_device_get_hand_tracking(struct xrt_device *xdev,
                             enum xrt_input_name name,
                             int64_t desired_timestamp_ns,
                             struct xrt_hand_joint_set *out_value,
                             int64_t *out_timestamp_ns)
{
	return xdev->get_hand_tracking(xdev, name, desired_timestamp_ns, out_value, out_timestamp_ns);
}

/*!
 * Helper function for @ref xrt_device::get_face_tracking.
 *
 * @copydoc xrt_device::get_face_tracking
 *
 * @public @memberof xrt_device
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_device_get_face_tracking(struct xrt_device *xdev,
                             enum xrt_input_name facial_expression_type,
                             int64_t at_timestamp_ns,
                             struct xrt_facial_expression_set *out_value)
{
	return xdev->get_face_tracking(xdev, facial_expression_type, at_timestamp_ns, out_value);
}

/*!
 * Helper function for @ref xrt_device::get_face_calibration_state_android.
 *
 * @copydoc xrt_device::get_face_calibration_state_android
 *
 * @public @memberof xrt_device
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_device_get_face_calibration_state_android(struct xrt_device *xdev, bool *out_face_is_calibrated)
{
	return xdev->get_face_calibration_state_android(xdev, out_face_is_calibrated);
}

/*!
 * Helper function for @ref xrt_device::get_body_skeleton.
 *
 * @copydoc xrt_device::get_body_skeleton
 *
 * @public @memberof xrt_device
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_device_get_body_skeleton(struct xrt_device *xdev,
                             enum xrt_input_name body_tracking_type,
                             struct xrt_body_skeleton *out_value)
{
	return xdev->get_body_skeleton(xdev, body_tracking_type, out_value);
}

/*!
 * Helper function for @ref xrt_device::get_body_joints.
 *
 * @copydoc xrt_device::get_body_joints
 *
 * @public @memberof xrt_device
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_device_get_body_joints(struct xrt_device *xdev,
                           enum xrt_input_name body_tracking_type,
                           int64_t desired_timestamp_ns,
                           struct xrt_body_joint_set *out_value)
{
	return xdev->get_body_joints(xdev, body_tracking_type, desired_timestamp_ns, out_value);
}

/*!
 * Helper function for @ref xrt_device::reset_body_tracking_calibration_meta.
 *
 * @copydoc xrt_device::reset_body_tracking_calibration_meta
 *
 * @public @memberof xrt_device
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_device_reset_body_tracking_calibration_meta(struct xrt_device *xdev)
{
	if (xdev->reset_body_tracking_calibration_meta == NULL) {
		return XRT_ERROR_NOT_IMPLEMENTED;
	}
	return xdev->reset_body_tracking_calibration_meta(xdev);
}

/*!
 * Helper function for @ref xrt_device::set_body_tracking_calibration_override_meta.
 *
 * @copydoc xrt_device::set_body_tracking_calibration_override_meta
 *
 * @public @memberof xrt_device
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_device_set_body_tracking_calibration_override_meta(struct xrt_device *xdev, float new_body_height)
{
	if (xdev->set_body_tracking_calibration_override_meta == NULL) {
		return XRT_ERROR_NOT_IMPLEMENTED;
	}
	return xdev->set_body_tracking_calibration_override_meta(xdev, new_body_height);
}

/*!
 * Helper function for @ref xrt_device::set_body_tracking_fidelity_meta.
 *
 * @copydoc xrt_device::set_body_tracking_fidelity_meta
 *
 * @public @memberof xrt_device
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_device_set_body_tracking_fidelity_meta(struct xrt_device *xdev, enum xrt_body_tracking_fidelity_meta new_fidelity)
{
	if (xdev->set_body_tracking_fidelity_meta == NULL) {
		return XRT_ERROR_NOT_IMPLEMENTED;
	}
	return xdev->set_body_tracking_fidelity_meta(xdev, new_fidelity);
}

/*!
 * Helper function for @ref xrt_device::set_output.
 *
 * @copydoc xrt_device::set_output
 *
 * @public @memberof xrt_device
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_device_set_output(struct xrt_device *xdev, enum xrt_output_name name, const struct xrt_output_value *value)
{
	xdev->set_output(xdev, name, value);
	return XRT_SUCCESS;
}

XRT_NONNULL_ALL static inline xrt_result_t
xrt_device_get_output_limits(struct xrt_device *xdev, struct xrt_output_limits *limits)
{
	if (xdev->get_output_limits) {
		return xdev->get_output_limits(xdev, limits);
	} else {
		return XRT_ERROR_NOT_IMPLEMENTED;
	}
}

/*!
 * Helper function for @ref xrt_device::begin_plane_detection_ext.
 *
 * @public @memberof xrt_device
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_device_begin_plane_detection_ext(struct xrt_device *xdev,
                                     const struct xrt_plane_detector_begin_info_ext *begin_info,
                                     uint64_t plane_detection_id,
                                     uint64_t *out_plane_detection_id)
{
	return xdev->begin_plane_detection_ext(xdev, begin_info, plane_detection_id, out_plane_detection_id);
}

/*!
 * Helper function for @ref xrt_device::destroy_plane_detection_ext.
 *
 * @public @memberof xrt_device
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_device_destroy_plane_detection_ext(struct xrt_device *xdev, uint64_t plane_detection_id)
{
	return xdev->destroy_plane_detection_ext(xdev, plane_detection_id);
}

/*!
 * Helper function for @ref xrt_device::get_plane_detections_ext.
 *
 * @public @memberof xrt_device
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_device_get_plane_detection_state_ext(struct xrt_device *xdev,
                                         uint64_t plane_detection_id,
                                         enum xrt_plane_detector_state_ext *out_state)
{
	return xdev->get_plane_detection_state_ext(xdev, plane_detection_id, out_state);
}

/*!
 * Helper function for @ref xrt_device::get_plane_detections_ext.
 *
 * @public @memberof xrt_device
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_device_get_plane_detections_ext(struct xrt_device *xdev,
                                    uint64_t plane_detection_id,
                                    struct xrt_plane_detections_ext *out_detections)
{
	return xdev->get_plane_detections_ext(xdev, plane_detection_id, out_detections);
}

/*!
 * Helper function for @ref xrt_device::get_view_poses.
 *
 * @copydoc xrt_device::get_view_poses
 * @public @memberof xrt_device
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_device_get_view_poses(struct xrt_device *xdev,
                          const struct xrt_vec3 *default_eye_relation,
                          int64_t at_timestamp_ns,
                          enum xrt_view_type view_type,
                          uint32_t view_count,
                          struct xrt_space_relation *out_head_relation,
                          struct xrt_fov *out_fovs,
                          struct xrt_pose *out_poses)
{
	return xdev->get_view_poses( //
	    xdev,                    //
	    default_eye_relation,    //
	    at_timestamp_ns,         //
	    view_type,               //
	    view_count,              //
	    out_head_relation,       //
	    out_fovs,                //
	    out_poses);              //
}

/*!
 * Helper function for @ref xrt_device::compute_distortion.
 *
 * @copydoc xrt_device::compute_distortion
 *
 * @public @memberof xrt_device
 */
XRT_NONNULL_ALL static inline xrt_result_t
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
XRT_NONNULL_ALL static inline xrt_result_t
xrt_device_get_visibility_mask(struct xrt_device *xdev,
                               enum xrt_visibility_mask_type type,
                               uint32_t view_index,
                               struct xrt_visibility_mask **out_mask)
{
	return xdev->get_visibility_mask(xdev, type, view_index, out_mask);
}

/*!
 * Helper function for @ref xrt_device::ref_space_usage.
 *
 * @copydoc xrt_device::ref_space_usage
 *
 * @public @memberof xrt_device
 */
XRT_NONNULL_ALL static inline xrt_result_t
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
XRT_NONNULL_ALL static inline xrt_result_t
xrt_device_is_form_factor_available(struct xrt_device *xdev, enum xrt_form_factor form_factor, bool *out_available)
{
	return xdev->is_form_factor_available(xdev, form_factor, out_available);
}

/*!
 * Helper function for @ref xrt_device::get_battery_status.
 *
 * @copydoc xrt_device::get_battery_status
 *
 * @public @memberof xrt_device
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_device_get_battery_status(struct xrt_device *xdev, bool *out_present, bool *out_charging, float *out_charge)
{
	return xdev->get_battery_status(xdev, out_present, out_charging, out_charge);
}

/*!
 * Helper function for @ref xrt_device::get_brightness.
 *
 * @copydoc xrt_device::get_brightness
 * @public @memberof xrt_device
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_device_get_brightness(struct xrt_device *xdev, float *out_brightness)
{
	return xdev->get_brightness(xdev, out_brightness);
}

/*!
 * Helper function for @ref xrt_device::set_brightness.
 *
 * @copydoc xrt_device::set_brightness
 * @public @memberof xrt_device
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_device_set_brightness(struct xrt_device *xdev, float brightness, bool relative)
{
	return xdev->set_brightness(xdev, brightness, relative);
}

/*!
 * Helper function for @ref xrt_device::get_compositor_info.
 *
 * @copydoc xrt_device::get_compositor_info
 *
 * @public @memberof xrt_device
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_device_get_compositor_info(struct xrt_device *xdev,
                               const struct xrt_device_compositor_mode *mode,
                               struct xrt_device_compositor_info *out_info)
{
	return xdev->get_compositor_info(xdev, mode, out_info);
}

/*!
 * Helper function for @ref xrt_device::begin_feature.
 *
 * @copydoc xrt_device::begin_feature
 *
 * @public @memberof xrt_device
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_device_begin_feature(struct xrt_device *xdev, enum xrt_device_feature_type type)
{
	return xdev->begin_feature(xdev, type);
}

/*!
 * Helper function for @ref xrt_device::end_feature.
 *
 * @copydoc xrt_device::end_feature
 *
 * @public @memberof xrt_device
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_device_end_feature(struct xrt_device *xdev, enum xrt_device_feature_type type)
{
	return xdev->end_feature(xdev, type);
}

/*!
 * Helper function for @ref xrt_device::notify_chirality.
 *
 * @copydoc xrt_device::notify_chirality
 *
 * @public @memberof xrt_device
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_device_notify_chirality(struct xrt_device *xdev, bool has_chirality, enum xrt_hand chirality)
{
	return xdev->notify_chirality(xdev, has_chirality, chirality);
}

/*!
 * Helper function for @ref xrt_device::destroy.
 *
 * Handles nulls, sets your pointer to null.
 *
 * @public @memberof xrt_device
 */
XRT_NONNULL_ALL static inline void
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
