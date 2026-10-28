// Copyright 2025, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Code to parse and handle the Vive Pro 2 configuration data.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_vp2
 */

#include "util/u_json.hpp"
#include "util/u_logging.h"

#include "math/m_api.h"

#include "vp2_config.h"

#include <string.h>
#include <math.h>


using xrt::auxiliary::util::json::JSONNode;

bool
vp2_config_parse(const char *config_data, size_t config_size, vp2_config *out_config)
{
	// Zero-init to be in a good state
	memset(out_config, 0, sizeof(*out_config));

	JSONNode root{std::string{config_data, config_size}};
	if (root.isInvalid()) {
		U_LOG_E("Failed to parse JSON config data.");
		return false;
	}

	out_config->direct_mode_edid_pid = root["direct_mode_edid_pid"].asInt();
	out_config->direct_mode_edid_vid = root["direct_mode_edid_vid"].asInt();

	if (!htc_config_parse(config_data, config_size, &out_config->base)) {
		return false;
	}

	return true;
}
