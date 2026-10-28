// Copyright 2019, Collabora, Ltd.
// Copyright 2024-2025, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Auto detect OS and certain features.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup xrt_iface
 */

#pragma once


/*
 *
 * Auto detect OS.
 *
 */

#if defined(__ANDROID__)
#include "xrt/xrt_config_android.h"
#define XRT_OS_ANDROID
#if defined(XRT_FEATURE_AHARDWARE_BUFFER)
#define XRT_OS_ANDROID_USE_AHB
#endif
#define XRT_OS_LINUX
#define XRT_OS_UNIX
#define XRT_OS_WAS_AUTODETECTED
#endif

#if defined(__linux__) && !defined(XRT_OS_WAS_AUTODETECTED)
#define XRT_OS_LINUX
#define XRT_OS_UNIX
#define XRT_OS_WAS_AUTODETECTED
#endif

#if defined(_WIN32)
#define XRT_OS_WINDOWS
#define XRT_OS_WAS_AUTODETECTED
#endif

#if defined(__MINGW32__)
#define XRT_ENV_MINGW
#endif

#if defined(__APPLE__) && defined(__MACH__)
#include <TargetConditionals.h>
#if TARGET_OS_MAC == 1
#define XRT_OS_OSX
#define XRT_OS_UNIX
#define XRT_OS_WAS_AUTODETECTED
#endif // TARGET_OS_MAC
#endif

#ifndef XRT_OS_WAS_AUTODETECTED
#error "OS type not found during compile"
#endif
#undef XRT_OS_WAS_AUTODETECTED
