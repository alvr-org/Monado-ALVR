// Copyright 2021-2023, Collabora Ltd.
// Copyright 2026, Beyley Cardellio
// Author: Jakob Bornecrantz <jakob@collabora.com>
// Author: Beyley Cardellio <ep1cm1n10n123@gmail.com>
// SPDX-License-Identifier: BSL-1.0

#extension GL_GOOGLE_include_directive : require

#include "srgb.inc.glsl"

#ifndef BLIT_SOURCE_MULTISAMPLED
#error "BLIT_SOURCE_MULTISAMPLED must be defined to either 0 or 1"
#endif

const int COLOR_TRANSFORM_MODE_NONE = 0;
const int COLOR_TRANSFORM_MODE_GAMMA_IN_LINEAR_FORMAT = 1;
const int COLOR_TRANSFORM_MODE_LINEAR_IN_SRGB_FORMAT = 2;

layout(constant_id = 0) const int k_color_transform_mode = COLOR_TRANSFORM_MODE_NONE;

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

#if BLIT_SOURCE_MULTISAMPLED
layout(set = 0, binding = 0) uniform sampler2DMS source;
#else
layout(set = 0, binding = 0) uniform sampler2D source;
#endif

layout(set = 0, binding = 1, rgba8) uniform writeonly restrict image2D target;
layout(push_constant) uniform Config
{
	vec4 src_rect;
	ivec4 target_view;
} push;

vec2 position_to_uv(ivec2 extent, uint ix, uint iy)
{
	// Turn the index into floating point.
	vec2 xy = vec2(float(ix), float(iy));

	// The inverse of the extent of the target image is the pixel size in [0 .. 1] space.
	vec2 extent_pixel_size = vec2(1.0 / float(extent.x), 1.0 / float(extent.y));

	// Per-target pixel we move the size of the pixels.
	vec2 uv = xy * extent_pixel_size;

	// Emulate a triangle sample position by offset half target pixel size.
	uv = uv + extent_pixel_size / 2.0;

	// Transform with the normalized sample rect.
	uv = push.src_rect.xy + (uv * push.src_rect.zw);

	return uv;
}

vec4 apply_pre_resolve_color_transform(vec4 rgba)
{
	if (k_color_transform_mode == COLOR_TRANSFORM_MODE_GAMMA_IN_LINEAR_FORMAT) {
		return vec4(from_srgb_to_linear(rgba.rgb), rgba.a);
	}

	if (k_color_transform_mode == COLOR_TRANSFORM_MODE_LINEAR_IN_SRGB_FORMAT) {
		return vec4(from_linear_to_srgb(rgba.rgb), rgba.a);
	}

	return rgba;
}

vec4 sample_source(vec2 uv)
{
#if BLIT_SOURCE_MULTISAMPLED
	ivec2 src_extent = textureSize(source);
	ivec2 texel = ivec2(uv * vec2(src_extent));
	texel = clamp(texel, ivec2(0), src_extent - ivec2(1));

	int sample_count = textureSamples(source);
	vec4 sum = vec4(0.0);
	for (int i = 0; i < sample_count; i++) {
		sum += apply_pre_resolve_color_transform(texelFetch(source, texel, i));
	}

	return sum / float(sample_count);
#else
	return apply_pre_resolve_color_transform(texture(source, uv));
#endif
}

void blit()
{
	uint ix = gl_GlobalInvocationID.x;
	uint iy = gl_GlobalInvocationID.y;

	ivec2 offset = ivec2(push.target_view.xy);
	ivec2 extent = ivec2(push.target_view.zw);

	if (ix >= extent.x || iy >= extent.y) {
		return;
	}

	// Get the UV we should sample from.
	vec2 uv = position_to_uv(extent, ix, iy);

	// Do the sample.
	vec4 rgba = sample_source(uv);

	if (k_color_transform_mode == COLOR_TRANSFORM_MODE_GAMMA_IN_LINEAR_FORMAT) {
		rgba = vec4(from_linear_to_srgb(rgba.rgb), rgba.a);
	}

	// And finally write out.
	imageStore(target, ivec2(offset.x + ix, offset.y + iy), rgba);
}
