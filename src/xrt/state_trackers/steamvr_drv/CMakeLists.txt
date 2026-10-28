# Copyright 2020-2021, Collabora, Ltd.
# Copyright 2026, NVIDIA CORPORATION.
# SPDX-License-Identifier: BSL-1.0

add_subdirectory(steamvr_bindings)

add_library(st_ovrd STATIC ovrd_driver.cpp ovrd_interface.h)

target_include_directories(st_ovrd INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
	st_ovrd PRIVATE xrt-interfaces xrt-external-openvr aux_math ovrd_generated_bindings
	)
