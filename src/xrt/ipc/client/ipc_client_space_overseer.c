// Copyright 2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  IPC Client space overseer.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup ipc_client
 */

#include "client/ipc_client.h"
#include "client/ipc_client_connection.h"
#include "shared/ipc_message_channel.h"
#include "xrt/xrt_defines.h"
#include "xrt/xrt_space.h"

#include "ipc_client_generated.h"


struct ipc_client_space
{
	struct xrt_space base;

	struct ipc_connection *ipc_c;

	uint32_t id;
};

struct ipc_client_space_overseer
{
	struct xrt_space_overseer base;

	struct ipc_connection *ipc_c;

	struct xrt_reference ref_space_use[XRT_SPACE_REFERENCE_TYPE_COUNT];
};


/*
 *
 * Helpers
 *
 */

static inline struct ipc_client_space *
ipc_client_space(struct xrt_space *xs)
{
	return (struct ipc_client_space *)xs;
}

static inline struct ipc_client_space_overseer *
ipc_client_space_overseer(struct xrt_space_overseer *xso)
{
	return (struct ipc_client_space_overseer *)xso;
}


/*
 *
 * Space member functions.
 *
 */

static void
space_destroy(struct xrt_space *xs)
{
	struct ipc_client_space *icsp = ipc_client_space(xs);

	ipc_call_space_destroy(icsp->ipc_c, icsp->id);

	free(xs);
}

static void
alloc_space_with_id(struct ipc_client_space_overseer *icspo, uint32_t id, struct xrt_space **out_space)
{
	struct ipc_client_space *icsp = U_TYPED_CALLOC(struct ipc_client_space);
	icsp->base.reference.count = 1;
	icsp->base.destroy = space_destroy;
	icsp->ipc_c = icspo->ipc_c;
	icsp->id = id;

	*out_space = &icsp->base;
}


/*
 *
 * Overseer member functions.
 *
 */

static xrt_result_t
create_offset_space(struct xrt_space_overseer *xso,
                    struct xrt_space *parent,
                    const struct xrt_pose *offset,
                    struct xrt_space **out_space)
{
	struct ipc_client_space_overseer *icspo = ipc_client_space_overseer(xso);
	xrt_result_t xret;
	uint32_t parent_id = ipc_client_space(parent)->id;
	uint32_t id = 0;

	xret = ipc_call_space_create_offset(icspo->ipc_c, parent_id, offset, &id);
	IPC_CHK_AND_RET(icspo->ipc_c, xret, "ipc_call_space_create_offset");

	alloc_space_with_id(icspo, id, out_space);

	return XRT_SUCCESS;
}

static xrt_result_t
create_pose_space(struct xrt_space_overseer *xso,
                  struct xrt_device *xdev,
                  enum xrt_input_name name,
                  struct xrt_space **out_space)
{
	struct ipc_client_space_overseer *icspo = ipc_client_space_overseer(xso);
	xrt_result_t xret;
	uint32_t xdev_id = ipc_client_xdev(xdev)->device_id;
	uint32_t id = 0;

	xret = ipc_call_space_create_pose(icspo->ipc_c, xdev_id, name, &id);
	IPC_CHK_AND_RET(icspo->ipc_c, xret, "ipc_call_space_create_pose");

	alloc_space_with_id(icspo, id, out_space);

	return XRT_SUCCESS;
}

static xrt_result_t
locate_space(struct xrt_space_overseer *xso,
             struct xrt_space *base_space,
             const struct xrt_pose *base_offset,
             int64_t at_timestamp_ns,
             struct xrt_space *space,
             const struct xrt_pose *offset,
             struct xrt_space_relation *out_relation)
{
	struct ipc_client_space_overseer *icspo = ipc_client_space_overseer(xso);
	xrt_result_t xret;

	struct ipc_client_space *icsp_base_space = ipc_client_space(base_space);
	struct ipc_client_space *icsp_space = ipc_client_space(space);

	xret = ipc_call_space_locate_space( //
	    icspo->ipc_c,                   //
	    icsp_base_space->id,            //
	    base_offset,                    //
	    at_timestamp_ns,                //
	    icsp_space->id,                 //
	    offset,                         //
	    out_relation);                  //
	IPC_CHK_ALWAYS_RET(icspo->ipc_c, xret, "ipc_call_space_locate_space");
}

static xrt_result_t
locate_spaces(struct xrt_space_overseer *xso,
              struct xrt_space *base_space,
              const struct xrt_pose *base_offset,
              int64_t at_timestamp_ns,
              struct xrt_space **spaces,
              uint32_t space_count,
              const struct xrt_pose *offsets,
              struct xrt_space_relation *out_relations)
{
	struct ipc_client_space_overseer *icspo = ipc_client_space_overseer(xso);
	struct ipc_connection *ipc_c = icspo->ipc_c;

