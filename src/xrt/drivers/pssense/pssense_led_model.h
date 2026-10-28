// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  PlayStation Sense controller LED models. Data extracted from Sony's PC driver.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_pssense
 */

#pragma once

#include "tracking/t_constellation.h"

#include "math/m_api.h"


static const float LED_RADIUS_M = 0.006f; // 6mm
static const float LED_VISIBILITY_ANGLE = DEG_TO_RAD(90);
static const struct t_constellation_tracker_led_model_match_parameters DEFAULT_MATCH_PARAMETERS = {
    .min_leds_for_correspondence_search_without_prior = 7,
    .min_leds_for_correspondence_search_with_prior = 5,
};

//! Rotation on X axis from LED -> IMU.
const float pssense_imu_angle = DEG_TO_RAD(-50.2703f);

//! The offset from the LED origin -> IMU origin, for the left controller
static struct xrt_vec3 T_led_imu_left = {
    .x = 0.0093727f,
    .y = -0.0122481f,
    .z = -0.0060039f,
};
//! The offset from the LED origin -> IMU origin, for the right controller
static struct xrt_vec3 T_led_imu_right = {
    .x = -0.0200727f,
    .y = -0.0122481f,
    .z = -0.0060039f,
};

/*
 * Intercepted at address 0xdde10 in the driver, within a vector in the first
 * parameter object.
 */
static const struct t_constellation_tracker_led pssense_left_leds[] = {
    {
        .position = {-0.012054, -0.055213, 0.038152},
        .normal = {-0.176485, -0.811967, 0.556384},
        .radius_m = LED_RADIUS_M,
        .visibility_angle = LED_VISIBILITY_ANGLE,
        .id = 0,
    },
    {
        .position = {-0.033088, -0.055236, 0.020190},
        .normal = {-0.426232, -0.891845, -0.151455},
        .radius_m = LED_RADIUS_M,
        .visibility_angle = LED_VISIBILITY_ANGLE,
        .id = 1,
    },
    {
        .position = {-0.053695, -0.031009, 0.028350},
        .normal = {-0.788493, -0.456350, 0.412339},
        .radius_m = LED_RADIUS_M,
        .visibility_angle = LED_VISIBILITY_ANGLE,
        .id = 2,
    },
    {
        .position = {-0.066986, -0.010505, 0.008679},
        .normal = {-0.980055, -0.153898, 0.125726},
        .radius_m = LED_RADIUS_M,
        .visibility_angle = LED_VISIBILITY_ANGLE,
        .id = 3,
    },
    {
        .position = {-0.063505, 0.024601, 0.004185},
        .normal = {-0.930796, 0.360502, 0.060482},
        .radius_m = LED_RADIUS_M,
        .visibility_angle = LED_VISIBILITY_ANGLE,
        .id = 4,
    },
    {
        .position = {-0.047803, 0.044966, 0.018502},
        .normal = {-0.701817, 0.660177, 0.267618},
        .radius_m = LED_RADIUS_M,
        .visibility_angle = LED_VISIBILITY_ANGLE,
        .id = 5,
    },
    {
        .position = {0.045472, -0.027042, 0.043022},
        .normal = {0.667492, -0.396842, 0.630056},
        .radius_m = LED_RADIUS_M,
        .visibility_angle = LED_VISIBILITY_ANGLE,
        .id = 6,
    },
    {
        .position = {0.059535, -0.013977, 0.028536},
        .normal = {0.961512, -0.273488, -0.026435},
        .radius_m = LED_RADIUS_M,
        .visibility_angle = LED_VISIBILITY_ANGLE,
        .id = 7,
    },
    {
        .position = {0.061060, 0.014608, 0.024725},
        .normal = {0.984435, 0.154641, -0.083507},
        .radius_m = LED_RADIUS_M,
        .visibility_angle = LED_VISIBILITY_ANGLE,
        .id = 8,
    },
    {
        .position = {0.050994, 0.027255, 0.036184},
        .normal = {0.748123, 0.399672, 0.529693},
        .radius_m = LED_RADIUS_M,
        .visibility_angle = LED_VISIBILITY_ANGLE,
        .id = 9,
    },
    {
        .position = {0.048475, 0.043025, 0.018776},
        .normal = {0.795992, 0.580182, -0.172584},
        .radius_m = LED_RADIUS_M,
        .visibility_angle = LED_VISIBILITY_ANGLE,
        .id = 10,
    },
    {
        .position = {0.030569, 0.053446, 0.029350},
        .normal = {0.448555, 0.783814, 0.429458},
        .radius_m = LED_RADIUS_M,
        .visibility_angle = LED_VISIBILITY_ANGLE,
        .id = 11,
    },
    {
        .position = {0.018498, 0.063913, 0.011230},
        .normal = {0.346695, 0.893406, -0.285708},
        .radius_m = LED_RADIUS_M,
        .visibility_angle = LED_VISIBILITY_ANGLE,
        .id = 12,
    },
    {
        .position = {-0.002748, 0.064260, 0.022718},
        .normal = {-0.040150, 0.942362, 0.332178},
        .radius_m = LED_RADIUS_M,
        .visibility_angle = LED_VISIBILITY_ANGLE,
        .id = 13,
    },
    {
        .position = {-0.029078, 0.013435, -0.060735},
        .normal = {-0.417589, 0.258817, -0.870996},
        .radius_m = LED_RADIUS_M,
        .visibility_angle = LED_VISIBILITY_ANGLE,
        .id = 14,
    },
    {
        .position = {0.001067, 0.013435, -0.067327},
        .normal = {0.014114, 0.258800, -0.965828},
        .radius_m = LED_RADIUS_M,
        .visibility_angle = LED_VISIBILITY_ANGLE,
        .id = 15,
    },
    {
        .position = {0.031783, 0.013435, -0.059270},
        .normal = {0.458628, 0.258803, -0.850106},
        .radius_m = LED_RADIUS_M,
        .visibility_angle = LED_VISIBILITY_ANGLE,
        .id = 16,
    },
};

