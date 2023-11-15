// Copyright 2019-2022, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header for null compositor.
 *
 * Based on src/xrt/compositor/main/comp_compositor.h
 *
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @ingroup comp_null
 */

#pragma once

#include "xrt/xrt_gfx_vk.h"
#include "xrt/xrt_instance.h"

#include "os/os_time.h"

#include "util/u_threading.h"
#include "util/u_logging.h"
#include "util/u_pacing.h"

#include "util/comp_base.h"


#ifdef __cplusplus
extern "C" {
#endif

/*!
 * @defgroup comp_null Null compositor
 * @ingroup xrt
 * @brief A non-rendering alternate for the main compositor that still can
 * support applications fully.
 *
 * Monado's design is highly modular, including allowing alternate compositors to
 * be used. If you are looking to write an additional or alternate compositor to
 * the one in `src/xrt/compositor/main`, this code is your starting point. It is
 * the basic implementation of @ref xrt_compositor_native extracted from there,
 * renamed, and with most implementations removed. Compare with similarly-named
 * files there to see what was removed, and what helper functionality has been
 * factored out and may be reusable. For example, you may be able to use @ref
 * comp_renderer, @ref comp_resources, @ref comp_shaders, and @ref comp_target,
 * among others.
 */


/*
 *
 * Structs, enums and defines.
 *
 */

/*!
 * Tracking frame state.
 *
 * @ingroup comp_null
 */
struct null_comp_frame
{
	int64_t id;
	uint64_t predicted_display_time_ns;
	uint64_t desired_present_time_ns;
	uint64_t present_slop_ns;
};

/*!
 * Main compositor struct tying everything in the compositor together.
 *
 * This ultimately implements @ref xrt_compositor_native but does so by
 * extending @ref comp_base, similar to how @ref comp_compositor works.
 *
 * @extends comp_base
 * @ingroup comp_null
 */
struct null_compositor
{
	struct comp_base base;

	//! The device we are displaying to.
	struct xrt_device *xdev;

	//! Pacing helper to drive us forward.
	struct u_pacing_compositor *upc;

	struct
	{
		enum u_logging_level log_level;

		//! Frame interval that we are using.
		uint64_t frame_interval_ns;
	} settings;

	// Kept here for convenience.
	struct xrt_system_compositor_info sys_info;

	//! @todo Insert your own required members here

	struct
	{
		struct null_comp_frame waited;
		struct null_comp_frame rendering;
	} frame;
};


/*
 *
 * Functions and helpers.
 *
 */

/*!
 * Convenience function to convert a xrt_compositor to a null_compositor.
 * (Down-cast helper.)
 *
 * @private @memberof null_compositor
 * @ingroup comp_null
 */
static inline struct null_compositor *
null_compositor(struct xrt_compositor *xc)
{
	return (struct null_compositor *)xc;
}

/*!
 * Spew level logging.
 *
 * @relates null_compositor
 * @ingroup comp_null
 */
#define NULL_TRACE(c, ...) U_LOG_IFL_T(c->settings.log_level, __VA_ARGS__);

/*!
 * Debug level logging.
 *
 * @relates null_compositor
 */
#define NULL_DEBUG(c, ...) U_LOG_IFL_D(c->settings.log_level, __VA_ARGS__);

/*!
 * Info level logging.
 *
 * @relates null_compositor
 * @ingroup comp_null
 */
#define NULL_INFO(c, ...) U_LOG_IFL_I(c->settings.log_level, __VA_ARGS__);

/*!
 * Warn level logging.
 *
 * @relates null_compositor
 * @ingroup comp_null
 */
#define NULL_WARN(c, ...) U_LOG_IFL_W(c->settings.log_level, __VA_ARGS__);

/*!
 * Error level logging.
 *
 * @relates null_compositor
 * @ingroup comp_null
 */
#define NULL_ERROR(c, ...) U_LOG_IFL_E(c->settings.log_level, __VA_ARGS__);


#ifdef __cplusplus
}
#endif
