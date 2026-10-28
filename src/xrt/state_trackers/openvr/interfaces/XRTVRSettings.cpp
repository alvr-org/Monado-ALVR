// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of the latest IVRSettings_003 interface version.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_interfaces
 */

#include "common/openvr_error.hpp"
#include "common/openvr_logger.hpp"

#include "XRTVRSettings.hpp"


namespace xrt::state_trackers::openvr {

XRTVRSettings_003::XRTVRSettings_003(XRTVRClientCore_003 *clientCore) : core(clientCore)
{
	(void)this->core; // silence warning for now, since nothing is implemented yet.
}

const char *
XRTVRSettings_003::GetSettingsErrorNameFromEnum(EVRSettingsError eError)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetSettingsErrorNameFromEnum(eError=%llu) -> %s",
	                             "TODO: GetSettingsErrorNameFromEnum", static_cast<unsigned long long>(eError),
	                             "TODO: GetSettingsErrorNameFromEnum");
}

void
XRTVRSettings_003::SetBool(const char *pchSection, const char *pchSettingsKey, bool bValue, EVRSettingsError *peError)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(logger, "SetBool(pchSection=%s, pchSettingsKey=%s, bValue=%d, peError=%p)",
	                         pchSection != nullptr ? pchSection : "(null)",
	                         pchSettingsKey != nullptr ? pchSettingsKey : "(null)", static_cast<int>(bValue),
	                         static_cast<void *>(peError));
}

void
XRTVRSettings_003::SetInt32(const char *pchSection,
                            const char *pchSettingsKey,
                            int32_t nValue,
                            EVRSettingsError *peError)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(logger, "SetInt32(pchSection=%s, pchSettingsKey=%s, nValue=%llu, peError=%p)",
	                         pchSection != nullptr ? pchSection : "(null)",
	                         pchSettingsKey != nullptr ? pchSettingsKey : "(null)",
	                         static_cast<unsigned long long>(nValue), static_cast<void *>(peError));
}

void
XRTVRSettings_003::SetFloat(const char *pchSection,
                            const char *pchSettingsKey,
                            float flValue,
                            EVRSettingsError *peError)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(logger, "SetFloat(pchSection=%s, pchSettingsKey=%s, flValue=%f, peError=%p)",
	                         pchSection != nullptr ? pchSection : "(null)",
	                         pchSettingsKey != nullptr ? pchSettingsKey : "(null)", static_cast<double>(flValue),
	                         static_cast<void *>(peError));
}

void
XRTVRSettings_003::SetString(const char *pchSection,
                             const char *pchSettingsKey,
                             const char *pchValue,
                             EVRSettingsError *peError)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(logger, "SetString(pchSection=%s, pchSettingsKey=%s, pchValue=%s, peError=%p)",
	                         pchSection != nullptr ? pchSection : "(null)",
	                         pchSettingsKey != nullptr ? pchSettingsKey : "(null)",
	                         pchValue != nullptr ? pchValue : "(null)", static_cast<void *>(peError));
}

bool
XRTVRSettings_003::GetBool(const char *pchSection, const char *pchSettingsKey, EVRSettingsError *peError)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetBool(pchSection=%s, pchSettingsKey=%s, peError=%p) -> %d", false,
	                             pchSection != nullptr ? pchSection : "(null)",
	                             pchSettingsKey != nullptr ? pchSettingsKey : "(null)",
	                             static_cast<void *>(peError), false);
}

int32_t
XRTVRSettings_003::GetInt32(const char *pchSection, const char *pchSettingsKey, EVRSettingsError *peError)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(
	    logger, "GetInt32(pchSection=%s, pchSettingsKey=%s, peError=%p) -> %llu", static_cast<int32_t>(0),
	    pchSection != nullptr ? pchSection : "(null)", pchSettingsKey != nullptr ? pchSettingsKey : "(null)",
	    static_cast<void *>(peError), static_cast<unsigned long long>(static_cast<int32_t>(0)));
}

float
XRTVRSettings_003::GetFloat(const char *pchSection, const char *pchSettingsKey, EVRSettingsError *peError)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED_RET(logger, "GetFloat(pchSection=%s, pchSettingsKey=%s, peError=%p) -> %f", 0.0f,
	                             pchSection != nullptr ? pchSection : "(null)",
	                             pchSettingsKey != nullptr ? pchSettingsKey : "(null)",
	                             static_cast<void *>(peError), 0.0);
}

void
XRTVRSettings_003::GetString(
    const char *pchSection, const char *pchSettingsKey, char *pchValue, uint32_t unValueLen, EVRSettingsError *peError)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(
	    logger, "GetString(pchSection=%s, pchSettingsKey=%s, pchValue=%p, unValueLen=%llu, peError=%p)",
	    pchSection != nullptr ? pchSection : "(null)", pchSettingsKey != nullptr ? pchSettingsKey : "(null)",
	    static_cast<void *>(pchValue), static_cast<unsigned long long>(unValueLen), static_cast<void *>(peError));
}

void
XRTVRSettings_003::RemoveSection(const char *pchSection, EVRSettingsError *peError)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(logger, "RemoveSection(pchSection=%s, peError=%p)",
	                         pchSection != nullptr ? pchSection : "(null)", static_cast<void *>(peError));
}

void
XRTVRSettings_003::RemoveKeyInSection(const char *pchSection, const char *pchSettingsKey, EVRSettingsError *peError)
{
	openvr_logger logger;
	OPENVR_LOGGER_INIT(logger);

	OPENVR_LOG_UNIMPLEMENTED(logger, "RemoveKeyInSection(pchSection=%s, pchSettingsKey=%s, peError=%p)",
	                         pchSection != nullptr ? pchSection : "(null)",
	                         pchSettingsKey != nullptr ? pchSettingsKey : "(null)", static_cast<void *>(peError));
}

}; // namespace xrt::state_trackers::openvr
