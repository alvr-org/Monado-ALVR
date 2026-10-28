// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the IVRIOBuffer interface version 001.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#pragma once

#include "openvr_interfaces_unified.h"
#include "openvr_forward_macros.h"

#include "interfaces/XRTVRIOBuffer.hpp"


namespace xrt::state_trackers::openvr {

using namespace vr;

class XRTVRIOBuffer_001 : public XRTVRIOBuffer_002, public vr::IVRIOBuffer_001
{
public:
	// @todo Remove when clang-format is updated in CI
	// clang-format off
	XRTVRIOBuffer_001(XRTVRClientCore_003 *core) : XRTVRIOBuffer_002(core) {};
	// clang-format on

	FnTable_IVRIOBuffer_001();

	ForwardDeclareBase_IVRIOBuffer(XRTVRIOBuffer_002);

	Forward_IVRIOBuffer_Open();
	Forward_IVRIOBuffer_Close();
	Forward_IVRIOBuffer_Read();
	Forward_IVRIOBuffer_Write();
	Forward_IVRIOBuffer_PropertyContainer();
};

}; // namespace xrt::state_trackers::openvr
