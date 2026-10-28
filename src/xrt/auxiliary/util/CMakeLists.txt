# Copyright 2019-2024, Collabora, Ltd.
# Copyright 2024-2026, NVIDIA CORPORATION.
# SPDX-License-Identifier: BSL-1.0

####
# Main util library
#
# Avoid linking with libraries that brings in DSOs as this
# library is used everywhere. Including libraries loaded in
# by applications like the OpenXR runtime library.
#

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/u_git_tag.c.in" "${CMAKE_CURRENT_BINARY_DIR}/u_git_tag.c"
	@ONLY
	)

add_library(
	aux_util STATIC
	u_autoexpgain.c
	u_autoexpgain.h
	u_bitwise.c
	u_bitwise.h
	u_body_tracker.c
	u_body_tracker.h
	u_body_tracker_ni.c
	u_body_tracker_ni.h
	u_builder_helpers.c
	u_builder_helpers.h
	u_builder_search.c
	u_builder_search.h
	u_cobs.c
	u_cobs.h
	u_debug.c
	u_debug.h
	u_deque.cpp
	u_deque.h
	u_device.c
	u_device.h
	u_device_id.c
	u_device_id.h
	u_device_ni.c
	u_device_ni.h
	u_distortion.c
	u_distortion.h
	u_distortion_mesh.c
	u_distortion_mesh.h
	u_distortion_mesh_rift.c
	u_documentation.h
	u_file.c
	u_file.cpp
	u_file.h
	u_format.c
	u_format.h
	u_frame.c
	u_frame.h
	u_frameserver.c
	u_frameserver.h
	u_frameserver_ni.c
	u_frameserver_ni.h
	u_future.h
	u_future.cpp
	u_generic_callbacks.hpp
	u_git_tag.h
	u_hand_tracking.c
	u_hand_tracking.h
	u_hand_simulation.c
	u_hand_simulation.h
	u_handles.c
	u_handles.h
	u_hashmap.cpp
	u_hashmap.h
	u_hashset.cpp
	u_hashset.h
	u_id_ringbuffer.cpp
	u_id_ringbuffer.h
	u_imu_sink_split.c
	u_imu_sink_force_monotonic.c
	u_json.c
	u_json.h
	u_json.hpp
	u_limited_unique_id.cpp
	u_limited_unique_id.h
	u_live_stats.cpp
	u_live_stats.h
	u_logging.c
	u_logging.h
	u_metrics.c
	u_metrics.h
	u_misc.c
	u_misc.h
	u_native_images.h
	u_native_images_debug.h
	u_pacing.h
	u_pacing_app.c
	u_pacing_compositor.c
	u_pacing_compositor_fake.c
	u_pretty_print.c
	u_pretty_print.h
	u_prober.c
	u_prober.h
	u_resampler.c
	u_resampler.h
	u_str_to_enum.c
	u_str_to_enum.h
	u_extension_list.cpp
	u_extension_list.h
	u_extension_list.hpp
	u_system_devices.c
	u_system_devices.h
	u_system_devices_ni.c
	u_system_devices_ni.h
	u_system_helpers.c
	u_system_helpers.h
	u_template_historybuf.hpp
	u_thread_priority.c
	u_thread_priority.h
	u_time.cpp
	u_time.h
	u_trace_marker.c
	u_trace_marker.h
	u_tracked_imu_3dof.c
	u_tracked_imu_3dof.h
	u_var.cpp
	u_var.h
	u_vector.cpp
	u_vector.h
	u_config_json.c
	u_config_json.h
	u_verify.h
	u_visibility_mask.c
	u_visibility_mask.h
	u_win32_com_guard.cpp
	u_win32_com_guard.hpp
	u_weak_ptr.hpp
	u_worker.c
	u_worker.cpp
	u_worker.h
	u_worker.hpp
	"${CMAKE_CURRENT_BINARY_DIR}/u_git_tag.c"
	)
target_link_libraries(
	aux_util
	PUBLIC
		xrt-external-metrics
		xrt-external-bcdec
		aux-includes
		aux_os
		aux_math
	)

# Only uses normal Windows libraries, doesn't add anything extra.
if(WIN32)
	target_sources(aux_util PRIVATE u_windows.c u_windows.h)
endif()

