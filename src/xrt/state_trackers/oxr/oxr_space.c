// Copyright 2019-2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  So much space!
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup oxr_main
 */


#include "xrt/xrt_space.h"

#include "math/m_api.h"
#include "math/m_space.h"

#include "util/u_debug.h"
#include "util/u_misc.h"

#include "oxr_objects.h"
#include "oxr_logger.h"
#include "oxr_handle.h"
#include "oxr_input_transform.h"
#include "oxr_chain.h"
#include "oxr_pretty_print.h"
#include "oxr_conversions.h"
#include "oxr_xret.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*
 *
 * To xrt_space functions.
 *
 */

static XrResult
get_xrt_space_action(struct oxr_logger *log, struct oxr_space *spc, struct xrt_space **out_xspace)
{

	struct oxr_action_input *input = NULL;

	XrResult ret = oxr_action_get_pose_input(spc->sess, spc->act_key, &spc->subaction_paths, &input);
	if (ret != XR_SUCCESS) {
		return ret;
	}

	// Clear the cache.
	if (input == NULL) {
		xrt_space_reference(&spc->action.xs, NULL);
		spc->action.name = 0;
		spc->action.xdev = NULL;
		return XR_SUCCESS;
	}

	struct xrt_device *xdev = input->xdev;
	enum xrt_input_name name = input->input->name;

	assert(xdev != NULL);
	assert(name != 0);

	if (xdev != spc->action.xdev || name != spc->action.name) {
		xrt_space_reference(&spc->action.xs, NULL);

		xrt_result_t xret = xrt_space_overseer_create_pose_space( //
		    spc->sess->sys->xso,                                  //
		    xdev,                                                 //
		    name,                                                 //
		    &spc->action.xs);                                     //
		if (xret != XRT_SUCCESS) {
			oxr_warn(log, "Failed to create pose space");
		} else {
			spc->action.xdev = xdev;
			spc->action.name = name;
		}
	}

	*out_xspace = spc->action.xs;

	return XR_SUCCESS;
}

static XrResult
get_xrt_space(struct oxr_logger *log, struct oxr_space *spc, struct xrt_space **out_xspace)
{
	assert(out_xspace != NULL);
	assert(*out_xspace == NULL);

	struct xrt_space *xspace = NULL;
	switch (spc->space_type) {
	case OXR_SPACE_TYPE_ACTION: return get_xrt_space_action(log, spc, out_xspace);
	case OXR_SPACE_TYPE_XDEV_POSE: xspace = spc->xdev_pose.xs; break;
	case OXR_SPACE_TYPE_REFERENCE_VIEW: xspace = spc->sess->sys->xso->semantic.view; break;
	case OXR_SPACE_TYPE_REFERENCE_LOCAL: xspace = spc->sess->sys->xso->semantic.local; break;
	case OXR_SPACE_TYPE_REFERENCE_LOCAL_FLOOR: xspace = spc->sess->sys->xso->semantic.local_floor; break;
	case OXR_SPACE_TYPE_REFERENCE_STAGE: xspace = spc->sess->sys->xso->semantic.stage; break;
	case OXR_SPACE_TYPE_REFERENCE_UNBOUNDED_MSFT: xspace = spc->sess->sys->xso->semantic.unbounded; break;
	case OXR_SPACE_TYPE_REFERENCE_COMBINED_EYE_VARJO: xspace = NULL; break;
	case OXR_SPACE_TYPE_REFERENCE_LOCALIZATION_MAP_ML: xspace = NULL; break;
	}

	if (xspace == NULL) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Reference space without internal semantic space!");
	}

	*out_xspace = xspace;

	return XR_SUCCESS;
}


/*
 *
 * Space creation and destroy functions.
 *
 */

static XrResult
oxr_space_destroy(struct oxr_logger *log, struct oxr_handle_base *hb)
{
	struct oxr_space *spc = (struct oxr_space *)hb;

	// Unreference the reference space.
	enum xrt_reference_space_type xtype = oxr_ref_space_to_xrt(spc->space_type);
	if (xtype != XRT_SPACE_REFERENCE_TYPE_INVALID) {
		xrt_space_overseer_ref_space_dec(spc->sess->sys->xso, xtype);
	}

	xrt_space_reference(&spc->xdev_pose.xs, NULL);
	xrt_space_reference(&spc->action.xs, NULL);
	spc->action.xdev = NULL;
	spc->action.name = 0;

	free(spc);

	return XR_SUCCESS;
}

