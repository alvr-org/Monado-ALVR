// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Misc helpers for @ref xrt_body_tracker implementations.
 * @ingroup aux_util
 */

#include "u_body_tracker.h"
#include "u_body_tracker_ni.h"
#include "util/u_logging.h"

#include <assert.h>


void
u_body_tracker_populate_function_pointers(struct xrt_body_tracker *xbt, u_body_tracker_destroy_function_t destroy_fn)
{
	assert(xbt != NULL);

	if (destroy_fn == NULL) {
		U_LOG_E("Got destroy_fn == NULL!");
		assert(destroy_fn != NULL);
	}

	xbt->locate = u_body_tracker_ni_locate;
	xbt->get_skeleton = u_body_tracker_ni_get_skeleton;
	xbt->reset_calibration_meta = u_body_tracker_ni_reset_calibration_meta;
	xbt->set_calibration_override_meta = u_body_tracker_ni_set_calibration_override_meta;
	xbt->set_fidelity_meta = u_body_tracker_ni_set_fidelity_meta;
	xbt->destroy = destroy_fn;
}
