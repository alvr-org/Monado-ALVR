// Copyright 2019-2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Macros for generating extension-related tables and code and
 * inspecting Monado's extension support.
 *
 * MOSTLY GENERATED CODE - see below!
 *
 * To add support for a new extension, edit and run generate_oxr_ext_support.py,
 * then run clang-format on this file. Two entire chunks of this file get
 * replaced by that script: comments indicate where they are.
 *
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 */

#pragma once

#include "xrt/xrt_config_build.h"
#include "xrt/xrt_openxr_includes.h"

// beginning of GENERATED defines - do not modify - used by scripts

/*
 * XR_KHR_android_create_instance
 */
#if defined(XR_KHR_android_create_instance) && defined(XR_USE_PLATFORM_ANDROID)
#define OXR_HAVE_KHR_android_create_instance
#define OXR_EXTENSION_SUPPORT_KHR_android_create_instance(_) _(KHR_android_create_instance, KHR_ANDROID_CREATE_INSTANCE)
#else
#define OXR_EXTENSION_SUPPORT_KHR_android_create_instance(_)
#endif


/*
 * XR_KHR_android_thread_settings
 */
#if defined(XR_KHR_android_thread_settings) && defined(XR_USE_PLATFORM_ANDROID)
#define OXR_HAVE_KHR_android_thread_settings
#define OXR_EXTENSION_SUPPORT_KHR_android_thread_settings(_) _(KHR_android_thread_settings, KHR_ANDROID_THREAD_SETTINGS)
#else
#define OXR_EXTENSION_SUPPORT_KHR_android_thread_settings(_)
#endif


/*
 * XR_KHR_binding_modification
 */
#if defined(XR_KHR_binding_modification)
#define OXR_HAVE_KHR_binding_modification
#define OXR_EXTENSION_SUPPORT_KHR_binding_modification(_) _(KHR_binding_modification, KHR_BINDING_MODIFICATION)
#else
#define OXR_EXTENSION_SUPPORT_KHR_binding_modification(_)
#endif


/*
 * XR_KHR_composition_layer_color_scale_bias
 */
#if defined(XR_KHR_composition_layer_color_scale_bias) && defined(XRT_FEATURE_OPENXR_LAYER_COLOR_SCALE_BIAS)
#define OXR_HAVE_KHR_composition_layer_color_scale_bias
#define OXR_EXTENSION_SUPPORT_KHR_composition_layer_color_scale_bias(_)                                                \
	_(KHR_composition_layer_color_scale_bias, KHR_COMPOSITION_LAYER_COLOR_SCALE_BIAS)
#else
#define OXR_EXTENSION_SUPPORT_KHR_composition_layer_color_scale_bias(_)
#endif


/*
 * XR_KHR_composition_layer_cube
 */
#if defined(XR_KHR_composition_layer_cube) && defined(XRT_FEATURE_OPENXR_LAYER_CUBE)
#define OXR_HAVE_KHR_composition_layer_cube
#define OXR_EXTENSION_SUPPORT_KHR_composition_layer_cube(_) _(KHR_composition_layer_cube, KHR_COMPOSITION_LAYER_CUBE)
#else
#define OXR_EXTENSION_SUPPORT_KHR_composition_layer_cube(_)
#endif


/*
 * XR_KHR_composition_layer_cylinder
 */
#if defined(XR_KHR_composition_layer_cylinder) && defined(XRT_FEATURE_OPENXR_LAYER_CYLINDER)
#define OXR_HAVE_KHR_composition_layer_cylinder
#define OXR_EXTENSION_SUPPORT_KHR_composition_layer_cylinder(_)                                                        \
	_(KHR_composition_layer_cylinder, KHR_COMPOSITION_LAYER_CYLINDER)
#else
#define OXR_EXTENSION_SUPPORT_KHR_composition_layer_cylinder(_)
#endif


/*
 * XR_KHR_composition_layer_depth
 */
#if defined(XR_KHR_composition_layer_depth) && defined(XRT_FEATURE_OPENXR_LAYER_DEPTH)
#define OXR_HAVE_KHR_composition_layer_depth
#define OXR_EXTENSION_SUPPORT_KHR_composition_layer_depth(_) _(KHR_composition_layer_depth, KHR_COMPOSITION_LAYER_DEPTH)
#else
#define OXR_EXTENSION_SUPPORT_KHR_composition_layer_depth(_)
#endif


