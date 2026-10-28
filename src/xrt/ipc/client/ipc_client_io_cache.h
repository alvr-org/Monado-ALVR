// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief IPC client @ref xrt_io_cache proxy.
 * @ingroup ipc_client
 */

#pragma once

#include "xrt/xrt_io_cache.h"


#ifdef __cplusplus
extern "C" {
#endif

struct ipc_connection;

xrt_result_t
ipc_client_io_cache_create(struct ipc_connection *ipc_c, uint32_t id, struct xrt_io_cache **out_xic);

#ifdef __cplusplus
}
#endif
