// Copyright 2026, Beyley Cardellio
// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Not implemented function helpers for system devices.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup aux_util
 */

#include "util/u_system_devices_ni.h"
#include "util/u_logging.h"


/*
 *
 * Not implemented function helpers.
 *
 */

#define E(FN) U_LOG_E("Function " #FN " is not implemented for this system devices implementation")

xrt_result_t
u_system_devices_ni_create_body_tracker(struct xrt_system_devices *xsysd,
                                        const struct xrt_body_tracker_create_info *info,
                                        struct xrt_body_tracker **out_xbt)
{
	E(create_body_tracker);
	return XRT_ERROR_FEATURE_NOT_SUPPORTED;
}

xrt_result_t
u_system_devices_ni_create_hand_tracker(struct xrt_system_devices *xsysd,
                                        const struct xrt_hand_tracker_create_info *info,
                                        struct xrt_hand_tracker **out_xht)
{
	E(create_hand_tracker);
	return XRT_ERROR_FEATURE_NOT_SUPPORTED;
}

xrt_result_t
u_system_devices_ni_create_io_cache(struct xrt_system_devices *xsysd,
                                    struct xrt_space_overseer *xso,
                                    struct xrt_io_cache **out_xic)
{
	E(create_io_cache);
	return XRT_ERROR_FEATURE_NOT_SUPPORTED;
}

xrt_result_t
u_system_devices_ni_feature_inc(struct xrt_system_devices *xsysd, enum xrt_device_feature_type type)
{
	E(feature_inc);
	return XRT_ERROR_FEATURE_NOT_SUPPORTED;
}

xrt_result_t
u_system_devices_ni_feature_dec(struct xrt_system_devices *xsysd, enum xrt_device_feature_type type)
{
	E(feature_dec);
	return XRT_ERROR_FEATURE_NOT_SUPPORTED;
}
