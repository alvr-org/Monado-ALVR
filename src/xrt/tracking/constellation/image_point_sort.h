// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Sorting helper with argument.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup tracking
 */

#include "correspondence_search.h"


#ifdef __cplusplus
extern "C" {
#endif

void
sort_image_points(struct cs_image_point **points, size_t num_points, struct cs_image_point *anchor);

#ifdef __cplusplus
}
#endif
