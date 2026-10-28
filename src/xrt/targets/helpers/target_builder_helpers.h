// Copyright 2022-2023, Collabora, Ltd.
// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Helpers for @ref xrt_builder implementations.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup xrt_iface
 */

#pragma once

#include "xrt/xrt_space.h"
#include "xrt/xrt_prober.h"
#include "xrt/xrt_system.h"


#ifdef __cplusplus
extern "C" {
#endif

struct t_builder_options;


/*!
 * Argument to @ref t_builder_roles_helper_open_system and implemented by
 * @ref t_builder::open_system_static_roles function.
 *
 * A builder implement this function is free to focus on only creating the
 * devices and assigning them initial roles. With this implementation details
 * of the @ref xrt_system_devices and @ref xrt_space_overseer is taken care of
 * by the caller of this function.
 *
 * @todo `tbo` should be a true `out` parameter, rather than being a structure with defaults filled in.
 *
 * @ingroup xrt_iface
 */
typedef xrt_result_t (*t_builder_open_system_fn)(struct xrt_builder *xb,
                                                 cJSON *config,
                                                 struct xrt_prober *xp,
                                                 struct xrt_tracking_origin *origin,
                                                 struct xrt_system_devices *xsysd,
                                                 struct xrt_frame_context *xfctx,
                                                 struct t_builder_options *tbo);

/*!
 * This small helper struct is for @ref t_builder_roles_helper_open_system,
 * lets a builder focus on opening devices rather then dealing with Monado
 * structs like @ref xrt_system_devices and the like.
 *
 * @ingroup xrt_iface
 */
struct t_builder_options
{
	//! The offset from stage -> local space, defaulting to a 1.6m height adjustment.
	struct xrt_pose T_stage_local;

	struct xrt_device *head;
	struct xrt_device *eyes;
	struct xrt_device *face;
	struct xrt_device *left;
	struct xrt_device *right;
	struct xrt_device *gamepad;

	struct
	{
		struct
		{
			struct xrt_device *left;
			struct xrt_device *right;
		} unobstructed;

		struct
		{
			struct xrt_device *left;
			struct xrt_device *right;
		} conforming;
	} hand_tracking;
};

/*!
 * This helper struct makes it easier to implement the builder interface, but it
 * also comes with a set of integration that may not be what all builders want.
 * See the below functions for more information.
 *
 * * @ref t_builder_open_system_static_roles
 * * @ref t_builder_roles_helper_open_system
 *
 * @ingroup xrt_iface
 */
struct t_builder
{
	//! Base for this struct.
	struct xrt_builder base;

	/*!
	 * @copydoc t_builder_open_system_fn
	 */
	t_builder_open_system_fn open_system_static_roles;
};


/*
 *
 * Functions.
 *
 */

/*!
 * Create a legacy space overseer, most builders probably want to have a more
 * advanced setup then this, especially stand alone ones. Uses
 * @ref u_builder_setup_tracking_origins internally and
 * @ref b_space_overseer_legacy_setup.
 *
 * @ingroup xrt_iface
 */
void
t_builder_create_space_overseer_legacy(struct xrt_session_event_sink *broadcast,
                                       struct xrt_device *head,
                                       struct xrt_device *eyes,
                                       struct xrt_device *left,
                                       struct xrt_device *right,
                                       struct xrt_device *gamepad,
                                       struct xrt_device **xdevs,
                                       uint32_t xdev_count,
                                       bool root_is_unbounded,
                                       const struct xrt_pose *T_stage_local,
                                       bool per_app_local_spaces,
                                       struct xrt_space_overseer **out_xso);

/*!
 * Helper to create a system devices that has static roles and a appropriate
 * space overseer. Currently uses the functions below to create a full system
 * with the help of @p fn argument. But this might change in the future.
 *
 * * @ref b_system_devices_static_allocate
 * * @ref b_system_devices_static_finalize
 * * @ref t_builder_create_space_overseer_legacy
 *
 * @ingroup xrt_iface
 */
xrt_result_t
t_builder_roles_helper_open_system(struct xrt_builder *xb,
                                   cJSON *config,
                                   struct xrt_prober *xp,
                                   struct xrt_session_event_sink *broadcast,
                                   struct xrt_system_devices **out_xsysd,
                                   struct xrt_space_overseer **out_xso,
                                   t_builder_open_system_fn fn);

/*!
 * Implementation for xrt_builder::open_system to be used with @ref t_builder.
 * Uses @ref t_builder_roles_helper_open_system internally, a builder that uses
 * the @ref t_builder should use this function for xrt_builder::open_system.
 *
 * When using this function the builder must have @ref t_builder and implement
 * the @ref t_builder::open_system_static_roles function, see documentation for
 * @ref t_builder_open_system_fn about requirements.
 *
 * @ingroup xrt_iface
 */
xrt_result_t
t_builder_open_system_static_roles(struct xrt_builder *xb,
                                   cJSON *config,
                                   struct xrt_prober *xp,
                                   struct xrt_session_event_sink *broadcast,
                                   struct xrt_system_devices **out_xsysd,
                                   struct xrt_space_overseer **out_xso);


#ifdef __cplusplus
}
#endif
