// Copyright 2020-2024, Collabora, Ltd.
// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Client side wrapper of instance.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup ipc_client
 */

#include "xrt/xrt_results.h"
#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "xrt/xrt_instance.h"
#include "xrt/xrt_handles.h"
#include "xrt/xrt_config_os.h"
#include "xrt/xrt_config_android.h"

#include "util/u_var.h"
#include "util/u_misc.h"
#include "util/u_file.h"
#include "util/u_debug.h"
#include "util/u_git_tag.h"

#include "shared/ipc_protocol.h"
#include "shared/ipc_shmem.h"
#include "client/ipc_client.h"
#include "client/ipc_client_system_devices.h"
#include "client/ipc_client_interface.h"
#include "client/ipc_client_connection.h"
#include "client/ipc_client_tracking_origin.h"

#include "ipc_client_generated.h"


#include <stdio.h>
#if defined(XRT_OS_WINDOWS)
#include <timeapi.h>
#else
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#endif
#include <limits.h>

#ifdef XRT_GRAPHICS_BUFFER_HANDLE_IS_AHARDWAREBUFFER
#include "android/android_ahardwarebuffer_allocator.h"
#endif

#ifdef XRT_OS_ANDROID
#include "xrt/xrt_android.h"
#include "android/ipc_client_android.h"
#include "android/android_instance_base.h"
#endif // XRT_OS_ANDROID

DEBUG_GET_ONCE_LOG_OPTION(ipc_log, "IPC_LOG", U_LOGGING_WARN)


/*
 *
 * Struct and helpers.
 *
 */

/*!
 * @implements xrt_instance
 */
struct ipc_client_instance
{
	//! @public Base
	struct xrt_instance base;

	struct ipc_connection ipc_c;

#ifdef XRT_OS_ANDROID
	struct android_instance_base android;
#endif
};

static inline struct ipc_client_instance *
ipc_client_instance(struct xrt_instance *xinst)
{
	return (struct ipc_client_instance *)xinst;
}

static xrt_result_t
create_system_compositor(struct ipc_client_instance *ii,
                         struct xrt_device *xdev,
                         struct xrt_system_compositor **out_xsysc)
{
	struct xrt_system_compositor *xsysc = NULL;
	struct xrt_image_native_allocator *xina = NULL;
	xrt_result_t xret;

#ifdef XRT_GRAPHICS_BUFFER_HANDLE_IS_AHARDWAREBUFFER
	// On Android, we allocate images natively on the client side.
	xina = android_ahardwarebuffer_allocator_create();
#endif // XRT_GRAPHICS_BUFFER_HANDLE_IS_AHARDWAREBUFFER

	xret = ipc_client_create_system_compositor(&ii->ipc_c, xina, xdev, &xsysc);
	IPC_CHK_WITH_GOTO(&ii->ipc_c, xret, "ipc_client_create_system_compositor", err_xina);

	// Paranoia.
	if (xsysc == NULL) {
		xret = XRT_ERROR_IPC_FAILURE;
		IPC_ERROR(&ii->ipc_c, "Variable xsysc NULL!");
		goto err_xina;
	}

	*out_xsysc = xsysc;

	return XRT_SUCCESS;

err_xina:
	xrt_images_destroy(&xina);
	return xret;
}


/*
 *
 * Member functions.
 *
 */

static xrt_result_t
ipc_client_instance_is_system_available(struct xrt_instance *xinst, bool *out_available)
{
	struct ipc_client_instance *ii = ipc_client_instance(xinst);
	xrt_result_t xret = ipc_call_instance_is_system_available(&ii->ipc_c, out_available);
	IPC_CHK_ALWAYS_RET(&ii->ipc_c, xret, "ipc_call_instance_is_system_available");
}

static xrt_result_t
ipc_client_instance_create_system(struct xrt_instance *xinst,
                                  struct xrt_system **out_xsys,
                                  struct xrt_system_devices **out_xsysd,
                                  struct xrt_space_overseer **out_xso,
                                  struct xrt_system_compositor **out_xsysc)
{
	struct ipc_client_instance *ii = ipc_client_instance(xinst);
	xrt_result_t xret = XRT_SUCCESS;

	assert(out_xsys != NULL);
	assert(*out_xsys == NULL);
	assert(out_xsysd != NULL);
	assert(*out_xsysd == NULL);
	assert(out_xsysc == NULL || *out_xsysc == NULL);

	struct xrt_system_compositor *xsysc = NULL;

	// Allocate a helper xrt_system_devices struct.
	struct ipc_client_system_devices *icsd = NULL;
	xret = ipc_client_system_devices_create(&ii->ipc_c, &icsd);
	IPC_CHK_AND_RET(&ii->ipc_c, xret, "ipc_client_system_devices_create");

	struct xrt_system_devices *xsysd = &icsd->base.base;

	// Query the server for the list of devices
	struct ipc_device_list device_list = {0};
	xret = ipc_call_system_devices_get_list(&ii->ipc_c, &device_list);
	IPC_CHK_AND_RET(&ii->ipc_c, xret, "ipc_call_system_devices_get_list");

