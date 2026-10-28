// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0

/*
 * This file is included by both C code, like comp_render_cs.c, and GLSL code,
 * like layer.comp, so it uses a very limited set of features of both.
 */
#ifndef LAYER_DEFINES_INC_GLSL
#define LAYER_DEFINES_INC_GLSL

//! To handle invalid/unsupported layer types.
#define LAYER_COMP_TYPE_NOOP 0
//! Maps to XRT_LAYER_QUAD (not numerically)
#define LAYER_COMP_TYPE_QUAD 1
//! Maps to XRT_LAYER_CYLINDER (not numerically)
#define LAYER_COMP_TYPE_CYLINDER 2
//! Maps to XRT_LAYER_EQUIRECT2 (not numerically)
#define LAYER_COMP_TYPE_EQUIRECT2 3
//! Maps to XRT_LAYER_PROJECTION[_DEPTH] (not numerically)
#define LAYER_COMP_TYPE_PROJECTION 4
//! Context (background) part of a split quad view projection layer.
#define LAYER_COMP_TYPE_CONTEXT 5
//! Inset part of a split quad view projection layer.
#define LAYER_COMP_TYPE_INSET 6

#endif // LAYER_DEFINES_INC_GLSL
