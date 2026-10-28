# Copyright 2019-2020, Collabora, Ltd.
# Copyright 2026, NVIDIA CORPORATION.
# Copyright 2026, Beyley Cardellio
# SPDX-License-Identifier: BSL-1.0

######
# This is where we collect all of the pieces from the different parts of
# the source tree and build a complete driver or integration part.

add_subdirectory(helpers)
add_subdirectory(common)

# Monado management library
if(XRT_FEATURE_SERVICE AND XRT_HAVE_LINUX)
	add_subdirectory(libmonado)
endif()

if(XRT_FEATURE_OPENXR)
	add_subdirectory(openxr)
	add_subdirectory(binding_inspector)
endif()

if(XRT_FEATURE_OPENVR)
	add_subdirectory(openvr)
endif()

if(XRT_MODULE_MONADO_CLI)
	add_subdirectory(cli)
endif()

if(XRT_MODULE_MONADO_GUI)
	add_subdirectory(gui)
endif()

if(XRT_FEATURE_SERVICE AND NOT WIN32)
	add_subdirectory(ctl)
endif()

if(XRT_FEATURE_SERVICE AND XRT_FEATURE_OPENXR)
	if(ANDROID)
		add_subdirectory(service-lib)
	else()
		add_subdirectory(service)
	endif()
endif()

if(XRT_FEATURE_STEAMVR_PLUGIN)
	add_subdirectory(steamvr_drv)
endif()

if(XRT_FEATURE_SERVICE
   AND XRT_HAVE_SDL2
   AND XRT_HAVE_VULKAN
	)
	add_subdirectory(sdl_test)
endif()
