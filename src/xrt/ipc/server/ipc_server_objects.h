// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Tracking objects to IDs.
 * @author Jakob Bornecrantz <tbornecrantz@nvidia.com>
 * @ingroup ipc_server
 */

#pragma once

#include "xrt/xrt_results.h"

struct xrt_space;
struct xrt_body_tracker;
struct xrt_hand_tracker;
struct xrt_io_cache;
struct ipc_client_state;


/*
 *
 * Device functions.
 *
 */

/*!
 * Get a device by its per-client-connection slot index.
 *
 * The @p id is a @c uint32_t index into @c ics->objects.xdevs used for IPC
 * message lookups. It is not @ref xrt_device::id.
 *
 * Must only be called from the per client thread as this function accesses the
 * client state's memory.
 *
 * @param ics The client state instance.
 * @param id The per-client slot index.
 * @param out_xdev Will be filled with the device object on success.
 * @return XRT_SUCCESS on success, some other result on failure.
 *
 * @ingroup ipc_server
 */
xrt_result_t
ipc_server_objects_get_xdev_and_validate(volatile struct ipc_client_state *ics,
                                         uint32_t id,
                                         struct xrt_device **out_xdev);

/*!
 * Get or allocate a per-client-connection slot index for a device object.
 *
 * The returned @c uint32_t is used in IPC messages to look up @p xdev on the
 * server. It is not @ref xrt_device::id, which is assigned at device creation
 * and synchronized to clients via @ref ipc_device_info.
 *
 * Must only be called from the per client thread as this function accesses the
 * client state's memory.
 *
 * @param ics The client state instance.
 * @param xdev The device object.
 * @param out_id Will be filled with the slot index on success.
 * @return XRT_SUCCESS on success, some other result on failure.
 *
 * @ingroup ipc_server
 */
xrt_result_t
ipc_server_objects_get_xdev_id_or_add(volatile struct ipc_client_state *ics, struct xrt_device *xdev, uint32_t *out_id);


/*
 *
 * Tracking origin functions.
 *
 */

/*!
 * Get a tracking origin by ID, must only be called from the per client
 * thread as this function accesses the client state's memory.
 *
 * @param ics The client state instance.
 * @param id The tracking origin ID.
 * @param out_xtrack Will be filled with the tracking origin object on success.
 * @return XRT_SUCCESS on success, some other result on failure.
 *
 * @ingroup ipc_server
 */
xrt_result_t
ipc_server_objects_get_xtrack_and_validate(volatile struct ipc_client_state *ics,
                                           uint32_t id,
                                           struct xrt_tracking_origin **out_xtrack);

/*!
 * Get a tracking origin ID for a given tracking origin object, must only be
 * called from the per client thread as this function accesses the client
 * state's memory.
 *
 * @param ics The client state instance.
 * @param xtrack The tracking origin object.
 * @param out_id Will be filled with the tracking origin ID on success.
 * @return XRT_SUCCESS on success, some other result on failure.
 *
 * @ingroup ipc_server
 */
xrt_result_t
ipc_server_objects_get_xtrack_id_or_add(volatile struct ipc_client_state *ics,
                                        struct xrt_tracking_origin *xtrack,
                                        uint32_t *out_id);


/*
 *
 * Space functions.
 *
 */

/*!
 * Get a space by ID, must only be called from the per client thread.
 *
 * @ingroup ipc_server
 */
xrt_result_t
ipc_server_objects_get_xspc_and_validate(volatile struct ipc_client_state *ics,
                                         uint32_t id,
                                         struct xrt_space **out_xspc);

/*!
 * Track a space and return its ID. Grabs and stores a reference if newly
 * tracked.
 *
 * @ingroup ipc_server
 */
xrt_result_t
ipc_server_objects_get_xspc_id_or_add(volatile struct ipc_client_state *ics, struct xrt_space *xspc, uint32_t *out_id);

/*!
 * Destroy a tracked space.
 *
 * @ingroup ipc_server
 */
xrt_result_t
ipc_server_objects_destroy_xspc(volatile struct ipc_client_state *ics, uint32_t id);