	xrt_result_t xret;

	struct ipc_client_space *icsp_base_space = ipc_client_space(base_space);

	uint32_t *space_ids = U_TYPED_ARRAY_CALLOC(uint32_t, space_count);
	if (space_ids == NULL) {
		IPC_ERROR(ipc_c, "Failed to allocate space_ids");
		return XRT_ERROR_ALLOCATION;
	}

	ipc_client_connection_lock(ipc_c);

	xret =
	    ipc_send_space_locate_spaces_locked(ipc_c, icsp_base_space->id, base_offset, space_count, at_timestamp_ns);
	IPC_CHK_WITH_GOTO(ipc_c, xret, "ipc_send_space_locate_spaces_locked", locate_spaces_out);

	enum xrt_result received_result = XRT_SUCCESS;
	xret = ipc_receive(&ipc_c->imc, &received_result, sizeof(enum xrt_result));
	IPC_CHK_WITH_GOTO(ipc_c, xret, "ipc_receive: Receive spaces allocation result", locate_spaces_out);

	// now check if the service sent a success code or an error code about allocating memory for spaces.
	xret = received_result;
	IPC_CHK_WITH_GOTO(ipc_c, xret, "ipc_receive: service side spaces allocation failed", locate_spaces_out);

	for (uint32_t i = 0; i < space_count; i++) {
		if (spaces[i] == NULL) {
			space_ids[i] = UINT32_MAX;
		} else {
			struct ipc_client_space *icsp_space = ipc_client_space(spaces[i]);
			space_ids[i] = icsp_space->id;
		}
	}

	xret = ipc_send(&ipc_c->imc, space_ids, sizeof(uint32_t) * space_count);
	IPC_CHK_WITH_GOTO(ipc_c, xret, "ipc_send: Send spaces ids", locate_spaces_out);

	xret = ipc_send(&ipc_c->imc, offsets, sizeof(struct xrt_pose) * space_count);
	IPC_CHK_WITH_GOTO(ipc_c, xret, "ipc_send: Send spaces offsets", locate_spaces_out);

	xret = ipc_receive(&ipc_c->imc, out_relations, sizeof(struct xrt_space_relation) * space_count);
	IPC_CHK_WITH_GOTO(ipc_c, xret, "ipc_receive: Receive spaces relations", locate_spaces_out);

locate_spaces_out:
	free(space_ids);
	ipc_client_connection_unlock(ipc_c);
	return xret;
}

static xrt_result_t
locate_device(struct xrt_space_overseer *xso,
              struct xrt_space *base_space,
              const struct xrt_pose *base_offset,
              int64_t at_timestamp_ns,
              struct xrt_device *xdev,
              struct xrt_space_relation *out_relation)
{
	struct ipc_client_space_overseer *icspo = ipc_client_space_overseer(xso);
	xrt_result_t xret;

	struct ipc_client_space *icsp_base_space = ipc_client_space(base_space);
	uint32_t xdev_id = ipc_client_xdev(xdev)->device_id;

	xret = ipc_call_space_locate_device( //
	    icspo->ipc_c,                    //
	    icsp_base_space->id,             //
	    base_offset,                     //
	    at_timestamp_ns,                 //
	    xdev_id,                         //
	    out_relation);                   //
	IPC_CHK_ALWAYS_RET(icspo->ipc_c, xret, "ipc_call_space_locate_device");
}

static xrt_result_t
ref_space_inc(struct xrt_space_overseer *xso, enum xrt_reference_space_type type)
{
	struct ipc_client_space_overseer *icspo = ipc_client_space_overseer(xso);
	xrt_result_t xret;

	// No more checking then this.
	assert(type < XRT_SPACE_REFERENCE_TYPE_COUNT);

	// If it wasn't zero nothing to do.
	if (!xrt_reference_inc_and_was_zero(&icspo->ref_space_use[type])) {
		return XRT_SUCCESS;
	}

	xret = ipc_call_space_mark_ref_space_in_use(icspo->ipc_c, type);
	IPC_CHK_ALWAYS_RET(icspo->ipc_c, xret, "ipc_call_space_mark_ref_space_in_use");
}

static xrt_result_t
ref_space_dec(struct xrt_space_overseer *xso, enum xrt_reference_space_type type)
{
	struct ipc_client_space_overseer *icspo = ipc_client_space_overseer(xso);
	xrt_result_t xret;

	// No more checking then this.
	assert(type < XRT_SPACE_REFERENCE_TYPE_COUNT);

	// If it is not zero we are done.
	if (!xrt_reference_dec_and_is_zero(&icspo->ref_space_use[type])) {
		return XRT_SUCCESS;
	}

	xret = ipc_call_space_unmark_ref_space_in_use(icspo->ipc_c, type);
	IPC_CHK_ALWAYS_RET(icspo->ipc_c, xret, "ipc_call_space_unmark_ref_space_in_use");
}

