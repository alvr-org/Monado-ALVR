// Copyright 2022-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Helpers for @ref xrt_builder implementations.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_util
 */

#pragma once

#include "xrt/xrt_space.h"
#include "xrt/xrt_prober.h"


#ifdef __cplusplus
extern "C" {
#endif

struct xrt_prober_device;
struct u_builder_roles_helper;


/*!
 * Max return of the number @ref xrt_prober_device.
 *
 * @ingroup aux_util
 */
#define U_BUILDER_SEARCH_MAX (16) // 16 Vive trackers

/*!
 * Argument to @ref u_builder_roles_helper_open_system and implemented by
 * @ref u_builder::open_system_static_roles function.
 *
 * A builder implement this function is free to focus on only creating the
 * devices and assigning them initial roles. With this implementation details
 * of the @ref xrt_system_devices and @ref xrt_space_overseer is taken care of
 * by the caller of this function.
 *
 * @ingroup aux_util
 */
typedef xrt_result_t (*u_builder_open_system_fn)(struct xrt_builder *xb,
                                                 cJSON *config,
                                                 struct xrt_prober *xp,
                                                 struct xrt_tracking_origin *origin,
                                                 struct xrt_system_devices *xsysd,
                                                 struct xrt_frame_context *xfctx,
                                                 struct u_builder_roles_helper *ubrh);

/*!
 * A filter to match the against.
 *
 * @ingroup aux_util
 */
struct u_builder_search_filter
{
	uint16_t vendor_id;
	uint16_t product_id;
	enum xrt_bus_type bus_type;
};

/*!
 * Results of a search of devices.
 *
 * @ingroup aux_util
 */
struct u_builder_search_results
{
	//! Out field of found @ref xrt_prober_device.
	struct xrt_prober_device *xpdevs[U_BUILDER_SEARCH_MAX];

	//! Number of found devices.
	size_t xpdev_count;
};

/*!
 * This small helper struct is for @ref u_builder_roles_helper_open_system,
 * lets a builder focus on opening devices rather then dealing with Monado
 * structs like @ref xrt_system_devices and the like.
 *
 * @ingroup aux_util
 */
struct u_builder_roles_helper
{
	struct xrt_device *head;
	struct xrt_device *left;
	struct xrt_device *right;

	struct
	{
		struct xrt_device *left;
		struct xrt_device *right;
	} hand_tracking;
};

/*!
 * This helper struct makes it easier to implement the builder interface, but it
 * also comes with a set of integration that may not be what all builders want.
 * See the below functions for more information.
 *
 * * @ref u_builder_open_system_static_roles
 * * @ref u_builder_roles_helper_open_system
 *
 * @ingroup aux_util
 */
struct u_builder
{
	//! Base for this struct.
	struct xrt_builder base;

	/*!
	 * @copydoc u_builder_open_system_fn
	 */
	u_builder_open_system_fn open_system_static_roles;
};


/*
 *
 * Functions.
 *
 */

/*!
 * Find the first @ref xrt_prober_device in the prober list.
 *
 * @ingroup aux_util
 */
struct xrt_prober_device *
u_builder_find_prober_device(struct xrt_prober_device *const *xpdevs,
                             size_t xpdev_count,
                             uint16_t vendor_id,
                             uint16_t product_id,
                             enum xrt_bus_type bus_type);

/*!
 * Find all of the @ref xrt_prober_device that matches any in the given list of
 * @ref u_builder_search_filter filters.
 *
 * @ingroup aux_util
 */
void
u_builder_search(struct xrt_prober *xp,
                 struct xrt_prober_device *const *xpdevs,
                 size_t xpdev_count,
                 const struct u_builder_search_filter *filters,
                 size_t filter_count,
                 struct u_builder_search_results *results);

/*!
 * Helper function for setting up tracking origins. Applies 3dof offsets for devices with XRT_TRACKING_TYPE_NONE.
 *
 * @ingroup aux_util
 */
void
u_builder_setup_tracking_origins(struct xrt_device *head,
                                 struct xrt_device *left,
                                 struct xrt_device *right,
                                 struct xrt_vec3 *global_tracking_origin_offset);

/*!
 * Create a legacy space overseer, most builders probably want to have a more
 * advanced setup then this, especially stand alone ones. Uses
 * @ref u_builder_setup_tracking_origins internally and
 * @ref u_space_overseer_legacy_setup.
 *
 * @ingroup aux_util
 */
void
u_builder_create_space_overseer_legacy(struct xrt_session_event_sink *broadcast,
                                       struct xrt_device *head,
                                       struct xrt_device *left,
                                       struct xrt_device *right,
                                       struct xrt_device **xdevs,
                                       uint32_t xdev_count,
                                       bool root_is_unbounded,
                                       struct xrt_space_overseer **out_xso);

/*!
 * Helper to create a system devices that has static roles and a appropriate
 * space overseer. Currently uses the functions below to create a full system
 * with the help of @p fn argument. But this might change in the future.
 *
 * * @ref u_system_devices_static_allocate
 * * @ref u_system_devices_static_finalize
 * * @ref u_builder_create_space_overseer_legacy
 *
 * @ingroup aux_util
 */
xrt_result_t
u_builder_roles_helper_open_system(struct xrt_builder *xb,
                                   cJSON *config,
                                   struct xrt_prober *xp,
                                   struct xrt_session_event_sink *broadcast,
                                   struct xrt_system_devices **out_xsysd,
                                   struct xrt_space_overseer **out_xso,
                                   u_builder_open_system_fn fn);

/*!
 * Implementation for xrt_builder::open_system to be used with @ref u_builder.
 * Uses @ref u_builder_roles_helper_open_system internally, a builder that uses
 * the @ref u_builder should use this function for xrt_builder::open_system.
 *
 * When using this function the builder must have @ref u_builder and implement
 * the @ref u_builder::open_static_roles function, see documentation for
 * @ref u_builder_open_system_fn about requirements.
 *
 * @ingroup aux_util
 */
xrt_result_t
u_builder_open_system_static_roles(struct xrt_builder *xb,
                                   cJSON *config,
                                   struct xrt_prober *xp,
                                   struct xrt_session_event_sink *broadcast,
                                   struct xrt_system_devices **out_xsysd,
                                   struct xrt_space_overseer **out_xso);


#ifdef __cplusplus
}
#endif
