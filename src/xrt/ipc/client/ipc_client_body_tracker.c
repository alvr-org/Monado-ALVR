// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief IPC client @ref xrt_body_tracker proxy.
 * @ingroup ipc_client
 */

#include "util/u_body_tracker.h"
#include "util/u_misc.h"

#include "ipc_client_generated.h"

#include "client/ipc_client.h"
#include "client/ipc_client_body_tracker.h"


struct ipc_client_body_tracker
{
	struct xrt_body_tracker base;

	struct ipc_connection *ipc_c;

	uint32_t id;
};

static inline struct ipc_client_body_tracker *
ipc_client_body_tracker(struct xrt_body_tracker *xbt)
{
	return (struct ipc_client_body_tracker *)xbt;
}

static xrt_result_t
body_tracker_locate(struct xrt_body_tracker *xbt,
                    struct xrt_space_overseer *xso,
                    struct xrt_space *base_space,
                    const struct xrt_pose *base_offset,
                    int64_t at_timestamp_ns,
                    struct xrt_body_tracker_location *out_location)
{
	struct ipc_client_body_tracker *icbt = ipc_client_body_tracker(xbt);
	uint32_t base_space_id = ipc_client_space_get_id(base_space);
	(void)xso;

	return ipc_call_body_tracker_locate( //
	    icbt->ipc_c,                     //
	    icbt->id,                        //
	    base_space_id,                   //
	    base_offset,                     //
	    at_timestamp_ns,                 //
	    out_location);                   //
}

static xrt_result_t
body_tracker_get_skeleton(struct xrt_body_tracker *xbt, struct xrt_body_skeleton *out_skeleton)
{
	struct ipc_client_body_tracker *icbt = ipc_client_body_tracker(xbt);

	return ipc_call_body_tracker_get_skeleton(icbt->ipc_c, icbt->id, out_skeleton);
}

static xrt_result_t
body_tracker_reset_calibration_meta(struct xrt_body_tracker *xbt)
{
	struct ipc_client_body_tracker *icbt = ipc_client_body_tracker(xbt);

	return ipc_call_body_tracker_reset_calibration_meta(icbt->ipc_c, icbt->id);
}

static xrt_result_t
body_tracker_set_calibration_override_meta(struct xrt_body_tracker *xbt, float body_height)
{
	struct ipc_client_body_tracker *icbt = ipc_client_body_tracker(xbt);

	return ipc_call_body_tracker_set_calibration_override_meta(icbt->ipc_c, icbt->id, body_height);
}

static xrt_result_t
body_tracker_set_fidelity_meta(struct xrt_body_tracker *xbt, enum xrt_body_tracking_fidelity_meta fidelity)
{
	struct ipc_client_body_tracker *icbt = ipc_client_body_tracker(xbt);

	return ipc_call_body_tracker_set_fidelity_meta(icbt->ipc_c, icbt->id, fidelity);
}

static void
body_tracker_destroy(struct xrt_body_tracker *xbt)
{
	struct ipc_client_body_tracker *icbt = ipc_client_body_tracker(xbt);

	ipc_call_body_tracker_destroy(icbt->ipc_c, icbt->id);

	free(icbt);
}

xrt_result_t
ipc_client_body_tracker_create(struct ipc_connection *ipc_c,
                               uint32_t id,
                               const struct xrt_body_tracker_supported *supported,
                               struct xrt_body_tracker **out_xbt)
{
	struct ipc_client_body_tracker *icbt = U_TYPED_CALLOC(struct ipc_client_body_tracker);
	if (icbt == NULL) {
		return XRT_ERROR_ALLOCATION;
	}

	u_body_tracker_populate_function_pointers(&icbt->base, body_tracker_destroy);

	icbt->base.locate = body_tracker_locate;
	icbt->base.get_skeleton = body_tracker_get_skeleton;
	icbt->base.reset_calibration_meta = body_tracker_reset_calibration_meta;
	icbt->base.set_calibration_override_meta = body_tracker_set_calibration_override_meta;
	icbt->base.set_fidelity_meta = body_tracker_set_fidelity_meta;
	icbt->base.supported = *supported;
	icbt->ipc_c = ipc_c;
	icbt->id = id;

	*out_xbt = &icbt->base;

	return XRT_SUCCESS;
}
