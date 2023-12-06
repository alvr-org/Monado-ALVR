# Copyright 2019-2024, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0

###
# Main code
#

add_library(oxr-interfaces INTERFACE)
target_include_directories(
	oxr-interfaces INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
	)

# aux_os, because oxr_objects.h includes os_threading.h
target_link_libraries(
	oxr-interfaces INTERFACE xrt-external-openxr aux_os Vulkan::Vulkan xrt-external-renderdoc
	)

add_library(
	st_oxr STATIC
	oxr_api_action.c
	oxr_api_funcs.h
	oxr_api_instance.c
	oxr_api_negotiate.c
	oxr_api_session.c
	oxr_api_space.c
	oxr_api_swapchain.c
	oxr_api_system.c
	oxr_api_verify.h
	oxr_api_xdev.c
	oxr_binding.c
	oxr_chain.h
	oxr_dpad.c
	oxr_event.c
	oxr_extension_support.h
	oxr_frame_sync.c
	oxr_frame_sync.h
	oxr_handle_base.c
	oxr_input.c
	oxr_input_transform.c
	oxr_input_transform.h
	oxr_instance.c
	oxr_logger.c
	oxr_logger.h
	oxr_objects.h
	oxr_path.c
	oxr_pretty_print.c
	oxr_pretty_print.h
	oxr_session.c
	oxr_session_frame_end.c
	oxr_space.c
	oxr_swapchain.c
	oxr_system.c
	oxr_two_call.h
	oxr_verify.c
	oxr_xdev.c
	)

if(XRT_HAVE_VULKAN)
	target_sources(st_oxr PRIVATE oxr_session_gfx_vk.c oxr_swapchain_vk.c oxr_vulkan.c)
	target_link_libraries(st_oxr PUBLIC Vulkan::Vulkan)
endif()

if(XRT_HAVE_OPENGL OR XRT_HAVE_OPENGLES)
	target_sources(st_oxr PRIVATE oxr_swapchain_gl.c)
	target_link_libraries(st_oxr PUBLIC aux_ogl)
endif()

if(XRT_HAVE_OPENGL_GLX AND XRT_HAVE_XLIB)
	target_sources(st_oxr PRIVATE oxr_session_gfx_gl_xlib.c)
endif()

if(XRT_HAVE_EGL)
	target_sources(st_oxr PRIVATE oxr_session_gfx_egl.c)
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

if(XRT_FEATURE_OPENXR_FACIAL_TRACKING_HTC)
	target_sources(st_oxr PRIVATE oxr_api_face_tracking.c oxr_face_tracking.c)
endif()

if(XRT_FEATURE_OPENXR_FACE_TRACKING2_FB)
	target_sources(st_oxr PRIVATE oxr_api_face_tracking2_fb.c oxr_face_tracking2_fb.c)
endif()

target_link_libraries(
	st_oxr
	PRIVATE
		xrt-interfaces
		xrt-external-openxr
		aux_util
		aux_math
		aux_generated_bindings
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