/*
 * XR_KHR_composition_layer_equirect
 */
#if defined(XR_KHR_composition_layer_equirect) && defined(XRT_FEATURE_OPENXR_LAYER_EQUIRECT1)
#define OXR_HAVE_KHR_composition_layer_equirect
#define OXR_EXTENSION_SUPPORT_KHR_composition_layer_equirect(_)                                                        \
	_(KHR_composition_layer_equirect, KHR_COMPOSITION_LAYER_EQUIRECT)
#else
#define OXR_EXTENSION_SUPPORT_KHR_composition_layer_equirect(_)
#endif


/*
 * XR_KHR_composition_layer_equirect2
 */
#if defined(XR_KHR_composition_layer_equirect2) && defined(XRT_FEATURE_OPENXR_LAYER_EQUIRECT2)
#define OXR_HAVE_KHR_composition_layer_equirect2
#define OXR_EXTENSION_SUPPORT_KHR_composition_layer_equirect2(_)                                                       \
	_(KHR_composition_layer_equirect2, KHR_COMPOSITION_LAYER_EQUIRECT2)
#else
#define OXR_EXTENSION_SUPPORT_KHR_composition_layer_equirect2(_)
#endif


/*
 * XR_KHR_convert_timespec_time
 */
#if defined(XR_KHR_convert_timespec_time) && defined(XR_USE_TIMESPEC) && (!defined(XR_USE_PLATFORM_WIN32))
#define OXR_HAVE_KHR_convert_timespec_time
#define OXR_EXTENSION_SUPPORT_KHR_convert_timespec_time(_) _(KHR_convert_timespec_time, KHR_CONVERT_TIMESPEC_TIME)
#else
#define OXR_EXTENSION_SUPPORT_KHR_convert_timespec_time(_)
#endif


/*
 * XR_KHR_D3D11_enable
 */
#if defined(XR_KHR_D3D11_enable) && defined(XR_USE_GRAPHICS_API_D3D11)
#define OXR_HAVE_KHR_D3D11_enable
#define OXR_EXTENSION_SUPPORT_KHR_D3D11_enable(_) _(KHR_D3D11_enable, KHR_D3D11_ENABLE)
#else
#define OXR_EXTENSION_SUPPORT_KHR_D3D11_enable(_)
#endif


/*
 * XR_KHR_D3D12_enable
 */
#if defined(XR_KHR_D3D12_enable) && defined(XR_USE_GRAPHICS_API_D3D12)
#define OXR_HAVE_KHR_D3D12_enable
#define OXR_EXTENSION_SUPPORT_KHR_D3D12_enable(_) _(KHR_D3D12_enable, KHR_D3D12_ENABLE)
#else
#define OXR_EXTENSION_SUPPORT_KHR_D3D12_enable(_)
#endif


/*
 * XR_KHR_loader_init
 */
#if defined(XR_KHR_loader_init) && defined(XR_USE_PLATFORM_ANDROID)
#define OXR_HAVE_KHR_loader_init
#define OXR_EXTENSION_SUPPORT_KHR_loader_init(_) _(KHR_loader_init, KHR_LOADER_INIT)
#else
#define OXR_EXTENSION_SUPPORT_KHR_loader_init(_)
#endif


/*
 * XR_KHR_loader_init_android
 */
#if defined(XR_KHR_loader_init_android) && defined(OXR_HAVE_KHR_loader_init) && defined(XR_USE_PLATFORM_ANDROID)
#define OXR_HAVE_KHR_loader_init_android
#define OXR_EXTENSION_SUPPORT_KHR_loader_init_android(_) _(KHR_loader_init_android, KHR_LOADER_INIT_ANDROID)
#else
#define OXR_EXTENSION_SUPPORT_KHR_loader_init_android(_)
#endif


/*
 * XR_KHR_opengl_enable
 */
#if defined(XR_KHR_opengl_enable) && defined(XR_USE_GRAPHICS_API_OPENGL)
#define OXR_HAVE_KHR_opengl_enable
#define OXR_EXTENSION_SUPPORT_KHR_opengl_enable(_) _(KHR_opengl_enable, KHR_OPENGL_ENABLE)
#else
#define OXR_EXTENSION_SUPPORT_KHR_opengl_enable(_)
#endif


/*
 * XR_KHR_opengl_es_enable
 */
