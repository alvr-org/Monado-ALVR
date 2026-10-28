// Copyright 2021-2026, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Code to deal with bounding boxes for camera-based hand-tracking.
 * @author Moshi Turner <moshiturner@protonmail.com>
 * @author Marcus Edel <marcus.edel@collabora.com>
 * @ingroup aux_util
 */

#pragma once

#include <math.h>
#include <cmath>
#include "xrt/xrt_defines.h"

namespace xrt::auxiliary::util::box_iou {

/// 2D Box defined by center and size (width and height)
struct Box
{
	float cx;
	float cy;
	float w;
	float h;

	// No uninitialized memory!
	Box() : cx(0.0f), cy(0.0f), w(0.0f), h(0.0f) {}
	Box(float cx, float cy, float w, float h) : cx(cx), cy(cy), w(w), h(h) {}
	Box(float cx, float cy, float size) : cx(cx), cy(cy), w(size), h(size) {}
	Box(const xrt_vec2 &center, const float size) : cx(center.x), cy(center.y), w(size), h(size) {}
};

static float
overlap(float x1, float w1, float x2, float w2)
{
	// farthest-right left edge
	float l1 = x1 - w1 / 2;
	float l2 = x2 - w2 / 2;
	float left = std::max(l1, l2);


	// farthest-left right edge
	float r1 = x1 + w1 / 2;
	float r2 = x2 + w2 / 2;
	float right = std::min(r1, r2);

	return right - left;
}

static float
boxIntersection(const Box &a, const Box &b)
{
	float w = overlap(a.cx, a.w, b.cx, b.w);
	float h = overlap(a.cy, a.h, b.cy, b.h);

	if (w < 0 || h < 0)
		return 0;

	return w * h;
}

static float
boxUnion(const Box &a, const Box &b)
{
	return a.w * a.h + b.w * b.h - boxIntersection(a, b);
}

/// Ratio of box intersection area vs box union area (Intersection Over Union)
static float
boxIOU(const Box &a, const Box &b)
{
	return boxIntersection(a, b) / boxUnion(a, b);
}
} // namespace xrt::auxiliary::util::box_iou
