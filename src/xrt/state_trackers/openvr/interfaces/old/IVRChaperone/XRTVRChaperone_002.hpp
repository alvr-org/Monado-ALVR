// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVRChaperone interface version 002.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "common/openvr_logger.hpp"

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVRChaperone.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVRChaperone_002 : public XRTVRChaperone_004, public vr::IVRChaperone_002
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVRChaperone_002(XRTVRClientCore_003 *core) : XRTVRChaperone_004(core) {};
	// clang-format on

	void *
	getFnTable()
	{
		// No FnTable available for IVRChaperone_002.
		return nullptr;
	}

	ForwardDeclareBase_IVRChaperone(XRTVRChaperone_004);

	Forward_IVRChaperone_GetCalibrationState();

	bool
	GetSoftBoundsInfo(ChaperoneSoftBoundsInfo_t *pInfo) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetSoftBoundsInfo(pInfo=%p) -> false", false,
		                             static_cast<const void *>(pInfo));
	}

	bool
	GetHardBoundsInfo(VR_OUT_ARRAY_COUNT(punQuadsCount) HmdQuad_t *pQuadsBuffer, uint32_t *punQuadsCount) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetHardBoundsInfo(pQuadsBuffer=%p, punQuadsCount=%p) -> false",
		                             false, static_cast<const void *>(pQuadsBuffer),
		                             static_cast<const void *>(punQuadsCount));
	}

	bool
	GetSeatedBoundsInfo(ChaperoneSeatedBoundsInfo_t *pInfo) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetSeatedBoundsInfo(pInfo=%p) -> false", false,
		                             static_cast<const void *>(pInfo));
	}

	Forward_IVRChaperone_ReloadInfo();
	Forward_IVRChaperone_SetSceneColor();

	void
	GetBoundsColor(HmdColor_t *pOutputColorArray, int nNumOutputColors) override
	{
		openvr_logger logger;
		OPENVR_LOGGER_INIT(logger);

		OPENVR_LOG_UNIMPLEMENTED(logger, "GetBoundsColor(pOutputColorArray=%p, nNumOutputColors=%lld) -> false",
		                         static_cast<const void *>(pOutputColorArray),
		                         static_cast<long long>(nNumOutputColors));
	}

	Forward_IVRChaperone_AreBoundsVisible();
	Forward_IVRChaperone_ForceBoundsVisible();
};

}; // namespace xrt::state_trackers::openvr
