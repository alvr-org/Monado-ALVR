// Copyright 2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Testing UV to tangent values.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 */

#include "catch/catch.hpp"

#include "math/m_mathinclude.h"
#include "render/render_interface.h"


#define QUARTER_PI (M_PI / 4)
#define SIXTH_PI (M_PI / 6)
#define MARGIN (0.000001)

static inline xrt_normalized_rect
rect(float x, float y, float w, float h)
{
	xrt_normalized_rect rect = {x, y, w, h};
	return rect;
}

static inline void
check(xrt_normalized_rect &result, xrt_normalized_rect &&truth)
{
	REQUIRE_THAT(result.x, Catch::WithinAbs(truth.x, MARGIN));
	REQUIRE_THAT(result.y, Catch::WithinAbs(truth.y, MARGIN));
	REQUIRE_THAT(result.w, Catch::WithinAbs(truth.w, MARGIN));
	REQUIRE_THAT(result.h, Catch::WithinAbs(truth.h, MARGIN));
}

#define CAPUTER_FOV(FOV) CAPTURE(FOV.angle_up, FOV.angle_down, FOV.angle_left, FOV.angle_right);


TEST_CASE("render_calc_uv_to_tangent_lengths_rect")
{
	// Sanity check.
	REQUIRE_THAT(tan(QUARTER_PI), Catch::WithinAbs(1.0, MARGIN));

	SECTION("45_degrees")
	{
		struct xrt_fov f45 = XRT_STRUCT_INIT;
		f45.angle_down = -QUARTER_PI;
		f45.angle_up = QUARTER_PI;
		f45.angle_left = -QUARTER_PI;
		f45.angle_right = QUARTER_PI;

		SECTION("normal")
		{
			CAPTURE(f45.angle_up, f45.angle_down, f45.angle_left, f45.angle_right);
			struct xrt_normalized_rect result;
			render_calc_uv_to_tangent_lengths_rect(&f45, &result);

			/*
			 * We expect a fov of 45 degrees in all angles to have tangets
			 * of 1 or -1. In order to transform uv [0 .. 1] to [-1 .. 1]
			 * we need to have a width of 2 and a offset of -1.
			 */
			check(result, rect(-1, -1, 2, 2));
		}

		SECTION("flipped_vertical")
		{
			f45.angle_down = -f45.angle_down;
			f45.angle_up = -f45.angle_up;

			CAPUTER_FOV(f45);
			struct xrt_normalized_rect result;
			render_calc_uv_to_tangent_lengths_rect(&f45, &result);

			// We expect the same values as normal but with y and h negated.
			check(result, rect(-1, 1, 2, -2));
		}

		SECTION("flipped_horizontal")
		{
			f45.angle_left = -f45.angle_left;
			f45.angle_right = -f45.angle_right;

			CAPUTER_FOV(f45);
			struct xrt_normalized_rect result;
			render_calc_uv_to_tangent_lengths_rect(&f45, &result);

			// We expect the same values as normal but with x and w negated.
			check(result, rect(1, -1, -2, 2));
		}
	}


	SECTION("30_degrees")
	{
		struct xrt_fov f30 = XRT_STRUCT_INIT;
		f30.angle_down = -SIXTH_PI;
		f30.angle_up = SIXTH_PI;
		f30.angle_left = -SIXTH_PI;
		f30.angle_right = SIXTH_PI;

		CAPUTER_FOV(f30);
		struct xrt_normalized_rect result;
		render_calc_uv_to_tangent_lengths_rect(&f30, &result);

		float t = tan(SIXTH_PI);
		float t2 = t * 2;

		// We expect the offset to be -tan(PI/6) and the lengths to be tan(PI/6) * 2.
		check(result, rect(-t, -t, t2, t2));
	}
}