XrResult
oxr_space_action_create(struct oxr_logger *log,
                        struct oxr_session *sess,
                        uint32_t key,
                        const XrActionSpaceCreateInfo *createInfo,
                        struct oxr_space **out_space)
{
	struct oxr_instance *inst = sess->sys->inst;
	struct oxr_subaction_paths subaction_paths = {0};

	struct oxr_space *spc = NULL;
	OXR_ALLOCATE_HANDLE_OR_RETURN(log, spc, OXR_XR_DEBUG_SPACE, oxr_space_destroy, &sess->handle);

	oxr_classify_subaction_paths(log, inst, 1, &createInfo->subactionPath, &subaction_paths);

	spc->sess = sess;
	spc->space_type = OXR_SPACE_TYPE_ACTION;
	spc->subaction_paths = subaction_paths;
	spc->act_key = key;
	memcpy(&spc->pose, &createInfo->poseInActionSpace, sizeof(spc->pose));

	*out_space = spc;

	return XR_SUCCESS;
}

XrResult
oxr_space_get_reference_bounds_rect(struct oxr_logger *log,
                                    struct oxr_session *sess,
                                    XrReferenceSpaceType referenceSpaceType,
                                    XrExtent2Df *bounds)
{
	struct xrt_compositor *xc = &sess->xcn->base;

	enum xrt_reference_space_type reference_space_type = xr_ref_space_to_xrt(referenceSpaceType);

	xrt_result_t xret = xrt_comp_get_reference_bounds_rect(xc, reference_space_type, (struct xrt_vec2 *)bounds);
	if (xret == XRT_SPACE_BOUNDS_UNAVAILABLE) {
		// `bounds` must be set to 0 on XR_SPACE_BOUNDS_UNAVAILABLE
		*bounds = (XrExtent2Df){0};
		return XR_SPACE_BOUNDS_UNAVAILABLE;
	}
	OXR_CHECK_XRET(log, sess, xret, oxr_space_get_reference_bounds_rect);

	return oxr_session_success_result(sess);
}

XrResult
oxr_space_reference_create(struct oxr_logger *log,
                           struct oxr_session *sess,
                           const XrReferenceSpaceCreateInfo *createInfo,
                           struct oxr_space **out_space)
{
	if (!math_pose_validate((struct xrt_pose *)&createInfo->poseInReferenceSpace)) {
		return oxr_error(log, XR_ERROR_POSE_INVALID, "(createInfo->poseInReferenceSpace)");
	}

	// Convert the type into the different enums.
	enum oxr_space_type oxr_type = xr_ref_space_to_oxr(createInfo->referenceSpaceType);
	enum xrt_reference_space_type xtype = oxr_ref_space_to_xrt(oxr_type);

	struct oxr_space *spc = NULL;
	OXR_ALLOCATE_HANDLE_OR_RETURN(log, spc, OXR_XR_DEBUG_SPACE, oxr_space_destroy, &sess->handle);
	spc->sess = sess;
	spc->space_type = oxr_type;
	memcpy(&spc->pose, &createInfo->poseInReferenceSpace, sizeof(spc->pose));

	// Reference the reference space, if not supported by Monado just skip.
	if (xtype != XRT_SPACE_REFERENCE_TYPE_INVALID) {
		xrt_space_overseer_ref_space_inc(sess->sys->xso, xtype);
	}

	*out_space = spc;

	return XR_SUCCESS;
}

XrResult
oxr_space_xdev_pose_create(struct oxr_logger *log,
                           struct oxr_session *sess,
                           struct xrt_device *xdev,
                           enum xrt_input_name name,
                           const struct xrt_pose *pose,
                           struct oxr_space **out_space)
{
	if (!math_pose_validate(pose)) {
		return oxr_error(log, XR_ERROR_POSE_INVALID, "(createInfo->offset)");
	}

