// Copyright 2019-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  OpenGL helper functions for drawing GUI elements.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Moses Turner <moses@collabora.com>
 * @ingroup gui
 */

#include "math/m_api.h"

#include "gui_ogl.h"
#include "gui_imgui.h"


/*
 *
 * Helpers.
 *
 */

static void
get_uvs(ImVec2 *uv0, ImVec2 *uv1, bool rotate_180, bool flip_y)
{
	// Flip direction of y (x) if we are rotating.
	float u0 = rotate_180 ? 1.f : 0.f;
	float u1 = rotate_180 ? 0.f : 1.f;

	// Flip direction of v (y) if either flip_y or rotate_180 is true.
	float v0 = (rotate_180 ^ flip_y) ? 1.f : 0.f;
	float v1 = (rotate_180 ^ flip_y) ? 0.f : 1.f;

	// Note: We can't easily do 90 or 270-degree rotations: https://github.com/ocornut/imgui/issues/3267
	*uv0 = (ImVec2){u0, v0};
	*uv1 = (ImVec2){u1, v1};
}


/*
 *
 * 'Exported' functions.
 *
 */

void
gui_ogl_draw_image(uint32_t width, uint32_t height, uint32_t tex_id, float scale, bool rotate_180, bool flip_y)
{
	ImVec2 uv0, uv1;
	get_uvs(&uv0, &uv1, rotate_180, flip_y);

	// Need to go via ints to get integer steps.
	int w = (float)width * scale;
	int h = (float)height * scale;

	ImVec2 size = {(float)w, (float)h};
	ImVec4 white = {1, 1, 1, 1};
	ImVec4 black = {0, 0, 0, 1};
	ImTextureID id = (ImTextureID)(intptr_t)tex_id;

	igImageBg(id, size, uv0, uv1, white, white, black);
}

void
gui_ogl_draw_background(uint32_t width, uint32_t height, uint32_t tex_id, bool rotate_180, bool flip_y)
{
	ImVec2 uv0, uv1;
	get_uvs(&uv0, &uv1, rotate_180, flip_y);

	const ImU32 white = 0xffffffff;
	ImTextureID id = (ImTextureID)(intptr_t)tex_id;

	ImGuiIO *io = igGetIO();

	float in_w = (float)width;
	float in_h = (float)height;
	float out_w = io->DisplaySize.x;
	float out_h = io->DisplaySize.y;

	float scale_w = (float)out_w / in_w; // 128 / 1280 = 0.1
	float scale_h = (float)out_h / in_h; // 128 / 800 =  0.16

	float scale = MIN(scale_w, scale_h); // 0.1

	float inside_w = in_w * scale;
	float inside_h = in_h * scale;

	float translate_x = (out_w - inside_w) / 2; // Should be 0 for 1280x800
	float translate_y = (out_h - inside_h) / 2; // Should be (1280 - 800) / 2 = 240

	ImVec2 p_min = {translate_x, translate_y};
	ImVec2 p_max = {translate_x + inside_w, translate_y + inside_h};

	ImDrawList *bg = igGetBackgroundDrawList();
	ImDrawList_AddImage(bg, id, p_min, p_max, uv0, uv1, white);
}
