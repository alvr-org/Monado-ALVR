// Copyright 2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  A very simple generator to create process unique ids.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_util
 */

#include "u_limited_unique_id.h"

#include <atomic>


/*
 * This code is in C++ and not C because MSVC doesn't implement C atomics yet,
 * but to make things even more fun the C++11 standard defines atomic_uint64_t
 * as optional, but atomic_uint_fast64_t is listed as required, so here we are.
 */
static std::atomic_uint_fast64_t generator;

extern "C" xrt_limited_unique_id_t
u_limited_unique_id_get(void)
{
	return xrt_limited_unique_id_t{static_cast<uint64_t>(++generator)};
}
