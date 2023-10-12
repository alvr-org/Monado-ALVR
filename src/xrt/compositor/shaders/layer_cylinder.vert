// Copyright 2023, Collabora Ltd.
// Author: Jakob Bornecrantz <jakob@collabora.com>
// SPDX-License-Identifier: BSL-1.0

#version 460


// Number of subdivisions.
layout(constant_id = 0) const uint subdivision_count = 360;

layout (binding = 0, std140) uniform Config
{
	vec4 post_transform;
	mat4 mvp;
	float radius;
	float central_angle;
	float aspect_ratio;
	float _pad;
} ubo;

layout (location = 0) out vec2 out_uv;

out gl_PerVertex
{
	vec4 gl_Position;
};


vec2 get_uv_for_vertex()
{
	// One edge on either endstop and one between each subdivision.
	uint edges = subdivision_count + 1;

	// Goes from [0 .. 2^31], two vertices per edge.
	uint x_u32 = bitfieldExtract(uint(gl_VertexIndex), 1, 31);

	// Goes from [0 .. 1] every other vertex.
	uint y_u32 = bitfieldExtract(uint(gl_VertexIndex), 0, 1);

	// Starts at zero to get to [0 .. 1], there is two vertices per edge.
	float x = float(x_u32) / float(edges);

	// Already in [0 .. 1] just transform to float.
	float y = float(y_u32);

	return vec2(x, y);
}

vec3 get_position_for_uv(vec2 uv)
{
	float radius = ubo.radius;
	float angle = ubo.central_angle;
	float ratio = ubo.aspect_ratio;

	// [0 .. 1] to [-0.5 .. 0.5]
	float mixed_u = uv.x - 0.5;

	// [-0.5 .. 0.5] to [-angle / 2 .. angle / 2].
	float a = mixed_u * angle;

	// [0 .. 1] to [0.5 .. -0.5] notice fliped sign to be in OpenXR space.
	float mixed_v = 0.5 - uv.y;

	// This the total height acording to the spec.
	float total_height = (angle * radius) / ratio;

	// Calculate the position.
	float x = sin(a) * radius; // At angle zero at x = 0.
	float y = total_height * mixed_v;
	float z = -cos(a) * radius; // At angle zero at z = -1.

	return vec3(x, y, z);
}

void main()
{
	// We now get a unmodified UV position.
	vec2 raw_uv = get_uv_for_vertex();

	// Get the position for the raw UV.
	vec3 position = get_position_for_uv(raw_uv);

	// To deal with OpenGL flip and sub image view.
	vec2 uv = fma(raw_uv, ubo.post_transform.zw, ubo.post_transform.xy);

	gl_Position = ubo.mvp * vec4(position, 1.0);
	out_uv = uv;
}