#if defined(XR_KHR_opengl_es_enable) && defined(XR_USE_GRAPHICS_API_OPENGL_ES)
#define OXR_HAVE_KHR_opengl_es_enable
#define OXR_EXTENSION_SUPPORT_KHR_opengl_es_enable(_) _(KHR_opengl_es_enable, KHR_OPENGL_ES_ENABLE)
#else
#define OXR_EXTENSION_SUPPORT_KHR_opengl_es_enable(_)
#endif


/*
 * XR_KHR_swapchain_usage_input_attachment_bit
 */
#if defined(XR_KHR_swapchain_usage_input_attachment_bit)
#define OXR_HAVE_KHR_swapchain_usage_input_attachment_bit
#define OXR_EXTENSION_SUPPORT_KHR_swapchain_usage_input_attachment_bit(_)                                              \
	_(KHR_swapchain_usage_input_attachment_bit, KHR_SWAPCHAIN_USAGE_INPUT_ATTACHMENT_BIT)
#else
#define OXR_EXTENSION_SUPPORT_KHR_swapchain_usage_input_attachment_bit(_)
#endif


/*
 * XR_KHR_visibility_mask
 */
#if defined(XR_KHR_visibility_mask) && defined(XRT_FEATURE_OPENXR_VISIBILITY_MASK)
#define OXR_HAVE_KHR_visibility_mask
#define OXR_EXTENSION_SUPPORT_KHR_visibility_mask(_) _(KHR_visibility_mask, KHR_VISIBILITY_MASK)
#else
#define OXR_EXTENSION_SUPPORT_KHR_visibility_mask(_)
#endif


/*
 * XR_KHR_vulkan_enable
 */
#if defined(XR_KHR_vulkan_enable) && defined(XR_USE_GRAPHICS_API_VULKAN)
#define OXR_HAVE_KHR_vulkan_enable
#define OXR_EXTENSION_SUPPORT_KHR_vulkan_enable(_) _(KHR_vulkan_enable, KHR_VULKAN_ENABLE)
#else
#define OXR_EXTENSION_SUPPORT_KHR_vulkan_enable(_)
#endif


/*
 * XR_KHR_vulkan_enable2
 */
#if defined(XR_KHR_vulkan_enable2) && defined(XR_USE_GRAPHICS_API_VULKAN)
#define OXR_HAVE_KHR_vulkan_enable2
#define OXR_EXTENSION_SUPPORT_KHR_vulkan_enable2(_) _(KHR_vulkan_enable2, KHR_VULKAN_ENABLE2)
#else
#define OXR_EXTENSION_SUPPORT_KHR_vulkan_enable2(_)
#endif


/*
 * XR_KHR_vulkan_swapchain_format_list
 */
#if defined(XR_KHR_vulkan_swapchain_format_list) && defined(XR_USE_GRAPHICS_API_VULKAN) &&                             \
    defined(XRT_FEATURE_OPENXR_VULKAN_SWAPCHAIN_FORMAT_LIST)
#define OXR_HAVE_KHR_vulkan_swapchain_format_list
#define OXR_EXTENSION_SUPPORT_KHR_vulkan_swapchain_format_list(_)                                                      \
	_(KHR_vulkan_swapchain_format_list, KHR_VULKAN_SWAPCHAIN_FORMAT_LIST)
#else
#define OXR_EXTENSION_SUPPORT_KHR_vulkan_swapchain_format_list(_)
#endif


/*
 * XR_KHR_win32_convert_performance_counter_time
 */
#if defined(XR_KHR_win32_convert_performance_counter_time) && defined(XR_USE_PLATFORM_WIN32)
#define OXR_HAVE_KHR_win32_convert_performance_counter_time
#define OXR_EXTENSION_SUPPORT_KHR_win32_convert_performance_counter_time(_)                                            \
	_(KHR_win32_convert_performance_counter_time, KHR_WIN32_CONVERT_PERFORMANCE_COUNTER_TIME)
#else
#define OXR_EXTENSION_SUPPORT_KHR_win32_convert_performance_counter_time(_)
#endif


/*
 * XR_EXT_debug_utils
 */
#if defined(XR_EXT_debug_utils) && defined(XRT_FEATURE_OPENXR_DEBUG_UTILS)
#define OXR_HAVE_EXT_debug_utils
#define OXR_EXTENSION_SUPPORT_EXT_debug_utils(_) _(EXT_debug_utils, EXT_DEBUG_UTILS)
#else
#define OXR_EXTENSION_SUPPORT_EXT_debug_utils(_)
#endif


