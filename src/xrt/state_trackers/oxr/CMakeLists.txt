# Copyright 2019-2024, Collabora, Ltd.
# Copyright 2025-2026, NVIDIA CORPORATION.
# SPDX-License-Identifier: BSL-1.0

###
# Main code
#

add_subdirectory(bindings)
add_subdirectory(extension_support)

add_library(
	st_oxr STATIC
	actions/oxr_api_action.c
	actions/oxr_api_action.h
	actions/oxr_binding.c
	actions/oxr_binding.h
	actions/oxr_binding_profile.c
	actions/oxr_binding_profile.h
	actions/oxr_binding_point_store.c
	actions/oxr_binding_point_store.h
	actions/oxr_binding_point.h
	actions/oxr_binding_registry.c
	actions/oxr_binding_registry.h
	actions/oxr_dpad_state.c
	actions/oxr_dpad_state.h
	actions/oxr_get_state.c
	actions/oxr_get_state.h
	actions/oxr_haptic.c
	actions/oxr_haptic.h
	actions/oxr_input.c
	actions/oxr_input.h
	actions/oxr_input_transform.c
	actions/oxr_input_transform.h
	actions/oxr_instance_action_context.c
	actions/oxr_instance_action_context.h
	actions/oxr_pair_hashset.c
	actions/oxr_pair_hashset.h
	actions/oxr_priority_group_cache.c
	actions/oxr_priority_group_cache.h
	actions/oxr_resolver_for_bindings.c
	actions/oxr_resolver_for_bindings.h
	actions/oxr_resolver_for_profiles.c
	actions/oxr_resolver_for_profiles.h
	actions/oxr_rebinding_profile.c
	actions/oxr_rebinding_profile.h
	actions/oxr_instance_path_cache.c
	actions/oxr_instance_path_cache.h
	actions/oxr_io_cache.c
	actions/oxr_io_cache.h
	actions/oxr_session_action_context.c
	actions/oxr_session_action_context.h
	actions/oxr_session_attached_actions.c
	actions/oxr_session_attached_actions.h
	actions/oxr_set_haptic.c
	actions/oxr_set_haptic.h
	actions/oxr_subaction_store.c
	actions/oxr_subaction_store.h
	actions/oxr_subaction.c
	actions/oxr_subaction.h
	actions/oxr_suggested_profile.c
	actions/oxr_suggested_profile.h
	actions/oxr_suggested_profile_array.c
	actions/oxr_suggested_profile_array.h
	common/oxr_defines.h
	common/oxr_forward_declarations.h
	common/oxr_handle_array.c
	common/oxr_handle_array.h
	common/oxr_handle_base.c
	common/oxr_handle_base.h
	common/oxr_handle_cast.h
	common/oxr_refcounted.h
	path/oxr_path_store.c
	path/oxr_path_store.h
	path/oxr_path_wrappers.h
	oxr_api_funcs.h
	oxr_api_instance.c
	oxr_api_negotiate.c
	oxr_api_session.c
	oxr_api_space.c
	oxr_api_swapchain.c
	oxr_api_system.c
	oxr_api_verify.h
	oxr_api_xdev.c
	oxr_chain.h
	oxr_event.c
	oxr_frame_sync.c
	oxr_frame_sync.h
	oxr_instance.c
	oxr_logger.c
	oxr_logger.h
	oxr_objects.h
	oxr_pretty_print.c
	oxr_pretty_print.h
	oxr_roles.c
	oxr_roles.h
	oxr_session.c
	oxr_session_frame_end.c
	oxr_space.c
	oxr_swapchain.c
	oxr_system.c
	oxr_two_call.h
	oxr_verify.c
	oxr_xdev.c
	${CMAKE_CURRENT_BINARY_DIR}/bindings/oxr_generated_bindings.c
	${CMAKE_CURRENT_BINARY_DIR}/bindings/oxr_generated_bindings.h
	${CMAKE_CURRENT_BINARY_DIR}/extension_support/oxr_extension_support.h
	)

# For OpenXR unit tests to find common files like oxr_defines.h file.
target_include_directories(st_oxr PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/common)

