// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVRSettings interface version 002.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "common/openvr_logger.hpp"

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVRSettings.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVRSettings_002 : public XRTVRSettings_003, public vr::IVRSettings_002
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVRSettings_002(XRTVRClientCore_003 *core) : XRTVRSettings_003(core) {};
	// clang-format on

	FnTable_IVRSettings_002();

	ForwardDeclareBase_IVRSettings(XRTVRSettings_003);

	Forward_IVRSettings_GetSettingsErrorNameFromEnum();

	bool
	Sync(bool bForce = false, EVRSettingsError *peError = nullptr) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "Sync(bForce=%d, peError=%p) -> false", false,
		                             static_cast<int>(bForce), static_cast<const void *>(peError));
	}

	Forward_IVRSettings_SetBool();
	Forward_IVRSettings_SetInt32();
	Forward_IVRSettings_SetFloat();
	Forward_IVRSettings_SetString();
	Forward_IVRSettings_GetBool_003();
	Forward_IVRSettings_GetInt32_003();
	Forward_IVRSettings_GetFloat_003();
	Forward_IVRSettings_GetString_003();
	Forward_IVRSettings_RemoveSection();
	Forward_IVRSettings_RemoveKeyInSection();
};

}; // namespace xrt::state_trackers::openvr