	// Create client devices for each device in the list
	uint32_t count = 0;
	struct ipc_client_tracking_origin_manager *ictom = &icsd->tracking_origin_manager;
	for (uint32_t i = 0; i < device_list.device_count; i++) {
		struct ipc_device_list_entry *entry = &device_list.devices[i];

		// Create the appropriate device type
		if (entry->device_type == XRT_DEVICE_TYPE_HMD) {
			xsysd->static_xdevs[count] = ipc_client_hmd_create(&ii->ipc_c, ictom, entry->id);
		} else {
			xsysd->static_xdevs[count] = ipc_client_device_create(&ii->ipc_c, ictom, entry->id);
		}

		// Check if device creation succeeded
		if (xsysd->static_xdevs[count] != NULL) {
			count++;
		} else {
			IPC_ERROR(&ii->ipc_c, "Failed to create device %u", i);
		}
	}
	xsysd->static_xdev_count = count;

#define SET_ROLE(ROLE)                                                                                                 \
	do {                                                                                                           \
		int32_t index = ii->ipc_c.ism->roles.ROLE;                                                             \
		xsysd->static_roles.ROLE = index >= 0 ? xsysd->static_xdevs[index] : NULL;                             \
	} while (false)

	SET_ROLE(head);
	SET_ROLE(eyes);
	SET_ROLE(face);
	SET_ROLE(body);
	SET_ROLE(hand_tracking.unobstructed.left);
	SET_ROLE(hand_tracking.unobstructed.right);
	SET_ROLE(hand_tracking.conforming.left);
	SET_ROLE(hand_tracking.conforming.right);

#undef SET_ROLE

	// Done here now.
	if (out_xsysc == NULL) {
		goto out;
	}

	if (xsysd->static_roles.head == NULL) {
		IPC_ERROR((&ii->ipc_c), "No head device found but asking for system compositor!");
		xret = XRT_ERROR_IPC_FAILURE;
		goto err_destroy;
	}

	xret = create_system_compositor(ii, xsysd->static_roles.head, &xsysc);
	if (xret != XRT_SUCCESS) {
		goto err_destroy;
	}

out:
	*out_xsys = ipc_client_system_create(&ii->ipc_c, xsysc);
	*out_xsysd = xsysd;
	*out_xso = ipc_client_space_overseer_create(&ii->ipc_c);

	if (xsysc != NULL) {
		assert(out_xsysc != NULL);
		*out_xsysc = xsysc;
	}

	return XRT_SUCCESS;

err_destroy:
	xrt_system_devices_destroy(&xsysd);

	return xret;
}

static xrt_result_t
ipc_client_instance_get_prober(struct xrt_instance *xinst, struct xrt_prober **out_xp)
{
	*out_xp = NULL;

	return XRT_ERROR_PROBER_NOT_SUPPORTED;
}

static xrt_result_t
ipc_client_instance_create_app_instance(struct xrt_instance *xinst, struct xrt_app_instance **out_xainst)
{
	struct ipc_client_instance *ii = ipc_client_instance(xinst);

	struct xrt_app_instance *xainst = ipc_client_create_app_instance(&ii->ipc_c);
	if (xainst == NULL) {
		return XRT_ERROR_IPC_FAILURE;
	}

	(*out_xainst) = xainst;

	return XRT_SUCCESS;
}

static void
ipc_client_instance_destroy(struct xrt_instance *xinst)
{
	struct ipc_client_instance *ii = ipc_client_instance(xinst);

	// service considers us to be connected until fd is closed
	ipc_client_connection_fini(&ii->ipc_c);

#ifdef XRT_OS_ANDROID
	android_instance_base_cleanup(&(ii->android), xinst);
	ipc_client_android_destroy(&(ii->ipc_c.ica));
#endif // XRT_OS_ANDROID

#ifdef XRT_OS_WINDOWS
	timeEndPeriod(1);
#endif

	ipc_shmem_destroy(&ii->ipc_c.ism_handle, (void **)&ii->ipc_c.ism, sizeof(struct ipc_shared_memory));

	free(ii);
}


/*
 *
 * Exported function(s).
 *
 */

/*!
 * Constructor for xrt_instance IPC client proxy.
 *
 * @public @memberof ipc_instance
 */
xrt_result_t
ipc_instance_create(const struct xrt_instance_info *i_info, struct xrt_instance **out_xinst)
{
	struct ipc_client_instance *ii = U_TYPED_CALLOC(struct ipc_client_instance);
	ii->base.is_system_available = ipc_client_instance_is_system_available;
	ii->base.create_system = ipc_client_instance_create_system;
	ii->base.get_prober = ipc_client_instance_get_prober;
	ii->base.create_app_instance = ipc_client_instance_create_app_instance;
	ii->base.destroy = ipc_client_instance_destroy;

#ifdef XRT_OS_WINDOWS
	timeBeginPeriod(1);
#endif

	xrt_result_t xret;
#ifdef XRT_OS_ANDROID
	xret = android_instance_base_init(&ii->android, &ii->base, i_info);
	if (xret != XRT_SUCCESS) {
		free(ii);
		return xret;
	}
#endif

	xret = ipc_client_connection_init(&ii->ipc_c, debug_get_log_option_ipc_log(), i_info);
	if (xret != XRT_SUCCESS) {
#ifdef XRT_OS_ANDROID
		android_instance_base_cleanup(&(ii->android), &(ii->base));
#endif
		free(ii);
		return xret;
	}

	ii->base.startup_timestamp = ii->ipc_c.ism->startup_timestamp;

	*out_xinst = &ii->base;

	return XRT_SUCCESS;
}
