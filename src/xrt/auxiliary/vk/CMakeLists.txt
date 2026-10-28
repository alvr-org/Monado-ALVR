# Copyright 2019-2023, Collabora, Ltd.
# Copyright 2025-2026, NVIDIA CORPORATION.
# SPDX-License-Identifier: BSL-1.0

###
# Generate Vulkan helpers
#

set(VK_GENERATED_FILES
    vk_helpers_h_funcs.h.inc
    vk_helpers_h_ext.h.inc
    vk_bundle_init_instance_ext.c.inc
    vk_bundle_init_device_ext.c.inc
    vk_function_loaders_instance.c.inc
    vk_function_loaders_device.c.inc
	)

set(VK_GENERATED_OUTPUTS)
foreach(fn ${VK_GENERATED_FILES})
	set(OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/${fn}")
	list(APPEND VK_GENERATED_OUTPUTS "${OUTPUT_FILE}")

	add_custom_command(
		OUTPUT "${OUTPUT_FILE}"
		COMMAND
			${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/vk_generate_inc_files.py
			"${OUTPUT_FILE}"
		VERBATIM
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/vk_generate_inc_files.py
		COMMENT "Generating ${fn}"
		)
endforeach()

###
# Library
#

add_library(
	aux_vk STATIC
	vk_bundle_init.c
	vk_cmd.c
	vk_cmd.h
	vk_cmd_pool.c
	vk_cmd_pool.h
	vk_compositor_flags.c
	vk_compositor_flags.h
	vk_debug.c
	vk_documentation.h
	vk_enumerate.c
	vk_extensions_helpers.c
	vk_extensions_helpers.h
	vk_format.c
	vk_format.h
	vk_function_loaders.c
	vk_helpers.c
	vk_helpers.h
	vk_image_allocator.c
	vk_image_allocator.h
	vk_image_readback_to_xf_pool.c
	vk_image_readback_to_xf_pool.h
	vk_mini_helpers.h
	vk_queue_builder.c
	vk_queue_builder.h
	vk_queue_family.c
	vk_queue_family.h
	vk_print.c
	vk_print.h
	vk_state_creators.c
	vk_submit_helpers.c
	vk_submit_helpers.h
	vk_surface_info.c
	vk_surface_info.h
	vk_sync_objects.c
	vk_time.c
	${VK_GENERATED_OUTPUTS}
	)
target_include_directories(aux_vk PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(aux_vk PUBLIC xrt-external-vulkan-headers aux_os aux_util)
target_link_libraries(aux_vk PRIVATE xrt-external-vulkan-loader)

if(ANDROID)
	target_link_libraries(aux_vk PUBLIC aux_android)
	target_link_libraries(aux_vk PUBLIC ${ANDROID_LIBRARY})
endif()

if(XRT_HAVE_WAYLAND)
	target_include_directories(aux_vk PRIVATE ${WAYLAND_INCLUDE_DIR})
endif()
if(XRT_HAVE_XCB)
	target_include_directories(aux_vk PRIVATE ${XCB_INCLUDE_DIR})
endif()
if(XRT_HAVE_XRANDR)
	target_include_directories(aux_vk PRIVATE ${XRANDR_INCLUDE_DIR} ${X11_INCLUDE_DIR})
endif()
