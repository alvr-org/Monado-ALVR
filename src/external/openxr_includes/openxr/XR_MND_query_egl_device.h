// Copyright 2026, Stanislav Aleksandrov
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Preview header for XR_MND_query_egl_device extension.
 * @author Stanislav Aleksandrov <lightofmysoul@gmail.com>
 * @ingroup external_openxr
 */
#ifndef XR_MND_QUERY_EGL_DEVICE_H
#define XR_MND_QUERY_EGL_DEVICE_H 1

#include "openxr_extension_helpers.h"

#ifdef XR_USE_PLATFORM_EGL

#ifdef __cplusplus
extern "C" {
#endif

#define XR_MND_query_egl_device 1
#define XR_MND_query_egl_device_SPEC_VERSION 1
#define XR_MND_QUERY_EGL_DEVICE_EXTENSION_NAME "XR_MND_query_egl_device"

XR_RESULT_ENUM(XR_ERROR_EGL_EXTENSION_NOT_AVAILABLE_MND, -1000445001);
XR_RESULT_ENUM(XR_ERROR_EGL_DEVICE_NOT_FOUND_MND, -1000445002);


XR_STRUCT_ENUM(XR_TYPE_SYSTEM_EGL_DEVICE_GET_INFO_MND, 1000445001);
typedef struct XrSystemEGLDeviceGetInfoMND {
    XrStructureType             type;
    const void* XR_MAY_ALIAS    next;
    XrSystemId                  systemId;
    PFN_xrEglGetProcAddressMNDX getProcAddress;
} XrSystemEGLDeviceGetInfoMND;

XR_STRUCT_ENUM(XR_TYPE_SYSTEM_EGL_DEVICE_MND, 1000445002);
typedef struct XrSystemEGLDeviceMND {
    XrStructureType       type;
    void* XR_MAY_ALIAS    next;
    EGLDeviceEXT          eglDevice;
} XrSystemEGLDeviceMND;


typedef XrResult (XRAPI_PTR *PFN_xrGetSystemEGLDeviceMND)(
    XrInstance instance,
    const XrSystemEGLDeviceGetInfoMND *info,
    XrSystemEGLDeviceMND *device);

#ifndef XR_NO_PROTOTYPES
#ifdef XR_EXTENSION_PROTOTYPES
XRAPI_ATTR XrResult XRAPI_CALL xrGetSystemEGLDeviceMND(
    XrInstance instance,
    const XrSystemEGLDeviceGetInfoMND *info,
    XrSystemEGLDeviceMND *device);
#endif /* XR_EXTENSION_PROTOTYPES */
#endif /* !XR_NO_PROTOTYPES */


#ifdef __cplusplus
}
#endif

#endif /* XR_USE_PLATFORM_EGL */

#endif
