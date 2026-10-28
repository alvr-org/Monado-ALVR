// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRIOBuffer_002 interface version.
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

class XRTVRIOBuffer_002 : public vr::IVRIOBuffer_002
{
private:
	XRTVRClientCore_003 *core;

public:
	XRTVRIOBuffer_002(XRTVRClientCore_003 *clientCore);

	virtual ~XRTVRIOBuffer_002() = default;

	FnTable_IVRIOBuffer_002();

	vr::EIOBufferError
	Open(const char *pchPath,
	     vr::EIOBufferMode mode,
	     uint32_t unElementSize,
	     uint32_t unElements,
	     vr::IOBufferHandle_t *pulBuffer) override;

	vr::EIOBufferError
	Close(vr::IOBufferHandle_t ulBuffer) override;

	vr::EIOBufferError
	Read(vr::IOBufferHandle_t ulBuffer, void *pDst, uint32_t unBytes, uint32_t *punRead) override;

	vr::EIOBufferError
	Write(vr::IOBufferHandle_t ulBuffer, void *pSrc, uint32_t unBytes) override;

	vr::PropertyContainerHandle_t
	PropertyContainer(vr::IOBufferHandle_t ulBuffer) override;

	bool
	HasReaders(vr::IOBufferHandle_t ulBuffer) override;
};

} // namespace xrt::state_trackers::openvr