	struct xrt_space *xspace = NULL;
	xrt_result_t xret = xrt_space_overseer_create_pose_space( //
	    sess->sys->xso,                                       //
	    xdev,                                                 //
	    name,                                                 //
	    &xspace);                                             //
	OXR_CHECK_XRET(log, sess, xret, xrt_space_overseer_create_pose_space);

	struct oxr_space *spc = NULL;
	OXR_ALLOCATE_HANDLE_OR_RETURN(log, spc, OXR_XR_DEBUG_SPACE, oxr_space_destroy, &sess->handle);
	spc->sess = sess;
	spc->pose = *pose;
	spc->space_type = OXR_SPACE_TYPE_XDEV_POSE;
	xrt_space_reference(&spc->xdev_pose.xs, xspace);
	xrt_space_reference(&xspace, NULL);

	*out_space = spc;

	return XR_SUCCESS;
}


/*
 *
 * OpenXR API functions.
 *
 */

static void
free_spaces(struct xrt_space ***xtargets, struct xrt_pose **offsets, struct xrt_space_relation **results)
{
	free(*xtargets);
	*xtargets = NULL;

	free(*offsets);
	*offsets = NULL;

	free(*results);
	*results = NULL;
}

XrResult
oxr_spaces_locate(struct oxr_logger *log,
                  struct oxr_space **spcs,
                  uint32_t spc_count,
                  struct oxr_space *baseSpc,
                  XrTime time,
                  XrSpaceLocations *locations)
{
	struct oxr_sink_logger slog = {0};
	struct oxr_system *sys = baseSpc->sess->sys;
	bool print = sys->inst->debug_spaces;
	if (print) {
		for (uint32_t i = 0; i < spc_count; i++) {
			oxr_pp_space_indented(&slog, spcs[i], "space");
		}
		oxr_pp_space_indented(&slog, baseSpc, "baseSpace");
	}

	// Used in a lot of places.
	XrSpaceVelocitiesKHR *vels =
	    OXR_GET_OUTPUT_FROM_CHAIN(locations->next, XR_TYPE_SPACE_VELOCITIES_KHR, XrSpaceVelocitiesKHR);

	// XrEyeGazeSampleTimeEXT can not be chained anywhere in xrLocateSpaces

	/*
	 * Seek knowledge about the spaces from the space overseer.
	 */

	struct xrt_space *xbase = NULL;

	struct xrt_space **xspcs = U_TYPED_ARRAY_CALLOC(struct xrt_space *, spc_count);
	struct xrt_pose *offsets = U_TYPED_ARRAY_CALLOC(struct xrt_pose, spc_count);


	XrResult ret = XR_SUCCESS;

	for (uint32_t i = 0; i < spc_count; i++) {
		// Once we hit an error, we don't need to continue. Also make sure to not overwrite the error.
		if (ret != XR_SUCCESS) {
			break;
		}
		struct xrt_space *xt = NULL;
		ret = get_xrt_space(log, spcs[i], &xt);
		xspcs[i] = xt;
		offsets[i] = spcs[i]->pose;
	}

	// Make sure not to overwrite error return
	if (ret == XR_SUCCESS) {
		ret = get_xrt_space(log, baseSpc, &xbase);
	}

	// Only fill this out if the above succeeded. Zero initialized means relation flags == 0.
	struct xrt_space_relation *results = U_TYPED_ARRAY_CALLOC(struct xrt_space_relation, spc_count);

	if (ret == XR_SUCCESS) {
		// Convert at_time to monotonic and give to device.
		uint64_t at_timestamp_ns = time_state_ts_to_monotonic_ns(sys->inst->timekeeping, time);

		// Ask the space overseer to locate the spaces.
		enum xrt_result xret = xrt_space_overseer_locate_spaces( //
		    sys->xso,                                            //
		    xbase,                                               //
		    &baseSpc->pose,                                      //
		    at_timestamp_ns,                                     //
		    xspcs,                                               //
		    spc_count,                                           //
		    offsets,                                             //
		    results);                                            //
		if (xret != XRT_SUCCESS) {
			//! @todo  results locationFlags should still be 0. But should this hard fail? goto "Print"?
			oxr_warn(log, "Failed to locate spaces (%d)", xret);

			if (xret != XRT_SUCCESS) {
				ret = XR_ERROR_RUNTIME_FAILURE;

				if (xret == XRT_ERROR_ALLOCATION) {
					//! @todo spec: function not specified to return out of memory
					// ret = XR_ERROR_OUT_OF_MEMORY;
				}
			}
		}
	}

	/*
	 * Validate results
	 */

	for (uint32_t i = 0; i < spc_count; i++) {
		if (results[i].relation_flags == XRT_SPACE_RELATION_BITMASK_NONE) {
			locations->locations[i].locationFlags = 0;

			OXR_XRT_POSE_TO_XRPOSEF(XRT_POSE_IDENTITY, locations->locations[i].pose);

			if (vels) {
				vels->velocities[i].velocityFlags = 0;
				U_ZERO(&vels->velocities[i].linearVelocity);
				U_ZERO(&vels->velocities[i].angularVelocity);
			}

			oxr_slog(&slog, "\n\tReturning invalid pose locations->locations[%d]", i);
		} else {

			/*
			 * Combine and copy
			 */

			OXR_XRT_POSE_TO_XRPOSEF(results[i].pose, locations->locations[i].pose);
			locations->locations[i].locationFlags =
			    xrt_to_xr_space_location_flags(results[i].relation_flags);

			if (vels) {
				vels->velocities[i].velocityFlags = 0;
				if ((results[i].relation_flags & XRT_SPACE_RELATION_LINEAR_VELOCITY_VALID_BIT) != 0) {
					vels->velocities[i].linearVelocity.x = results[i].linear_velocity.x;
					vels->velocities[i].linearVelocity.y = results[i].linear_velocity.y;
					vels->velocities[i].linearVelocity.z = results[i].linear_velocity.z;
					vels->velocities[i].velocityFlags |= XR_SPACE_VELOCITY_LINEAR_VALID_BIT;
				} else {
					U_ZERO(&vels->velocities[i].linearVelocity);
				}

				if ((results[i].relation_flags & XRT_SPACE_RELATION_ANGULAR_VELOCITY_VALID_BIT) != 0) {
					vels->velocities[i].angularVelocity.x = results[i].angular_velocity.x;
					vels->velocities[i].angularVelocity.y = results[i].angular_velocity.y;
					vels->velocities[i].angularVelocity.z = results[i].angular_velocity.z;
					vels->velocities[i].velocityFlags |= XR_SPACE_VELOCITY_ANGULAR_VALID_BIT;
				} else {
					U_ZERO(&vels->velocities[i].angularVelocity);
				}
			}

			oxr_pp_relation_indented(&slog, &results[i], "relation");
		}
	}


	/*
	 * Print
	 */

	if (print) {
		oxr_log_slog(log, &slog);
	} else {
		oxr_slog_cancel(&slog);
	}

	free_spaces(&xspcs, &offsets, &results);

	if (ret != XR_SUCCESS) {
		return ret;
	}

	// all spaces must be on the same session
	return oxr_session_success_result(baseSpc->sess);
}

