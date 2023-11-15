// Copyright 2020-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Header for D3D12-backed image buffer allocator factory function.
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @author Fernando Velazquez Innella <finnella@magicleap.com>
 * @ingroup aux_d3d
 */

#pragma once

#include "xrt/xrt_compositor.h"

#include <d3d12.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create a XINA that allocates D3D12 textures.
 *
 * @param device A device to allocate the textures with. Be sure it will not be used from other threads while this
 * allocator allocates.
 *
 * @return struct xrt_image_native_allocator*
 */
struct xrt_image_native_allocator *
d3d12_allocator_create(ID3D12Device *device);

#ifdef __cplusplus
}
#endif
