# Copyright 2025-2026, NVIDIA CORPORATION.
#
# SPDX-License-Identifier: BSL-1.0

# Glue library - header-only interface
add_library(aux_glue INTERFACE)

target_link_libraries(aux_glue INTERFACE xrt-interfaces aux_util)
