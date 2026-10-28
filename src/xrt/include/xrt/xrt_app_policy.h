// Copyright 2026, Beyley Cardellio.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header for application policy object.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup xrt_iface
 */

#pragma once

#include "xrt/xrt_defines.h"
#include "xrt/xrt_limits.h"


#ifdef __cplusplus
extern "C" {
#endif


struct xrt_system;
struct xrt_app_system;
struct xrt_view_config;

/*!
 * @interface xrt_app_instance
 *
 * This interface acts as a way to store per-application policy for @ref xrt_instance.
 *
 * Only a single one of these should be created per application (aka XrInstance, in OpenXR terminology).
 *
 * @sa xrt_instance
 * @sa xrt_instance_create_app_instance
 */
struct xrt_app_instance
{
	/*!
	 * @name Interface Methods
	 *
	 * All implementations of the xrt_app_instance interface must
	 * populate all these function pointers with their implementation
	 * methods. To use this interface, see the helper functions.
	 * @{
	 */

	/*!
	 * Creates an application system from the passed system.
	 *
	 * All @ref xrt_app_system instances created by this function are expected to be destroyed before the
	 * xrt_app_instance is destroyed.
	 *
	 * @note Code consuming this interface should use xrt_app_instance_create_app_system()
	 *
	 * @param      xainst    Pointer to self
	 * @param      xsys      Pointer to system to wrap.
	 * @param[out] out_xasys Return of application system, required.
	 */
	xrt_result_t (*create_app_system)(struct xrt_app_instance *xainst,
	                                  struct xrt_system *xsys,
	                                  struct xrt_app_system **out_xasys);

	/*!
	 * Destroy the application instance and its owned objects.
	 *
	 * Code consuming this interface should use xrt_app_instance_destroy().
	 *
	 * @param xainst Pointer to self
	 */
	void (*destroy)(struct xrt_app_instance *xainst);

	/*!
	 * @}
	 */
};

/*!
 * @copydoc xrt_app_instance::create_app_system
 *
 * Helper for calling through the function pointer.
 *
 * @public @memberof xrt_app_instance
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_app_instance_create_app_system(struct xrt_app_instance *xainst,
                                   struct xrt_system *xsys,
                                   struct xrt_app_system **out_xasys)
{
	return xainst->create_app_system(xainst, xsys, out_xasys);
}

/*!
 * @copydoc xrt_app_instance::destroy
 *
 * Helper for calling through the function pointer.
 *
 * @public @memberof xrt_app_instance
 */
XRT_NONNULL_ALL static inline void
xrt_app_instance_destroy(struct xrt_app_instance **xainst_ptr)
{
	struct xrt_app_instance *xainst = *xainst_ptr;
	if (xainst == NULL) {
		return;
	}

	xainst->destroy(xainst);
	*xainst_ptr = NULL;
}


/*!
 * Type of an @ref xrt_app_system event.
 *
 * @see xrt_app_system_event
 * @ingroup xrt_iface
 */
enum xrt_app_system_event_type
{
	//! This app system has no events.
	XRT_APP_SYSTEM_EVENT_NONE = 0,

	//! The recommended view configuration of some view type has changed.
	XRT_APP_SYSTEM_EVENT_RECOMMENDED_VIEW_CONFIGURATION_CHANGED = 1,
};

/*!
 * Recommended view configuration changed event, type @ref XRT_APP_SYSTEM_EVENT_RECOMMENDED_VIEW_CONFIGURATION_CHANGED.
 *
 * @see xrt_app_system_event
 * @ingroup xrt_iface
 */
struct xrt_app_system_event_recommended_view_configuration_changed
{
	//! The @ref xrt_view_type that has changed.
	enum xrt_view_type view_type;
};

/*!
 * Union of all app system events, used to return multiple events through one interface.
 *
 * @see xrt_app_system_event
 * @ingroup xrt_iface
 */
struct xrt_app_system_event
{
	enum xrt_app_system_event_type type;
	union {
		struct xrt_app_system_event_recommended_view_configuration_changed
		    recommended_view_configuration_changed;
	};
};

/*!
 * A single view in a recommended view configuration
 *
 * @ingroup xrt_iface
 */
struct xrt_recommended_view_config_view
{
	//! The width of the view.
	uint32_t width_pixels;
	//! The height of the view.
	uint32_t height_pixels;
	//! The amount of samples for the swapchain.
	uint32_t sample_count;
};

/*!
 * A recommended view configuration
 *
 * @ingroup xrt_iface
 */
struct xrt_recommended_view_config
{
	//! Whether the recommendation is valid.
	bool valid;
	//! The number of views in the configuration, invariant.
	uint32_t view_count;
	//! The views in the configuration.
	struct xrt_recommended_view_config_view views[XRT_MAX_VIEWS];
};

/*!
 * @interface xrt_app_system
 *
 * This interface acts as a way to store per-application policy for @ref xrt_system.
 *
 * Only a single one of these should be created per @ref xrt_system per application
 * (aka per XrInstance, in OpenXR terminology).
 *
 * @sa xrt_app_instance_create_app_system
 */
struct xrt_app_system
{
	/*!
	 * @name Interface Methods
	 *
	 * All implementations of the xrt_app_system interface must
	 * populate all these function pointers with their implementation
	 * methods. To use this interface, see the helper functions.
	 * @{
	 */

	/*!
	 * Poll a single event from this app system, if no event is available then
	 * the type of the event will be @ref XRT_APP_SYSTEM_EVENT_NONE.
	 *
	 * @param      xasys      Pointer to self
	 * @param[out] out_xasyse Event to be returned.
	 */
	xrt_result_t (*poll_events)(struct xrt_app_system *xasys, struct xrt_app_system_event *out_xasyse);

	/*!
	 * Gets the recommended view configuration for the given view type, if available and supported.
	 *
	 * @param      xasys                       Pointer to self
	 * @param      view_type                   The view type to get the recommended view configuration for.
	 * @param[out] out_recommended_view_config The recommendation for the client.
	 */
	xrt_result_t (*get_recommended_view_configuration)(
	    struct xrt_app_system *xasys,
	    enum xrt_view_type view_type,
	    struct xrt_recommended_view_config *out_recommended_view_config);

	/*!
	 * Sets the recommended view configuration for this application, for the given view type.
	 *
	 * @note The xrt_app_system may not do verification on the passed configuration, it is up to the callee to not
	 *       pass invalid data into here.
	 *
	 * @param     xasys                   Pointer to self
	 * @param     view_type               The view type to set the recommended view configuration for.
	 * @param[in] recommended_view_config The recommended view configuration to set for the given view type.
	 */
	xrt_result_t (*set_recommended_view_configuration)(
	    struct xrt_app_system *xasys,
	    enum xrt_view_type view_type,
	    const struct xrt_recommended_view_config *recommended_view_config);

	/*!
	 * Destroy the application system and its owned objects.
	 *
	 * @note Code consuming this interface should use xrt_app_system_destroy().
	 *
	 * @param xasys Pointer to self
	 */
	void (*destroy)(struct xrt_app_system *xasys);

	/*!
	 * @}
	 */
};

/*!
 * @copydoc xrt_app_system::poll_events
 *
 * Helper for calling through the function pointer.
 *
 * @public @memberof xrt_app_system
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_app_system_poll_events(struct xrt_app_system *xasys, struct xrt_app_system_event *out_xasyse)
{
	return xasys->poll_events(xasys, out_xasyse);
}

/*!
 * @copydoc xrt_app_system::get_recommended_view_configuration
 *
 * Helper for calling through the function pointer.
 *
 * @public @memberof xrt_app_system
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_app_system_get_recommended_view_configuration(struct xrt_app_system *xasys,
                                                  enum xrt_view_type view_type,
                                                  struct xrt_recommended_view_config *out_recommended_view_config)
{
	return xasys->get_recommended_view_configuration(xasys, view_type, out_recommended_view_config);
}

/*!
 * @copydoc xrt_app_system::set_recommended_view_configuration
 *
 * Helper for calling through the function pointer.
 *
 * @public @memberof xrt_app_system
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_app_system_set_recommended_view_configuration(struct xrt_app_system *xasys,
                                                  enum xrt_view_type view_type,
                                                  const struct xrt_recommended_view_config *recommended_view_config)
{
	return xasys->set_recommended_view_configuration(xasys, view_type, recommended_view_config);
}

/*!
 * @copydoc xrt_app_system::destroy
 *
 * Helper for calling through the function pointer.
 *
 * @public @memberof xrt_app_system
 */
XRT_NONNULL_ALL static inline void
xrt_app_system_destroy(struct xrt_app_system **xasys_ptr)
{
	struct xrt_app_system *xasys = *xasys_ptr;
	if (xasys == NULL) {
		return;
	}

	xasys->destroy(xasys);
	*xasys_ptr = NULL;
}


#ifdef __cplusplus
};
#endif
