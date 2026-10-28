# Copyright 2019, Collabora, Ltd.
# Copyright 2026, Beyley Cardellio
# SPDX-License-Identifier: BSL-1.0

if(XRT_HAVE_OPENGL)
	add_subdirectory(gui)
endif()

if(XRT_MODULE_OPENXR_STATE_TRACKER)
	add_subdirectory(oxr)
endif()

if(XRT_MODULE_OPENVR_STATE_TRACKER)
	add_subdirectory(openvr)
endif()

add_subdirectory(prober)

if(XRT_FEATURE_STEAMVR_PLUGIN)
	add_subdirectory(steamvr_drv)
endif()
