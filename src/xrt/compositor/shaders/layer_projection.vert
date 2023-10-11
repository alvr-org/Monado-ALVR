// Copyright 2023, Collabora Ltd.
// Author: Jakob Bornecrantz <jakob@collabora.com>
// SPDX-License-Identifier: BSL-1.0

#version 460


layout (binding = 0, std140) uniform Config
{
	vec4 post_transform;
	vec4 to_tanget;
	mat4 mvp;
} ubo;

layout (location = 0) out vec2 out_uv;

out gl_PerVertex
{
	vec4 gl_Position;
};

vec2 pos[4] = {
	vec2(0, 0),
	vec2(0, 1),
	vec2(1, 0),
	vec2(1, 1),
};

void main()
{
	// We now get a unmodified UV position.
	vec2 in_uv = pos[gl_VertexIndex % 4];

	// Turn the UV into tanget angle space.
	vec2 pos = fma(in_uv, ubo.to_tanget.zw, ubo.to_tanget.xy);

	// Flip to OpenXR coordinate system.
	pos.y = -pos.y;

	// Do timewarp by moving from a plane that is 1m in front of the
	// origin of the projection layer into the view of the new position.
	vec4 position = ubo.mvp * vec4(pos, -1.0f, 1.0f);

	// To deal with OpenGL flip and sub image view.
	vec2 uv = fma(in_uv, ubo.post_transform.zw, ubo.post_transform.xy);

	gl_Position = position;
	out_uv = uv;
}
