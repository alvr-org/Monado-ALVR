// Copyright 2018-2024, Collabora, Ltd.
// Copyright 2023-2026, NVIDIA CORPORATION.
// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  The objects representing OpenXR handles, and prototypes for internal functions used in the state tracker.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup oxr_main
 */

#pragma once

#include "xrt/xrt_space.h"
#include "xrt/xrt_limits.h"
#include "xrt/xrt_system.h"
#include "xrt/xrt_future.h"
#include "xrt/xrt_device.h"
#include "xrt/xrt_tracking.h"
#include "xrt/xrt_compositor.h"
#include "xrt/xrt_vulkan_includes.h"
#include "xrt/xrt_openxr_includes.h"
#include "xrt/xrt_config_os.h"
#include "xrt/xrt_config_have.h"
#include "xrt/xrt_app_policy.h"

#include "os/os_threading.h"

#include "util/u_index_fifo.h"
#include "util/u_hashset.h"
#include "util/u_hashmap.h"
#include "util/u_device.h"

#include "oxr_extension_support.h"
#include "oxr_defines.h"
#include "oxr_handle_cast.h"
#include "oxr_handle_base.h"
#include "oxr_frame_sync.h"
#include "oxr_forward_declarations.h"
#include "oxr_refcounted.h"

#include "path/oxr_path_store.h"

#include "actions/oxr_subaction.h"
#include "actions/oxr_dpad_state.h"
#include "actions/oxr_session_action_context.h"
#include "actions/oxr_session_attached_actions.h"
#include "actions/oxr_instance_path_cache.h"
#include "actions/oxr_instance_action_context.h"


#if defined(XRT_HAVE_D3D11) || defined(XRT_HAVE_D3D12)
#include <dxgi.h>
#include <d3dcommon.h>
#endif

#ifdef XRT_FEATURE_RENDERDOC
#include "renderdoc_app.h"
#ifndef XRT_OS_WINDOWS
#include <dlfcn.h>
#endif // !XRT_OS_WINDOWS
#endif // XRT_FEATURE_RENDERDOC

#ifdef XRT_OS_ANDROID
#include "xrt/xrt_android.h"
#endif // #ifdef XRT_OS_ANDROID

