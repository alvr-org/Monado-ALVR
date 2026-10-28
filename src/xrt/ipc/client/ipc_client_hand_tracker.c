// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief IPC client @ref xrt_hand_tracker proxy.
 * @ingroup ipc_client
 */

#include "client/ipc_client.h"
#include "client/ipc_client_hand_tracker.h"

#include "ipc_client_generated.h"

#include "util/u_misc.h"


struct ipc_client_hand_tracker
{
	struct xrt_hand_tracker base;

	struct ipc_connection *ipc_c;

	uint32_t id;
};

static inline struct ipc_client_hand_tracker *
ipc_client_hand_tracker(struct xrt_hand_tracker *xht)
{
	return (struct ipc_client_hand_tracker *)xht;
}

static xrt_result_t
hand_tracker_locate(struct xrt_hand_tracker *xht,
                    struct xrt_space_overseer *xso,
                    struct xrt_space *base_space,
                    const struct xrt_pose *base_offset,
                    int64_t at_timestamp_ns,
                    struct xrt_hand_tracker_location *out_location)
{
	struct ipc_client_hand_tracker *icht = ipc_client_hand_tracker(xht);
	uint32_t base_space_id = ipc_client_space_get_id(base_space);
	(void)xso;

	return ipc_call_hand_tracker_locate( //
	    icht->ipc_c,                     //
	    icht->id,                        //
	    base_space_id,                   //
	    base_offset,                     //
	    at_timestamp_ns,                 //
	    out_location);                   //
}

static xrt_result_t
hand_tracker_set_output(struct xrt_hand_tracker *xht, enum xrt_output_name name, const struct xrt_output_value *value)
{
	struct ipc_client_hand_tracker *icht = ipc_client_hand_tracker(xht);

	return ipc_call_hand_tracker_set_output(icht->ipc_c, icht->id, name, value);
}

static void
hand_tracker_destroy(struct xrt_hand_tracker *xht)
{
	struct ipc_client_hand_tracker *icht = ipc_client_hand_tracker(xht);

	ipc_call_hand_tracker_destroy(icht->ipc_c, icht->id);

	free(icht);
}

xrt_result_t
ipc_client_hand_tracker_create(struct ipc_connection *ipc_c, uint32_t id, struct xrt_hand_tracker **out_xht)
{
	struct ipc_client_hand_tracker *icht = U_TYPED_CALLOC(struct ipc_client_hand_tracker);
	if (icht == NULL) {
		return XRT_ERROR_ALLOCATION;
	}

	icht->base.locate = hand_tracker_locate;
	icht->base.set_output = hand_tracker_set_output;
	icht->base.destroy = hand_tracker_destroy;
	icht->ipc_c = ipc_c;
	icht->id = id;

	*out_xht = &icht->base;

	return XRT_SUCCESS;
}