/*
 * XR_EXT_dpad_binding
 */
#if defined(XR_EXT_dpad_binding)
#define OXR_HAVE_EXT_dpad_binding
#define OXR_EXTENSION_SUPPORT_EXT_dpad_binding(_) _(EXT_dpad_binding, EXT_DPAD_BINDING)
#else
#define OXR_EXTENSION_SUPPORT_EXT_dpad_binding(_)
#endif


/*
 * XR_EXT_eye_gaze_interaction
 */
#if defined(XR_EXT_eye_gaze_interaction) && defined(XRT_FEATURE_OPENXR_INTERACTION_EXT_EYE_GAZE)
#define OXR_HAVE_EXT_eye_gaze_interaction
#define OXR_EXTENSION_SUPPORT_EXT_eye_gaze_interaction(_) _(EXT_eye_gaze_interaction, EXT_EYE_GAZE_INTERACTION)
#else
#define OXR_EXTENSION_SUPPORT_EXT_eye_gaze_interaction(_)
#endif


/*
 * XR_EXT_hand_interaction
 */
#if defined(XR_EXT_hand_interaction) && defined(XRT_FEATURE_OPENXR_INTERACTION_EXT_HAND)
#define OXR_HAVE_EXT_hand_interaction
#define OXR_EXTENSION_SUPPORT_EXT_hand_interaction(_) _(EXT_hand_interaction, EXT_HAND_INTERACTION)
#else
#define OXR_EXTENSION_SUPPORT_EXT_hand_interaction(_)
#endif


/*
 * XR_EXT_hand_tracking
 */
#if defined(XR_EXT_hand_tracking)
#define OXR_HAVE_EXT_hand_tracking
#define OXR_EXTENSION_SUPPORT_EXT_hand_tracking(_) _(EXT_hand_tracking, EXT_HAND_TRACKING)
#else
#define OXR_EXTENSION_SUPPORT_EXT_hand_tracking(_)
#endif


/*
 * XR_EXT_hp_mixed_reality_controller
 */
#if defined(XR_EXT_hp_mixed_reality_controller) && defined(XRT_FEATURE_OPENXR_INTERACTION_WINMR)
#define OXR_HAVE_EXT_hp_mixed_reality_controller
#define OXR_EXTENSION_SUPPORT_EXT_hp_mixed_reality_controller(_)                                                       \
	_(EXT_hp_mixed_reality_controller, EXT_HP_MIXED_REALITY_CONTROLLER)
#else
#define OXR_EXTENSION_SUPPORT_EXT_hp_mixed_reality_controller(_)
#endif


/*
 * XR_EXT_local_floor
 */
#if defined(XR_EXT_local_floor) && defined(XRT_FEATURE_OPENXR_SPACE_LOCAL_FLOOR)
#define OXR_HAVE_EXT_local_floor
#define OXR_EXTENSION_SUPPORT_EXT_local_floor(_) _(EXT_local_floor, EXT_LOCAL_FLOOR)
#else
#define OXR_EXTENSION_SUPPORT_EXT_local_floor(_)
#endif


/*
 * XR_EXT_palm_pose
 */
#if defined(XR_EXT_palm_pose) && defined(XRT_FEATURE_OPENXR_INTERACTION_EXT_PALM_POSE)
#define OXR_HAVE_EXT_palm_pose
#define OXR_EXTENSION_SUPPORT_EXT_palm_pose(_) _(EXT_palm_pose, EXT_PALM_POSE)
#else
#define OXR_EXTENSION_SUPPORT_EXT_palm_pose(_)
#endif


/*
 * XR_EXT_performance_settings
 */
#if defined(XR_EXT_performance_settings) && defined(XRT_FEATURE_OPENXR_PERFORMANCE_SETTINGS)
#define OXR_HAVE_EXT_performance_settings
#define OXR_EXTENSION_SUPPORT_EXT_performance_settings(_) _(EXT_performance_settings, EXT_PERFORMANCE_SETTINGS)
#else
#define OXR_EXTENSION_SUPPORT_EXT_performance_settings(_)
#endif


/*
 * XR_EXT_samsung_odyssey_controller
 */
