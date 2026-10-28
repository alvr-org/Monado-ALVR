# Copyright 2026, NVIDIA CORPORATION.
# SPDX-License-Identifier: BSL-1.0

add_library(base STATIC)
target_include_directories(base PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(
	base
	PRIVATE aux_util aux_os aux_math
	PUBLIC xrt-interfaces
	)

target_sources(
	base
	PRIVATE
		b_app_policy.c
		b_app_policy.h
		b_documentation.h
		b_body_tracker.cpp
		b_body_tracker.h
		b_body_tracker.hpp
		b_hand_tracker.cpp
		b_hand_tracker.h
		b_hand_tracker.hpp
		b_io_cache.cpp
		b_io_cache.h
		b_io_cache.hpp
		b_session.c
		b_session.h
		b_space_overseer.c
		b_space_overseer.h
		b_tracked_device.c
		b_tracked_device.h
		b_system.c
		b_system.h
		b_system_devices.c
		b_system_devices.h
		b_timing_source.c
		b_timing_source.h
	)
