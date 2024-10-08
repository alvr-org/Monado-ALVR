# Copyright 2020-2023, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0

# Catch2
if(BUILD_TESTING)
	add_library(xrt-external-catch2 STATIC Catch2/catch_amalgamated.cpp)
	target_include_directories(
		xrt-external-catch2 SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/Catch2
		)
endif()

# OpenVR
add_library(xrt-external-openvr INTERFACE)
target_include_directories(
	xrt-external-openvr SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/openvr_includes
	)

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
	nanopb/monado_metrics.pb.c
	nanopb/monado_metrics.pb.h
	nanopb/pb.h
	nanopb/pb_common.c
	nanopb/pb_common.h
	nanopb/pb_decode.c
	nanopb/pb_decode.h
	nanopb/pb_encode.c
	nanopb/pb_encode.h
	)
target_include_directories(xrt-external-nanopb PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/nanopb)

# OpenXR
add_library(xrt-external-openxr INTERFACE)
target_include_directories(
	xrt-external-openxr INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/openxr_includes
	)

# External VIT interface
if(XRT_FEATURE_SLAM)
	add_library(xrt-external-vit INTERFACE)
	target_include_directories(
		xrt-external-vit INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/vit_includes
		)
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

# renderdoc
add_library(xrt-external-renderdoc INTERFACE)
target_include_directories(
	xrt-external-renderdoc SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/renderdoc_api
	)

# imgui
if(XRT_HAVE_OPENGL)
	add_library(
		xrt-external-imgui STATIC
		imgui/imgui/cimgui.cpp
		imgui/imgui/cimgui.h
		imgui/imgui/cimplot.cpp
		imgui/imgui/cimplot.h
		imgui/imgui/imconfig.h
		imgui/imgui/imgui.cpp
		imgui/imgui/imgui.h
		imgui/imgui/imgui_demo.cpp
		imgui/imgui/imgui_draw.cpp
		imgui/imgui/imgui_impl_opengl3.cpp
		imgui/imgui/imgui_impl_opengl3.h
		imgui/imgui/imgui_internal.h
		imgui/imgui/imgui_widgets.cpp
		imgui/imgui/implot.cpp
		imgui/imgui/implot.h
		imgui/imgui/implot_demo.cpp
		imgui/imgui/implot_internal.h
		imgui/imgui/implot_items.cpp
		imgui/imgui/imstb_rectpack.h
		imgui/imgui/imstb_textedit.h
		imgui/imgui/imstb_truetype.h
		imgui/imgui_monado/imgui_monado.cpp
		)

	target_include_directories(
		xrt-external-imgui SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/imgui
		)
	target_include_directories(xrt-external-imgui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
	target_compile_definitions(xrt-external-imgui PUBLIC CIMGUI_NO_EXPORT)
	set_target_properties(xrt-external-imgui PROPERTIES INTERPROCEDURAL_OPTIMIZATION OFF)
	target_link_libraries(xrt-external-imgui PUBLIC xrt-external-glad)

	if(XRT_HAVE_SDL2)
		add_library(
			xrt-external-imgui-sdl2 STATIC
			imgui/imgui/cimgui_sdl.cpp imgui/imgui/imgui_impl_sdl.cpp
			imgui/imgui/imgui_impl_sdl.h
			)
		target_link_libraries(
			xrt-external-imgui-sdl2 PUBLIC xrt-external-imgui xrt-external-sdl2
			)
		set_target_properties(
			xrt-external-imgui-sdl2 PROPERTIES INTERPROCEDURAL_OPTIMIZATION OFF
			)
	endif()

endif()

# tinyceres
add_library(xrt-external-tinyceres INTERFACE)
target_include_directories(
	xrt-external-tinyceres SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/tinyceres/include
	)

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
