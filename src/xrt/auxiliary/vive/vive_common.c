// Copyright 2020-2021, Collabora, Ltd.
// Copyright 2025, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Common things like defines for Vive and Index.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @author Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * @author Moshi Turner <moshiturner@protonmail.com>
 * @ingroup aux_vive
 */

#include <util/u_logging.h>

#include "vive_common.h"

#include <string.h>


enum VIVE_VARIANT
vive_determine_variant(const char *model_number)
{
	enum VIVE_VARIANT variant = VIVE_UNKNOWN;

	if (strcmp(model_number, "Utah MP") == 0 || strcmp(model_number, "Index") == 0) {
		variant = VIVE_VARIANT_INDEX;
		U_LOG_D("Found Valve Index HMD");
	} else if (strcmp(model_number, "Vive MV") == 0 || strcmp(model_number, "Vive MV.") == 0 ||
	           strcmp(model_number, "Vive. MV") == 0) {
		variant = VIVE_VARIANT_VIVE;
		U_LOG_D("Found HTC Vive HMD");
	} else if (strcmp(model_number, "Vive_Pro MV") == 0 || strcmp(model_number, "VIVE_Pro MV") == 0) {
		variant = VIVE_VARIANT_PRO;
		U_LOG_D("Found HTC Vive Pro HMD");
	} else if (strcmp(model_number, "Vive_Pro 2 MV") == 0 || strcmp(model_number, "VIVE_Pro 2 MV") == 0 ||
	           strcmp(model_number, "VIVE_Pro 2 PV") == 0 || strcmp(model_number, "Vive_Pro 2 PV") == 0) {
		variant = VIVE_VARIANT_PRO2;
		U_LOG_D("Found HTC Vive Pro 2 HMD");
	} else if (strcmp(model_number, "Beyond") == 0) {
		variant = VIVE_VARIANT_BEYOND;
		U_LOG_D("Found Bigscreen Beyond HMD");
	} else {
		U_LOG_W("Failed to parse Vive HMD variant!\n\tfirmware.model_[number|name]: '%s'", model_number);
	}

	return variant;
}

bool
vive_variant_scanout_info(enum VIVE_VARIANT variant,
                          time_duration_ns frame_period_ns,
                          time_duration_ns *out_scanout_time_ns,
                          enum xrt_scanout_direction *out_scanout_direction,
                          enum xrt_panel_refresh_type *out_panel_refresh_type)
{
	switch (variant) {
	case VIVE_VARIANT_PRO:
		*out_panel_refresh_type = XRT_PANEL_REFRESH_TYPE_ROLLING;
		*out_scanout_direction = XRT_SCANOUT_DIRECTION_TOP_TO_BOTTOM;
		*out_scanout_time_ns = frame_period_ns * 1600.0 / 1624.0;
		return true;
	case VIVE_VARIANT_BEYOND:
		*out_panel_refresh_type = XRT_PANEL_REFRESH_TYPE_ROLLING;
		*out_scanout_direction = XRT_SCANOUT_DIRECTION_TOP_TO_BOTTOM;
		*out_scanout_time_ns = frame_period_ns * 2544.0 / 2568.0;
		return true;
	case VIVE_VARIANT_PRO2:
		*out_panel_refresh_type = XRT_PANEL_REFRESH_TYPE_ROLLING;
		*out_scanout_direction = XRT_SCANOUT_DIRECTION_TOP_TO_BOTTOM;
		*out_scanout_time_ns = frame_period_ns * 2448.0 / 2574.0;
		return true;
	default:
		*out_panel_refresh_type = XRT_PANEL_REFRESH_TYPE_GLOBAL;
		*out_scanout_direction = XRT_SCANOUT_DIRECTION_NONE;
		*out_scanout_time_ns = 0;
		return false;
	}
}
