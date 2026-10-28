// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVRSettings interface version 001.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "common/openvr_logger.hpp"

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVRSettings.hpp"
#include "XRTVRSettings_002.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVRSettings_001 : public XRTVRSettings_002, public vr::IVRSettings_001
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVRSettings_001(XRTVRClientCore_003 *core) : XRTVRSettings_002(core) {};
	// clang-format on

	FnTable_IVRSettings_001();

	ForwardDeclareBase_IVRSettings(XRTVRSettings_002);

	Forward_IVRSettings_GetSettingsErrorNameFromEnum();
	Forward_IVRSettings_Sync();

	bool
	GetBool(const char *pchSection,
	        const char *pchSettingsKey,
	        bool bDefaultValue,
	        EVRSettingsError *peError = nullptr) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger, "GetBool(pchSection=%s, pchSettingsKey=%s, bDefaultValue=%d, peError=%p) -> false", false,
		    pchSection, pchSettingsKey, static_cast<int>(bDefaultValue), static_cast<const void *>(peError));
	}

	Forward_IVRSettings_SetBool();

	int32_t
	GetInt32(const char *pchSection,
	         const char *pchSettingsKey,
	         int32_t nDefaultValue,
	         EVRSettingsError *peError = nullptr) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		SET_ERROR(peError, EVRSettingsError::VRSettingsError_ReadFailed);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger, "GetInt32(pchSection=%s, pchSettingsKey=%s, nDefaultValue=%lld, peError=%p) -> 0", 0,
		    pchSection, pchSettingsKey, static_cast<long long>(nDefaultValue),
		    static_cast<const void *>(peError));
	}

	Forward_IVRSettings_SetInt32();

	float
	GetFloat(const char *pchSection,
	         const char *pchSettingsKey,
	         float flDefaultValue,
	         EVRSettingsError *peError = nullptr) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		SET_ERROR(peError, EVRSettingsError::VRSettingsError_ReadFailed);

		OPENVR_LOG_UNIMPLEMENTED_RET(
		    logger, "GetFloat(pchSection=%s, pchSettingsKey=%s, flDefaultValue=%f, peError=%p) -> 0.0", 0.0f,
		    pchSection, pchSettingsKey, flDefaultValue, static_cast<const void *>(peError));
	}

	Forward_IVRSettings_SetFloat();

	void
	GetString(const char *pchSection,
	          const char *pchSettingsKey,
	          VR_OUT_STRING() char *pchValue,
	          uint32_t unValueLen,
	          const char *pchDefaultValue,
	          EVRSettingsError *peError = nullptr) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		SET_ERROR(peError, EVRSettingsError::VRSettingsError_ReadFailed);

		OPENVR_LOG_UNIMPLEMENTED(logger,
		                         "GetString(pchSection=%s, pchSettingsKey=%s, pchValue=%p, unValueLen=%llu, "
		                         "pchDefaultValue=%s, peError=%p)",
		                         pchSection, pchSettingsKey, static_cast<const void *>(pchValue),
		                         static_cast<unsigned long long>(unValueLen), pchDefaultValue,
		                         static_cast<const void *>(peError));
	}

	Forward_IVRSettings_SetString();
	Forward_IVRSettings_RemoveSection();
	Forward_IVRSettings_RemoveKeyInSection();
};

}; // namespace xrt::state_trackers::openvr
