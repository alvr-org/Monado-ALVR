# Copyright 2026, NVIDIA CORPORATION.
# SPDX-License-Identifier: BSL-1.0

cmake_minimum_required(VERSION 3.20)
project(
	minimal_example
	VERSION 1.0.0
	LANGUAGES C CXX
	)

set(MONADO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
include(${MONADO_ROOT}/cmake/MonadoSetOptions.cmake)
monado_set_options(KEEP XRT_FEATURE_SSE2)
add_subdirectory(${MONADO_ROOT} monado)

add_executable(minimal src/main.c)
target_link_libraries(minimal PRIVATE aux_util)
