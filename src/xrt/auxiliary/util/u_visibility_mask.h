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
 * Default visibility mask, only returns a very simple mask with four small
 * triangles in each corner, scaled to the given FoV so it matches the OpenXR
 * conventions. The caller must take care of de-allocating the mask once done
 * with it.
 *
 * @ingroup aux_util
 */
void
u_visibility_mask_get_default(enum xrt_visibility_mask_type type,
                              const struct xrt_fov *fov,
                              struct xrt_visibility_mask **out_mask);


#ifdef __cplusplus
}
#endif
