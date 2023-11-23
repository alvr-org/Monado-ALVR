// Copyright 2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  IPC Client space overseer.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup ipc_client
 */

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
             uint64_t at_timestamp_ns,
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
locate_device(struct xrt_space_overseer *xso,
              struct xrt_space *base_space,
              const struct xrt_pose *base_offset,
              uint64_t at_timestamp_ns,
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
	icspo->base.locate_device = locate_device;
	icspo->base.ref_space_inc = ref_space_inc;
	icspo->base.ref_space_dec = ref_space_dec;
	icspo->base.recenter_local_spaces = recenter_local_spaces;
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
