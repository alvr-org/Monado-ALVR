// Copyright 2020-2024 Jan Schmidt
// Copyright 2025-2026 Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  LED search model management code
 * @author Jan Schmidt <jan@centricular.com>
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup tracking
 */

#pragma once

#include "tracking/t_constellation.h"

#include "math/m_vec3.h"

#include <stdlib.h>
#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif

struct t_constellation_search_led_candidate
{
	struct t_constellation_tracker_led *led;

	//! Transform to rotate the anchor LED to face forward @ 0,0,0
	struct xrt_pose pose;

	//! List of possible neighbours for this LED, sorted by distance
	uint8_t num_neighbours;
	//! The neighbours of this LED, sorted by distance. These are pointers to the LEDs in the same model.
	struct t_constellation_tracker_led **neighbours;
};

struct t_constellation_search_model
{
	t_constellation_device_id_t device_id;

	uint8_t num_points;
	struct t_constellation_search_led_candidate **points;

	struct t_constellation_tracker_led_model *led_model;
};

struct t_constellation_search_model *
t_constellation_search_model_new(t_constellation_device_id_t device_id,
                                 struct t_constellation_tracker_led_model *led_model);

void
t_constellation_search_model_free(struct t_constellation_search_model *model);

#ifdef __cplusplus
}
#endif
