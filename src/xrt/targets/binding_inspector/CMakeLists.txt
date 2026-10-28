# Copyright 2026, NVIDIA CORPORATION.
# SPDX-License-Identifier: BSL-1.0

add_executable(binding_inspector binding_inspector.c)

target_link_libraries(binding_inspector PRIVATE st_oxr aux_util xrt-interfaces xrt-external-openxr)

target_include_directories(
	binding_inspector PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../state_trackers/oxr
				  ${CMAKE_CURRENT_SOURCE_DIR}/../../state_trackers/oxr/actions
	)
