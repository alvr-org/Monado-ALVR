# Copyright 2020-2021, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0

# Generate a header containing defines for each enabled driver
set(FILE_CONTENTS "")
foreach(driver ${AVAILABLE_DRIVERS})
	if(XRT_BUILD_DRIVER_${driver})
		string(APPEND FILE_CONTENTS "#define XRT_BUILD_DRIVER_${driver}\n")
	endif()
endforeach()

# First setup all of the config headers.
configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/xrt_config_drivers.h.cmake_in
	${CMAKE_CURRENT_BINARY_DIR}/xrt_config_drivers.h @ONLY
	)
configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/xrt_config_have.h.cmake_in
	${CMAKE_CURRENT_BINARY_DIR}/xrt_config_have.h @ONLY
	)
configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/xrt_config_build.h.cmake_in
	${CMAKE_CURRENT_BINARY_DIR}/xrt_config_build.h @ONLY
	)
configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/xrt_config_vulkan.h.cmake_in
	${CMAKE_CURRENT_BINARY_DIR}/xrt_config_vulkan.h @ONLY
	)

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/xrt_config_android.h.cmake_in
	${CMAKE_CURRENT_BINARY_DIR}/xrt_config_android.h @ONLY
	)

if(XRT_HAVE_VULKAN)
	set(XR_USE_GRAPHICS_API_VULKAN ON)
endif()

if(XRT_HAVE_OPENGL)
	set(XR_USE_GRAPHICS_API_OPENGL ON)
endif()

if(XRT_HAVE_OPENGLES)
	set(XR_USE_GRAPHICS_API_OPENGL_ES ON)
endif()

if(XRT_HAVE_OPENGL_GLX AND XRT_HAVE_XLIB)
	set(XR_USE_PLATFORM_XLIB ON)
endif()

if(XRT_HAVE_EGL)
	set(XR_USE_PLATFORM_EGL ON)
endif()

if(XRT_HAVE_D3D11)
	set(XR_USE_GRAPHICS_API_D3D11 ON)
endif()

if(XRT_HAVE_D3D12)
	set(XR_USE_GRAPHICS_API_D3D12 ON)
endif()

if(ANDROID)
	set(XR_USE_PLATFORM_ANDROID ON)
endif()

if(WIN32)
	set(XR_USE_PLATFORM_WIN32 ON)
endif()

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/xrt_openxr_config.h.cmake_in
	${CMAKE_CURRENT_BINARY_DIR}/xrt_openxr_config.h @ONLY
	)
