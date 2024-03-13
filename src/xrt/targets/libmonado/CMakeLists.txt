# Copyright 2019-2024, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0

add_library(monado SHARED monado.c libmonado.def)
set(LIBMONADO_HEADER_DIR ${CMAKE_INSTALL_INCLUDEDIR}/monado)
target_link_libraries(monado PRIVATE aux_util ipc_client)
target_include_directories(
	monado INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
			 $<INSTALL_INTERFACE:${LIBMONADO_HEADER_DIR}>
	)

set_target_properties(
	monado PROPERTIES VERSION ${CMAKE_PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR}
	)

install(TARGETS monado RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES monado.h DESTINATION ${LIBMONADO_HEADER_DIR})

add_executable(libmonado-example example.c)
target_link_libraries(libmonado-example PRIVATE monado)