#if defined(XR_EXT_samsung_odyssey_controller) && defined(XRT_FEATURE_OPENXR_INTERACTION_WINMR)
#define OXR_HAVE_EXT_samsung_odyssey_controller
#define OXR_EXTENSION_SUPPORT_EXT_samsung_odyssey_controller(_)                                                        \
	_(EXT_samsung_odyssey_controller, EXT_SAMSUNG_ODYSSEY_CONTROLLER)
#else
#define OXR_EXTENSION_SUPPORT_EXT_samsung_odyssey_controller(_)
#endif


/*
 * XR_FB_composition_layer_alpha_blend
 */
#if defined(XR_FB_composition_layer_alpha_blend) && defined(XRT_FEATURE_OPENXR_LAYER_FB_ALPHA_BLEND)
#define OXR_HAVE_FB_composition_layer_alpha_blend
#define OXR_EXTENSION_SUPPORT_FB_composition_layer_alpha_blend(_)                                                      \
	_(FB_composition_layer_alpha_blend, FB_COMPOSITION_LAYER_ALPHA_BLEND)
#else
#define OXR_EXTENSION_SUPPORT_FB_composition_layer_alpha_blend(_)
#endif


/*
 * XR_FB_composition_layer_image_layout
 */
#if defined(XR_FB_composition_layer_image_layout) && defined(XRT_FEATURE_OPENXR_LAYER_FB_IMAGE_LAYOUT)
#define OXR_HAVE_FB_composition_layer_image_layout
#define OXR_EXTENSION_SUPPORT_FB_composition_layer_image_layout(_)                                                     \
	_(FB_composition_layer_image_layout, FB_COMPOSITION_LAYER_IMAGE_LAYOUT)
#else
#define OXR_EXTENSION_SUPPORT_FB_composition_layer_image_layout(_)
#endif


/*
 * XR_FB_composition_layer_settings
 */
#if defined(XR_FB_composition_layer_settings) && defined(XRT_FEATURE_OPENXR_LAYER_FB_SETTINGS)
#define OXR_HAVE_FB_composition_layer_settings
#define OXR_EXTENSION_SUPPORT_FB_composition_layer_settings(_)                                                         \
	_(FB_composition_layer_settings, FB_COMPOSITION_LAYER_SETTINGS)
#else
#define OXR_EXTENSION_SUPPORT_FB_composition_layer_settings(_)
#endif

/*
 * XR_FB_composition_layer_depth_test
 */
#if defined(XR_FB_composition_layer_depth_test) && defined(XRT_FEATURE_OPENXR_LAYER_FB_DEPTH_TEST)
#define OXR_HAVE_FB_composition_layer_depth_test
#define OXR_EXTENSION_SUPPORT_FB_composition_layer_depth_test(_)                                                       \
	_(FB_composition_layer_depth_test, FB_COMPOSITION_LAYER_DEPTH_TEST)
#else
#define OXR_EXTENSION_SUPPORT_FB_composition_layer_depth_test(_)
#endif

/*
 * XR_FB_display_refresh_rate
 */
#if defined(XR_FB_display_refresh_rate) && defined(XRT_FEATURE_OPENXR_DISPLAY_REFRESH_RATE)
#define OXR_HAVE_FB_display_refresh_rate
#define OXR_EXTENSION_SUPPORT_FB_display_refresh_rate(_) _(FB_display_refresh_rate, FB_DISPLAY_REFRESH_RATE)
#else
#define OXR_EXTENSION_SUPPORT_FB_display_refresh_rate(_)
#endif

/*
 * XR_FB_passthrough
 */
#if defined(XR_FB_passthrough) && defined(XRT_FEATURE_OPENXR_LAYER_PASSTHROUGH)
#define OXR_HAVE_FB_passthrough
#define OXR_EXTENSION_SUPPORT_FB_passthrough(_) _(FB_passthrough, FB_PASSTHROUGH)
#else
#define OXR_EXTENSION_SUPPORT_FB_passthrough(_)
#endif

/*
 * XR_ML_ml2_controller_interaction
 */
#if defined(XR_ML_ml2_controller_interaction) && defined(XRT_FEATURE_OPENXR_INTERACTION_ML2)
#define OXR_HAVE_ML_ml2_controller_interaction
#define OXR_EXTENSION_SUPPORT_ML_ml2_controller_interaction(_)                                                         \
	_(ML_ml2_controller_interaction, ML_ML2_CONTROLLER_INTERACTION)