static const struct t_constellation_tracker_led pssense_right_leds[] = {
    {
        .position = {0.012054, -0.055213, 0.038152},
        .normal = {0.176485, -0.811967, 0.556384},
        .radius_m = LED_RADIUS_M,
        .visibility_angle = LED_VISIBILITY_ANGLE,
        .id = 0,
    },
    {
        .position = {0.033088, -0.055236, 0.020190},
        .normal = {0.426232, -0.891845, -0.151455},
        .radius_m = LED_RADIUS_M,
        .visibility_angle = LED_VISIBILITY_ANGLE,
        .id = 1,
    },
    {
        .position = {0.053695, -0.031009, 0.028350},
        .normal = {0.788493, -0.456350, 0.412339},
        .radius_m = LED_RADIUS_M,
        .visibility_angle = LED_VISIBILITY_ANGLE,
        .id = 2,
    },
    {
        .position = {0.066986, -0.010505, 0.008679},
        .normal = {0.980055, -0.153898, 0.125726},
        .radius_m = LED_RADIUS_M,
        .visibility_angle = LED_VISIBILITY_ANGLE,
        .id = 3,
    },
    {
        .position = {0.063505, 0.024601, 0.004185},
        .normal = {0.930796, 0.360502, 0.060482},
        .radius_m = LED_RADIUS_M,
        .visibility_angle = LED_VISIBILITY_ANGLE,
        .id = 4,
    },
    {
        .position = {0.047803, 0.044966, 0.018502},
        .normal = {0.701817, 0.660177, 0.267618},
        .radius_m = LED_RADIUS_M,
        .visibility_angle = LED_VISIBILITY_ANGLE,
        .id = 5,
    },
    {
        .position = {-0.045472, -0.027042, 0.043022},
        .normal = {-0.667492, -0.396842, 0.630056},
        .radius_m = LED_RADIUS_M,
        .visibility_angle = LED_VISIBILITY_ANGLE,
        .id = 6,
    },
    {
        .position = {-0.059535, -0.013977, 0.028536},
        .normal = {-0.961512, -0.273488, -0.026435},
        .radius_m = LED_RADIUS_M,
        .visibility_angle = LED_VISIBILITY_ANGLE,
        .id = 7,
    },
    {
        .position = {-0.061060, 0.014608, 0.024725},
        .normal = {-0.984435, 0.154641, -0.083507},
        .radius_m = LED_RADIUS_M,
        .visibility_angle = LED_VISIBILITY_ANGLE,
        .id = 8,
    },
    {
        .position = {-0.050994, 0.027255, 0.036184},
        .normal = {-0.748123, 0.399672, 0.529693},
        .radius_m = LED_RADIUS_M,
        .visibility_angle = LED_VISIBILITY_ANGLE,
        .id = 9,
    },
    {
        .position = {-0.048475, 0.043025, 0.018776},
        .normal = {-0.795992, 0.580182, -0.172584},
        .radius_m = LED_RADIUS_M,
        .visibility_angle = LED_VISIBILITY_ANGLE,
        .id = 10,
    },
    {
        .position = {-0.030569, 0.053446, 0.029350},
        .normal = {-0.448555, 0.783814, 0.429458},
        .radius_m = LED_RADIUS_M,
        .visibility_angle = LED_VISIBILITY_ANGLE,
        .id = 11,
    },
    {
        .position = {-0.018498, 0.063913, 0.011230},
        .normal = {-0.346695, 0.893406, -0.285708},
        .radius_m = LED_RADIUS_M,
        .visibility_angle = LED_VISIBILITY_ANGLE,
        .id = 12,
    },
    {
        .position = {0.002748, 0.064260, 0.022718},
        .normal = {0.040150, 0.942362, 0.332178},
        .radius_m = LED_RADIUS_M,
        .visibility_angle = LED_VISIBILITY_ANGLE,
        .id = 13,
    },
    {
        .position = {0.029078, 0.013435, -0.060735},
        .normal = {0.417589, 0.258817, -0.870996},
        .radius_m = LED_RADIUS_M,
        .visibility_angle = LED_VISIBILITY_ANGLE,
        .id = 14,
    },
    {
        .position = {-0.001067, 0.013435, -0.067327},
        .normal = {-0.014114, 0.258800, -0.965828},
        .radius_m = LED_RADIUS_M,
        .visibility_angle = LED_VISIBILITY_ANGLE,
        .id = 15,
    },
    {
        .position = {-0.031783, 0.013435, -0.059270},
        .normal = {-0.458628, 0.258803, -0.850106},
        .radius_m = LED_RADIUS_M,
        .visibility_angle = LED_VISIBILITY_ANGLE,
        .id = 16,
    },
};
