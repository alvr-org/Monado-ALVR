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

layout (binding = 1) uniform sampler2D image;

layout (location = 0) flat in vec3 in_camera_position;
layout (location = 1)      in vec3 in_camera_ray_unnormalized;
layout (location = 0)     out vec4 out_color;

const float PI = acos(-1);


vec2 sphere_intersect(vec3 ray_origin, vec3 ray_direction, vec3 sphere_center, float radius)
{
	vec3 ray_sphere_diff = ray_origin - sphere_center;

	float B = dot(ray_sphere_diff, ray_direction);

	vec3 QC = ray_sphere_diff - B * ray_direction;

	float H = radius * radius - dot(QC, QC);

	if (H < 0.0) {
		return vec2(-1.0); // no intersection
	}

	H = sqrt(H);

	return vec2(-B - H, -B + H);
}

void main ()
{
	vec3 ray_origin = in_camera_position;
	vec3 ray_dir = normalize(in_camera_ray_unnormalized);

	vec3 dir_from_sph;
	// CPU code will set +INFINITY to zero.
	if (ubo.radius == 0) {
		dir_from_sph = ray_dir;
	} else {
		vec2 distances = sphere_intersect(ray_origin, ray_dir, vec3(0, 0, 0), ubo.radius);

		if (distances.y < 0) {
			out_color = vec4(0.0);
			return;
		}

		vec3 pos = ray_origin + (ray_dir * distances.y);
		dir_from_sph = normalize(pos);
	}

	float lon = atan(dir_from_sph.x, -dir_from_sph.z) / (2 * PI) + 0.5;
	float lat = acos(dir_from_sph.y) / PI;

#ifdef DEBUG
	int lon_int = int(lon * 1000.0);
	int lat_int = int(lat * 1000.0);

	if (lon < 0.001 && lon > -0.001) {
		out_color = vec4(1, 0, 0, 1);
	} else if (lon_int % 50 == 0) {
		out_color = vec4(1, 1, 1, 1);
	} else if (lat_int % 50 == 0) {
		out_color = vec4(1, 1, 1, 1);
	} else {
		out_color = vec4(lon, lat, 0, 1);
	}
#endif

	float chan = ubo.central_horizontal_angle / (PI * 2.0f);

	// Normalize [0, 2π] to [0, 1]
	float uhan = 0.5 + chan / 2.0f;
	float lhan = 0.5 - chan / 2.0f;

	// Normalize [-π/2, π/2] to [0, 1]
	float uvan = ubo.upper_vertical_angle / PI + 0.5f;
	float lvan = ubo.lower_vertical_angle / PI + 0.5f;

	if (lat < uvan && lat > lvan && lon < uhan && lon > lhan) {
		// map configured display region to whole texture
		vec2 ll_offset = vec2(lhan, lvan);
		vec2 ll_extent = vec2(uhan - lhan, uvan - lvan);
		vec2 sample_point = (vec2(lon, lat) - ll_offset) / ll_extent;

		vec2 uv_sub = fma(sample_point, ubo.post_transform.zw, ubo.post_transform.xy);

#ifdef DEBUG
		out_color += texture(image, uv_sub) / 2.0;
#else
		out_color = texture(image, uv_sub);
	} else {
		out_color = vec4(0, 0, 0, 0);
#endif
	}
}
