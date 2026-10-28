# Copyright 2019-2025, Collabora, Ltd.
# Copyright 2026, NVIDIA CORPORATION.
# SPDX-License-Identifier: BSL-1.0

# needed globally for steamvr input profile generation in steamvr target
set_property(GLOBAL PROPERTY AUX_BINDINGS_DIR_PROP "${CMAKE_CURRENT_SOURCE_DIR}")

set(AUX_BINDINGS_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}"
    CACHE PATH "Path to the shared bindings generation code"
	)
