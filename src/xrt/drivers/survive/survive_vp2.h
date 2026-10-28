// Copyright 2026 Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Vive Pro 2 related code for Libsurvive.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_survive
 */

#pragma once

#include "survive_internal.h"


bool
survive_vp2_init(struct survive_device *survive, struct survive_system *sys);

void
survive_vp2_setup_hmd(struct survive_device *survive);

void
survive_vp2_teardown(struct survive_device *survive);

xrt_result_t
survive_vp2_compute_distortion(
    struct survive_device *survive, uint32_t view, float u, float v, struct xrt_uv_triplet *result);

xrt_result_t
survive_vp2_set_brightness(struct survive_device *survive, float brightness, bool relative);
