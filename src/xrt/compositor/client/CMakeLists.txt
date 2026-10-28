# Copyright 2019-2024, Collabora, Ltd.
# Copyright 2025, NVIDIA CORPORATION.
# SPDX-License-Identifier: BSL-1.0

###
# Client library
#

add_library(comp_client STATIC)

target_link_libraries(
	comp_client
	PUBLIC xrt-interfaces comp_includes
	PRIVATE aux_util
	)

if(XRT_HAVE_VULKAN)
	target_sources(comp_client PRIVATE comp_vk_client.c comp_vk_client.h comp_vk_glue.c)
	target_link_libraries(comp_client PRIVATE aux_vk)
endif()

if(XRT_HAVE_OPENGL OR XRT_HAVE_OPENGLES)
	target_sources(
		comp_client PRIVATE comp_gl_client.c comp_gl_client.h comp_gl_memobj_swapchain.c
				    comp_gl_memobj_swapchain.h
		)
	target_link_libraries(comp_client PRIVATE aux_ogl)
endif()

if(XRT_HAVE_OPENGL)
	target_sources(comp_client PRIVATE comp_gl_glue.c)
endif()

if(XRT_HAVE_OPENGLES)
	target_sources(comp_client PRIVATE comp_gles_glue.c)
endif()

if(XRT_HAVE_EGL)
	target_sources(
		comp_client PRIVATE comp_gl_eglimage_swapchain.c comp_gl_eglimage_swapchain.h
				    comp_egl_client.c comp_egl_client.h
		)
endif()

if(XRT_HAVE_OPENGL_GLX AND XRT_HAVE_XLIB)
	target_sources(
		comp_client PRIVATE comp_gl_xlib_client.c comp_gl_xlib_client.h comp_gl_xlib_glue.c
		)
	target_link_libraries(comp_client PRIVATE OpenGL::GLX)
endif()

if(XRT_HAVE_OPENGL AND WIN32)
	target_sources(
		comp_client PRIVATE comp_gl_win32_client.c comp_gl_win32_client.h
				    comp_gl_win32_glue.c
		)
endif()

if(XRT_HAVE_D3D11 OR XRT_HAVE_D3D12)
	target_link_libraries(comp_client PRIVATE aux_d3d)
	target_sources(comp_client PRIVATE comp_d3d_common.cpp comp_d3d_common.hpp)
endif()

if(XRT_HAVE_D3D11)
	target_sources(
		comp_client PRIVATE comp_d3d11_client.cpp comp_d3d11_client.h comp_d3d11_glue.c
		)
endif()

if(XRT_HAVE_D3D12)
	target_sources(
		comp_client PRIVATE comp_d3d12_client.cpp comp_d3d12_client.h comp_d3d12_glue.c
		)
endif()
