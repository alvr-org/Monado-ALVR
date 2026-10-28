// Copyright 2019-2023, Collabora, Ltd.
// Copyright 2024-2025, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  SDL2 Debug UI implementation
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Moshi Turner <moshiturner@protonmail.com>
 */

#pragma once

#include "xrt/xrt_compiler.h"


#ifdef __cplusplus
extern "C" {
#endif

#define U_DEBUG_GUI_WINDOW_TITLE_MAX (256)

struct xrt_instance;
struct xrt_system_devices;

struct u_debug_gui;

/*!
 * Controls if the debug gui window is opened, allowing code to always call
 * create and progmatically or external control if the window is opened.
 *
 * @ingroup aux_util
 */
enum u_debug_gui_open
{
	//! Opens the window if the environmental variable XRT_DEBUG_GUI is true.
	U_DEBUG_GUI_OPEN_AUTO,
	//! Always (if supported) opens the window.
	U_DEBUG_GUI_OPEN_ALWAYS,
	//! Never opens the window.
	U_DEBUG_GUI_OPEN_NEVER,
};

/*!
 * Argument to the function @ref u_debug_gui_create.
 *
 * @ingroup aux_util
 */
struct u_debug_gui_create_info
{
	char window_title[U_DEBUG_GUI_WINDOW_TITLE_MAX];

	enum u_debug_gui_open open;
};

/*!
 * Creates the debug gui, may not create it.
 *
 * If the debug gui is disabled through the means listed below this function
 * will return 0, but not create any struct and set @p out_debug_gui to NULL.
 * It is safe to call the other functions with a NULL @p debug_gui argument.
 *
 * The window will be disabled and 0 returned if:
 * * Monado was compiled without the needed dependencies, like SDL.
 * * The @p open field on the info struct set to NEVER.
 * * The XRT_DEBUG_GUI env variable is false (or unset).
 *
 * @ingroup aux_util
 */
int
u_debug_gui_create(const struct u_debug_gui_create_info *info, struct u_debug_gui **out_debug_gui);

/*!
 * Starts the debug gui, also passes in some structs that might be needed.
 *
 * @ingroup aux_util
 */
void
u_debug_gui_start(struct u_debug_gui *debug_gui, struct xrt_instance *xinst, struct xrt_system_devices *xsysd);

/*!
 * Stops the debug gui, closing the window and freeing resources.
 *
 * @ingroup aux_util
 */
void
u_debug_gui_stop(struct u_debug_gui **debug_gui);


#ifdef __cplusplus
}
#endif
