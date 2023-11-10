// Copyright 2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Visibility mask utilitary header
 * @author Simon Zeni <simon.zeni@collabora.com>
 * @ingroup aux_util
 */

#include "xrt/xrt_defines.h"
#include "xrt/xrt_visibility_mask.h"


#ifdef __cplusplus
extern "C" {
#endif


/*!
 * Default visibility mask. The caller must take care of de-allocating the mask once done with it.
 *
 * @ingroup aux_util
 */
void
u_visibility_mask_get_default(enum xrt_visibility_mask_type type, struct xrt_visibility_mask **out_mask);


#ifdef __cplusplus
}
#endif
