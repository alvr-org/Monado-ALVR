// Copyright 2025-2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Interface to Oculus Rift sensor probing/initialization
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_rift_sensor
 */

#pragma once

#include "rift/rift_interface.h"

#include "tracking/t_camera_models.h"


#define RIFT_SENSOR_CLOCK_FREQ (40000000)
// @todo Remove when clang-format is updated in CI
// clang-format off
#define RIFT_SENSOR_CLOCK_TO_NS(x) ((timepoint_ns)(x) * 1000 / 40)
// clang-format on
#define RIFT_SENSOR_WIDTH 1280
#define RIFT_SENSOR_HEIGHT 960
#define RIFT_SENSOR_FRAME_SIZE (RIFT_SENSOR_WIDTH * RIFT_SENSOR_HEIGHT)

struct rift_sensor;
struct rift_sensor_context;

int
rift_sensor_context_create(struct rift_sensor_context **out_context, struct xrt_frame_context *xfctx);

int
rift_sensor_enable_exposure_sync(struct rift_sensor_context *context, struct rift_sensor *sensor, uint8_t radio_id[5]);

int
rift_sensor_context_start(struct rift_sensor_context *context);

int
rift_sensor_context_get_sensors(struct rift_sensor_context *context,
                                struct rift_sensor ***out_sensors,
                                uint32_t *out_count);

struct xrt_fs *
rift_sensor_get_frame_server(struct rift_sensor *sensor);

enum rift_variant
rift_sensor_get_variant(struct rift_sensor *sensor);

void
rift_sensor_get_calibration(struct rift_sensor *sensor, struct t_camera_calibration *out_calibration);

void
rift_sensor_setup_frame_timestamp_callback(struct rift_sensor *sensor, struct rift_hmd *hmd);
