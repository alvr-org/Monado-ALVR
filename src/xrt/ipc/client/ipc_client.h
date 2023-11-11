// Copyright 2020-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Common client side code.
 * @author Pete Black <pblack@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup ipc_client
 */

#pragma once

#include "xrt/xrt_compiler.h"
#include "xrt/xrt_config_os.h"

#include "util/u_threading.h"
#include "util/u_logging.h"

#include "shared/ipc_protocol.h"
#include "shared/ipc_message_channel.h"

#include <stdio.h>


/*
 *
 * Logging
 *
 */

#define IPC_TRACE(IPC_C, ...) U_LOG_IFL_T((IPC_C)->imc.log_level, __VA_ARGS__)
#define IPC_DEBUG(IPC_C, ...) U_LOG_IFL_D((IPC_C)->imc.log_level, __VA_ARGS__)
#define IPC_INFO(IPC_C, ...) U_LOG_IFL_I((IPC_C)->imc.log_level, __VA_ARGS__)
#define IPC_WARN(IPC_C, ...) U_LOG_IFL_W((IPC_C)->imc.log_level, __VA_ARGS__)
#define IPC_ERROR(IPC_C, ...) U_LOG_IFL_E((IPC_C)->imc.log_level, __VA_ARGS__)

/*
 *
 * Structs
 *
 */

struct xrt_compositor_native;


/*!
 * Connection.
 */
struct ipc_connection
{
	struct ipc_message_channel imc;

	struct ipc_shared_memory *ism;
	xrt_shmem_handle_t ism_handle;

	struct os_mutex mutex;

#ifdef XRT_OS_ANDROID
	struct ipc_client_android *ica;
#endif // XRT_OS_ANDROID
};

/*!
 * An IPC client proxy for an @ref xrt_device.
 *
 * @implements xrt_device
 * @ingroup ipc_client
 */
struct ipc_client_xdev
{
	struct xrt_device base;

	struct ipc_connection *ipc_c;

	uint32_t device_id;
};


/*
 *
 * Internal functions.
 *
 */

/*!
 * Convenience helper to go from a xdev to @ref ipc_client_xdev.
 *
 * @ingroup ipc_client
 */
static inline struct ipc_client_xdev *
ipc_client_xdev(struct xrt_device *xdev)
{
	return (struct ipc_client_xdev *)xdev;
}

/*!
 * Create an IPC client system compositor.
 *
 * @param ipc_c IPC connection
 * @param xina Optional native image allocator for client-side allocation. Takes
 * ownership if one is supplied.
 * @param xdev Taken in but not used currently @todo remove this param?
 * @param[out] out_xcs Pointer to receive the created xrt_system_compositor.
 */
int
ipc_client_create_system_compositor(struct ipc_connection *ipc_c,
                                    struct xrt_image_native_allocator *xina,
                                    struct xrt_device *xdev,
                                    struct xrt_system_compositor **out_xcs);

struct xrt_device *
ipc_client_hmd_create(struct ipc_connection *ipc_c, struct xrt_tracking_origin *xtrack, uint32_t device_id);

struct xrt_device *
ipc_client_device_create(struct ipc_connection *ipc_c, struct xrt_tracking_origin *xtrack, uint32_t device_id);

struct xrt_space_overseer *
ipc_client_space_overseer_create(struct ipc_connection *ipc_c);

struct xrt_system_devices *
ipc_client_system_devices_create(struct ipc_connection *ipc_c);