#ifdef __cplusplus
extern "C" {
#endif


/*!
 * @defgroup oxr OpenXR state tracker
 *
 * Client application facing code.
 *
 * @ingroup xrt
 */

/*!
 * @defgroup oxr_main OpenXR main code
 *
 * Gets called from @ref oxr_api functions and talks to devices and
 * @ref comp using @ref xrt_iface.
 *
 * @ingroup oxr
 * @{
 */


#define OXR_MAX_BINDINGS_PER_ACTION 32


/*
 *
 * Helpers
 *
 */

/*!
 * Safely copy an xrt_pose to an XrPosef.
 */
#define OXR_XRT_POSE_TO_XRPOSEF(FROM, TO)                                                                              \
	do {                                                                                                           \
		union {                                                                                                \
			struct xrt_pose xrt;                                                                           \
			XrPosef oxr;                                                                                   \
		} safe_copy = {FROM};                                                                                  \
		TO = safe_copy.oxr;                                                                                    \
	} while (false)

/*!
 * Safely copy an xrt_fov to an XrFovf.
 */
#define OXR_XRT_FOV_TO_XRFOVF(FROM, TO)                                                                                \
	do {                                                                                                           \
		union {                                                                                                \
			struct xrt_fov xrt;                                                                            \
			XrFovf oxr;                                                                                    \
		} safe_copy = {FROM};                                                                                  \
		TO = safe_copy.oxr;                                                                                    \
	} while (false)


/*!
 *
 * @name oxr_instance.c
 * @{
 *
 */

/*!
 * To go back to a OpenXR object.
 *
 * @relates oxr_instance
 */
static inline XrInstance
oxr_instance_to_openxr(struct oxr_instance *inst)
{
	return XRT_CAST_PTR_TO_OXR_HANDLE(XrInstance, inst);
}

/*!
 * Creates a instance, does minimal validation of @p createInfo.
 *
 * @param[in]  log        Logger
 * @param[in]  createInfo OpenXR creation info.
 * @param[in]  extensions Parsed extension list to be enabled.
 * @param[out] out_inst   Pointer to pointer to a instance, returned instance.
 *
 * @public @static @memberof oxr_instance
 */
XrResult
oxr_instance_create(struct oxr_logger *log,
                    const XrInstanceCreateInfo *createInfo,
                    XrVersion major_minor,
                    const struct oxr_extension_status *extensions,
                    struct oxr_instance **out_inst);

/*!
 * Must be called with oxr_instance::system_init_lock held.
 *
 * @public @memberof oxr_instance
 */
XrResult
oxr_instance_init_system_locked(struct oxr_logger *log, struct oxr_instance *inst);

/*!
 * @public @memberof oxr_instance
 */
XrResult
oxr_instance_get_properties(struct oxr_logger *log,
                            struct oxr_instance *inst,
                            XrInstanceProperties *instanceProperties);

#if XR_USE_TIMESPEC

/*!
 * @public @memberof oxr_instance
 */
XrResult
oxr_instance_convert_time_to_timespec(struct oxr_logger *log,
                                      struct oxr_instance *inst,
                                      XrTime time,
                                      struct timespec *timespecTime);

/*!
 * @public @memberof oxr_instance
 */
XrResult
oxr_instance_convert_timespec_to_time(struct oxr_logger *log,
                                      struct oxr_instance *inst,
                                      const struct timespec *timespecTime,
                                      XrTime *time);
#endif // XR_USE_TIMESPEC

#ifdef XR_USE_PLATFORM_WIN32

/*!
 * @public @memberof oxr_instance
 */
XrResult
oxr_instance_convert_time_to_win32perfcounter(struct oxr_logger *log,
                                              struct oxr_instance *inst,
                                              XrTime time,
                                              LARGE_INTEGER *win32perfcounterTime);

/*!
 * @public @memberof oxr_instance
 */
XrResult
oxr_instance_convert_win32perfcounter_to_time(struct oxr_logger *log,
                                              struct oxr_instance *inst,
                                              const LARGE_INTEGER *win32perfcounterTime,
                                              XrTime *time);

#endif // XR_USE_PLATFORM_WIN32

/*!
 * @}
 */

/*!
 * To go back to a OpenXR object.
 *
 * @relates oxr_action_set
 */
static inline XrActionSet
oxr_action_set_to_openxr(struct oxr_action_set *act_set)
{
	return XRT_CAST_PTR_TO_OXR_HANDLE(XrActionSet, act_set);
}

/*!
 * To go back to a OpenXR object.
 *
 * @relates oxr_hand_tracker
 */
static inline XrHandTrackerEXT
oxr_hand_tracker_to_openxr(struct oxr_hand_tracker *hand_tracker)
{
	return XRT_CAST_PTR_TO_OXR_HANDLE(XrHandTrackerEXT, hand_tracker);
}

#ifdef OXR_HAVE_EXT_plane_detection
/*!
 * To go back to a OpenXR object.
 *
 * @relates oxr_plane_detector
 */
static inline XrPlaneDetectorEXT
oxr_plane_detector_to_openxr(struct oxr_plane_detector_ext *plane_detector)
{
	return XRT_CAST_PTR_TO_OXR_HANDLE(XrPlaneDetectorEXT, plane_detector);
}
#endif // OXR_HAVE_EXT_plane_detection

/*!
 * To go back to a OpenXR object.
 *
 * @relates oxr_action
 */
static inline XrAction
oxr_action_to_openxr(struct oxr_action *act)
{
	return XRT_CAST_PTR_TO_OXR_HANDLE(XrAction, act);
}

#ifdef OXR_HAVE_HTC_facial_tracking
/*!
 * To go back to a OpenXR object.
 *
 * @relates oxr_facial_tracker_htc
 */
static inline XrFacialTrackerHTC
oxr_facial_tracker_htc_to_openxr(struct oxr_facial_tracker_htc *face_tracker_htc)
{
	return XRT_CAST_PTR_TO_OXR_HANDLE(XrFacialTrackerHTC, face_tracker_htc);
}
#endif

#ifdef OXR_HAVE_FB_body_tracking
/*!
 * To go back to a OpenXR object.
 *
 * @relates oxr_facial_tracker_htc
 */
static inline XrBodyTrackerFB
oxr_body_tracker_fb_to_openxr(struct oxr_body_tracker_fb *body_tracker_fb)
{
	return XRT_CAST_PTR_TO_OXR_HANDLE(XrBodyTrackerFB, body_tracker_fb);
}
#endif

#ifdef OXR_HAVE_BD_body_tracking
/*!
 * To go back to a OpenXR object.
 *
 * @relates oxr_body_tracker_bd
 */
static inline XrBodyTrackerBD
oxr_body_tracker_bd_to_openxr(struct oxr_body_tracker_bd *body_tracker_bd)
{
	return XRT_CAST_PTR_TO_OXR_HANDLE(XrBodyTrackerBD, body_tracker_bd);
}
#endif

#ifdef OXR_HAVE_FB_face_tracking2
/*!
 * To go back to a OpenXR object.
 *
 * @relates oxr_face_tracker2_fb
 */
static inline XrFaceTracker2FB
oxr_face_tracker2_fb_to_openxr(struct oxr_face_tracker2_fb *face_tracker2_fb)
{
	return XRT_CAST_PTR_TO_OXR_HANDLE(XrFaceTracker2FB, face_tracker2_fb);
}
#endif

#ifdef OXR_HAVE_ANDROID_face_tracking
/*!
 * To go back to a OpenXR object.
 *
 * @relates oxr_facial_tracker_htc
 */
static inline XrFaceTrackerANDROID
oxr_face_tracker_android_to_openxr(struct oxr_face_tracker_android *face_tracker_android)
{
	return XRT_CAST_PTR_TO_OXR_HANDLE(XrFaceTrackerANDROID, face_tracker_android);
}
#endif

/*!
 *
 * @name oxr_session.c
 * @{
 *
 */

/*!
 * To go back to a OpenXR object.
 *
 * @relates oxr_session
 */
static inline XrSession
oxr_session_to_openxr(struct oxr_session *sess)
{
	return XRT_CAST_PTR_TO_OXR_HANDLE(XrSession, sess);
}

XrResult
oxr_session_create(struct oxr_logger *log,
                   struct oxr_system *sys,
                   const XrSessionCreateInfo *createInfo,
                   struct oxr_session **out_session);

XrResult
oxr_session_enumerate_formats(struct oxr_logger *log,
                              struct oxr_session *sess,
                              uint32_t formatCapacityInput,
                              uint32_t *formatCountOutput,
                              int64_t *formats);

#ifdef OXR_HAVE_EXT_enumerate_swapchain_formats2
XrResult
oxr_session_enumerate_formats2(struct oxr_logger *log,
                               struct oxr_session *sess,
                               XrSwapchainFormatsEnumerateInfoEXT *enumerateInfo,
                               uint32_t formatCapacityInput,
                               uint32_t *formatCountOutput,
                               int64_t *formats);
#endif

/*!
 * Change the state of the session, queues a event.
 */
void
oxr_session_change_state(struct oxr_logger *log, struct oxr_session *sess, XrSessionState state, XrTime time);

XrResult
oxr_session_begin(struct oxr_logger *log, struct oxr_session *sess, const XrSessionBeginInfo *beginInfo);

XrResult
oxr_session_end(struct oxr_logger *log, struct oxr_session *sess);

XrResult
oxr_session_request_exit(struct oxr_logger *log, struct oxr_session *sess);

XRT_CHECK_RESULT XrResult
oxr_session_poll(struct oxr_logger *log, struct oxr_session *sess);

XrResult
oxr_session_locate_views(struct oxr_logger *log,
                         struct oxr_session *sess,
                         const XrViewLocateInfo *viewLocateInfo,
                         XrViewState *viewState,
                         uint32_t viewCapacityInput,
                         uint32_t *viewCountOutput,
                         XrView *views);

XrResult
oxr_session_frame_wait(struct oxr_logger *log, struct oxr_session *sess, XrFrameState *frameState);

XrResult
oxr_session_frame_begin(struct oxr_logger *log, struct oxr_session *sess);

XrResult
oxr_session_frame_end(struct oxr_logger *log, struct oxr_session *sess, const XrFrameEndInfo *frameEndInfo);

/*
 * Gets the body pose in the base space.
 */
XrResult
oxr_get_base_body_pose(struct oxr_logger *log,
                       const struct xrt_body_joint_set *body_joint_set,
                       struct oxr_space *base_spc,
                       struct xrt_device *body_xdev,
                       XrTime at_time,
                       struct xrt_space_relation *out_base_body);

#ifdef OXR_HAVE_KHR_android_thread_settings
XrResult
oxr_session_android_thread_settings(struct oxr_logger *log,
                                    struct oxr_session *sess,
                                    XrAndroidThreadTypeKHR threadType,
                                    uint32_t threadId);
#endif // OXR_HAVE_KHR_android_thread_settings

#ifdef OXR_HAVE_KHR_visibility_mask
XrResult
oxr_session_get_visibility_mask(struct oxr_logger *log,
                                struct oxr_session *session,
                                XrVisibilityMaskTypeKHR visibilityMaskType,
                                uint32_t viewIndex,
                                XrVisibilityMaskKHR *visibilityMask);

XrResult
oxr_event_push_XrEventDataVisibilityMaskChangedKHR(struct oxr_logger *log,
                                                   struct oxr_session *sess,
                                                   XrViewConfigurationType viewConfigurationType,
                                                   uint32_t viewIndex);
#endif // OXR_HAVE_KHR_visibility_mask

#ifdef OXR_HAVE_EXT_performance_settings
XrResult
oxr_session_set_perf_level(struct oxr_logger *log,
                           struct oxr_session *sess,
                           XrPerfSettingsDomainEXT domain,
                           XrPerfSettingsLevelEXT level);
#endif // OXR_HAVE_EXT_performance_settings

/*
 *
 * oxr_space.c
 *
 */

/*!
 * To go back to a OpenXR object.
 */
static inline XrSpace
oxr_space_to_openxr(struct oxr_space *spc)
{
	return XRT_CAST_PTR_TO_OXR_HANDLE(XrSpace, spc);
}

XrResult
oxr_space_action_create(struct oxr_logger *log,
                        struct oxr_session *sess,
                        uint32_t key,
                        const XrActionSpaceCreateInfo *createInfo,
                        struct oxr_space **out_space);

XrResult
oxr_space_get_reference_bounds_rect(struct oxr_logger *log,
                                    struct oxr_session *sess,
                                    XrReferenceSpaceType referenceSpaceType,
                                    XrExtent2Df *bounds);

XrResult
oxr_space_reference_create(struct oxr_logger *log,
                           struct oxr_session *sess,
                           const XrReferenceSpaceCreateInfo *createInfo,
                           struct oxr_space **out_space);

/*!
 * Monado special space that always points to a specific @ref xrt_device and
 * pose, useful when you want to bypass the action binding system for instance.
 */
XrResult
oxr_space_xdev_pose_create(struct oxr_logger *log,
                           struct oxr_session *sess,
                           struct xrt_device *xdev,
                           enum xrt_input_name name,
                           const struct xrt_pose *pose,
                           struct oxr_space **out_space);

XrResult
oxr_space_locate(
    struct oxr_logger *log, struct oxr_space *spc, struct oxr_space *baseSpc, XrTime time, XrSpaceLocation *location);

XrResult
oxr_spaces_locate(struct oxr_logger *log,
                  struct oxr_space **spcs,
                  uint32_t spc_count,
                  struct oxr_space *baseSpc,
                  XrTime time,
                  XrSpaceLocations *locations);

/*!
 * Locate the @ref xrt_device in the given base space, useful for implementing
 * hand tracking location look ups and the like.
 *
 * @param      log          Logging struct.
 * @param      xdev         Device to locate in the base space.
 * @param      baseSpc      Base space where the device is to be located.
 * @param[in]  time         Time in OpenXR domain.
 * @param[out] out_relation Returns T_base_xdev, aka xdev in base space.
 *
 * @return Any errors, XR_SUCCESS, pose might not be valid on XR_SUCCESS.
 */
XRT_CHECK_RESULT XrResult
oxr_space_locate_device(struct oxr_logger *log,
                        struct xrt_device *xdev,
                        struct oxr_space *baseSpc,
                        XrTime time,
                        struct xrt_space_relation *out_relation);

/*!
 * Get the xrt_space associated with this oxr_space, the @ref xrt_space will
 * be reference counted by this function so the caller will need to call
 * @ref xrt_space_reference to decrement the reference count.
 *
 * @param      log          Logging struct.
 * @param      spc          Oxr space to get the xrt_space from.
 * @param[out] out_xspace   Returns the xrt_space associated with this oxr_space.
 * @return Any errors, XR_SUCCESS, xspace is not set on XR_ERROR_*.
 */
XRT_CHECK_RESULT XrResult
oxr_space_get_xrt_space(struct oxr_logger *log, struct oxr_space *spc, struct xrt_space **out_xspace);


/*
 *
 * oxr_swapchain.c
 *
 */

/*!
 * To go back to a OpenXR object.
 */
static inline XrSwapchain
oxr_swapchain_to_openxr(struct oxr_swapchain *sc)
{
	return XRT_CAST_PTR_TO_OXR_HANDLE(XrSwapchain, sc);
}


/*
 *
 * oxr_messenger.c
 *
 */

/*!
 * To go back to a OpenXR object.
 */
static inline XrDebugUtilsMessengerEXT
oxr_messenger_to_openxr(struct oxr_debug_messenger *mssngr)
{
	return XRT_CAST_PTR_TO_OXR_HANDLE(XrDebugUtilsMessengerEXT, mssngr);
}

XrResult
oxr_create_messenger(struct oxr_logger *,
                     struct oxr_instance *inst,
                     const XrDebugUtilsMessengerCreateInfoEXT *,
                     struct oxr_debug_messenger **out_mssngr);
XrResult
oxr_destroy_messenger(struct oxr_logger *log, struct oxr_debug_messenger *mssngr);


/*
 *
 * oxr_system.c
 *
 */

XrResult
oxr_system_select(struct oxr_logger *log,
                  struct oxr_system **systems,
                  uint32_t system_count,
                  XrFormFactor form_factor,
                  struct oxr_system **out_selected);

XrResult
oxr_system_fill_in(struct oxr_logger *log,
                   struct oxr_instance *inst,
                   XrSystemId systemId,
                   uint32_t view_count,
                   struct oxr_system *sys);

XrResult
oxr_system_verify_id(struct oxr_logger *log, const struct oxr_instance *inst, XrSystemId systemId);

XrResult
oxr_system_get_by_id(struct oxr_logger *log,
                     struct oxr_instance *inst,
                     XrSystemId systemId,
                     struct oxr_system **system);

XrResult
oxr_system_get_properties(struct oxr_logger *log, struct oxr_system *sys, XrSystemProperties *properties);

XrResult
oxr_system_enumerate_view_confs(struct oxr_logger *log,
                                struct oxr_system *sys,
                                uint32_t viewConfigurationTypeCapacityInput,
                                uint32_t *viewConfigurationTypeCountOutput,
                                XrViewConfigurationType *viewConfigurationTypes);

XrResult
oxr_system_enumerate_blend_modes(struct oxr_logger *log,
                                 struct oxr_system *sys,
                                 XrViewConfigurationType viewConfigurationType,
                                 uint32_t environmentBlendModeCapacityInput,
                                 uint32_t *environmentBlendModeCountOutput,
                                 XrEnvironmentBlendMode *environmentBlendModes);

XrResult
oxr_system_get_view_conf_properties(struct oxr_logger *log,
                                    struct oxr_system *sys,
                                    XrViewConfigurationType viewConfigurationType,
                                    XrViewConfigurationProperties *configurationProperties);

XrResult
oxr_system_enumerate_view_conf_views(struct oxr_logger *log,
                                     struct oxr_system *sys,
                                     XrViewConfigurationType viewConfigurationType,
                                     uint32_t viewCapacityInput,
                                     uint32_t *viewCountOutput,
                                     XrViewConfigurationView *views);

bool
oxr_system_get_hand_tracking_support(struct oxr_logger *log, struct oxr_instance *inst);

bool
oxr_system_get_eye_gaze_support(struct oxr_logger *log, struct oxr_instance *inst);

bool
oxr_system_get_force_feedback_support(struct oxr_logger *log, struct oxr_instance *inst);

void
oxr_system_get_face_tracking_android_support(struct oxr_logger *log, struct oxr_instance *inst, bool *supported);

void
oxr_system_get_face_tracking_htc_support(struct oxr_logger *log,
                                         struct oxr_instance *inst,
                                         bool *supports_eye,
                                         bool *supports_lip);

void
oxr_system_get_face_tracking2_fb_support(struct oxr_logger *log,
                                         struct oxr_instance *inst,
                                         bool *supports_audio,
                                         bool *supports_visual);

bool
oxr_system_get_body_tracking_fb_support(struct oxr_logger *log, struct oxr_instance *inst);

bool
oxr_system_get_full_body_tracking_meta_support(struct oxr_logger *log, struct oxr_instance *inst);

bool
oxr_system_get_body_tracking_calibration_meta_support(struct oxr_logger *log, struct oxr_instance *inst);

bool
oxr_system_get_body_tracking_fidelity_meta_support(struct oxr_logger *log, struct oxr_instance *inst);

XRT_CHECK_RESULT XrResult
oxr_system_poll(struct oxr_logger *log, struct oxr_system *sys);

/*
 *
 * oxr_event.cpp
 *
 */

XrResult
oxr_event_push_XrEventDataSessionStateChanged(struct oxr_logger *log,
                                              struct oxr_session *sess,
                                              XrSessionState state,
                                              XrTime time);

XrResult
oxr_event_push_XrEventDataInteractionProfileChanged(struct oxr_logger *log, struct oxr_session *sess);

XrResult
oxr_event_push_XrEventDataReferenceSpaceChangePending(struct oxr_logger *log,
                                                      struct oxr_session *sess,
                                                      XrReferenceSpaceType referenceSpaceType,
                                                      XrTime changeTime,
                                                      XrBool32 poseValid,
                                                      const XrPosef *poseInPreviousSpace);

#ifdef OXR_HAVE_FB_display_refresh_rate
XrResult
oxr_event_push_XrEventDataDisplayRefreshRateChangedFB(struct oxr_logger *log,
                                                      struct oxr_session *sess,
                                                      float fromDisplayRefreshRate,
                                                      float toDisplayRefreshRate);
#endif // OXR_HAVE_FB_display_refresh_rate

#ifdef OXR_HAVE_EXTX_overlay
XrResult
oxr_event_push_XrEventDataMainSessionVisibilityChangedEXTX(struct oxr_logger *log,
                                                           struct oxr_session *sess,
                                                           bool visible);
#endif // OXR_HAVE_EXTX_overlay

#ifdef OXR_HAVE_EXT_performance_settings
XrResult
oxr_event_push_XrEventDataPerfSettingsEXT(struct oxr_logger *log,
                                          struct oxr_session *sess,
                                          enum xrt_perf_domain domain,
                                          enum xrt_perf_sub_domain subDomain,
                                          enum xrt_perf_notify_level fromLevel,
                                          enum xrt_perf_notify_level toLevel);
#endif // OXR_HAVE_EXT_performance_settings

#ifdef OXR_HAVE_EXT_view_configuration_views_change
XrResult
oxr_event_push_XrEventDataViewConfigurationViewsChangedEXT(struct oxr_logger *log,
                                                           struct oxr_system *system,
                                                           enum xrt_view_type viewConfigurationType);
#endif // OXR_HAVE_EXT_view_configuration_views_change

/*!
 * This clears all pending events refers to the given session.
 */
XrResult
oxr_event_remove_session_events(struct oxr_logger *log, struct oxr_session *sess);

/*!
 * Will return one event if available, also drain the sessions event queues.
 */
XrResult
oxr_poll_event(struct oxr_logger *log, struct oxr_instance *inst, XrEventDataBuffer *eventData);


/*
 *
 * oxr_xdev.c
 *
 */

void
oxr_xdev_destroy(struct xrt_device **xdev_ptr);

/*!
 * Return true if it finds an input of that name on this device.
 */
bool
oxr_xdev_find_input(struct xrt_device *xdev, enum xrt_input_name name, struct xrt_input **out_input);

/*!
 * Return true if it finds an output of that name on this device.
 */
bool
oxr_xdev_find_output(struct xrt_device *xdev, enum xrt_output_name name, struct xrt_output **out_output);

#ifdef OXR_HAVE_MNDX_xdev_space
static inline XrXDevListMNDX
oxr_xdev_list_to_openxr(struct oxr_xdev_list *sc)
{
	return XRT_CAST_PTR_TO_OXR_HANDLE(XrXDevListMNDX, sc);
}

XrResult
oxr_xdev_list_create(struct oxr_logger *log,
                     struct oxr_session *sess,
                     const XrCreateXDevListInfoMNDX *createInfo,
                     struct oxr_xdev_list **out_xdl);

XrResult
oxr_xdev_list_get_properties(struct oxr_logger *log,
                             struct oxr_xdev_list *xdl,
                             uint32_t index,
                             XrXDevPropertiesMNDX *properties);

XrResult
oxr_xdev_list_space_create(struct oxr_logger *log,
                           struct oxr_xdev_list *xdl,
                           const XrCreateXDevSpaceInfoMNDX *createInfo,
                           uint32_t index,
                           struct oxr_space **out_space);

bool
oxr_xdev_list_get_xdev(struct oxr_logger *log,
                       struct oxr_xdev_list *xdl,
                       XrXDevIdMNDX id,
                       struct xrt_device **out_xdev);

#endif // OXR_HAVE_MNDX_xdev_space


/*
 *
 * OpenGL, located in various files.
 *
 */

#ifdef XR_USE_GRAPHICS_API_OPENGL
#ifdef XR_USE_PLATFORM_XLIB

XrResult
oxr_session_populate_gl_xlib(struct oxr_logger *log,
                             struct oxr_system *sys,
                             XrGraphicsBindingOpenGLXlibKHR const *next,
                             struct oxr_session *sess);
#endif // XR_USE_PLATFORM_XLIB

#ifdef XR_USE_PLATFORM_WIN32
XrResult
oxr_session_populate_gl_win32(struct oxr_logger *log,
                              struct oxr_system *sys,
                              XrGraphicsBindingOpenGLWin32KHR const *next,
                              struct oxr_session *sess);
#endif // XR_USE_PLATFORM_WIN32
#endif // XR_USE_GRAPHICS_API_OPENGL

#if defined(XR_USE_GRAPHICS_API_OPENGL) || defined(XR_USE_GRAPHICS_API_OPENGL_ES)
XrResult
oxr_swapchain_gl_create(struct oxr_logger * /*log*/,
                        struct oxr_session *sess,
                        const XrSwapchainCreateInfo * /*createInfo*/,
                        struct oxr_swapchain **out_swapchain);

#endif // XR_USE_GRAPHICS_API_OPENGL || XR_USE_GRAPHICS_API_OPENGL_ES

#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
#if defined(XR_USE_PLATFORM_ANDROID)
XrResult
oxr_session_populate_gles_android(struct oxr_logger *log,
                                  struct oxr_system *sys,
                                  XrGraphicsBindingOpenGLESAndroidKHR const *next,
                                  struct oxr_session *sess);
#endif // XR_USE_PLATFORM_ANDROID
#endif // XR_USE_GRAPHICS_API_OPENGL_ES


/*
 *
 * Vulkan, located in various files.
 *
 */

#ifdef XR_USE_GRAPHICS_API_VULKAN

XrResult
oxr_vk_get_instance_exts(struct oxr_logger *log,
                         struct oxr_system *sys,
                         uint32_t namesCapacityInput,
                         uint32_t *namesCountOutput,
                         char *namesString);

XrResult
oxr_vk_get_device_exts(struct oxr_logger *log,
                       struct oxr_system *sys,
                       uint32_t namesCapacityInput,
                       uint32_t *namesCountOutput,
                       char *namesString);

XrResult
oxr_vk_get_requirements(struct oxr_logger *log,
                        struct oxr_system *sys,
                        XrGraphicsRequirementsVulkanKHR *graphicsRequirements);

XrResult
oxr_vk_create_vulkan_instance(struct oxr_logger *log,
                              struct oxr_system *sys,
                              const XrVulkanInstanceCreateInfoKHR *createInfo,
                              VkInstance *vulkanInstance,
                              VkResult *vulkanResult);

XrResult
oxr_vk_create_vulkan_device(struct oxr_logger *log,
                            struct oxr_system *sys,
                            const XrVulkanDeviceCreateInfoKHR *createInfo,
                            VkDevice *vulkanDevice,
                            VkResult *vulkanResult);

XrResult
oxr_vk_get_physical_device(struct oxr_logger *log,
                           struct oxr_instance *inst,
                           struct oxr_system *sys,
                           VkInstance vkInstance,
                           PFN_vkGetInstanceProcAddr getProc,
                           VkPhysicalDevice *vkPhysicalDevice);

XrResult
oxr_session_populate_vk(struct oxr_logger *log,
                        struct oxr_system *sys,
                        XrGraphicsBindingVulkanKHR const *next,
                        struct oxr_session *sess);

XrResult
oxr_swapchain_vk_create(struct oxr_logger * /*log*/,
                        struct oxr_session *sess,
                        const XrSwapchainCreateInfo * /*createInfo*/,
                        struct oxr_swapchain **out_swapchain);

#endif


/*
 *
 * EGL, located in various files.
 *
 */

#ifdef XR_USE_PLATFORM_EGL

XrResult
oxr_session_populate_egl(struct oxr_logger *log,
                         struct oxr_system *sys,
                         XrGraphicsBindingEGLMNDX const *next,
                         struct oxr_session *sess);

XrResult
oxr_egl_get_device(struct oxr_logger *log,
                   struct oxr_system *sys,
                   PFN_xrEglGetProcAddressMNDX getProcAddress,
                   EGLDeviceEXT *out_egl_device);

#endif

/*
 *
 * D3D version independent routines, located in oxr_d3d.cpp
 *
 */

#if defined(XRT_HAVE_D3D11) || defined(XRT_HAVE_D3D12) || defined(XRT_DOXYGEN)
/// Common GetRequirements call for D3D11 and D3D12
XrResult
oxr_d3d_get_requirements(struct oxr_logger *log,
                         struct oxr_system *sys,
                         LUID *adapter_luid,
                         D3D_FEATURE_LEVEL *min_feature_level);

/// Verify the provided LUID matches the expected one in @p sys
XrResult
oxr_d3d_check_luid(struct oxr_logger *log, struct oxr_system *sys, LUID *adapter_luid);
#endif

/*
 *
 * D3D11, located in various files.
 *
 */

#ifdef XR_USE_GRAPHICS_API_D3D11

XrResult
oxr_d3d11_get_requirements(struct oxr_logger *log,
                           struct oxr_system *sys,
                           XrGraphicsRequirementsD3D11KHR *graphicsRequirements);

/**
 * @brief Check to ensure the device provided at session create matches the LUID we returned earlier.
 *
 * @return XR_SUCCESS if the device matches the LUID
 */
XrResult
oxr_d3d11_check_device(struct oxr_logger *log, struct oxr_system *sys, ID3D11Device *device);


XrResult
oxr_session_populate_d3d11(struct oxr_logger *log,
                           struct oxr_system *sys,
                           XrGraphicsBindingD3D11KHR const *next,
                           struct oxr_session *sess);

XrResult
oxr_swapchain_d3d11_create(struct oxr_logger *,
                           struct oxr_session *sess,
                           const XrSwapchainCreateInfo *,
                           struct oxr_swapchain **out_swapchain);

#endif

/*
 *
 * D3D12, located in various files.
 *
 */

#ifdef XR_USE_GRAPHICS_API_D3D12

XrResult
oxr_d3d12_get_requirements(struct oxr_logger *log,
                           struct oxr_system *sys,
                           XrGraphicsRequirementsD3D12KHR *graphicsRequirements);

/**
 * @brief Check to ensure the device provided at session create matches the LUID we returned earlier.
 *
 * @return XR_SUCCESS if the device matches the LUID
 */
XrResult
oxr_d3d12_check_device(struct oxr_logger *log, struct oxr_system *sys, ID3D12Device *device);


XrResult
oxr_session_populate_d3d12(struct oxr_logger *log,
                           struct oxr_system *sys,
                           XrGraphicsBindingD3D12KHR const *next,
                           struct oxr_session *sess);

XrResult
oxr_swapchain_d3d12_create(struct oxr_logger *,
                           struct oxr_session *sess,
                           const XrSwapchainCreateInfo *,
                           struct oxr_swapchain **out_swapchain);

#endif

/*
 *
 * Structs
 *
 */


/*!
 * Holds the properties that a system supports for a view configuration type.
 *
 * @relates oxr_system
 */
struct oxr_view_config_properties
{
	XrViewConfigurationType view_config_type;

	uint32_t view_count;
	XrViewConfigurationView views[XRT_MAX_COMPOSITOR_VIEW_CONFIGS_VIEW_COUNT];

	uint32_t blend_mode_count;
	XrEnvironmentBlendMode blend_modes[3];
};

/*!
 * Single or multiple devices grouped together to form a system that sessions
 * can be created from. Might need to open devices to get all
 * properties from it, but shouldn't.
 *
 * Not strictly an object, but an atom.
 *
 * Valid only within an XrInstance (@ref oxr_instance)
 *
 * @obj{XrSystemId}
 */
struct oxr_system
{
	struct oxr_instance *inst;

	//! The @ref xrt_iface level system.
	struct xrt_system *xsys;

	//! The @ref xrt_iface level app system.
	struct xrt_app_system *xasys;

	//! System devices used in all session types.
	struct xrt_system_devices *xsysd;

	//! Space overseer used in all session types.
	struct xrt_space_overseer *xso;

	//! System compositor, used to create session compositors.
	struct xrt_system_compositor *xsysc;

	XrSystemId systemId;

	//! Have the client application called the gfx api requirements func?
	bool gotten_requirements;

	bool view_config_mutex_created;
	struct os_mutex view_config_mutex;
	uint32_t view_config_count;
	struct oxr_view_config_properties view_configs[XRT_MAX_COMPOSITOR_VIEW_CONFIGS_COUNT];
	bool pending_view_config_update[XRT_MAX_COMPOSITOR_VIEW_CONFIGS_COUNT];
	timepoint_ns view_update_config_time[XRT_MAX_COMPOSITOR_VIEW_CONFIGS_COUNT];

	XrReferenceSpaceType reference_spaces[5];
	uint32_t reference_space_count;

	struct xrt_visibility_mask *visibility_mask[XRT_MAX_COMPOSITOR_VIEW_CONFIGS_VIEW_COUNT];

#ifdef OXR_HAVE_MNDX_xdev_space
	bool supports_xdev_space;
#endif

#ifdef XR_USE_GRAPHICS_API_VULKAN
	//! The instance/device we create when vulkan_enable2 is used
	VkInstance vulkan_enable2_instance;
	//! The device returned with the last xrGetVulkanGraphicsDeviceKHR or xrGetVulkanGraphicsDevice2KHR call.
	//! XR_NULL_HANDLE if neither has been called.
	VkPhysicalDevice suggested_vulkan_physical_device;

	/*!
	 * Stores the vkGetInstanceProcAddr passed to xrCreateVulkanInstanceKHR to be
	 * used when looking up Vulkan functions used by xrGetVulkanGraphicsDevice2KHR.
	 */
	PFN_vkGetInstanceProcAddr vk_get_instance_proc_addr;

	struct
	{
		// No better place to keep this state.
		bool external_fence_fd_enabled;
		bool external_semaphore_fd_enabled;
		bool timeline_semaphore_enabled;
		bool debug_utils_enabled;
		bool image_format_list_enabled;
	} vk;

#endif

#if defined(XR_USE_GRAPHICS_API_D3D11) || defined(XR_USE_GRAPHICS_API_D3D12)
	LUID suggested_d3d_luid;
	bool suggested_d3d_luid_valid;
#endif
};

/*
 * Extensions helpers.
 */

#define MAKE_EXT_STATUS(mixed_case, all_caps) bool mixed_case;
/*!
 * Structure tracking which extensions are enabled for a given instance.
 *
 * Names are systematic: the extension name with the XR_ prefix removed.
 */
struct oxr_extension_status
{
	OXR_EXTENSION_SUPPORT_GENERATE(MAKE_EXT_STATUS)
};
#undef MAKE_EXT_STATUS

#define XRT_MAX_DEBUG_MESSENGERS 256

/*!
 * Main object that ties everything together.
 *
 * No parent type/handle: this is the root handle.
 *
 * @obj{XrInstance}
 * @extends oxr_handle_base
 */
struct oxr_instance
{
	//! Common structure for things referred to by OpenXR handles.
	struct oxr_handle_parent_base handle;

	struct u_debug_gui *debug_ui;

	//! The @ref xrt_iface level instance.
	struct xrt_instance *xinst;

	//! The @ref xrt_iface level app instance.
	struct xrt_app_instance *xainst;

	//! Enabled extensions
	struct oxr_extension_status extensions;

	//! The OpenXR version requested in the app info. It determines the instance's OpenXR version.
	struct
	{
		//! Stores only major.minor version. Simplifies comparisons for e.g. "at least OpenXR 1.1".
		XrVersion major_minor;
	} openxr_version;

	// Protects the function oxr_instance_init_system_locked.
	struct os_mutex system_init_lock;

	// Hardcoded single system.
	struct oxr_system system;

	struct time_state *timekeeping;

	//! Path store for managing paths.
	struct oxr_path_store path_store;

	// Event queue.
	struct
	{
		struct os_mutex mutex;
		struct oxr_event *last;
		struct oxr_event *next;
	} event;

	//! Protects the sessions list @ref sessions_locked, taken before the event mutex.
	struct os_mutex sessions_mutex;
	//! The session list, protected by @ref sessions_mutex
	struct oxr_session *sessions_locked;

	//! Path cache for actions, needs path_store to work.
	struct oxr_instance_path_cache path_cache;

	//! Binding registry (reference-counted), shared by all action contexts of this instance.
	struct oxr_binding_registry *binding_registry;

	//! The default action context (reference-counted). Owned by instance; action sets also hold references.
	struct oxr_instance_action_context *action_context;

	struct
	{
		struct
		{
			struct
			{
				uint32_t major;
				uint32_t minor;
				uint32_t patch;
				const char *name; //< Engine name, not freed.
			} engine;
		} detected;
	} appinfo;

	struct
	{
		/*!
		 * Some applications can't handle depth formats, or they trigger
		 * a bug in a specific version of the application or engine.
		 * This flag only disables depth formats
		 * @see disable_vulkan_format_depth_stencil for depth-stencil formats.
		 */
		bool disable_vulkan_format_depth;

		/*!
		 * Some applications can't handle depth stencil formats, or they
		 * trigger a bug in a specific version of the application or
		 * engine.
		 *
		 * This flag only disables depth-stencil formats,
		 * @see disable_vulkan_format_depth flag for depth only formats.
		 *
		 * In the past it was used to work around a bug in Unreal's
		 * VulkanRHI backend.
		 */
		bool disable_vulkan_format_depth_stencil;

		/*!
		 * Disable the listing of quad views as a supported view config,
		 * the extensions are still exposed.
		 */
		bool disable_quad_views;

		//! Unreal 4 has a bug calling xrEndSession; the function should just exit
		bool skip_end_session;

		/*!
		 * Return XR_ERROR_REFERENCE_SPACE_UNSUPPORTED instead of
		 * XR_ERROR_VALIDATION_FAILURE in xrCreateReferenceSpace.
		 */
		bool no_validation_error_in_create_ref_space;

		//! For applications that rely on views being parallel, notably some OpenVR games with OpenComposite.
		bool parallel_views;

		//! For applications that use stage and don't offer recentering.
		bool map_stage_to_local_floor;

		/*!
		 * Beat Saber submits its projection layer with XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT set.
		 * This breaks rendering because the game uses the alpha texture to store data for the bloom shader,
		 * causing most of the game to render as black, only showing glowing parts of the image.
		 */
		bool no_texture_source_alpha;

		/*!
		 * Don't return XR_ERROR_VALIDATION_FAILURE if an
		 * application sets unsupported usage flags when
		 * calling xrCreateSwapchain.
		 */
		bool no_usage_bit_validation_in_create_swapchain;

		/*!
		 * Don't return XR_ERROR_VALIDATION_FAILURE if an
		 * application sets unsupported flags in XrCompositionLayer
		 * when calling xrEndFrame.
		 */
		bool no_layer_flags_bit_validation;
	} quirks;

	//! Debug messengers
	struct oxr_debug_messenger *messengers[XRT_MAX_DEBUG_MESSENGERS];

	bool lifecycle_verbose;
	bool debug_views;
	bool debug_spaces;
	bool debug_bindings;

#ifdef XRT_FEATURE_RENDERDOC
	RENDERDOC_API_1_4_1 *rdoc_api;
#endif

#ifdef XRT_OS_ANDROID
	enum xrt_android_lifecycle_event activity_state;
#endif // XRT_OS_ANDROID
};

/*
 * This includes needs to be here because it has static inline functions that
 * de-references the oxr_instance object. This refactor was made mid-patchset
 * so doing too many changes to other files would have been really disruptive
 * but this include will be removed soon.
 */
#include "path/oxr_path_wrappers.h"


/*!
 * Object that client program interact with.
 *
 * Parent type/handle is @ref oxr_instance
 *
 * @obj{XrSession}
 * @extends oxr_handle_base
 */
struct oxr_session
{
	//! Common structure for things referred to by OpenXR handles.
	struct oxr_handle_parent_base handle;
	struct oxr_system *sys;

	//! What graphics type was this session created with.
	enum oxr_session_graphics_ext gfx_ext;

	//! The @ref xrt_session backing this session.
	struct xrt_session *xs;

	//! Native compositor that is wrapped by client compositors.
	struct xrt_compositor_native *xcn;

	struct xrt_compositor *compositor;

	struct oxr_session *next;

	XrSessionState state;

	/*!
	 * This is set in xrBeginSession and is the primaryViewConfiguration
	 * argument, this is then used in xrEndFrame to know which view
	 * configuration the application is submitting it's frame in.
	 */
	XrViewConfigurationType current_view_config_type;

	//! State of the head's XRT_INPUT_GENERIC_HEAD_DETECT input, for oxr_poll_event.
	bool presence;

	/*!
	 * There is a extra state between xrBeginSession has been called and
	 * the first xrEndFrame has been called. These are to track this.
	 */
	bool has_ended_once;

	bool compositor_visible;
	bool compositor_focused;

	// the number of xrWaitFrame calls that did not yet have a corresponding
	// xrEndFrame or xrBeginFrame (discarded frame) call
	int active_wait_frames;
	struct os_mutex active_wait_frames_lock;

	bool frame_started;
	bool exiting;

	struct
	{
		int64_t waited;
		int64_t begun;
	} frame_id;

	struct oxr_frame_sync frame_sync;

	/*!
	 * Used to implement precise extra sleeping in wait frame.
	 */
	struct os_precise_sleeper sleeper;

	/*!
	 * Holds all the action state that belongs on the session level.
	 */
	struct oxr_session_action_context action_context;

	/*!
	 * Per-session map of action key to action attachment (thread-safe).
	 */
	struct oxr_session_attached_actions attached_actions;

	/*!
	 * IPD, to be expanded to a proper 3D relation.
	 */
	float ipd_meters;

	/*!
	 * Frame timing debug output.
	 */
	bool frame_timing_spew;

	//! Extra sleep in wait frame.
	uint32_t frame_timing_wait_sleep_ms;

	/*!
	 * To pipe swapchain creation to right code.
	 */
	XrResult (*create_swapchain)(struct oxr_logger *,
	                             struct oxr_session *sess,
	                             const XrSwapchainCreateInfo *,
	                             struct oxr_swapchain **);

	/*! initial relation of head in "global" space.
	 * Used as reference for local space.  */
	// struct xrt_space_relation local_space_pure_relation;

	bool has_lost;
};

/*!
 * Returns XR_SUCCESS or XR_SESSION_LOSS_PENDING as appropriate.
 *
 * @public @memberof oxr_session
 */
static inline XrResult
oxr_session_success_result(struct oxr_session *session)
{
	switch (session->state) {
	case XR_SESSION_STATE_LOSS_PENDING: return XR_SESSION_LOSS_PENDING;
	default: return XR_SUCCESS;
	}
}

/*!
 * Returns XR_SUCCESS, XR_SESSION_LOSS_PENDING, or XR_SESSION_NOT_FOCUSED, as
 * appropriate.
 *
 * @public @memberof oxr_session
 */
static inline XrResult
oxr_session_success_focused_result(struct oxr_session *session)
{
	switch (session->state) {
	case XR_SESSION_STATE_LOSS_PENDING: return XR_SESSION_LOSS_PENDING;
	case XR_SESSION_STATE_FOCUSED: return XR_SUCCESS;
	default: return XR_SESSION_NOT_FOCUSED;
	}
}

#ifdef OXR_HAVE_FB_display_refresh_rate
XrResult
oxr_session_get_display_refresh_rate(struct oxr_logger *log, struct oxr_session *sess, float *displayRefreshRate);

XrResult
oxr_session_request_display_refresh_rate(struct oxr_logger *log, struct oxr_session *sess, float displayRefreshRate);
#endif // OXR_HAVE_FB_display_refresh_rate


/*!
 * @defgroup oxr_input OpenXR input
 * @ingroup oxr_main
 *
 * @brief The action-set/action-based input subsystem of OpenXR.
 *
 *
 * Action sets are created as children of the Instance, but are primarily used
 * with one or more Sessions. They may be used with multiple sessions at a time,
 * so we can't just put the per-session information directly in the action set
 * or action. Instead, we have the `_attachment `structures, which mirror the
 * action sets and actions but are rooted under the Session:
 *
 * - For every action set attached to a session, that session owns a @ref
 *   oxr_action_set_attachment.
 * - For each action in those attached action sets, the action set attachment
 *   owns an @ref oxr_action_attachment.
 *
 * We go from the public handle to the `_attachment` structure by using a `key`
 * value and a hash map: specifically, we look up the
 * oxr_action_set::act_set_key and oxr_action::act_key in the session.
 *
 * ![](monado-input-class-relationships.drawio.svg)
 */

/*!
 * The data associated with the attachment of an Action Set (@ref
 * oxr_action_set) to as Session (@ref oxr_session).
 *
 * This structure has no pointer to the @ref oxr_action_set that created it
 * because the application is allowed to destroy an action before the session,
 * which should change nothing except not allow the application to use the
 * corresponding data anymore.
 *
 * @ingroup oxr_input
 *
 * @see oxr_action_set
 */
struct oxr_action_set_attachment
{
	//! Owning session action context.
	struct oxr_session_action_context *sess_context;

	//! Action set refcounted data
	struct oxr_action_set_ref *act_set_ref;

	//! Unique key for the session hashmap.
	uint32_t act_set_key;

	//! Which sub-action paths are requested on the latest sync.
	struct oxr_subaction_paths requested_subaction_paths;

	/*!
	 * Effective action-set priority from the latest xrSyncActions.
	 *
	 * Reset to @ref oxr_action_set_ref::priority at the start of each
	 * successful focused sync, then overwritten only for action sets in that
	 * sync's active list (including XR_EXT_active_action_set_priority
	 * overrides). Used after sync, for example when enumerating bound
	 * sources, to apply the same priority suppression as input combining.
	 */
	uint32_t last_sync_priority;

	//! An array of action attachments we own.
	struct oxr_action_attachment *act_attachments;

	/*!
	 * Length of @ref oxr_action_set_attachment::act_attachments.
	 */
	size_t action_attachment_count;
};



/*!
 * The state of a action input.
 *
 * @ingroup oxr_input
 *
 * @see oxr_action_attachment
 */
struct oxr_action_state
{
	/*!
	 * The actual value - must interpret using action type
	 */
	union xrt_input_value value;

	//! Is this active (bound and providing input)?
	bool active;

	// Was this changed.
	bool changed;

	//! When was this last changed.
	XrTime timestamp;
};

/*!
 * A resolved input binding for an action, along with the required transform.
 *
 * @ingroup oxr_input
 *
 * @see oxr_io_cache
 * @see xrt_input
 */
struct oxr_action_input
{
	//! Cache this input is resolved against (owned by the action context).
	const struct oxr_io_cache *io_cache;
	//! Device this input reads from.
	struct xrt_device_id device_id;
	//! The resolved input, pointing into @ref oxr_io_cache::inputs.
	struct xrt_input *input;
	const struct oxr_binding_point *binding_point;
	struct oxr_priority_group_state *priority_group_state;
	struct oxr_input_transform *transforms;
	size_t transform_count;
};

/*!
 * A output action pair of a @ref xrt_output_name and a @ref xrt_device.
 *
 * @ingroup oxr_input
 *
 * @see xrt_device
 * @see xrt_output_name
 */
struct oxr_action_output
{
	//! Cache this output is resolved against (owned by the action context).
	const struct oxr_io_cache *io_cache;
	//! Device this output routes to.
	struct xrt_device_id device_id;
	enum xrt_output_name name;
	const struct oxr_binding_point *binding_point;
};


/*!
 * The set of inputs/outputs for a single sub-action path for an action.
 *
 * Each @ref oxr_action_attachment has one of these for every known sub-action
 * path in the spec. Many, or even most, will be "empty".
 *
 * A single action will either be input or output, not both.
 *
 * @ingroup oxr_input
 *
 * @see oxr_action_attachment
 */
struct oxr_action_cache
{
	struct oxr_action_state current;

	size_t input_count;
	struct oxr_action_input *inputs;

	int64_t stop_output_time;
	size_t output_count;
	struct oxr_action_output *outputs;
};

/*!
 * Data associated with an Action that has been attached to a Session.
 *
 * More information on the action vs action attachment and action set vs action
 * set attachment parallel is in the docs for @ref oxr_input
 *
 * @ingroup oxr_input
 *
 * @see oxr_action
 */
struct oxr_action_attachment
{
	//! The owning action set attachment
	struct oxr_action_set_attachment *act_set_attached;

	//! This action's refcounted data
	struct oxr_action_ref *act_ref;

	//! The session attached actions this action attachment belongs to.
	struct oxr_session_attached_actions *attached_actions;

	//! Unique key for the session hashmap.
	uint32_t act_key;

	/*!
	 * For pose actions any subaction paths are special treated, at bind
	 * time we pick one subaction path and stick to it as long as the action
	 * lives.
	 */
	struct oxr_subaction_paths any_pose_subaction_path;

	struct oxr_action_state any_state;

#define OXR_CACHE_MEMBER(X) struct oxr_action_cache X;
	OXR_FOR_EACH_SUBACTION_PATH(OXR_CACHE_MEMBER)
#undef OXR_CACHE_MEMBER
};

/*!
 * @}
 */


static inline bool
oxr_space_type_is_reference(enum oxr_space_type space_type)
{
	switch (space_type) {
	case OXR_SPACE_TYPE_REFERENCE_VIEW:
	case OXR_SPACE_TYPE_REFERENCE_LOCAL:
	case OXR_SPACE_TYPE_REFERENCE_LOCAL_FLOOR:
	case OXR_SPACE_TYPE_REFERENCE_STAGE:
	case OXR_SPACE_TYPE_REFERENCE_UNBOUNDED_MSFT:
	case OXR_SPACE_TYPE_REFERENCE_COMBINED_EYE_VARJO:
	case OXR_SPACE_TYPE_REFERENCE_LOCALIZATION_MAP_ML:
	case OXR_SPACE_TYPE_REFERENCE_UNBOUNDED_ANDROID:
	case OXR_SPACE_TYPE_REFERENCE_STATIONARY_EXT:
		// These are reference spaces.
		return true;

	case OXR_SPACE_TYPE_ACTION:
	case OXR_SPACE_TYPE_XDEV_POSE:
		// These are not reference spaces.
		return false;
	}

	// Handles invalid value.
	return false;
}


/*!
 * Can be one of several reference space types, or a space that is bound to an
 * action.
 *
 * Parent type/handle is @ref oxr_session
 *
 * @obj{XrSpace}
 * @extends oxr_handle_base
 */
struct oxr_space
{
	//! Common structure for things referred to by OpenXR handles.
	struct oxr_handle_base handle;

	//! Owner of this space.
	struct oxr_session *sess;

	//! Pose that was given during creation.
	struct xrt_pose pose;

	//! Action key from which action this space was created from.
	uint32_t act_key;

	//! What kind of space is this?
	enum oxr_space_type space_type;

	//! Which sub action path is this?
	struct oxr_subaction_paths subaction_paths;

	struct
	{
		struct xrt_space *xs;
		struct xrt_device_id device_id;
		enum xrt_input_name name;

		bool feature_eye_tracking;
	} action;

	struct
	{
		struct xrt_space *xs;
	} xdev_pose;
};

/*!
 * A set of images used for rendering.
 *
 * Parent type/handle is @ref oxr_session
 *
 * @obj{XrSwapchain}
 * @extends oxr_handle_base
 */
struct oxr_swapchain
{
	//! Common structure for things referred to by OpenXR handles.
	struct oxr_handle_base handle;

	//! Owner of this swapchain.
	struct oxr_session *sess;

	//! Compositor swapchain.
	struct xrt_swapchain *swapchain;

	//! Swapchain size.
	uint32_t width, height;

	//! For 1 is 2D texture, greater then 1 2D array texture.
	uint32_t array_layer_count;

	//! The number of cubemap faces.  6 for cubemaps, 1 otherwise.
	uint32_t face_count;

	struct
	{
		enum oxr_image_state state;
	} images[XRT_MAX_SWAPCHAIN_IMAGES];

	struct
	{
		size_t num;
		struct u_index_fifo fifo;
	} acquired;

	struct
	{
		bool yes;
		int index;
	} inflight; // This is the image that the app is working on.

	struct
	{
		bool yes;
		int index;
	} released;

	// Is this a static swapchain, needed for acquire semantics.
	bool is_static;


	XrResult (*destroy)(struct oxr_logger *, struct oxr_swapchain *);

	XrResult (*enumerate_images)(struct oxr_logger *,
	                             struct oxr_swapchain *,
	                             uint32_t,
	                             XrSwapchainImageBaseHeader *);

	XrResult (*acquire_image)(struct oxr_logger *,
	                          struct oxr_swapchain *,
	                          const XrSwapchainImageAcquireInfo *,
	                          uint32_t *);

	XrResult (*wait_image)(struct oxr_logger *, struct oxr_swapchain *, const XrSwapchainImageWaitInfo *);

	XrResult (*release_image)(struct oxr_logger *, struct oxr_swapchain *, const XrSwapchainImageReleaseInfo *);
};

/*!
 * The reference-counted data of an action set.
 *
 * One or more sessions may still need this data after the application destroys
 * its XrActionSet handle, so this data is refcounted.
 *
 * @ingroup oxr_input
 *
 * @see oxr_action_set
 * @extends oxr_refcounted
 */
struct oxr_action_set_ref
{
	struct oxr_refcounted base;

	//! Application supplied name of this action.
	char name[XR_MAX_ACTION_SET_NAME_SIZE];

	/*!
	 * Has this action set even been attached to any session, marking it as
	 * immutable.
	 */
	bool ever_attached;

	//! Unique key for the session hashmap.
	uint32_t act_set_key;

	//! Application supplied action set priority.
	uint32_t priority;

	struct oxr_pair_hashset actions;

	struct oxr_subaction_paths permitted_subaction_paths;
};

/*!
 * A group of actions.
 *
 * Parent type/handle is @ref oxr_instance
 *
 * Note, however, that an action set must be "attached" to a session
 * ( @ref oxr_session ) to be used and not just configured.
 * The corresponding data is in @ref oxr_action_set_attachment.
 *
 * @ingroup oxr_input
 *
 * @obj{XrActionSet}
 * @extends oxr_handle_base
 */
struct oxr_action_set
{
	//! Common structure for things referred to by OpenXR handles.
	struct oxr_handle_parent_base handle;

	//! Owner of this action set.
	struct oxr_instance *inst;

	//! Reference to the instance action context (reference-counted).
	struct oxr_instance_action_context *inst_context;

	/*!
	 * The data for this action set that must live as long as any session we
	 * are attached to.
	 */
	struct oxr_action_set_ref *data;


	/*!
	 * Unique key for the session hashmap.
	 *
	 * Duplicated from oxr_action_set_ref::act_set_key for efficiency.
	 */
	uint32_t act_set_key;

	//! The item in the name hashset.
	struct u_hashset_item *name_item;

	//! The item in the localized hashset.
	struct u_hashset_item *loc_item;
};

/*!
 * The reference-counted data of an action.
 *
 * One or more sessions may still need this data after the application destroys
 * its XrAction handle, so this data is refcounted.
 *
 * @ingroup oxr_input
 *
 * @see oxr_action
 * @extends oxr_refcounted
 */
struct oxr_action_ref
{
	struct oxr_refcounted base;

	//! Application supplied name of this action.
	char name[XR_MAX_ACTION_NAME_SIZE];

	//! Unique key for the session hashmap.
	uint32_t act_key;

	//! Type this action was created with.
	XrActionType action_type;

	//! Which sub action paths that this action was created with.
	struct oxr_subaction_paths subaction_paths;
};

/*!
 * A single action.
 *
 * Parent type/handle is @ref oxr_action_set
 *
 * For actual usage, an action is attached to a session: the corresponding data
 * is in @ref oxr_action_attachment
 *
 * @ingroup oxr_input
 *
 * @obj{XrAction}
 * @extends oxr_handle_base
 */
struct oxr_action
{
	//! Common structure for things referred to by OpenXR handles.
	struct oxr_handle_base handle;

	//! Owner of this action.
	struct oxr_action_set *act_set;

	//! The data for this action that must live as long as any session we
	//! are attached to.
	struct oxr_action_ref *data;

	/*!
	 * Unique key for the session hashmap.
	 *
	 * Duplicated from oxr_action_ref::act_key for efficiency.
	 */
	uint32_t act_key;

	//! The item in the name hashset.
	struct u_hashset_item *name_item;

	//! The item in the localized hashset.
	struct u_hashset_item *loc_item;
};

/*!
 * Debug object created by the client program.
 *
 * Parent type/handle is @ref oxr_instance
 *
 * @obj{XrDebugUtilsMessengerEXT}
 */
struct oxr_debug_messenger
{
	//! Common structure for things referred to by OpenXR handles.
	struct oxr_handle_base handle;

	//! Owner of this messenger.
	struct oxr_instance *inst;

	//! Severities to submit to this messenger
	XrDebugUtilsMessageSeverityFlagsEXT message_severities;

	//! Types to submit to this messenger
	XrDebugUtilsMessageTypeFlagsEXT message_types;

	//! Callback function
	PFN_xrDebugUtilsMessengerCallbackEXT user_callback;

	//! Opaque user data
	void *XR_MAY_ALIAS user_data;
};

#ifdef OXR_HAVE_FB_passthrough

struct oxr_passthrough
{
	struct oxr_handle_base handle;

	struct oxr_session *sess;

	XrPassthroughFlagsFB flags;

	bool paused;
};

struct oxr_passthrough_layer
{
	struct oxr_handle_base handle;

	struct oxr_session *sess;

	XrPassthroughFB passthrough;

	XrPassthroughFlagsFB flags;

	XrPassthroughLayerPurposeFB purpose;

	bool paused;

	XrPassthroughStyleFB style;
	XrPassthroughColorMapMonoToRgbaFB monoToRgba;
	XrPassthroughColorMapMonoToMonoFB monoToMono;
	XrPassthroughBrightnessContrastSaturationFB brightnessContrastSaturation;
};

XrResult
oxr_passthrough_create(struct oxr_logger *log,
                       struct oxr_session *sess,
                       const XrPassthroughCreateInfoFB *createInfo,
                       struct oxr_passthrough **out_passthrough);

XrResult
oxr_passthrough_layer_create(struct oxr_logger *log,
                             struct oxr_session *sess,
                             const XrPassthroughLayerCreateInfoFB *createInfo,
                             struct oxr_passthrough_layer **out_layer);

static inline XrPassthroughFB
oxr_passthrough_to_openxr(struct oxr_passthrough *passthrough)
{
	return XRT_CAST_PTR_TO_OXR_HANDLE(XrPassthroughFB, passthrough);
}

static inline XrPassthroughLayerFB
oxr_passthrough_layer_to_openxr(struct oxr_passthrough_layer *passthroughLayer)
{
	return XRT_CAST_PTR_TO_OXR_HANDLE(XrPassthroughLayerFB, passthroughLayer);
}

XrResult
oxr_event_push_XrEventDataPassthroughStateChangedFB(struct oxr_logger *log,
                                                    struct oxr_session *sess,
                                                    XrPassthroughStateChangedFlagsFB flags);

#endif // OXR_HAVE_FB_passthrough

#ifdef OXR_HAVE_HTC_facial_tracking
/*!
 * HTC specific Facial tracker.
 *
 * Parent type/handle is @ref oxr_instance
 *
 *
 * @obj{XrFacialTrackerHTC}
 * @extends oxr_handle_base
 */
struct oxr_facial_tracker_htc
{
	//! Common structure for things referred to by OpenXR handles.
	struct oxr_handle_base handle;

	//! Owner of this face tracker.
	struct oxr_session *sess;

	//! xrt_device backing this face tracker
	struct xrt_device *xdev;

	//! Type of facial tracking, eyes or lips
	enum xrt_facial_tracking_type_htc facial_tracking_type;

	//! To track if the feature set has been incremented since creation.
	bool feature_incremented;
};

XrResult
oxr_facial_tracker_htc_create(struct oxr_logger *log,
                              struct oxr_session *sess,
                              const XrFacialTrackerCreateInfoHTC *createInfo,
                              struct oxr_facial_tracker_htc **out_face_tracker_htc);

XrResult
oxr_get_facial_expressions_htc(struct oxr_logger *log,
                               struct oxr_facial_tracker_htc *facial_tracker_htc,
                               XrFacialExpressionsHTC *facialExpressions);
#endif

#ifdef OXR_HAVE_FB_body_tracking
/*!
 * FB specific Body tracker.
 *
 * Parent type/handle is @ref oxr_instance
 *
 * @obj{XrBodyTrackerFB}
 * @extends oxr_handle_base
 */
struct oxr_body_tracker_fb
{
	//! Common structure for things referred to by OpenXR handles.
	struct oxr_handle_base handle;

	//! Owner of this face tracker.
	struct oxr_session *sess;

	//! xrt_body_tracker backing this body tracker
	struct xrt_body_tracker *xbt;

	//! Type of the body joint set e.g. XR_FB_body_tracking or XR_META_body_tracking_full_body
	enum xrt_body_joint_set_type_fb joint_set_type;
};

XrResult
oxr_create_body_tracker_fb(struct oxr_logger *log,
                           struct oxr_session *sess,
                           const XrBodyTrackerCreateInfoFB *createInfo,
                           struct oxr_body_tracker_fb **out_body_tracker_fb);

XrResult
oxr_get_body_skeleton_fb(struct oxr_logger *log,
                         struct oxr_body_tracker_fb *body_tracker_fb,
                         XrBodySkeletonFB *skeleton);

XrResult
oxr_locate_body_joints_fb(struct oxr_logger *log,
                          struct oxr_body_tracker_fb *body_tracker_fb,
                          struct oxr_space *base_spc,
                          const XrBodyJointsLocateInfoFB *locateInfo,
                          XrBodyJointLocationsFB *locations);
#endif

#ifdef OXR_HAVE_BD_body_tracking
/*!
 * BD (PICO) specific Body tracker.
 *
 * Parent type/handle is @ref oxr_session
 *
 * @obj{XrBodyTrackerBD}
 * @extends oxr_handle_base
 */
struct oxr_body_tracker_bd
{
	//! Common structure for things referred to by OpenXR handles.
	struct oxr_handle_base handle;

	//! Owner of this body tracker.
	struct oxr_session *sess;

	//! xrt_body_tracker backing this body tracker
	struct xrt_body_tracker *xbt;

	//! Type of the body joint set (with or without arms)
	enum xrt_body_joint_set_type_bd joint_set_type;
};

XrResult
oxr_create_body_tracker_bd(struct oxr_logger *log,
                           struct oxr_session *sess,
                           const XrBodyTrackerCreateInfoBD *createInfo,
                           struct oxr_body_tracker_bd **out_body_tracker_bd);

XrResult
oxr_locate_body_joints_bd(struct oxr_logger *log,
                          struct oxr_body_tracker_bd *body_tracker_bd,
                          struct oxr_space *base_spc,
                          const XrBodyJointsLocateInfoBD *locateInfo,
                          XrBodyJointLocationsBD *locations);

bool
oxr_system_get_body_tracking_bd_support(struct oxr_logger *log, struct oxr_instance *inst);
#endif

#ifdef OXR_HAVE_FB_face_tracking2
/*!
 * FB specific Face tracker2.
 *
 * Parent type/handle is @ref oxr_instance
 *
 * @obj{XrFaceTracker2FB}
 * @extends oxr_handle_base
 */
struct oxr_face_tracker2_fb
{
	//! Common structure for things referred to by OpenXR handles.
	struct oxr_handle_base handle;

	//! Owner of this face tracker.
	struct oxr_session *sess;

	//! xrt_device backing this face tracker
	struct xrt_device *xdev;

	bool audio_enabled;
	bool visual_enabled;

	//! To track if the feature set has been incremented since creation.
	bool feature_incremented;
};

XrResult
oxr_face_tracker2_fb_create(struct oxr_logger *log,
                            struct oxr_session *sess,
                            const XrFaceTrackerCreateInfo2FB *createInfo,
                            struct oxr_face_tracker2_fb **out_face_tracker2_fb);

XrResult
oxr_get_face_expression_weights2_fb(struct oxr_logger *log,
                                    struct oxr_face_tracker2_fb *face_tracker2_fb,
                                    const XrFaceExpressionInfo2FB *expression_info,
                                    XrFaceExpressionWeights2FB *expression_weights);
#endif

#ifdef OXR_HAVE_MNDX_xdev_space
/*!
 * Object that holds a list of the current @ref xrt_devices.
 *
 * Parent type/handle is @ref oxr_session
 *
 * @obj{XrXDevList}
 * @extends oxr_handle_base
 */
struct oxr_xdev_list
{
	//! Common structure for things referred to by OpenXR handles.
	struct oxr_handle_base handle;

	//! Owner of this @ref xrt_device list.
	struct oxr_session *sess;

	//! Monotonically increasing number.
	uint64_t generation_number;

	uint64_t ids[XRT_SYSTEM_MAX_DEVICES];
	struct xrt_device *xdevs[XRT_SYSTEM_MAX_DEVICES];
	enum xrt_input_name names[XRT_SYSTEM_MAX_DEVICES];

	//! Counts ids, names and xdevs.
	uint32_t device_count;
};
#endif // OXR_HAVE_MNDX_xdev_space

#ifdef OXR_HAVE_EXT_plane_detection
/*!
 * A Plane Detector.
 *
 * Parent type/handle is @ref oxr_session
 *
 *
 * @obj{XrPlaneDetectorEXT}
 * @extends oxr_handle_base
 */
struct oxr_plane_detector_ext
{
	//! Common structure for things referred to by OpenXR handles.
	struct oxr_handle_base handle;

	//! Owner of this anchor.
	struct oxr_session *sess;

	//! Plane detector flags.
	enum xrt_plane_detector_flags_ext flags;

	//! The last known state of this plane detector.
	XrPlaneDetectionStateEXT state;

	//! Whether the last DONE plane detection has been retrieved from the xdev.
	bool retrieved;

	//! The device that this plane detector belongs to.
	struct xrt_device *xdev;

	//! Detected planes. xrt_plane_detector_locations_ext::relation is kept in xdev space and not updated.
	struct xrt_plane_detections_ext detections;

	//! Corresponds to xrt_plane_detections_ext::locations, but with OpenXR types and transformed into target space.
	//! Enables two call idiom.
	XrPlaneDetectorLocationEXT *xr_locations;

	//! A globally unique id for the current plane detection or 0, generated by the xrt_device.
	uint64_t detection_id;
};
#endif // OXR_HAVE_EXT_plane_detection

#ifdef OXR_HAVE_EXT_future
/*!
 * EXT futures.
 *
 * Parent type/handle is @ref oxr_instance
 *
 * @obj{XrFutureEXT}
 * @extends oxr_handle_base
 */
struct oxr_future_ext
{
	//! Common structure for things referred to by OpenXR handles.
	struct oxr_handle_base handle;

	//! (weak) reference to instance (may or not be a direct parent handle")
	struct oxr_instance *inst;

	//! Owning session.
	struct oxr_session *sess;

	//! xrt_future backing this future
	struct xrt_future *xft;
};

/*!
 * To go back to a OpenXR object.
 *
 * @relates oxr_future_ext
 */
static inline XrFutureEXT
oxr_future_ext_to_openxr(struct oxr_future_ext *future_ext)
{
	return XRT_CAST_PTR_TO_OXR_HANDLE(XrFutureEXT, future_ext);
}

XrResult
oxr_future_create(struct oxr_logger *log,
                  struct oxr_session *sess,
                  struct xrt_future *xft,
                  struct oxr_handle_parent_base *parent_handle,
                  struct oxr_future_ext **out_oxr_future_ext);

XrResult
oxr_future_invalidate(struct oxr_logger *log, struct oxr_future_ext *oxr_future);

XrResult
oxr_future_ext_poll(struct oxr_logger *log, const struct oxr_future_ext *oxr_future, XrFuturePollResultEXT *pollResult);

XrResult
oxr_future_ext_cancel(struct oxr_logger *log, struct oxr_future_ext *oxr_future);

XrResult
oxr_future_ext_complete(struct oxr_logger *log,
                        struct oxr_future_ext *oxr_future,
                        struct xrt_future_result *out_ft_result);

#endif

#ifdef OXR_HAVE_EXT_user_presence
XrResult
oxr_event_push_XrEventDataUserPresenceChangedEXT(struct oxr_logger *log, struct oxr_session *sess, bool isUserPresent);
#endif // OXR_HAVE_EXT_user_presence

#ifdef OXR_HAVE_ANDROID_face_tracking
/*!
 * Android specific Facial tracker.
 *
 * Parent type/handle is @ref oxr_instance
 *
 *
 * @obj{XrFaceTrackerANDROID}
 * @extends oxr_handle_base
 */
struct oxr_face_tracker_android
{
	//! Common structure for things referred to by OpenXR handles.
	struct oxr_handle_base handle;

	//! Owner of this face tracker.
	struct oxr_session *sess;

	//! xrt_device backing this face tracker
	struct xrt_device *xdev;

	//! To track if the feature set has been incremented since creation.
	bool feature_incremented;
};

XrResult
oxr_face_tracker_android_create(struct oxr_logger *log,
                                struct oxr_session *sess,
                                const XrFaceTrackerCreateInfoANDROID *createInfo,
                                XrFaceTrackerANDROID *faceTracker);

XrResult
oxr_get_face_state_android(struct oxr_logger *log,
                           struct oxr_face_tracker_android *facial_tracker_android,
                           const XrFaceStateGetInfoANDROID *getInfo,
                           XrFaceStateANDROID *faceStateOutput);

XrResult
oxr_get_face_calibration_state_android(struct oxr_logger *log,
                                       struct oxr_face_tracker_android *facial_tracker_android,
                                       XrBool32 *faceIsCalibratedOutput);
#endif // OXR_HAVE_ANDROID_face_tracking

/*!
 * @}
 */


#ifdef __cplusplus
}
#endif
