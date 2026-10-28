// Copyright 2025, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Helper to implement @ref xrt_future,
 *         A basic CPU based future implementation.
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup aux_util
 */
#pragma once

#include "xrt/xrt_future.h"

#ifdef __cplusplus
extern "C" {
#endif

struct xrt_future *
u_future_create(void);

#ifdef __cplusplus
}
#endif
