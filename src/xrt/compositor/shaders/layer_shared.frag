// Copyright 2023, Collabora Ltd.
// Author: Jakob Bornecrantz <jakob@collabora.com>
// SPDX-License-Identifier: BSL-1.0

#version 460


layout (binding = 1) uniform sampler2D image;

layout (location = 0)  in vec2 uv;
layout (location = 0) out vec4 out_color;


void main ()
{
	out_color = texture(image, uv);
}
