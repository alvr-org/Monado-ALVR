// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRControlPanel_001 interface version.
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

class XRTVRControlPanel_001 : public vr::IVRControlPanel_001
{
private:
	XRTVRClientCore_003 *core;

public:
	XRTVRControlPanel_001(XRTVRClientCore_003 *clientCore);

	virtual ~XRTVRControlPanel_001() = default;

	void *
	getFnTable()
	{
		// Unsupported.
		return nullptr;
	}

	uint32_t
	GetDriverCount() override;

	uint32_t
	GetDriverId(uint32_t unDriverIndex, char *pchBuffer, uint32_t unBufferLen) override;

	uint32_t
	GetDriverDisplayCount(const char *pchDriverId) override;

	uint32_t
	GetDriverDisplayId(const char *pchDriverId,
	                   uint32_t unDisplayIndex,
	                   char *pchBuffer,
	                   uint32_t unBufferLen) override;

	uint32_t
	GetDriverDisplayModelNumber(const char *pchDriverId,
	                            const char *pchDisplayId,
	                            char *pchBuffer,
	                            uint32_t unBufferLen) override;

	uint32_t
	GetDriverDisplaySerialNumber(const char *pchDriverId,
	                             const char *pchDisplayId,
	                             char *pchBuffer,
	                             uint32_t unBufferLen) override;

	class IVRSystem *
	GetCurrentDisplayInterface(const char *pchHmdInterfaceVersion) override;

	uint32_t
	LoadSharedResource(const char *pchResourceName, char *pchBuffer, uint32_t unBufferLen) override;

	float
	GetIPD() override;

	void
	SetIPD(float fIPD) override;

	class vr::IVRCompositor *
	GetCurrentCompositorInterface(const char *pchInterfaceVersion) override;

	bool
	QuitProcess(uint32_t pidProcessToQuit) override;

	uint32_t
	StartVRProcess(const char *pchExecutable,
	               const char **pchArguments,
	               uint32_t unArgumentCount,
	               const char *pchWorkingDirectory) override;

	void
	SetMasterProcessToThis() override;

	void
	StartAutolaunchOverlays() override;

	bool
	ForceQuitProcess(uint32_t pidProcessToQuit) override;

	void
	AbortTransition() override;
};

} // namespace xrt::state_trackers::openvr