XrResult
oxr_space_locate(
    struct oxr_logger *log, struct oxr_space *spc, struct oxr_space *baseSpc, XrTime time, XrSpaceLocation *location)
{
	struct oxr_sink_logger slog = {0};
	struct oxr_system *sys = spc->sess->sys;
	bool print = sys->inst->debug_spaces;
	if (print) {
		oxr_pp_space_indented(&slog, spc, "space");
		oxr_pp_space_indented(&slog, baseSpc, "baseSpace");
	}

	// Used in a lot of places.
	XrSpaceVelocity *vel = OXR_GET_OUTPUT_FROM_CHAIN(location->next, XR_TYPE_SPACE_VELOCITY, XrSpaceVelocity);
	XrEyeGazeSampleTimeEXT *gaze_sample_time =
	    OXR_GET_OUTPUT_FROM_CHAIN(location->next, XR_TYPE_EYE_GAZE_SAMPLE_TIME_EXT, XrEyeGazeSampleTimeEXT);
	if (gaze_sample_time) {
		gaze_sample_time->time = 0;
	}


	/*
	 * Seek knowledge about the spaces from the space overseer.
	 */

	struct xrt_space *xtarget = NULL;
	struct xrt_space *xbase = NULL;
	XrResult ret;

	ret = get_xrt_space(log, spc, &xtarget);
	// Make sure not to overwrite error return
	if (ret == XR_SUCCESS) {
		ret = get_xrt_space(log, baseSpc, &xbase);
	}

