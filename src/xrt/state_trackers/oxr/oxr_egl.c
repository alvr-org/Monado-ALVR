// Copyright 2026, Stanislav Aleksandrov
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  EGL graphics device selection for the OpenXR state tracker.
 * @author Stanislav Aleksandrov <lightofmysoul@gmail.com>
 * @ingroup oxr_main
 */

#define EGL_NO_X11              // libglvnd
#define MESA_EGL_NO_X11_HEADERS // mesa
#include <EGL/egl.h>

#include "oxr_objects.h"
#include "oxr_logger.h"

#include <string.h>


#ifndef XR_USE_PLATFORM_EGL
#error "Must build this file with EGL enabled!"
#endif


/*
 *
 * EGL device extension types.
 *
 * We define these inline rather than depending on EGL extension headers,
 * since we load the functions dynamically via getProcAddress.
 *
 */

typedef EGLBoolean(EGLAPIENTRYP PFNEGLQUERYDEVICESEXTPROC_)(EGLint max_devices,
                                                            EGLDeviceEXT *devices,
                                                            EGLint *num_devices);

typedef EGLBoolean(EGLAPIENTRYP PFNEGLQUERYDEVICEBINARYEXTPROC_)(
    EGLDeviceEXT device, EGLint name, EGLint max_size, void *value, EGLint *size);

#ifndef EGL_DEVICE_UUID_EXT
#define EGL_DEVICE_UUID_EXT 0x335C
#endif

#define MAX_EGL_DEVICES 16


/*
 *
 * Exported function.
 *
 */

XrResult
oxr_egl_get_device(struct oxr_logger *log,
                   struct oxr_system *sys,
                   PFN_xrEglGetProcAddressMNDX getProcAddress,
                   EGLDeviceEXT *out_egl_device)
{
	// Load EGL device functions.
	PFNEGLQUERYDEVICESEXTPROC_ eglQueryDevicesEXT =
	    (PFNEGLQUERYDEVICESEXTPROC_)getProcAddress("eglQueryDevicesEXT");
	PFNEGLQUERYDEVICEBINARYEXTPROC_ eglQueryDeviceBinaryEXT =
	    (PFNEGLQUERYDEVICEBINARYEXTPROC_)getProcAddress("eglQueryDeviceBinaryEXT");

	if (eglQueryDevicesEXT == NULL) {
		return oxr_error(log, XR_ERROR_EGL_EXTENSION_NOT_AVAILABLE_MND,
		                 "EGL_EXT_device_enumeration not available "
		                 "(eglQueryDevicesEXT not found)");
	}

	if (eglQueryDeviceBinaryEXT == NULL) {
		return oxr_error(log, XR_ERROR_EGL_EXTENSION_NOT_AVAILABLE_MND,
		                 "EGL_EXT_device_persistent_id not available "
		                 "(eglQueryDeviceBinaryEXT not found)");
	}

	// Enumerate EGL devices.
	EGLDeviceEXT devices[MAX_EGL_DEVICES];
	EGLint num_devices = 0;

	if (!eglQueryDevicesEXT(MAX_EGL_DEVICES, devices, &num_devices) || num_devices == 0) {
		return oxr_error(log, XR_ERROR_EGL_DEVICE_NOT_FOUND_MND,
		                 "eglQueryDevicesEXT failed or returned 0 devices");
	}

	// Match by device UUID.
	const xrt_uuid_t *target_uuid = &sys->xsysc->info.client_vk_deviceUUID;

	for (EGLint i = 0; i < num_devices; i++) {
		uint8_t uuid[XRT_UUID_SIZE] = {0};
		EGLint size = 0;

		if (!eglQueryDeviceBinaryEXT(devices[i], EGL_DEVICE_UUID_EXT, sizeof(uuid), uuid, &size)) {
			continue;
		}
		if (size != XRT_UUID_SIZE) {
			continue;
		}

		if (memcmp(uuid, target_uuid->data, XRT_UUID_SIZE) == 0) {
			oxr_log(log, "Matched EGL device %d by UUID", i);
			*out_egl_device = devices[i];
			return XR_SUCCESS;
		}
	}

	return oxr_error(log, XR_ERROR_EGL_DEVICE_NOT_FOUND_MND, "No EGL device matched the compositor's device UUID");
}
