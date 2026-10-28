// Copyright 2018-2024, Collabora, Ltd.
// Copyright 2023-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Forward declarations for OpenXR state tracker structs.
 * @ingroup oxr_main
 */

#pragma once

#include "xrt/xrt_compiler.h"


#ifdef __cplusplus
extern "C" {
#endif


/*
 *
 * Forward declare structs.
 *
 */

struct xrt_instance;
struct xrt_device;

struct u_hashset;
struct u_hashmap_int;

struct time_state;

struct oxr_logger;
struct oxr_sink_logger;
struct oxr_extension_status;
struct oxr_instance;
struct oxr_instance_path_cache;
struct oxr_instance_action_context;
struct oxr_path;
struct oxr_path_store;
struct oxr_system;
struct oxr_session;
struct oxr_roles;
struct oxr_event;
struct oxr_swapchain;
struct oxr_space;
struct oxr_action_set;
struct oxr_action;
struct oxr_debug_messenger;
struct oxr_handle_base;
struct oxr_subaction_paths;
struct oxr_action_cache;
struct oxr_action_attachment;
struct oxr_action_set_attachment;
struct oxr_action_input;
struct oxr_action_output;
struct oxr_io_cache;
struct oxr_action_profile_bindings;
struct oxr_session_action_context;
struct oxr_session_attached_actions;
struct oxr_dpad_state;
struct oxr_binding_point;
struct oxr_binding_profile;
struct oxr_priority_group_cache;
struct oxr_priority_group_state;
struct oxr_binding_registry;
struct oxr_rebinding_profile;
struct oxr_action_set_ref;
struct oxr_action_ref;
struct oxr_hand_tracker;
struct oxr_face_tracker_android;
struct oxr_facial_tracker_htc;
struct oxr_face_tracker2_fb;
struct oxr_body_tracker_fb;
struct oxr_body_tracker_bd;
struct oxr_xdev_list;
struct oxr_plane_detector_ext;


#ifdef __cplusplus
}
#endif
