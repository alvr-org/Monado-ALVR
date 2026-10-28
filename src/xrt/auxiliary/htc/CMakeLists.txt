# Copyright 2026 Beyley Cardellio
# SPDX-License-Identifier: BSL-1.0

add_library(aux_htc STATIC htc_config.cpp htc_config.h)
target_link_libraries(aux_htc PRIVATE aux_util aux_math xrt-external-cjson)
