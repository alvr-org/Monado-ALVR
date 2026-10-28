// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRIOBuffer_002 interface version.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#include "common/openvr_error.hpp"
#include "common/openvr_logger.hpp"

#include "XRTVRIOBuffer.hpp"


namespace xrt::state_trackers::openvr {

XRTVRIOBuffer_002::XRTVRIOBuffer_002(XRTVRClientCore_003 *clientCore) : core(clientCore)
{
	(void)this->core; // silence warning for now, since nothing is implemented yet.
}

vr::EIOBufferError
XRTVRIOBuffer_002::Open(const char *pchPath,
                        vr::EIOBufferMode mode,
                        uint32_t unElementSize,
                        uint32_t unElements,
                        vr::IOBufferHandle_t *pulBuffer)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "Open(pchPath=%s, mode=%llu, unElementSize=%llu, unElements=%llu, pulBuffer=%p) -> %llu",
	    static_cast<vr::EIOBufferError>(0), pchPath != nullptr ? pchPath : "(null)",
	    static_cast<unsigned long long>(mode), static_cast<unsigned long long>(unElementSize),
	    static_cast<unsigned long long>(unElements), static_cast<void *>(pulBuffer),
	    static_cast<unsigned long long>(static_cast<vr::EIOBufferError>(0)));
}

vr::EIOBufferError
XRTVRIOBuffer_002::Close(vr::IOBufferHandle_t ulBuffer)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "Close(ulBuffer=%llu) -> %llu", static_cast<vr::EIOBufferError>(0),
	                             static_cast<unsigned long long>(ulBuffer),
	                             static_cast<unsigned long long>(static_cast<vr::EIOBufferError>(0)));
}

vr::EIOBufferError
XRTVRIOBuffer_002::Read(vr::IOBufferHandle_t ulBuffer, void *pDst, uint32_t unBytes, uint32_t *punRead)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "Read(ulBuffer=%llu, pDst=%p, unBytes=%llu, punRead=%p) -> %llu",
	                             static_cast<vr::EIOBufferError>(0), static_cast<unsigned long long>(ulBuffer),
	                             static_cast<void *>(pDst), static_cast<unsigned long long>(unBytes),
	                             static_cast<void *>(punRead),
	                             static_cast<unsigned long long>(static_cast<vr::EIOBufferError>(0)));
}

vr::EIOBufferError
XRTVRIOBuffer_002::Write(vr::IOBufferHandle_t ulBuffer, void *pSrc, uint32_t unBytes)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "Write(ulBuffer=%llu, pSrc=%p, unBytes=%llu) -> %llu",
	                             static_cast<vr::EIOBufferError>(0), static_cast<unsigned long long>(ulBuffer),
	                             static_cast<void *>(pSrc), static_cast<unsigned long long>(unBytes),
	                             static_cast<unsigned long long>(static_cast<vr::EIOBufferError>(0)));
}

vr::PropertyContainerHandle_t
XRTVRIOBuffer_002::PropertyContainer(vr::IOBufferHandle_t ulBuffer)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "PropertyContainer(ulBuffer=%llu) -> %llu",
	                             static_cast<vr::PropertyContainerHandle_t>(0),
	                             static_cast<unsigned long long>(ulBuffer),
	                             static_cast<unsigned long long>(static_cast<vr::PropertyContainerHandle_t>(0)));
}

bool
XRTVRIOBuffer_002::HasReaders(vr::IOBufferHandle_t ulBuffer)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "HasReaders(ulBuffer=%llu) -> %d", false,
	                             static_cast<unsigned long long>(ulBuffer), false);
}

}; // namespace xrt::state_trackers::openvr
