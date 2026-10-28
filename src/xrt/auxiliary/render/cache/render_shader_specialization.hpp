// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief C++ shader specialization map entries for render compute shaders.
 * @ingroup aux_render
 *
 * Entry point for build-generated specialization metadata. Include after the
 * @c render_shader_specialization custom target (or @c aux_shaders) has been built
 * so @c render_shader_specialization.hpp.inc is available on the include path.
 *
 * @par Declared here
 * - @c ShaderSpecializationMap<Key> primary template (specializations are generated below).
 *
 * @par Generated file (@c render_shader_specialization.hpp.inc)
 * - One @c ShaderSpecializationMap specialization per shader listed in
 *   @c shader_specializations().
 * - @c RENDER_SHADER_SPECIALIZATION_KEYS(X) — X-macro with one @c X(KeyStruct) entry
 *   per generated spec struct (e.g. @c render_distortion_spec).
 * - @c RENDER_SHADER_SPECIALIZATION_CACHE_NAMES(X) and @c RENDER_SHADER_SPECIALIZATION_ENTRIES(X)
 *   — same cache-name macros as in @ref render_shader_specialization.h.
 *
 * Struct definitions live in @ref render_shader_specialization.h.
 *
 * Produced by @c cmake/generate_shader_specialization.py via @c cmake/ShaderSpecialization.cmake.
 */

#pragma once

#include "xrt/xrt_vulkan_includes.h"

#include <array>
#include <cstddef>

//! Primary template; specializations are in generated @c .hpp.inc file.
template <typename Key> struct ShaderSpecializationMap;

#include "cache/render_shader_specialization.h"

#include "render_shader_specialization.hpp.inc"
