// Copyright 2020-2023, Collabora, Ltd.
// Copyright 2025-2026, NVIDIA CORPORATION.
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
#include "shared/ipc_utils.h"
#include "shared/ipc_protocol.h"
#include "shared/ipc_message_channel.h"

#include "ipc_client_tracking_origin.h"

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

#define IPC_CHK_AND_RET(IPC_C, ...) U_LOG_CHK_AND_RET((IPC_C)->imc.log_level, __VA_ARGS__)
#define IPC_CHK_WITH_GOTO(IPC_C, ...) U_LOG_CHK_WITH_GOTO((IPC_C)->imc.log_level, __VA_ARGS__)
#define IPC_CHK_WITH_RET(IPC_C, ...) U_LOG_CHK_WITH_RET((IPC_C)->imc.log_level, __VA_ARGS__)
#define IPC_CHK_ONLY_PRINT(IPC_C, ...) U_LOG_CHK_ONLY_PRINT((IPC_C)->imc.log_level, __VA_ARGS__)
#define IPC_CHK_ALWAYS_RET(IPC_C, ...) U_LOG_CHK_ALWAYS_RET((IPC_C)->imc.log_level, __VA_ARGS__)


/*
 *
 * Structs
 *
 */

struct xrt_compositor_native;
struct ipc_client_system_devices;


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


/*
 *
 * Internal functions.
 *
 */

/*!
 * Create an IPC client system compositor.
 *
 * It owns a special implementation of the @ref xrt_system_compositor interface.
 *
 * This actually creates an IPC client "native" compositor with deferred
 * initialization. The @ref ipc_client_create_native_compositor function
 * actually completes the deferred initialization of the compositor, effectively
 * finishing creation of a compositor IPC proxy.
 *
 * @param ipc_c IPC connection
 * @param xina Optional native image allocator for client-side allocation. Takes
 * ownership if one is supplied.
 * @param xdev Taken in but not used currently @todo remove this param?
 * @param[out] out_xcs Pointer to receive the created xrt_system_compositor.
 */
xrt_result_t
ipc_client_create_system_compositor(struct ipc_connection *ipc_c,
                                    struct xrt_image_native_allocator *xina,
                                    struct xrt_device *xdev,
                                    struct xrt_system_compositor **out_xcs);

/*!
 * Create a native compositor from a system compositor, this is used instead
 * of the normal xrt_system_compositor::create_native_compositor function
 * because it doesn't support events being generated on the app side. This will
 * also create the session on the service side.
 *
 * @param xsysc        IPC created system compositor.
 * @param xsi          Session information struct.
 * @param[out] out_xcn Pointer to receive the created xrt_compositor_native.
 */
xrt_result_t
ipc_client_create_native_compositor(struct xrt_system_compositor *xsysc,
                                    const struct xrt_session_info *xsi,
                                    struct xrt_compositor_native **out_xcn);

struct xrt_device *
ipc_client_hmd_create(struct ipc_connection *ipc_c,
                      struct ipc_client_tracking_origin_manager *ictom,
                      uint32_t device_id);

struct xrt_device *
ipc_client_device_create(struct ipc_connection *ipc_c,
                         struct ipc_client_tracking_origin_manager *ictom,
                         uint32_t device_id);

struct xrt_system *
ipc_client_system_create(struct ipc_connection *ipc_c, struct xrt_system_compositor *xsysc);

struct xrt_space_overseer *
ipc_client_space_overseer_create(struct ipc_connection *ipc_c);

uint32_t
ipc_client_space_get_id(struct xrt_space *space);

/*!
 * Wrap a service-side space id in a client @ref xrt_space handle.
 *
 * @param ipc_c     Connection the space id belongs to.
 * @param id        Service-side space id.
 * @param out_space Pointer to receive the created client space.
 */
void
ipc_client_space_create(struct ipc_connection *ipc_c, uint32_t id, struct xrt_space **out_space);

xrt_result_t
ipc_client_system_devices_create(struct ipc_connection *ipc_c, struct ipc_client_system_devices **out_icsd);

struct xrt_session *
ipc_client_session_create(struct ipc_connection *ipc_c);

struct xrt_future *
ipc_client_future_create(struct ipc_connection *ipc_c, uint32_t future_id);

struct xrt_app_instance *
ipc_client_create_app_instance(struct ipc_connection *ipc_c);
