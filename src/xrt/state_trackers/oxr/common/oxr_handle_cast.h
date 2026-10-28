// Copyright 2018-2024, Collabora, Ltd.
// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Macros for casting between OpenXR handles and internal pointers.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup oxr
 */

#pragma once

#include <stdint.h>


/*!
 * @brief Cast a pointer to an OpenXR handle in such a way as to avoid warnings.
 *
 * Avoids -Wpointer-to-int-cast by first casting to the same size int, then
 * promoting to the 64-bit int, then casting to the handle type. That's a lot of
 * no-ops on 64-bit, but a widening conversion on 32-bit.
 *
 * @ingroup oxr
 */
#define XRT_CAST_PTR_TO_OXR_HANDLE(HANDLE_TYPE, PTR) ((HANDLE_TYPE)(uint64_t)(uintptr_t)(PTR))

/*!
 * @brief Cast an OpenXR handle to a pointer in such a way as to avoid warnings.
 *
 * Avoids -Wint-to-pointer-cast by first casting to a 64-bit int, then to a
 * pointer-sized int, then to the desired pointer type. That's a lot of no-ops
 * on 64-bit, but a narrowing (!) conversion on 32-bit.
 *
 * @ingroup oxr
 */
#define XRT_CAST_OXR_HANDLE_TO_PTR(PTR_TYPE, HANDLE) ((PTR_TYPE)(uintptr_t)(uint64_t)(HANDLE))