#else
#define OXR_EXTENSION_SUPPORT_ML_ml2_controller_interaction(_)
#endif


/*
 * XR_MND_headless
 */
#if defined(XR_MND_headless) && defined(XRT_FEATURE_OPENXR_HEADLESS)
#define OXR_HAVE_MND_headless
#define OXR_EXTENSION_SUPPORT_MND_headless(_) _(MND_headless, MND_HEADLESS)
#else
#define OXR_EXTENSION_SUPPORT_MND_headless(_)
#endif


/*
 * XR_MND_swapchain_usage_input_attachment_bit
 */
#if defined(XR_MND_swapchain_usage_input_attachment_bit)
#define OXR_HAVE_MND_swapchain_usage_input_attachment_bit
#define OXR_EXTENSION_SUPPORT_MND_swapchain_usage_input_attachment_bit(_)                                              \
	_(MND_swapchain_usage_input_attachment_bit, MND_SWAPCHAIN_USAGE_INPUT_ATTACHMENT_BIT)
#else
#define OXR_EXTENSION_SUPPORT_MND_swapchain_usage_input_attachment_bit(_)
#endif


/*
 * XR_MSFT_hand_interaction
 */
#if defined(XR_MSFT_hand_interaction) && defined(XRT_FEATURE_OPENXR_INTERACTION_MSFT_HAND)
#define OXR_HAVE_MSFT_hand_interaction
#define OXR_EXTENSION_SUPPORT_MSFT_hand_interaction(_) _(MSFT_hand_interaction, MSFT_HAND_INTERACTION)
#else
#define OXR_EXTENSION_SUPPORT_MSFT_hand_interaction(_)
#endif


/*
 * XR_MSFT_unbounded_reference_space
 */
#if defined(XR_MSFT_unbounded_reference_space) && defined(XRT_FEATURE_OPENXR_SPACE_UNBOUNDED)
#define OXR_HAVE_MSFT_unbounded_reference_space
#define OXR_EXTENSION_SUPPORT_MSFT_unbounded_reference_space(_)                                                        \
	_(MSFT_unbounded_reference_space, MSFT_UNBOUNDED_REFERENCE_SPACE)
#else
#define OXR_EXTENSION_SUPPORT_MSFT_unbounded_reference_space(_)
#endif


/*
 * XR_OPPO_controller_interaction
 */
#if defined(XR_OPPO_controller_interaction) && defined(XRT_FEATURE_OPENXR_INTERACTION_OPPO)
#define OXR_HAVE_OPPO_controller_interaction
#define OXR_EXTENSION_SUPPORT_OPPO_controller_interaction(_) _(OPPO_controller_interaction, OPPO_CONTROLLER_INTERACTION)
#else
#define OXR_EXTENSION_SUPPORT_OPPO_controller_interaction(_)
#endif


/*
 * XR_EXTX_overlay
 */
#if defined(XR_EXTX_overlay) && defined(XRT_FEATURE_OPENXR_OVERLAY)
#define OXR_HAVE_EXTX_overlay
#define OXR_EXTENSION_SUPPORT_EXTX_overlay(_) _(EXTX_overlay, EXTX_OVERLAY)
#else
#define OXR_EXTENSION_SUPPORT_EXTX_overlay(_)
#endif


/*
 * XR_HTCX_vive_tracker_interaction
 */
#if defined(XR_HTCX_vive_tracker_interaction) && defined(ALWAYS_DISABLED)
#define OXR_HAVE_HTCX_vive_tracker_interaction
#define OXR_EXTENSION_SUPPORT_HTCX_vive_tracker_interaction(_)                                                         \
	_(HTCX_vive_tracker_interaction, HTCX_VIVE_TRACKER_INTERACTION)
#else
#define OXR_EXTENSION_SUPPORT_HTCX_vive_tracker_interaction(_)
#endif


/*
 * XR_MNDX_ball_on_a_stick_controller
 */
#if defined(XR_MNDX_ball_on_a_stick_controller) && defined(XRT_FEATURE_OPENXR_INTERACTION_MNDX)
#define OXR_HAVE_MNDX_ball_on_a_stick_controller
#define OXR_EXTENSION_SUPPORT_MNDX_ball_on_a_stick_controller(_)                                                       \
	_(MNDX_ball_on_a_stick_controller, MNDX_BALL_ON_A_STICK_CONTROLLER)
