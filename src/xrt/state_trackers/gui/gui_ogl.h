// Copyright 2019-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  OpenGL helper functions for drawing GUI elements.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup gui
 */

#pragma once

#include "xrt/xrt_compiler.h"


#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Draw the given textures as igImage, scale of 1.0f == 100%.
 *
 * @ingroup gui
 */
void
gui_ogl_draw_image(uint32_t width, uint32_t height, uint32_t tex_id, float scale, bool rotate_180, bool flip_y);


/*!
 * Draw the given texture to the background of the current OS window.
 *
 * @ingroup gui
 */
void
gui_ogl_draw_background(uint32_t width, uint32_t height, uint32_t tex_id, bool rotate_180, bool flip_y);


#ifdef __cplusplus
}
#endif
