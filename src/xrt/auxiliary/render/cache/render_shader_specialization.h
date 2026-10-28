// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Shader specialization structs for render compute shaders.
 * @ingroup aux_render
 *
 * Entry point for build-generated specialization metadata. Include after the
 * @c render_shader_specialization custom target (or @c aux_shaders) has been built
 * so @c render_shader_specialization.h.inc is available on the include path.
 *
 * @par Generated file (@c render_shader_specialization.h.inc)
 * - One padding-free @c struct per shader listed in @c shader_specializations().
 * - @c RENDER_SHADER_SPECIALIZATION_KEYS(X) — X-macro with one @c X(KeyStruct) entry
 *   per generated spec struct (e.g. @c render_distortion_spec).
 * - @c RENDER_SHADER_SPECIALIZATION_CACHE_NAMES(X) — X-macro with one @c X(CacheName)
 *   entry per shader (e.g. @c render_distortion).
 * - @c RENDER_SHADER_SPECIALIZATION_ENTRIES(X) — X-macro with one @c X(CacheName, KeyStruct)
 *   pair per shader (e.g. @c X(render_distortion, render_distortion_spec)).
 *
 * Produced by @c cmake/generate_shader_specialization.py via @c cmake/ShaderSpecialization.cmake.
 */

#pragma once

#include "xrt/xrt_vulkan_includes.h"

#include <stddef.h>

#include "render_shader_specialization.h.inc"