static xrt_result_t
recenter_local_spaces(struct xrt_space_overseer *xso)
{
	struct ipc_client_space_overseer *icspo = ipc_client_space_overseer(xso);

	return ipc_call_space_recenter_local_spaces(icspo->ipc_c);
}

static xrt_result_t
get_tracking_origin_offset(struct xrt_space_overseer *xso, struct xrt_tracking_origin *xto, struct xrt_pose *out_offset)
{
	return XRT_ERROR_NOT_IMPLEMENTED;
}

static xrt_result_t
set_tracking_origin_offset(struct xrt_space_overseer *xso,
                           struct xrt_tracking_origin *xto,
                           const struct xrt_pose *offset)
{
	return XRT_ERROR_NOT_IMPLEMENTED;
}

static xrt_result_t
get_reference_space_offset(struct xrt_space_overseer *xso,
                           enum xrt_reference_space_type type,
                           struct xrt_pose *out_offset)
{
	struct ipc_client_space_overseer *icspo = ipc_client_space_overseer(xso);
	return ipc_call_space_get_reference_space_offset(icspo->ipc_c, type, out_offset);
}

static xrt_result_t
set_reference_space_offset(struct xrt_space_overseer *xso,
                           enum xrt_reference_space_type type,
                           const struct xrt_pose *offset)
{
	struct ipc_client_space_overseer *icspo = ipc_client_space_overseer(xso);
	return ipc_call_space_set_reference_space_offset(icspo->ipc_c, type, offset);
}

static void
destroy(struct xrt_space_overseer *xso)
{
	struct ipc_client_space_overseer *icspo = ipc_client_space_overseer(xso);

	xrt_space_reference(&icspo->base.semantic.root, NULL);
	xrt_space_reference(&icspo->base.semantic.view, NULL);
	xrt_space_reference(&icspo->base.semantic.local, NULL);
	xrt_space_reference(&icspo->base.semantic.local_floor, NULL);
	xrt_space_reference(&icspo->base.semantic.stage, NULL);
	xrt_space_reference(&icspo->base.semantic.unbounded, NULL);

	free(icspo);
}


/*
 *
 * 'Exported' functions.
 *
 */

struct xrt_space_overseer *
ipc_client_space_overseer_create(struct ipc_connection *ipc_c)
{
	uint32_t root_id = UINT32_MAX;
	uint32_t view_id = UINT32_MAX;
	uint32_t local_id = UINT32_MAX;
	uint32_t local_floor_id = UINT32_MAX;
	uint32_t stage_id = UINT32_MAX;
	uint32_t unbounded_id = UINT32_MAX;
	xrt_result_t xret;

	xret = ipc_call_space_create_semantic_ids( //
	    ipc_c,                                 //
	    &root_id,                              //
	    &view_id,                              //
	    &local_id,                             //
	    &local_floor_id,                       //
	    &stage_id,                             //
	    &unbounded_id);                        //
	IPC_CHK_WITH_RET(ipc_c, xret, "ipc_call_space_create_semantic_ids", NULL);

	struct ipc_client_space_overseer *icspo = U_TYPED_CALLOC(struct ipc_client_space_overseer);
	icspo->base.create_offset_space = create_offset_space;
	icspo->base.create_pose_space = create_pose_space;
	icspo->base.locate_space = locate_space;
	icspo->base.locate_spaces = locate_spaces;
	icspo->base.locate_device = locate_device;
	icspo->base.ref_space_inc = ref_space_inc;
	icspo->base.ref_space_dec = ref_space_dec;
	icspo->base.recenter_local_spaces = recenter_local_spaces;
	icspo->base.get_tracking_origin_offset = get_tracking_origin_offset;
	icspo->base.set_tracking_origin_offset = set_tracking_origin_offset;
	icspo->base.get_reference_space_offset = get_reference_space_offset;
	icspo->base.set_reference_space_offset = set_reference_space_offset;
	icspo->base.destroy = destroy;
	icspo->ipc_c = ipc_c;

#define CREATE(NAME)                                                                                                   \
	do {                                                                                                           \
		if (NAME##_id == UINT32_MAX) {                                                                         \
			break;                                                                                         \
		}                                                                                                      \
		alloc_space_with_id(icspo, NAME##_id, &icspo->base.semantic.NAME);                                     \
	} while (false)

	CREATE(root);
	CREATE(view);
	CREATE(local);
	CREATE(local_floor);
	CREATE(stage);
	CREATE(unbounded);

#undef CREATE

	return &icspo->base;
}
