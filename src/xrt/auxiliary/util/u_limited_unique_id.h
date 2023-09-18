// Copyright 2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  A very simple generator to create process unique ids.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_util
 */


#pragma once

#include "xrt/xrt_defines.h"


#ifdef __cplusplus
extern "C" {
#endif


/*!
 * This function returns a unsigned 64 bit value that is guaranteed to be unique
 * within the current running process, and not zero. There is of course the
 * limit of running out of those ID once all values has been returned, but the
 * value is 64 bit so that should not be a practical limit. The value is useful
 * when needing to implement caching of a complex object, this lets us not use
 * memory addresses as keys which may be reused by underlying alloc
 * implementation and could lead to false hits.
 *
 * The current implementation is naive and is a simple monotonic counter.
 *
 * @ingroup aux_util
 */
xrt_limited_unique_id_t
u_limited_unique_id_get(void);


#ifdef __cplusplus
}
#endif
