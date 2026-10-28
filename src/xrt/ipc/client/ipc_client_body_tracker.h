// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief IPC client @ref xrt_body_tracker proxy.
 * @ingroup ipc_client
 */

#pragma once

#include "xrt/xrt_body_tracker.h"


#ifdef __cplusplus
extern "C" {
#endif

struct ipc_connection;

xrt_result_t
ipc_client_body_tracker_create(struct ipc_connection *ipc_c,
                               uint32_t id,
                               const struct xrt_body_tracker_supported *supported,
                               struct xrt_body_tracker **out_xbt);

#ifdef __cplusplus
}
#endif