#else
#define OXR_EXTENSION_SUPPORT_MNDX_ball_on_a_stick_controller(_)
#endif


/*
 * XR_MNDX_egl_enable
 */
#if defined(XR_MNDX_egl_enable) && defined(XR_USE_PLATFORM_EGL) && defined(XR_USE_GRAPHICS_API_OPENGL)
#define OXR_HAVE_MNDX_egl_enable
#define OXR_EXTENSION_SUPPORT_MNDX_egl_enable(_) _(MNDX_egl_enable, MNDX_EGL_ENABLE)
#else
#define OXR_EXTENSION_SUPPORT_MNDX_egl_enable(_)
#endif


/*
 * XR_MNDX_force_feedback_curl
 */
#if defined(XR_MNDX_force_feedback_curl) && defined(XRT_FEATURE_OPENXR_FORCE_FEEDBACK_CURL)
#define OXR_HAVE_MNDX_force_feedback_curl
#define OXR_EXTENSION_SUPPORT_MNDX_force_feedback_curl(_) _(MNDX_force_feedback_curl, MNDX_FORCE_FEEDBACK_CURL)
#else
#define OXR_EXTENSION_SUPPORT_MNDX_force_feedback_curl(_)
#endif


/*
 * XR_MNDX_hydra
 */
#if defined(XR_MNDX_hydra) && defined(XRT_FEATURE_OPENXR_INTERACTION_MNDX)
#define OXR_HAVE_MNDX_hydra
#define OXR_EXTENSION_SUPPORT_MNDX_hydra(_) _(MNDX_hydra, MNDX_HYDRA)
#else
#define OXR_EXTENSION_SUPPORT_MNDX_hydra(_)
#endif


/*
 * XR_MNDX_system_buttons
 */
#if defined(XR_MNDX_system_buttons) && defined(XRT_FEATURE_OPENXR_INTERACTION_MNDX)
#define OXR_HAVE_MNDX_system_buttons
#define OXR_EXTENSION_SUPPORT_MNDX_system_buttons(_) _(MNDX_system_buttons, MNDX_SYSTEM_BUTTONS)
#else
#define OXR_EXTENSION_SUPPORT_MNDX_system_buttons(_)
#endif


/*
 * XR_HTC_facial_tracking
 */
#if defined(XR_HTC_facial_tracking) && defined(XRT_FEATURE_OPENXR_FACIAL_TRACKING_HTC)
#define OXR_HAVE_HTC_facial_tracking
#define OXR_EXTENSION_SUPPORT_HTC_facial_tracking(_) _(HTC_facial_tracking, HTC_FACIAL_TRACKING)
#else
#define OXR_EXTENSION_SUPPORT_HTC_facial_tracking(_)
#endif

// end of GENERATED per-extension defines - do not modify - used by scripts

/*!
 * Call this, passing a macro taking two parameters, to
 * generate tables, code, etc. related to OpenXR extensions.
 * Upon including invoking OXR_EXTENSION_SUPPORT_GENERATE() with some
 * MY_HANDLE_EXTENSION(mixed_case, all_caps), MY_HANDLE_EXTENSION will be called
 * for each extension implemented in Monado and supported in this build:
 *
 * - The first parameter is the name of the extension without the leading XR_
 *   prefix: e.g. `KHR_opengl_enable`
 * - The second parameter is the same text as the first, but in all uppercase,
 *   since this transform cannot be done in the C preprocessor, and some
 *   extension-related entities use this instead of the exact extension name.
 *
 * Implementation note: This macro calls another macro for each extension: that
 * macro is either defined to call your provided macro, or defined to nothing,
 * depending on if the extension is supported in this build.
 *
 * @note Do not edit anything between `clang-format off` and `clang-format on` -
 * it will be replaced next time this file is generated!
 */
