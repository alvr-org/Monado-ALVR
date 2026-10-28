# Copyright 2022, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0

add_library(tracking-includes INTERFACE)
target_include_directories(
	tracking-includes INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
	)

if(XRT_MODULE_MERCURY_HANDTRACKING)
	add_subdirectory(hand)
endif()

if(XRT_MODULE_CONSTELLATION_TRACKING)
	add_subdirectory(constellation)
endif()
