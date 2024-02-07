// Copyright 2020, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Code to handle distortion parameters and fov.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_distortion
 */

#include "xrt/xrt_device.h"

#include "math/m_mathinclude.h"

#include "util/u_misc.h"
#include "util/u_device.h"
#include "util/u_distortion.h"


void
u_distortion_cardboard_calculate(const struct u_cardboard_distortion_arguments *args,
                                 struct xrt_hmd_parts *parts,
                                 struct u_cardboard_distortion *out_dist)
{
	/*
	 * HMD parts
	 */

	uint32_t view_count = parts->view_count;

	uint32_t w_pixels = args->screen.w_pixels / view_count;
	uint32_t h_pixels = args->screen.h_pixels;

	// Base assumption, the driver can change afterwards.
	if (parts->blend_mode_count == 0) {
		size_t idx = 0;
		parts->blend_modes[idx++] = XRT_BLEND_MODE_OPAQUE;
		parts->blend_mode_count = idx;
	}

	// Use the full screen.
	parts->screens[0].w_pixels = args->screen.w_pixels;
	parts->screens[0].h_pixels = args->screen.h_pixels;

	// Copy the arguments.
	out_dist->args = *args;

	// Save the results.
	for (uint32_t i = 0; i < view_count; ++i) {
		parts->views[i].viewport.x_pixels = 0 + i * w_pixels;
		parts->views[i].viewport.y_pixels = 0;
		parts->views[i].viewport.w_pixels = w_pixels;
		parts->views[i].viewport.h_pixels = h_pixels;
		parts->views[i].display.w_pixels = w_pixels;
		parts->views[i].display.h_pixels = h_pixels;
		parts->views[i].rot = u_device_rotation_ident;
		parts->distortion.fov[i] = args->fov;

		struct u_cardboard_distortion_values *values = &out_dist->values[i];
		values->distortion_k[0] = args->distortion_k[0];
		values->distortion_k[1] = args->distortion_k[1];
		values->distortion_k[2] = args->distortion_k[2];
		values->distortion_k[3] = args->distortion_k[3];
		values->distortion_k[4] = args->distortion_k[4];
		values->screen.size.x = args->screen.w_meters;
		values->screen.size.y = args->screen.h_meters;
		values->screen.offset.x =
		    (args->screen.w_meters + pow(-1, i + 1) * args->inter_lens_distance_meters) / view_count;
		values->screen.offset.y = args->lens_y_center_on_screen_meters;
		// clang-format on

		// Turn into tanangles
		values->screen.size.x /= args->screen_to_lens_distance_meters;
		values->screen.size.y /= args->screen_to_lens_distance_meters;
		values->screen.offset.x /= args->screen_to_lens_distance_meters;
		values->screen.offset.y /= args->screen_to_lens_distance_meters;

		// Tanangle to texture coordinates
		values->texture.size.x = tanf(-args->fov.angle_left) + tanf(args->fov.angle_right);
		values->texture.size.y = tanf(args->fov.angle_up) + tanf(-args->fov.angle_down);
		values->texture.offset.x = tanf(-args->fov.angle_left);
		values->texture.offset.y = tanf(-args->fov.angle_down);

		// Fix up views not covering the entire screen.
		values->screen.size.x /= view_count;
		values->screen.offset.x -= values->screen.size.x * i;
	}
}