// clang-format off
#define OXR_EXTENSION_SUPPORT_GENERATE(_) \
    OXR_EXTENSION_SUPPORT_KHR_android_create_instance(_) \
    OXR_EXTENSION_SUPPORT_KHR_android_thread_settings(_) \
    OXR_EXTENSION_SUPPORT_KHR_binding_modification(_) \
    OXR_EXTENSION_SUPPORT_KHR_composition_layer_color_scale_bias(_) \
    OXR_EXTENSION_SUPPORT_KHR_composition_layer_cube(_) \
    OXR_EXTENSION_SUPPORT_KHR_composition_layer_cylinder(_) \
    OXR_EXTENSION_SUPPORT_KHR_composition_layer_depth(_) \
    OXR_EXTENSION_SUPPORT_KHR_composition_layer_equirect(_) \
    OXR_EXTENSION_SUPPORT_KHR_composition_layer_equirect2(_) \
    OXR_EXTENSION_SUPPORT_KHR_convert_timespec_time(_) \
    OXR_EXTENSION_SUPPORT_KHR_D3D11_enable(_) \
    OXR_EXTENSION_SUPPORT_KHR_D3D12_enable(_) \
    OXR_EXTENSION_SUPPORT_KHR_loader_init(_) \
    OXR_EXTENSION_SUPPORT_KHR_loader_init_android(_) \
    OXR_EXTENSION_SUPPORT_KHR_opengl_enable(_) \
    OXR_EXTENSION_SUPPORT_KHR_opengl_es_enable(_) \
    OXR_EXTENSION_SUPPORT_KHR_swapchain_usage_input_attachment_bit(_) \
    OXR_EXTENSION_SUPPORT_KHR_visibility_mask(_) \
    OXR_EXTENSION_SUPPORT_KHR_vulkan_enable(_) \
    OXR_EXTENSION_SUPPORT_KHR_vulkan_enable2(_) \
    OXR_EXTENSION_SUPPORT_KHR_vulkan_swapchain_format_list(_) \
    OXR_EXTENSION_SUPPORT_KHR_win32_convert_performance_counter_time(_) \
    OXR_EXTENSION_SUPPORT_EXT_debug_utils(_) \
    OXR_EXTENSION_SUPPORT_EXT_dpad_binding(_) \
    OXR_EXTENSION_SUPPORT_EXT_eye_gaze_interaction(_) \
    OXR_EXTENSION_SUPPORT_EXT_hand_interaction(_) \
    OXR_EXTENSION_SUPPORT_EXT_hand_tracking(_) \
    OXR_EXTENSION_SUPPORT_EXT_hp_mixed_reality_controller(_) \
    OXR_EXTENSION_SUPPORT_EXT_local_floor(_) \
    OXR_EXTENSION_SUPPORT_EXT_palm_pose(_) \
    OXR_EXTENSION_SUPPORT_EXT_performance_settings(_) \
    OXR_EXTENSION_SUPPORT_EXT_samsung_odyssey_controller(_) \
    OXR_EXTENSION_SUPPORT_FB_composition_layer_alpha_blend(_) \
    OXR_EXTENSION_SUPPORT_FB_composition_layer_image_layout(_) \
    OXR_EXTENSION_SUPPORT_FB_composition_layer_settings(_) \
    OXR_EXTENSION_SUPPORT_FB_composition_layer_depth_test(_)  \
    OXR_EXTENSION_SUPPORT_FB_display_refresh_rate(_) \
    OXR_EXTENSION_SUPPORT_FB_passthrough(_) \
    OXR_EXTENSION_SUPPORT_ML_ml2_controller_interaction(_) \
    OXR_EXTENSION_SUPPORT_MND_headless(_) \
    OXR_EXTENSION_SUPPORT_MND_swapchain_usage_input_attachment_bit(_) \
    OXR_EXTENSION_SUPPORT_MSFT_hand_interaction(_) \
    OXR_EXTENSION_SUPPORT_MSFT_unbounded_reference_space(_) \
    OXR_EXTENSION_SUPPORT_OPPO_controller_interaction(_) \
    OXR_EXTENSION_SUPPORT_EXTX_overlay(_) \
    OXR_EXTENSION_SUPPORT_HTCX_vive_tracker_interaction(_) \
    OXR_EXTENSION_SUPPORT_MNDX_ball_on_a_stick_controller(_) \
    OXR_EXTENSION_SUPPORT_MNDX_egl_enable(_) \
    OXR_EXTENSION_SUPPORT_MNDX_force_feedback_curl(_) \
    OXR_EXTENSION_SUPPORT_MNDX_hydra(_) \
    OXR_EXTENSION_SUPPORT_MNDX_system_buttons(_) \
    OXR_EXTENSION_SUPPORT_HTC_facial_tracking(_)
// clang-format on
