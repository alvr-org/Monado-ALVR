// Copyright 2024-2025, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Content class function.
 * @author Simon Zeni <simon.zeni@collabora.com>
 * @ingroup aux_android
 */
#pragma once

#include <xrt/xrt_config_os.h>

#ifdef XRT_OS_ANDROID
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

bool
android_content_get_files_dir(void *context, char *dir, size_t size);


#ifdef __cplusplus
}
#endif

#endif
