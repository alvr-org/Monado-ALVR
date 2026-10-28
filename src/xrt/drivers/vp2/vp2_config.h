// Copyright 2025, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Code to parse and handle the Vive Pro 2 configuration data.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_vp2
 */

#pragma once

#include "xrt/xrt_defines.h"

#include "htc/htc_config.h"

#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif
struct vp2_config
{
	struct htc_config base;

	uint32_t direct_mode_edid_pid;
	uint32_t direct_mode_edid_vid;
};

bool
vp2_config_parse(const char *config_data, size_t config_size, struct vp2_config *out_config);

#ifdef __cplusplus
}
#endif
