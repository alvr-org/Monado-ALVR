// Copyright 2025, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  USB communications for the Oculus Rift.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_rift
 */

#include "rift_internal.h"


int
rift_get_lens_distortion(struct rift_hmd *hmd, struct rift_lens_distortion_report *lens_distortion);

int
rift_get_position_calibration_report(struct rift_hmd *hmd, struct rift_position_calibration_report *position_report);

void
rift_parse_distortion_report(struct rift_lens_distortion_report *report, struct rift_lens_distortion *out);

int
rift_send_keepalive(struct rift_hmd *hmd);

int
rift_get_config(struct rift_hmd *hmd, struct rift_config_report *config);

int
rift_set_config(struct rift_hmd *hmd, struct rift_config_report *config);

int
rift_get_tracking_report(struct rift_hmd *hmd, struct rift_tracking_report *tracking_report);

int
rift_set_tracking(struct rift_hmd *hmd, struct rift_tracking_report *tracking);

int
rift_set_custom_pattern(struct rift_hmd *hmd, struct rift_custom_pattern_report *pattern);

int
rift_get_display_info(struct rift_hmd *hmd, struct rift_display_info_report *display_info);

int
rift_enable_components(struct rift_hmd *hmd, struct rift_enable_components_report *enable_components);

int
rift_get_imu_calibration(struct rift_hmd *hmd, struct rift_imu_calibration *imu_calibration);

int
rift_send_radio_cmd(struct rift_hmd *hmd, bool radio_hid, struct rift_radio_cmd_report *radio_cmd);

int
rift_radio_send_data_read_cmd(struct rift_hmd *hmd, struct rift_radio_data_read_cmd *cmd);

int
rift_send_radio_data(struct rift_hmd *hmd, struct rift_radio_cmd_report *cmd, uint8_t *data, size_t len);

int
rift_get_radio_cmd_response(struct rift_hmd *hmd, bool wait, bool radio_hid);

int
rift_get_radio_address(struct rift_hmd *hmd, uint8_t out_address[]);

int
rift_radio_read_data(struct rift_hmd *hmd, uint8_t *data, uint16_t length, bool flash_read);

void
rift_unpack_int_sample(const uint8_t *in, struct xrt_vec3_i32 *out);

void
rift_unpack_float_sample(const uint8_t *in, float scale, struct xrt_vec3 *out);

void
rift_sample_to_imu_space(const int32_t *in, struct xrt_vec3 *out);
