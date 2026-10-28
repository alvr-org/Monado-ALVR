// Copyright 2025, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Blubur S1 protocol definitions.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_blubur_s1
 */

#pragma once

#include "xrt/xrt_defines.h"


#pragma pack(push, 1)

struct blubur_s1_report_header
{
	uint8_t id;
};

struct blubur_s1_report_sensor_data
{
	uint8_t data[0x20];
};

enum blubur_s1_status_bits
{
	BLUBUR_S1_STATUS_DISPLAY_CONNECTION = 0x01,
	BLUBUR_S1_STATUS_DISPLAY_ON = 0x04,
	BLUBUR_S1_STATUS_BUTTON = 0x20,
	BLUBUR_S1_STATUS_PRESENCE = 0x80,
};

struct blubur_s1_report_0x83
{
	uint8_t status;     //< blubur_s1_status_bits
	uint16_t timestamp; //< in milliseconds
	uint16_t unk;
	uint8_t unk2;
	int8_t unk3; //< float, ((int32_t)unk3 + 570) / 10.0f
	int32_t unkValues[4];
	struct blubur_s1_report_sensor_data sensor;
	uint8_t padding[8];
};

#pragma pack(pop)