/*
 *
 * Body tracker functions.
 *
 */

/*!
 * Get a body tracker by ID, must only be called from the per client thread.
 *
 * @ingroup ipc_server
 */
xrt_result_t
ipc_server_objects_get_xbt_and_validate(volatile struct ipc_client_state *ics,
                                        uint32_t id,
                                        struct xrt_body_tracker **out_xbt);

/*!
 * Track a newly created body tracker and return its ID. Takes ownership of
 * @p xbt until explicit destroy or client disconnect.
 *
 * @ingroup ipc_server
 */
xrt_result_t
ipc_server_objects_get_xbt_id_or_add(volatile struct ipc_client_state *ics,
                                     struct xrt_body_tracker *xbt,
                                     uint32_t *out_id);

/*!
 * Destroy a tracked body tracker.
 *
 * @ingroup ipc_server
 */
xrt_result_t
ipc_server_objects_destroy_xbt(volatile struct ipc_client_state *ics, uint32_t id);


/*
 *
 * Hand tracker functions.
 *
 */

/*!
 * Get a hand tracker by ID, must only be called from the per client thread.
 *
 * @ingroup ipc_server
 */
xrt_result_t
ipc_server_objects_get_xht_and_validate(volatile struct ipc_client_state *ics,
                                        uint32_t id,
                                        struct xrt_hand_tracker **out_xht);

/*!
 * Track a newly created hand tracker and return its ID. Takes ownership of
 * @p xht until explicit destroy or client disconnect.
 *
 * @ingroup ipc_server
 */
xrt_result_t
ipc_server_objects_get_xht_id_or_add(volatile struct ipc_client_state *ics,
                                     struct xrt_hand_tracker *xht,
                                     uint32_t *out_id);

/*!
 * Destroy a tracked hand tracker.
 *
 * @ingroup ipc_server
 */
xrt_result_t
ipc_server_objects_destroy_xht(volatile struct ipc_client_state *ics, uint32_t id);


/*
 *
 * App policy functions.
 *
 */

xrt_result_t
ipc_server_objects_get_xainst_and_validate(volatile struct ipc_client_state *ics,
                                           uint32_t id,
                                           struct xrt_app_instance **out_xainst);

xrt_result_t
ipc_server_objects_get_xainst_id_or_add(volatile struct ipc_client_state *ics,
                                        struct xrt_app_instance *xainst,
                                        uint32_t *out_id);

xrt_result_t
ipc_server_objects_destroy_xainst(volatile struct ipc_client_state *ics, uint32_t id);

xrt_result_t
ipc_server_objects_get_xasys_and_validate(volatile struct ipc_client_state *ics,
                                          uint32_t id,
                                          struct xrt_app_system **out_xasys);

xrt_result_t
ipc_server_objects_get_xasys_id_or_add(volatile struct ipc_client_state *ics,
                                       struct xrt_app_system *xasys,
                                       uint32_t *out_id);

xrt_result_t
ipc_server_objects_destroy_xasys(volatile struct ipc_client_state *ics, uint32_t id);


/*
 *
 * Input/output cache functions.
 *
 */

/*!
 * Get an input/output cache by ID, must only be called from the per client thread.
 *
 * @ingroup ipc_server
 */
xrt_result_t
ipc_server_objects_get_xic_and_validate(volatile struct ipc_client_state *ics,
                                        uint32_t id,
                                        struct xrt_io_cache **out_xic);

/*!
 * Track a newly created input/output cache and return its ID. Takes ownership of
 * @p xic until explicit destroy or client disconnect.
 *
 * @ingroup ipc_server
 */
xrt_result_t
ipc_server_objects_get_xic_id_or_add(volatile struct ipc_client_state *ics, struct xrt_io_cache *xic, uint32_t *out_id);

/*!
 * Destroy a tracked input/output cache.
 *
 * @ingroup ipc_server
 */
xrt_result_t
ipc_server_objects_destroy_xic(volatile struct ipc_client_state *ics, uint32_t id);
