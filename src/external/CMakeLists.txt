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
		# imgui 1.90.8-docking
		imgui/imgui/imconfig.h
		imgui/imgui/imgui.cpp
		imgui/imgui/imgui_tables.cpp
		imgui/imgui/imgui.h
		imgui/imgui/imgui_demo.cpp
		imgui/imgui/imgui_draw.cpp
		imgui/imgui/backends/imgui_impl_opengl3.cpp
		imgui/imgui/backends/imgui_impl_opengl3.h
		imgui/imgui/imgui_internal.h
		imgui/imgui/imgui_widgets.cpp
		imgui/imgui/imstb_rectpack.h
		imgui/imgui/imstb_textedit.h
		imgui/imgui/imstb_truetype.h
		# cimgui 7c16d31cdb9d2db3038b324fe967ffa76b02c8c4
		imgui/cimgui/cimgui.cpp
		imgui/cimgui/cimgui.h
		imgui/cimgui/cimgui_impl.h
		# implot 0.17 f156599faefe316f7dd20fe6c783bf87c8bb6fd9
		imgui/implot/implot.cpp
		imgui/implot/implot.h
		imgui/implot/implot_internal.h
		imgui/implot/implot_items.cpp
		imgui/implot/implot_demo.cpp
		# cimplot 439f705b1cfae0e9fbe42c13ab5b45becc47d404
		imgui/cimplot/cimplot.cpp
		imgui/cimplot/cimplot.h
		imgui/imgui_monado/imgui_monado.cpp
		)

	target_include_directories(
		xrt-external-imgui SYSTEM
		PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/imgui
		PRIVATE
			${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui
			${CMAKE_CURRENT_SOURCE_DIR}/imgui/cimgui
			${CMAKE_CURRENT_SOURCE_DIR}/imgui/implot
		)
	target_compile_definitions(xrt-external-imgui PUBLIC CIMGUI_NO_EXPORT CIMGUI_USE_OPENGL3)

	if(WIN32)
		target_compile_definitions(
			xrt-external-imgui
			PUBLIC "IMGUI_IMPL_API=extern\ \"C\" __declspec\(dllexport\)"
			)
	else()
		target_compile_definitions(xrt-external-imgui PUBLIC "IMGUI_IMPL_API=extern\ \"C\"")
	endif()

	set_target_properties(xrt-external-imgui PROPERTIES INTERPROCEDURAL_OPTIMIZATION OFF)
	target_link_libraries(xrt-external-imgui PUBLIC xrt-external-glad)

	if(XRT_HAVE_SDL2)
		add_library(
			xrt-external-imgui-sdl2 STATIC imgui/imgui/backends/imgui_impl_sdl2.cpp
						       imgui/imgui/backends/imgui_impl_sdl2.h
			)
		target_include_directories(
			xrt-external-imgui-sdl2 SYSTEM
			PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui
			)
		target_compile_definitions(xrt-external-imgui-sdl2 PUBLIC CIMGUI_USE_SDL2)
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
