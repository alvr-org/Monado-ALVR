// Copyright 2019-2022, Collabora, Ltd.
// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header for limits of the XRT interfaces.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup xrt_iface
 */

#pragma once

#include "xrt/xrt_config_os.h"
#include "xrt/xrt_compiler.h"


/*!
 * @addtogroup xrt_iface
 * @{
 */

/*!
 * Maximum number of devices simultaneously usable by an implementation of
 * @ref xrt_system_devices.
 */
#define XRT_SYSTEM_MAX_DEVICES (32)

/*
 * Max number of views supported by a compositor, artificial limit.
 */
#define XRT_MAX_VIEWS 2

/*
 * System needs to support at least 4 views for stereo with foveated inset.
 */
#define XRT_MAX_COMPOSITOR_VIEW_CONFIGS_VIEW_COUNT (XRT_MAX_VIEWS > 4 ? XRT_MAX_VIEWS : 4)

/*
 * Max number of view configurations a system compositor can support simultaneously.
 */
#define XRT_MAX_COMPOSITOR_VIEW_CONFIGS_COUNT 2

/*!
 * Maximum number of handles sent in one call.
 */
#define XRT_MAX_IPC_HANDLES 16

/*!
 * Max swapchain images, artificial limit.
 *
 * Must be smaller or the same as XRT_MAX_IPC_HANDLES.
 */
#define XRT_MAX_SWAPCHAIN_IMAGES 8

/*!
 * Max formats supported by a compositor, artificial limit.
 */
#define XRT_MAX_SWAPCHAIN_FORMATS 16

/*!
 * Max number of plane orientations that can be requested at a time.
 */
#define XRT_MAX_PLANE_ORIENTATIONS_EXT 256

/*!
 * Max number of plane semantic types that can be requested at a time.
 */
#define XRT_MAX_PLANE_SEMANTIC_TYPE_EXT 256

/*!
 * Max formats in the swapchain creation info formats list, artificial limit.
 */
#define XRT_MAX_SWAPCHAIN_CREATE_INFO_FORMAT_LIST_COUNT 8

/*!
 * Max number of supported display refresh rates, artificial limit.
 */
#define XRT_MAX_SUPPORTED_REFRESH_RATES 16

/*!
 * Max number of layers which can be handled at once.
 */
#ifdef XRT_OS_ANDROID
#define XRT_MAX_LAYERS 32
#elif defined(XRT_OS_LINUX) || defined(XRT_OS_WINDOWS) || defined(XRT_OS_OSX)
#define XRT_MAX_LAYERS 128
#else
#error "Unknown platform, define XRT_MAX_LAYERS for your OS"
#endif

/*!
 * @}
 */