	// Only fill this out if the above succeeded.
	struct xrt_space_relation result = XRT_SPACE_RELATION_ZERO;
	if (xtarget != NULL && xbase != NULL) {
		// Convert at_time to monotonic and give to device.
		uint64_t at_timestamp_ns = time_state_ts_to_monotonic_ns(sys->inst->timekeeping, time);

		// Ask the space overseer to locate the spaces.
		xrt_space_overseer_locate_space( //
		    sys->xso,                    //
		    xbase,                       //
		    &baseSpc->pose,              //
		    at_timestamp_ns,             //
		    xtarget,                     //
		    &spc->pose,                  //
		    &result);                    //
	}


	/*
	 * Validate results
	 */

	if (result.relation_flags == 0) {
		location->locationFlags = 0;

		OXR_XRT_POSE_TO_XRPOSEF(XRT_POSE_IDENTITY, location->pose);

		if (vel) {
			vel->velocityFlags = 0;
			U_ZERO(&vel->linearVelocity);
			U_ZERO(&vel->angularVelocity);
		}

		if (print) {
			oxr_slog(&slog, "\n\tReturning invalid pose");
			oxr_log_slog(log, &slog);
		} else {
			oxr_slog_cancel(&slog);
		}

		return ret; // Return any error.
	}


	/*
	 * Combine and copy
	 */

	OXR_XRT_POSE_TO_XRPOSEF(result.pose, location->pose);
	location->locationFlags = xrt_to_xr_space_location_flags(result.relation_flags);

	if (gaze_sample_time) {
		(void)gaze_sample_time; //! @todo Implement.
	}

	if (vel) {
		vel->velocityFlags = 0;
		if ((result.relation_flags & XRT_SPACE_RELATION_LINEAR_VELOCITY_VALID_BIT) != 0) {
			vel->linearVelocity.x = result.linear_velocity.x;
			vel->linearVelocity.y = result.linear_velocity.y;
			vel->linearVelocity.z = result.linear_velocity.z;
			vel->velocityFlags |= XR_SPACE_VELOCITY_LINEAR_VALID_BIT;
		} else {
			U_ZERO(&vel->linearVelocity);
		}

		if ((result.relation_flags & XRT_SPACE_RELATION_ANGULAR_VELOCITY_VALID_BIT) != 0) {
			vel->angularVelocity.x = result.angular_velocity.x;
			vel->angularVelocity.y = result.angular_velocity.y;
			vel->angularVelocity.z = result.angular_velocity.z;
			vel->velocityFlags |= XR_SPACE_VELOCITY_ANGULAR_VALID_BIT;
		} else {
			U_ZERO(&vel->angularVelocity);
		}
	}


	/*
	 * Print
	 */

	if (print) {
		oxr_pp_relation_indented(&slog, &result, "relation");
		oxr_log_slog(log, &slog);
	} else {
		oxr_slog_cancel(&slog);
	}

	return oxr_session_success_result(spc->sess);
}


/*
 *
 * 'Exported' functions.
 *
 */

XrResult
oxr_space_locate_device(struct oxr_logger *log,
                        struct xrt_device *xdev,
                        struct oxr_space *baseSpc,
                        XrTime time,
                        struct xrt_space_relation *out_relation)
{
	struct oxr_system *sys = baseSpc->sess->sys;

	struct xrt_space *xbase = NULL;
	XrResult ret;

	ret = get_xrt_space(log, baseSpc, &xbase);
	if (xbase == NULL) {
		return ret;
	}

	// Convert at_time to monotonic and give to device.
	uint64_t at_timestamp_ns = time_state_ts_to_monotonic_ns(sys->inst->timekeeping, time);

	// Ask the space overseer to locate the spaces.
	xrt_space_overseer_locate_device( //
	    sys->xso,                     //
	    xbase,                        //
	    &baseSpc->pose,               //
	    at_timestamp_ns,              //
	    xdev,                         //
	    out_relation);                //

	return ret;
}
