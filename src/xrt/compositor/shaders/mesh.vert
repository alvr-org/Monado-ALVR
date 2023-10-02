// Copyright 2019-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
// Author: Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
// Author: Pete Black <pete.black@collabora.com>
// Author: Jakob Bornecrantz <jakob@collabora.com>

#version 450

// Should we do timewarp.
layout(constant_id = 0) const bool do_timewarp = false;

layout (binding = 1, std140) uniform Config
{
	vec4 vertex_rot;
	vec4 post_transform;

	// Only used for timewarp.
	vec4 pre_transform;
	mat4 transform;
} ubo;

layout (location = 0)  in vec4 in_pos_ruv;
layout (location = 1)  in vec4 in_guv_buv;
layout (location = 0) out vec2 out_r_uv;
layout (location = 1) out vec2 out_g_uv;
layout (location = 2) out vec2 out_b_uv;

out gl_PerVertex
{
	vec4 gl_Position;
};


vec2 transform_uv_subimage(vec2 uv)
{
	vec2 values = uv;

	// To deal with OpenGL flip and sub image view.
	values.xy = fma(values.xy, ubo.post_transform.zw, ubo.post_transform.xy);

	// Ready to be used.
	return values.xy;
}

vec2 transform_uv_timewarp(vec2 uv)
{
	vec4 values = vec4(uv, -1, 1);

	// From uv to tan angle (tangent space).
	values.xy = fma(values.xy, ubo.pre_transform.zw, ubo.pre_transform.xy);
	values.y = -values.y; // Flip to OpenXR coordinate system.

	// Timewarp.
	values = ubo.transform * values;
	values.xy = values.xy * (1.0 / max(values.w, 0.00001));

	// From [-1, 1] to [0, 1]
	values.xy = values.xy * 0.5 + 0.5;

	// To deal with OpenGL flip and sub image view.
	values.xy = fma(values.xy, ubo.post_transform.zw, ubo.post_transform.xy);

	// Done.
	return values.xy;
}

vec2 transform_uv(vec2 uv)
{
	if (do_timewarp) {
		return transform_uv_timewarp(uv);
	} else {
		return transform_uv_subimage(uv);
	}
}

void main()
{
	mat2x2 rot = {
		ubo.vertex_rot.xy,
		ubo.vertex_rot.zw,
	};

	vec2 pos = rot * in_pos_ruv.xy;
	gl_Position = vec4(pos, 0.0f, 1.0f);

	vec2 r_uv = in_pos_ruv.zw;
	vec2 g_uv = in_guv_buv.xy;
	vec2 b_uv = in_guv_buv.zw;

	r_uv = transform_uv(r_uv);
	g_uv = transform_uv(g_uv);
	b_uv = transform_uv(b_uv);

	out_r_uv = r_uv;
	out_g_uv = g_uv;
	out_b_uv = b_uv;
}
