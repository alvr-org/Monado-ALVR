// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  @ref xrt_frame helpers for scribbling onto frames.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup aux_util
 */

#pragma once

#include "xrt/xrt_frame.h"


#ifdef __cplusplus
extern "C" {
#endif


/*!
 * Scribbles a rectangle onto a frame.
 *
 * @param frame The frame to scribble on.
 * @param rect The rectangle to scribble.
 * @param fill Whether to fill the rectangle or just draw the outline.
 * @param color The color to scribble with.
 */
void
u_frame_scribble_rect(struct xrt_frame *frame,
                      const struct xrt_rect *rect,
                      bool fill,
                      const struct xrt_colour_rgba_u8 *color);

/*!
 * Scribbles a cross onto a frame.
 *
 * @param frame The frame to scribble on.
 * @param x The x coordinate of the center of the cross.
 * @param y The y coordinate of the center of the cross.
 * @param size The size of the cross in pixels (from center to edge horizontally).
 * @param color The color to scribble with.
 */
void
u_frame_scribble_cross(
    struct xrt_frame *frame, uint32_t x, uint32_t y, uint32_t size, const struct xrt_colour_rgba_u8 *color);

/*!
 * Scribbles a line onto a frame.
 *
 * @param frame The frame to scribble on.
 * @param x0 The x coordinate of the start of the line.
 * @param y0 The y coordinate of the start of the line.
 * @param x1 The x coordinate of the end of the line.
 * @param y1 The y coordinate of the end of the line.
 * @param color The color to scribble with.
 */
void
u_frame_scribble_line(struct xrt_frame *frame,
                      uint32_t x0,
                      uint32_t y0,
                      uint32_t x1,
                      uint32_t y1,
                      const struct xrt_colour_rgba_u8 *color);

/*!
 * Scribbles text onto a frame.
 *
 * @param frame The frame to scribble on.
 * @param x The x coordinate of the text baseline.
 * @param y The y coordinate of the text baseline.
 * @param text The text to scribble.
 * @param size The desired pixel height of the text.
 * @param color The color to scribble with.
 */
void
u_frame_scribble_text(struct xrt_frame *frame,
                      uint32_t x,
                      uint32_t y,
                      const char *text,
                      float size,
                      const struct xrt_colour_rgba_u8 *color);

#ifdef __cplusplus
}
#endif
