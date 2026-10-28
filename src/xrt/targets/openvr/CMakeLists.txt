# Copyright 2019, Collabora, Ltd.
# Copyright 2025-2026, NVIDIA CORPORATION.
# Copyright 2026, Beyley Cardellio
# SPDX-License-Identifier: BSL-1.0

######
# Create a loadable OpenVR driver.

if(APPLE)
	# Needed to make a dynlib.
	add_library(openvr SHARED target.c)
else()
	add_library(openvr MODULE target.c libopenvr.def)
endif()

# Note: Order may matter in these lists!
target_link_libraries(openvr PUBLIC aux_vk aux_os aux_util aux_math)

if(XRT_FEATURE_IPC_CLIENT)
	target_link_libraries(openvr PUBLIC st_openvr ipc_client comp_client)
else()
	target_link_libraries(
		openvr
		PUBLIC
			st_openvr
			st_prober
			target_lists
			target_instance
			comp_main
			comp_client
		)
endif()

if(APPLE)
	# Only expose the negotiate symbol.
	set_property(
		TARGET openvr
		APPEND_STRING
		PROPERTY
			LINK_FLAGS
			"-Wl,-exported_symbol -Wl,_VRClientCoreFactory -Wl,-exported_symbol -Wl,_HmdSystemFactory"
		)
elseif(NOT WIN32)
	# Force the main "negotiate" symbol's inclusion
	# and use a version script to ensure that's the only one we expose.
	set_property(
		TARGET openvr
		APPEND_STRING
		PROPERTY
			LINK_FLAGS
			"-u VRClientCoreFactory -u HmdSystemFactory -Wl,--version-script=\"${CMAKE_CURRENT_SOURCE_DIR}/libopenvr.version\""
		)
	# Re-link if the version script changes.
	set_property(
		TARGET openvr
		APPEND
		PROPERTY LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/libopenvr.version"
		)
endif()

# Optional debug ui
if(XRT_FEATURE_CLIENT_DEBUG_GUI)
	target_link_libraries(openvr PRIVATE aux_util_debug_gui)
endif()

set_target_properties(openvr PROPERTIES OUTPUT_NAME "vrclient")
# Some random platforms don't use the platform subdir, but still have one defined.
set(OPENVR_USE_PLATFORM_SUBDIR ON)

# https://github.com/ValveSoftware/openvr/blob/0924064316de3effbcd1acf1e309182a2deb1c05/samples/shared/pathtools.h#L109
if(APPLE)
	set(OPENVR_TARGET_SUFFIX ".dylib")
	set(OPENVR_PLATFORM_SUBDIR "osx32")
elseif(WIN32)
	# https://github.com/ValveSoftware/openvr/blob/0924064316de3effbcd1acf1e309182a2deb1c05/src/openvr_api_public.cpp#L130
	set(OPENVR_USE_PLATFORM_SUBDIR OFF)

	set(OPENVR_TARGET_SUFFIX ".dll")
	if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
		set(OPENVR_PLATFORM_SUBDIR "win64")

		# https://github.com/ValveSoftware/openvr/blob/0924064316de3effbcd1acf1e309182a2deb1c05/src/openvr_api_public.cpp#L141
		set_target_properties(openvr PROPERTIES OUTPUT_NAME "vrclient_x64")
	else()
		set(OPENVR_PLATFORM_SUBDIR "win32")
	endif()
elseif(UNIX)
	set(OPENVR_TARGET_SUFFIX ".so")
	if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
		set(OPENVR_PLATFORM_SUBDIR "linux64")
	else()
		set(OPENVR_PLATFORM_SUBDIR "linux32")

		# https://github.com/ValveSoftware/openvr/blob/0924064316de3effbcd1acf1e309182a2deb1c05/src/openvr_api_public.cpp#L130
		set(OPENVR_USE_PLATFORM_SUBDIR OFF)
	endif()
else()
	message(FATAL_ERROR "Unsupported platform")
endif()

set_target_properties(openvr PROPERTIES PREFIX "")
set_target_properties(openvr PROPERTIES SUFFIX "${OPENVR_TARGET_SUFFIX}")
if(OPENVR_USE_PLATFORM_SUBDIR)
	set_target_properties(
		openvr PROPERTIES LIBRARY_OUTPUT_DIRECTORY
				  "${PROJECT_BINARY_DIR}/st-openvr/bin/${OPENVR_PLATFORM_SUBDIR}/"
		)
else()
	set_target_properties(
		openvr PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/st-openvr/bin/"
		)
endif()

# Copy the `version.txt` to st-openvr/bin/ so that Steam can find it
# The version in this file is set to the same value as the version of SteamVR we're matching compatibility with.
file(COPY version.txt DESTINATION "${PROJECT_BINARY_DIR}/st-openvr/bin/")

install(
	DIRECTORY "${PROJECT_BINARY_DIR}/st-openvr"
	DESTINATION "${CMAKE_INSTALL_PREFIX}/${XRT_OPENVR_STATE_TRACKER_INSTALL_DIR}"
	)
