# Copyright 2020-2025, Collabora, Ltd.
# Copyright 2025-2026, NVIDIA CORPORATION.
# SPDX-License-Identifier: BSL-1.0

# Catch2
if(BUILD_TESTING)
	add_library(xrt-external-catch2 STATIC Catch2/catch_amalgamated.cpp)
	target_include_directories(
		xrt-external-catch2 SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/Catch2
		)
	# clang-22 gives a warning for this in our build configuration, so let's disable it.
	target_compile_definitions(xrt-external-catch2 PUBLIC CATCH_CONFIG_NO_COUNTER)
	if(MSVC)
		# Needed for Visual Studio 2019.
		# https://github.com/catchorg/Catch2/issues/2916#issuecomment-2404082203
		target_compile_options(xrt-external-catch2 PRIVATE "/Zc:hiddenFriend-")
	endif()
endif()

# OpenVR
add_subdirectory(openvr_includes)

# cJSON
add_library(xrt-external-cjson INTERFACE)
if(XRT_HAVE_SYSTEM_CJSON)
	target_link_libraries(xrt-external-cjson INTERFACE cJSON::cJSON)
else()
	target_include_directories(
		xrt-external-cjson SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/cjson
		)
	target_compile_definitions(xrt-external-cjson INTERFACE CJSON_HIDE_SYMBOLS)
endif()

# FlexKalman
add_library(xrt-external-flexkalman INTERFACE)
target_include_directories(xrt-external-flexkalman INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/flexkalman)

# FlexKalman
add_library(xrt-external-util-headers INTERFACE)
target_include_directories(
	xrt-external-util-headers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/util-headers
	)

# Glad
add_library(xrt-external-glad INTERFACE)
target_include_directories(
	xrt-external-glad SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/glad/include
	)

# Hungarian graph algorithm
add_library(xrt-external-hungarian INTERFACE)
target_include_directories(
	xrt-external-hungarian SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/hungarian
	)

# JNIPP and Android JNI wrappers
if(ANDROID)
	add_library(xrt-external-jnipp STATIC jnipp/jnipp.cpp)
	target_include_directories(
		xrt-external-jnipp SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/jnipp
		)

	file(GLOB WRAP_SOURCES android-jni-wrap/wrap/*.cpp)
	add_library(xrt-external-jni-wrap STATIC ${WRAP_SOURCES})
	target_include_directories(
		xrt-external-jni-wrap PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/android-jni-wrap
		)
	target_link_libraries(xrt-external-jni-wrap PUBLIC xrt-external-jnipp)
endif()

# Nano Protobuf
add_library(
	xrt-external-nanopb STATIC
	nanopb/pb.h
	nanopb/pb_common.c
	nanopb/pb_common.h
	nanopb/pb_decode.c
	nanopb/pb_decode.h
	nanopb/pb_encode.c
	nanopb/pb_encode.h
	)
target_include_directories(xrt-external-nanopb PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/nanopb)

add_library(xrt-external-metrics STATIC metrics/monado_metrics.pb.c metrics/monado_metrics.pb.h)

if(ANDROID)
	target_compile_options(xrt-external-metrics PRIVATE -Wno-gnu-offsetof-extensions)
endif()

target_include_directories(xrt-external-metrics PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/metrics/)

target_link_libraries(xrt-external-metrics PUBLIC xrt-external-nanopb)

if(ANDROID)
	add_library(
		xrt-external-cardboard STATIC cardboard/cardboard_device.pb.h
					      cardboard/cardboard_device.pb.c
		)

	target_include_directories(
		xrt-external-cardboard PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/cardboard/
		)
	target_link_libraries(xrt-external-cardboard PUBLIC xrt-external-nanopb)
endif()

# OpenXR
add_subdirectory(openxr_includes)

# External VIT interface
if(XRT_FEATURE_SLAM)
	add_library(xrt-external-vit INTERFACE)
	target_include_directories(
		xrt-external-vit INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/vit_includes
		)
endif()

# Vulkan
if(XRT_HAVE_VULKAN)
	add_subdirectory(vulkan)
endif()

# SDL
if(XRT_HAVE_SDL2)
	add_library(xrt-external-sdl2 INTERFACE)
	target_link_libraries(xrt-external-sdl2 INTERFACE SDL2::SDL2)

	# Fix SDL2 build include for Alpine Linux
	find_path(SDL2_DIRECTFB_INCLUDE_DIR directfb.h PATH_SUFFIXES directfb)
	if(SDL2_DIRECTFB_INCLUDE_DIR)
		target_include_directories(xrt-external-sdl2 INTERFACE ${SDL2_DIRECTFB_INCLUDE_DIR})
	endif()
endif()

# STB
add_library(xrt-external-stb INTERFACE)
target_include_directories(xrt-external-stb SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/stb)

# bcdec
add_library(xrt-external-bcdec INTERFACE)
target_include_directories(xrt-external-bcdec SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/bcdec)

# renderdoc
add_library(xrt-external-renderdoc INTERFACE)
target_include_directories(
	xrt-external-renderdoc SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/renderdoc_api
	)

# imgui
if(XRT_HAVE_OPENGL)
	add_subdirectory(imgui)
endif()

# Ceres, from the system if we have it
if(XRT_HAVE_BIGCERES)
	add_subdirectory(bigceres)
endif()

add_subdirectory(tinyceres)

# Tracy
if(XRT_HAVE_TRACY)
	add_library(xrt-external-tracy STATIC tracy/TracyClient.cpp)
	target_include_directories(
		xrt-external-tracy SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/tracy
		)
	target_compile_definitions(xrt-external-tracy PRIVATE TRACY_ENABLE)
	target_link_libraries(xrt-external-tracy PRIVATE ${CMAKE_DL_LIBS})
endif()

# Valve VDF parser
add_library(xrt-external-vdf INTERFACE)
target_include_directories(
	xrt-external-vdf SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/valve-file-vdf
	)
