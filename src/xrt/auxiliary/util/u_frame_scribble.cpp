// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  @ref xrt_frame helpers for scribbling onto frames.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup aux_util
 */

#include "xrt/xrt_frame.h"

#include "xrt/xrt_config_have.h"

#include "u_frame_scribble.h"

#ifdef XRT_HAVE_OPENCV
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#endif


#ifdef XRT_HAVE_OPENCV
static bool
mat_from_frame(const struct xrt_frame *frame, cv::Mat &out_mat)
{
	int type = CV_8UC1; // Default to single channel

	switch (frame->format) {
	case XRT_FORMAT_R8G8B8A8:
	case XRT_FORMAT_R8G8B8X8:
		type = CV_8UC4; // Treat as RGBA, it should be fine
		break;
	case XRT_FORMAT_R8G8B8: type = CV_8UC3; break;
	case XRT_FORMAT_R8G8: type = CV_8UC2; break;
	case XRT_FORMAT_L8:
	case XRT_FORMAT_R8: type = CV_8UC1; break;
	default:
		// Unsupported format, return an empty Mat
		return false;
	}

	out_mat = cv::Mat(frame->height, frame->width, type, frame->data, frame->stride);
	return true;
}

static cv::Scalar
scalar_from_rgba_u8(const struct xrt_colour_rgba_u8 *color)
{
	return cv::Scalar(color->r, color->g, color->b, color->a);
}

static double
pixel_size_to_text_scale(float pixel_size, int font_face, int thickness)
{
	double font_scale = 100.;

	int test_thickness = 1;

	int baseline;
	auto size = cv::getTextSize("X", font_face, font_scale, test_thickness, &baseline);

	// Scale the size to match the desired pixel size
	return (pixel_size - thickness) / ((size.height - test_thickness) / font_scale);
}
#endif

void
u_frame_scribble_rect(struct xrt_frame *frame,
                      const struct xrt_rect *rect,
                      bool fill,
                      const struct xrt_colour_rgba_u8 *color)
{
#ifdef XRT_HAVE_OPENCV
	cv::Mat frame_mat;
	if (!mat_from_frame(frame, frame_mat)) {
		return;
	}

	cv::Scalar cv_color = scalar_from_rgba_u8(color);
	cv::Rect cv_rect(rect->offset.w, rect->offset.h, rect->extent.w, rect->extent.h);

	int thickness = fill ? cv::FILLED : 1;

	cv::rectangle(frame_mat, cv_rect, cv_color, thickness);
#else
	(void)frame;
	(void)rect;
	(void)fill;
	(void)color;
#endif
}

void
u_frame_scribble_cross(
    struct xrt_frame *frame, uint32_t x, uint32_t y, uint32_t size, const struct xrt_colour_rgba_u8 *color)
{
	u_frame_scribble_line(frame, x - size, y - size, x + size, y + size, color);
	u_frame_scribble_line(frame, x - size, y + size, x + size, y - size, color);
}

void
u_frame_scribble_line(
    struct xrt_frame *frame, uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, const struct xrt_colour_rgba_u8 *color)
{
#ifdef XRT_HAVE_OPENCV
	cv::Mat frame_mat;
	if (!mat_from_frame(frame, frame_mat)) {
		return;
	}

	cv::Scalar cv_color = scalar_from_rgba_u8(color);
	cv::line(frame_mat, cv::Point(x0, y0), cv::Point(x1, y1), cv_color);
#else
	(void)frame;
	(void)x0;
	(void)y0;
	(void)x1;
	(void)y1;
	(void)color;
#endif
}

void
u_frame_scribble_text(struct xrt_frame *frame,
                      uint32_t x,
                      uint32_t y,
                      const char *text,
                      float size,
                      const struct xrt_colour_rgba_u8 *color)
{
#ifdef XRT_HAVE_OPENCV
	cv::Mat frame_mat;
	if (!mat_from_frame(frame, frame_mat)) {
		return;
	}

	const int font_face = cv::FONT_HERSHEY_PLAIN;
	const int thickness = 1;

	cv::Scalar cv_color = scalar_from_rgba_u8(color);

	double font_scale = pixel_size_to_text_scale(size, font_face, thickness);

	cv::putText(frame_mat, text, cv::Point(x, y), font_face, font_scale, cv_color, thickness);
#else
	(void)frame;
	(void)x;
	(void)y;
	(void)text;
	(void)size;
	(void)color;
#endif
}
