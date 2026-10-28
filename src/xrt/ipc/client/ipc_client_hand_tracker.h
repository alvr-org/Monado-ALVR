// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief IPC client @ref xrt_hand_tracker proxy.
 * @ingroup ipc_client
 */

#pragma once

#include "xrt/xrt_hand_tracker.h"


#ifdef __cplusplus
extern "C" {
#endif

struct ipc_connection;

xrt_result_t
ipc_client_hand_tracker_create(struct ipc_connection *ipc_c, uint32_t id, struct xrt_hand_tracker **out_xht);

#ifdef __cplusplus
}
#endif