# To generate the oxr_generated_bindings.c and oxr_generated_bindings.h files.
add_dependencies(st_oxr generate_oxr_bindings)
# To enable finding oxr_objects.h in the oxr_generated_bindings.c file.
target_include_directories(st_oxr PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
# For OpenXR unit tests to find the oxr_generated_bindings.h file.
target_include_directories(st_oxr PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/bindings)

# To generate the oxr_extension_support.h file.
add_dependencies(st_oxr generate_oxr_extension_support)
# For OpenXR unit tests to find the oxr_extension_support.h file.
target_include_directories(st_oxr PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/extension_support)

if(XRT_HAVE_VULKAN)
	target_sources(st_oxr PRIVATE oxr_session_gfx_vk.c oxr_swapchain_vk.c oxr_vulkan.c)
	target_link_libraries(st_oxr PUBLIC aux_vk)
endif()

if(XRT_HAVE_OPENGL OR XRT_HAVE_OPENGLES)
	target_sources(st_oxr PRIVATE oxr_swapchain_gl.c)
	target_link_libraries(st_oxr PUBLIC aux_ogl)
endif()

if(XRT_HAVE_OPENGL_GLX AND XRT_HAVE_XLIB)
	target_sources(st_oxr PRIVATE oxr_session_gfx_gl_xlib.c)
endif()

if(XRT_HAVE_EGL)
	target_sources(st_oxr PRIVATE oxr_session_gfx_egl.c oxr_egl.c)
endif()

if(XRT_HAVE_D3D11 OR XRT_HAVE_D3D12)
	target_sources(st_oxr PRIVATE oxr_d3d.cpp)
	target_link_libraries(st_oxr PRIVATE aux_d3d)
endif()

if(XRT_HAVE_D3D11)
	target_sources(st_oxr PRIVATE oxr_session_gfx_d3d11.c oxr_swapchain_d3d11.c oxr_d3d11.cpp)
endif()

if(XRT_HAVE_D3D12)
	target_sources(st_oxr PRIVATE oxr_session_gfx_d3d12.c oxr_swapchain_d3d12.c oxr_d3d12.cpp)
	target_link_libraries(st_oxr PRIVATE aux_d3d)
endif()

if(ANDROID)
	target_sources(st_oxr PRIVATE oxr_session_gfx_gles_android.c)
	target_link_libraries(st_oxr PRIVATE aux_android)
endif()

if(WIN32)
	target_link_libraries(st_oxr PRIVATE kernel32)
endif()

if(WIN32 AND XRT_HAVE_OPENGL)
	target_sources(st_oxr PRIVATE oxr_session_gfx_gl_win32.c)
endif()

if(XRT_FEATURE_OPENXR_LAYER_FB_PASSTHROUGH)
	target_sources(st_oxr PRIVATE oxr_api_passthrough.c oxr_passthrough.c)
endif()

if(XRT_FEATURE_OPENXR_BODY_TRACKING_FB)
	target_sources(st_oxr PRIVATE oxr_api_body_tracking.c oxr_body_tracking.c)
endif()

if(XRT_FEATURE_OPENXR_BODY_TRACKING_BD)
	target_sources(st_oxr PRIVATE oxr_api_body_tracking_bd.c oxr_body_tracking_bd.c)
endif()

if(XRT_FEATURE_OPENXR_FACE_TRACKING_ANDROID)
	target_sources(st_oxr PRIVATE oxr_api_face_tracking_android.c oxr_face_tracking_android.c)
endif()

if(XRT_FEATURE_OPENXR_FACIAL_TRACKING_HTC)
	target_sources(st_oxr PRIVATE oxr_api_face_tracking_htc.c oxr_face_tracking_htc.c)
endif()

if(XRT_FEATURE_OPENXR_FACE_TRACKING2_FB)
	target_sources(st_oxr PRIVATE oxr_api_face_tracking2_fb.c oxr_face_tracking2_fb.c)
endif()

if(XRT_FEATURE_OPENXR_FUTURE_EXT)
	target_sources(st_oxr PRIVATE oxr_api_future.c oxr_future.c)
endif()

if(XRT_FEATURE_OPENXR_HAND_TRACKING_EXT)
	target_sources(
		st_oxr PRIVATE oxr_api_hand_tracking.c oxr_hand_tracking.c oxr_hand_tracking.h
		)
endif()

target_link_libraries(
	st_oxr
	PRIVATE
		xrt-interfaces
		xrt-external-openxr
		aux_util
		aux_math
		comp_client
		aux-includes
	PUBLIC aux_os
	)

if(XRT_FEATURE_RENDERDOC)
	target_link_libraries(st_oxr PUBLIC xrt-external-renderdoc)
	if(XRT_HAVE_LINUX)
		target_link_libraries(st_oxr PRIVATE ${CMAKE_DL_LIBS})
	endif()
endif()

target_include_directories(
	st_oxr
	PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
	INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..
	)
