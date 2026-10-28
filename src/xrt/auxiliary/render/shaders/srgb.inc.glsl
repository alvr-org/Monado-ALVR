// Copyright 2021, Collabora Ltd.
// Author: Jakob Bornecrantz <jakob@collabora.com>
// SPDX-License-Identifier: BSL-1.0


float from_linear_to_srgb_channel(float value)
{
	if (value < 0.0031308) {
		return 12.92 * value;
	} else {
		return 1.055 * pow(value, 1.0 / 2.4) - 0.055;
	}
}

vec3 from_linear_to_srgb(vec3 linear_rgb)
{
	return vec3(
		from_linear_to_srgb_channel(linear_rgb.r),
		from_linear_to_srgb_channel(linear_rgb.g),
		from_linear_to_srgb_channel(linear_rgb.b)
	);
}

float from_srgb_to_linear_channel(float value)
{
	if (value <= 0.04045) {
		return value / 12.92;
	} else {
		return pow((value + 0.055) / 1.055, 2.4);
	}
}

vec3 from_srgb_to_linear(vec3 srgb)
{
	return vec3(
		from_srgb_to_linear_channel(srgb.r),
		from_srgb_to_linear_channel(srgb.g),
		from_srgb_to_linear_channel(srgb.b)
	);
}