# WIL is lacking official support on MINGW.
# If necessary, please refer to https://github.com/microsoft/wil/pull/467
if(WIN32 AND NOT MINGW)
	target_link_libraries(aux_util PRIVATE WIL::WIL)
endif()

# Only uses POSIX/Linux libraries, doesn't add anything extra.
if(XRT_HAVE_LINUX OR ANDROID)
	target_sources(aux_util PRIVATE u_linux.c u_linux.h)
endif()

# Only uses Android NDK libraries, doesn't add anything extra.
if(ANDROID)
	target_sources(aux_util PRIVATE u_android.c u_android.h)
endif()

# Is basically used everywhere, unavoidable.
if(XRT_HAVE_SYSTEM_CJSON)
	target_link_libraries(aux_util PUBLIC cJSON::cJSON)
	target_compile_definitions(aux_util PRIVATE XRT_HAVE_SYSTEM_CJSON)
else()
	target_link_libraries(aux_util PUBLIC xrt-external-cjson)
endif()

# Is basically used everywhere, only used in debugging.
if(XRT_FEATURE_TRACING AND XRT_HAVE_PERCETTO)
	target_link_libraries(aux_util PUBLIC percetto::percetto)
endif()

# Is basically used everywhere, only used in debugging.
if(XRT_FEATURE_TRACING AND XRT_HAVE_TRACY)
	target_link_libraries(aux_util PUBLIC xrt-external-tracy)
endif()

# Is basically used everywhere, so link with here.
if(ANDROID)
	target_link_libraries(aux_util PUBLIC ${ANDROID_LOG_LIBRARY})
endif()

####
# Debug UI library
#

add_library(aux_util_debug_gui STATIC u_debug_gui.c u_debug_gui.h) # Always built, for stubs.
target_link_libraries(
	aux_util_debug_gui
	INTERFACE xrt-interfaces
	PRIVATE aux-includes
	)

if(XRT_FEATURE_DEBUG_GUI)
	target_link_libraries(aux_util_debug_gui PRIVATE aux_util)

	if(XRT_HAVE_OPENGL)
		target_link_libraries(aux_util_debug_gui PUBLIC aux_ogl)
	endif()

	if(XRT_HAVE_SDL2)
		target_link_libraries(
			aux_util_debug_gui PRIVATE st_gui_base xrt-external-imgui-sdl2
			)

		if(XRT_BUILD_DRIVER_QWERTY)
			target_link_libraries(
				aux_util_debug_gui PRIVATE drv_qwerty drv_qwerty_includes
				)
		endif()
	endif()
endif()

####
# Sink library
#

add_library(
	aux_util_sink STATIC
	u_sink.h
	u_sink_blob_visualizer.c
	u_sink_combiner.c
	u_sink_force_genlock.c
	u_sink_converter.c
	u_sink_deinterleaver.c
	u_sink_queue.c
	u_sink_simple_queue.c
	u_sink_quirk.c
	u_sink_split.c
	u_sink_stereo_sbs_split.c
	)
target_link_libraries(
	aux_util_sink
	PUBLIC
		aux-includes
		xrt-pthreads
		aux_os
		aux_math
		aux_util
	)

if(XRT_HAVE_JPEG)
	target_link_libraries(aux_util_sink PRIVATE ${JPEG_LIBRARIES})
	target_include_directories(aux_util_sink PRIVATE ${JPEG_INCLUDE_DIRS})
endif()

####
# Process library
#

add_library(aux_util_process STATIC u_process.c u_process.h)
target_link_libraries(aux_util_process PUBLIC aux-includes aux_util)

if(XRT_HAVE_LIBBSD)
	target_include_directories(aux_util_process SYSTEM PRIVATE ${LIBBSD_INCLUDE_DIRS})
	target_link_libraries(aux_util_process PUBLIC ${LIBBSD_LIBRARIES})
endif()

####
# Scribble library
#

add_library(aux_util_scribble STATIC u_frame_scribble.cpp u_frame_scribble.h)
target_link_libraries(aux_util_scribble PUBLIC aux-includes aux_util)

if(XRT_HAVE_OPENCV)
	target_link_libraries(aux_util_scribble PUBLIC ${OpenCV_LIBRARIES})
	target_include_directories(aux_util_scribble SYSTEM PRIVATE ${OpenCV_INCLUDE_DIRS})
endif()
