# Copyright 2020-2021, Collabora, Ltd.
# Copyright 2025-2026, NVIDIA CORPORATION.
# SPDX-License-Identifier: BSL-1.0

###
# Merge split protocol JSON files into single proto.json
file(GLOB PROTO_JSON_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shared/proto/*.json")

merge_json_files(
	OUTPUT
	"${CMAKE_CURRENT_BINARY_DIR}/proto.json"
	SOURCES
	${PROTO_JSON_FILES}
	IGNORE_SCHEMA
	)

###
# Generator

foreach(
	fn
	ipc_protocol_generated.h
	ipc_client_generated.h
	ipc_client_generated.c
	ipc_server_generated.h
	ipc_server_generated.c
	)
	add_custom_command(
		OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${fn}"
		COMMAND
			${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/shared/proto.py
			${CMAKE_CURRENT_BINARY_DIR}/proto.json "${CMAKE_CURRENT_BINARY_DIR}/${fn}"
		VERBATIM
		DEPENDS
			${CMAKE_CURRENT_SOURCE_DIR}/shared/proto.py
			${CMAKE_CURRENT_SOURCE_DIR}/shared/ipcproto/common.py
			${CMAKE_CURRENT_BINARY_DIR}/proto.json
			${CMAKE_CURRENT_SOURCE_DIR}/shared/ipc_protocol_generated.h.template
		COMMENT "Generating ${fn} from protocol JSON description"
		)
endforeach()

# Generate IPC structures list in the build root
add_custom_command(
	OUTPUT "${CMAKE_BINARY_DIR}/ipc-structs.txt"
	COMMAND
		${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/shared/proto.py
		${CMAKE_CURRENT_BINARY_DIR}/proto.json "${CMAKE_BINARY_DIR}/ipc-structs.txt"
	VERBATIM
	DEPENDS
		${CMAKE_CURRENT_SOURCE_DIR}/shared/proto.py
		${CMAKE_CURRENT_SOURCE_DIR}/shared/ipcproto/common.py
		${CMAKE_CURRENT_BINARY_DIR}/proto.json
	COMMENT "Generating ipc-structs.txt from protocol JSON description"
	)

add_custom_target(ipc_structs_list ALL DEPENDS "${CMAKE_BINARY_DIR}/ipc-structs.txt")

set(IPC_COMMON_SOURCES
    ${CMAKE_CURRENT_BINARY_DIR}/ipc_protocol_generated.h
    shared/ipc_message_channel.h
    shared/ipc_shmem.c
    shared/ipc_shmem.h
    shared/ipc_utils.c
    shared/ipc_utils.h
	)

add_library(ipc_shared STATIC ${IPC_COMMON_SOURCES})
target_include_directories(
	ipc_shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
	)

if(WIN32)
	target_sources(ipc_shared PRIVATE shared/ipc_message_channel_windows.cpp)
else()
	target_sources(ipc_shared PRIVATE shared/ipc_message_channel_unix.c)
endif()

target_link_libraries(ipc_shared PRIVATE aux_util)

if(RT_LIBRARY)
	target_link_libraries(ipc_shared PUBLIC ${RT_LIBRARY})
endif()

###
# Client

add_library(
	ipc_client STATIC
	${CMAKE_CURRENT_BINARY_DIR}/ipc_client_generated.c
	${CMAKE_CURRENT_BINARY_DIR}/ipc_client_generated.h
	client/ipc_client.h
	client/ipc_client_app_policy.c
	client/ipc_client_compositor.c
	client/ipc_client_connection.c
	client/ipc_client_device.c
	client/ipc_client_future.c
	client/ipc_client_body_tracker.c
	client/ipc_client_body_tracker.h
	client/ipc_client_hand_tracker.c
	client/ipc_client_hand_tracker.h
	client/ipc_client_io_cache.c
	client/ipc_client_io_cache.h
	client/ipc_client_hmd.c
	client/ipc_client_instance.c
	client/ipc_client_session.c
	client/ipc_client_space_overseer.c
	client/ipc_client_system.c
	client/ipc_client_system_devices.c
	client/ipc_client_system_devices.h
	client/ipc_client_tracking_origin.c
	client/ipc_client_tracking_origin.h
	client/ipc_client_xdev.c
	client/ipc_client_xdev.h
	)
target_include_directories(
	ipc_client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
	)
if(WIN32)
	if(XRT_SERVICE_EXECUTABLE)
		target_compile_definitions(
			ipc_client PUBLIC XRT_SERVICE_EXECUTABLE="${XRT_SERVICE_EXECUTABLE}.exe"
			)
	endif()

	target_link_libraries(ipc_client PRIVATE winmm)
endif()
target_link_libraries(ipc_client PRIVATE aux_util ipc_shared base)

if(ANDROID)
	# Need android_instance_base on the client side too.
	target_link_libraries(ipc_client PRIVATE aux_android)
endif()

###
# Server

add_library(
	ipc_server STATIC
	${CMAKE_CURRENT_BINARY_DIR}/ipc_server_generated.c
	${CMAKE_CURRENT_BINARY_DIR}/ipc_server_generated.h
	server/ipc_server.h
	server/ipc_server_handler.c
	server/ipc_server_handler_app_policy.c
	server/ipc_server_handler_body_tracker.c
	server/ipc_server_handler_hand_tracker.c
	server/ipc_server_handler_io_cache.c
	server/ipc_server_handler_swapchain.c
	server/ipc_server_per_client_thread.c
	server/ipc_server_process.c
	server/ipc_server_thread_shutdown.c
	server/ipc_server_thread_shutdown.h
	server/ipc_server_objects.c
	server/ipc_server_objects.h
	)
target_include_directories(
	ipc_server
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
	PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
	)
target_link_libraries(ipc_server PRIVATE aux_util aux_util_process aux_util_debug_gui ipc_shared)

if(XRT_HAVE_SYSTEMD)
	target_include_directories(ipc_server PRIVATE ${SYSTEMD_INCLUDE_DIRS})
	target_link_libraries(ipc_server PRIVATE ${SYSTEMD_LIBRARIES})
endif()

if(ANDROID)
	add_library(
		ipc_android STATIC
		android/ipc_client_android.cpp
		android/ipc_client_android.h
		android/org.freedesktop.monado.ipc.cpp
		android/org.freedesktop.monado.ipc.hpp
		android/org.freedesktop.monado.ipc.impl.hpp
		)
	target_link_libraries(
		ipc_android PUBLIC xrt-external-jni-wrap xrt-external-jnipp aux_android
		)
	target_sources(
		ipc_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/server/ipc_server_mainloop_android.c
		)
	target_link_libraries(
		ipc_shared
		PUBLIC ${ANDROID_LIBRARY}
		PRIVATE aux_android ipc_android
		)
elseif(XRT_HAVE_LINUX)
	target_sources(
		ipc_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/server/ipc_server_mainloop_linux.c
		)
elseif(WIN32)
	target_sources(
		ipc_server
		PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/server/ipc_server_mainloop_windows.cpp
		)

	target_link_libraries(ipc_server PRIVATE winmm)
endif()
