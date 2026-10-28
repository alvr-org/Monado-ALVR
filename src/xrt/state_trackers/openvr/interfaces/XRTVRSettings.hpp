// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRSettings_003 interface version.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "XRTVRClientCore.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVRSettings_003 : public vr::IVRSettings_003
{
private:
	XRTVRClientCore_003 *core;

public:
	XRTVRSettings_003(XRTVRClientCore_003 *clientCore);

	virtual ~XRTVRSettings_003() = default;

	FnTable_IVRSettings_003();

	const char *
	GetSettingsErrorNameFromEnum(EVRSettingsError eError) override;

	void
	SetBool(const char *pchSection, const char *pchSettingsKey, bool bValue, EVRSettingsError *peError) override;

	void
	SetInt32(const char *pchSection,
	         const char *pchSettingsKey,
	         int32_t nValue,
	         EVRSettingsError *peError) override;

	void
	SetFloat(const char *pchSection, const char *pchSettingsKey, float flValue, EVRSettingsError *peError) override;

	void
	SetString(const char *pchSection,
	          const char *pchSettingsKey,
	          const char *pchValue,
	          EVRSettingsError *peError) override;

	bool
	GetBool(const char *pchSection, const char *pchSettingsKey, EVRSettingsError *peError) override;

	int32_t
	GetInt32(const char *pchSection, const char *pchSettingsKey, EVRSettingsError *peError) override;

	float
	GetFloat(const char *pchSection, const char *pchSettingsKey, EVRSettingsError *peError) override;

	void
	GetString(const char *pchSection,
	          const char *pchSettingsKey,
	          char *pchValue,
	          uint32_t unValueLen,
	          EVRSettingsError *peError) override;

	void
	RemoveSection(const char *pchSection, EVRSettingsError *peError) override;

	void
	RemoveKeyInSection(const char *pchSection, const char *pchSettingsKey, EVRSettingsError *peError) override;
};

} // namespace xrt::state_trackers::openvr
