// Copyright 2023, Collabora Ltd.
// Author: Jakob Bornecrantz <jakob@collabora.com>
// Author: Charlton Rodda <charlton.rodda@collabora.com>
// Author: Simon Zeni <simon.zeni@collabora.com>
// SPDX-License-Identifier: BSL-1.0

#version 460

layout (binding = 0, std140) uniform Config
{
	vec4 post_transform;
	mat4 mv_inverse;
	vec4 to_tangent;
	float radius;
	float central_horizontal_angle;
	float upper_vertical_angle;
	float lower_vertical_angle;
} ubo;

layout (location = 0) flat out vec3 out_camera_position;
layout (location = 1)      out vec3 out_camera_ray_unnormalized;

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

vec3 intersection_with_unit_plane(vec2 uv_0_to_1)
{
	// [0 .. 1] to tangent lengths (at unit Z).
	vec2 tangent_factors = fma(uv_0_to_1, ubo.to_tangent.zw, ubo.to_tangent.xy);

	// With Z at the unit plane and flip y for OpenXR coordinate system.
	vec3 point_on_unit_plane = vec3(tangent_factors.x, -tangent_factors.y, -1);

	return point_on_unit_plane;
}

void main()
{
	vec2 uv = pos[gl_VertexIndex % 4];

	// Get camera position in model space.
	out_camera_position = (ubo.mv_inverse * vec4(0, 0, 0, 1)).xyz;

	// Get the point on the Z=-1 plane in view space that this pixel's ray intersects.
	vec3 out_camera_ray_in_view_space = intersection_with_unit_plane(uv);

	// Transform into model space. Normalising here doesn't work because
	// the values are interpeted linearly in space, where a normal
	// doesn't move linearly in space, so do that in the fragment shader.
	out_camera_ray_unnormalized = (ubo.mv_inverse * vec4(out_camera_ray_in_view_space, 0)).xyz;

	// Go from [0 .. 1] to [-1 .. 1] to fill whole view in NDC.
	vec2 pos = fma(uv, vec2(2.0), vec2(-1.0));

	gl_Position = vec4(pos, 0.f, 1.f);
}
