// Copyright 2026, Beyley Cardellio.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Helper to implement @ref xrt_app_instance and @ref xrt_app_system.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup base
 */

#pragma once

#include "xrt/xrt_app_policy.h"
#include "xrt/xrt_compositor.h"

#include "os/os_threading.h"


#ifdef __cplusplus
extern "C" {
#endif


/*!
 * @implements xrt_app_instance
 */
struct b_app_instance
{
	struct xrt_app_instance base;

	//! Data for stored app systems
	struct
	{
		struct os_mutex mutex;

		//! Number of application systems stored
		uint32_t count;
		//! Capacity of the storage pointer
		uint32_t capacity;
		//! The storage pointer for all the application systems
		struct xrt_app_system **storage;
	} app_systems;
};

struct b_app_system_event
{
	//! This event
	struct xrt_app_system_event base;
	//! The next event, or NULL if no event.
	struct b_app_system_event *next;
};

/*!
 * @implements xrt_app_system
 */
struct b_app_system
{
	struct xrt_app_system base;

	//! The parent @ref xrt_app_instance
	struct xrt_app_instance *xainst;

	//! The @ref xrt_system this is wrapping
	struct xrt_system *xsys;

	struct os_mutex mutex;
	struct xrt_recommended_view_config view_configs[XRT_VIEW_TYPE_MAX];
	struct b_app_system_event *events;
};


/*!
 * Create a @ref b_app_instance, a fully working @ref xrt_app_instance. Objects wishing to
 * use @ref b_app_instance as a parent class should use @ref b_app_instance_init.
 *
 * @public @memberof b_app_instance
 * @ingroup base
 * @see b_app_instance_init
 */
struct b_app_instance *
b_app_instance_create(void);

/*!
 * Inits a @ref b_app_instance struct when used as a parent class, only to be used externally by the derived class.
 * Not needed to be called if created by @ref b_app_instance_create.
 *
 * @protected @memberof b_app_instance
 * @ingroup base
 */
bool
b_app_instance_init(struct b_app_instance *bainst, void (*destroy_fn)(struct xrt_app_instance *));

/*!
 * Finalizes a @ref b_app_instance struct when used as a parent class, only to be used
 * by the derived class. This will not free the @ref b_app_instance pointer itself but will
 * free any resources created by the default implementation functions. Not
 * needed to be called if created by @ref b_app_instance_create, instead use
 * xrt_app_instance::destroy.
 *
 * @protected @memberof b_app_instance
 * @ingroup base
 */
void
b_app_instance_fini(struct b_app_instance *bainst);


/*!
 * Inits a @ref b_app_system struct when used as a parent class, only to be used
 * by the derived class. Not needed to be called if created by @ref b_app_instance_create_app_system.
 *
 * @protected @memberof b_app_system
 * @ingroup base
 */
bool
b_app_system_init(struct xrt_app_instance *xainst,
                  struct b_app_system *basys,
                  struct xrt_system *xsys,
                  void (*destroy_fn)(struct xrt_app_system *));

/*!
 * Finalizes a @ref b_app_system struct when used as a parent class, only to be used
 * by the derived class. This will not free the @ref b_app_system pointer itself but will
 * free any resources created by the default implementation functions. Not
 * needed to be called if created by @ref b_app_instance_create_app_system, instead use
 * xrt_app_system::destroy.
 *
 * @protected @memberof b_app_system
 * @ingroup base
 */
void
b_app_system_fini(struct b_app_system *basys);


#ifdef __cplusplus
}
#endif
