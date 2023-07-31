// Copyright 2020-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Interface for IPC client instance code.
 * @author Pete Black <pblack@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup ipc_client
 */

#include "xrt/xrt_results.h"

#ifdef __cplusplus
extern "C" {
#endif


struct xrt_instance;
struct xrt_instance_info;

/*!
 * Create a IPC client instance, connects to a IPC server.
 *
 * @see ipc_design
 * @ingroup ipc_client
 */
xrt_result_t
ipc_instance_create(struct xrt_instance_info *ii, struct xrt_instance **out_xinst);


#ifdef __cplusplus
}
#endif
