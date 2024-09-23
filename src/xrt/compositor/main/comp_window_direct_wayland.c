// Copyright 2019, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Wayland direct mode code.
 * @author Drew DeVault <sir@cmpwn.com>
 * @author Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
 * @author Simon Zeni <simon@bl4ckb0ne.ca>
 * @ingroup comp
 */

#include <errno.h>
#include <linux/input.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wayland-client.h>
#include <xf86drm.h>

#include "drm-lease-v1-client-protocol.h"
#include "xrt/xrt_compiler.h"
#include "main/comp_window.h"
#include "main/comp_window_direct.h"
#include "util/u_debug.h"

#ifndef VK_EXT_acquire_drm_display
#error "Wayland direct requires the Vulkan extension VK_EXT_acquire_drm_display"
#endif

DEBUG_GET_ONCE_OPTION(requested_connector, "XRT_COMPOSITOR_WAYLAND_CONNECTOR", NULL)

struct direct_wayland_lease
{
	struct comp_window_direct_wayland *w;

	int leased_fd;
	bool finished;
	struct wp_drm_lease_v1 *lease;
};

struct direct_wayland_lease_connector
{
	struct comp_window_direct_wayland *w;

	uint32_t id;
	char *name, *description;
	struct wp_drm_lease_connector_v1 *connector;
	struct direct_wayland_lease_device *dev;
	struct direct_wayland_lease_connector *next;
};

struct direct_wayland_lease_device
{
	struct comp_window_direct_wayland *w;

	int drm_fd;
	char *path;
	bool done;
	struct wp_drm_lease_device_v1 *device;
	struct direct_wayland_lease_connector *connectors;
	struct direct_wayland_lease_device *next;
};

struct comp_window_direct_wayland
{
	struct comp_target_swapchain base;

	struct wl_display *display;
	struct direct_wayland_lease_device *devices;
	struct direct_wayland_lease *lease;

	// The device and connector to use
	struct direct_wayland_lease_device *selected_device;
	struct direct_wayland_lease_connector *selected_connector;

	VkDisplayKHR vk_display;
};

static void
direct_wayland_lease_device_destroy(struct direct_wayland_lease_device *dev)
{
	struct direct_wayland_lease_connector *conn = dev->connectors, *next_conn;
	while (conn) {
		next_conn = conn->next;

		wp_drm_lease_connector_v1_destroy(conn->connector);

		free(conn->name);
		free(conn->description);
		free(conn);

		conn = next_conn;
	}

	wp_drm_lease_device_v1_destroy(dev->device);
	close(dev->drm_fd);

	free(dev->path);
	free(dev);
}

static void
comp_window_direct_wayland_destroy(struct comp_target *w)
{
	struct comp_window_direct_wayland *w_wayland = (struct comp_window_direct_wayland *)w;

	comp_target_swapchain_cleanup(&w_wayland->base);

	struct direct_wayland_lease_device *dev = w_wayland->devices, *next_dev;
	while (dev) {
		next_dev = dev->next;
		direct_wayland_lease_device_destroy(dev);
		dev = next_dev;
	}

	if (w_wayland->lease) {
		close(w_wayland->lease->leased_fd);
		wp_drm_lease_v1_destroy(w_wayland->lease->lease);
		free(w_wayland->lease);
	}

	if (w_wayland->display) {
		wl_display_disconnect(w_wayland->display);
	}
	free(w_wayland);
}

static inline struct vk_bundle *
get_vk(struct comp_window_direct_wayland *cww)
{
	return &cww->base.base.c->base.vk;
}

static void
_lease_fd(void *data, struct wp_drm_lease_v1 *wp_drm_lease_v1, int32_t leased_fd)
{
	struct direct_wayland_lease *lease = data;
	COMP_DEBUG(lease->w->base.base.c, "Lease granted");

	lease->leased_fd = leased_fd;
}

static void
_lease_finished(void *data, struct wp_drm_lease_v1 *wp_drm_lease_v1)
{
	struct direct_wayland_lease *lease = data;
	if (lease->leased_fd >= 0) {
		close(lease->leased_fd);
		lease->leased_fd = -1;
	}

	COMP_DEBUG(lease->w->base.base.c, "Lease has been closed");
	lease->finished = true;
	/* TODO handle graceful shutdown if lease is currently used */
}

static const struct wp_drm_lease_v1_listener lease_listener = {
    .lease_fd = _lease_fd,
    .finished = _lease_finished,
};

static VkResult
comp_window_direct_wayland_create_surface(struct comp_window_direct_wayland *w,
                                          VkSurfaceKHR *surface,
                                          uint32_t width,
                                          uint32_t height)
{
	struct vk_bundle *vk = get_vk(w);
	w->vk_display = VK_NULL_HANDLE;
	VkResult ret = VK_ERROR_INCOMPATIBLE_DISPLAY_KHR;

	if (w->selected_device == NULL || w->selected_connector == NULL || w->lease == NULL) {
		COMP_ERROR(w->base.base.c, "Connector was disconnected before it could be acquired");
		return VK_ERROR_INITIALIZATION_FAILED;
	}

	ret = vk->vkGetDrmDisplayEXT(vk->physical_device, w->selected_device->drm_fd, w->selected_connector->id,
	                             &w->vk_display);
	if (ret != VK_SUCCESS) {
		COMP_ERROR(w->base.base.c, "vkGetDrmDisplayEXT failed: %s", vk_result_string(ret));
		return ret;
	}

	ret = vk->vkAcquireDrmDisplayEXT(vk->physical_device, w->lease->leased_fd, w->vk_display);
	if (ret != VK_SUCCESS) {
		COMP_ERROR(w->base.base.c, "vkAcquireDrmDisplayEXT failed: %s", vk_result_string(ret));
		return ret;
	}

	ret = comp_window_direct_create_surface(&w->base, w->vk_display, width, height);
	if (ret != VK_SUCCESS) {
		COMP_ERROR(w->base.base.c, "Failed to create surface: %s", vk_result_string(ret));
	}

	return ret;
}

static bool
comp_window_direct_wayland_init_swapchain(struct comp_target *w, uint32_t width, uint32_t height)
{
	struct comp_window_direct_wayland *w_wayland = (struct comp_window_direct_wayland *)w;
	VkResult ret;

	ret = comp_window_direct_wayland_create_surface(w_wayland, &w_wayland->base.surface.handle, width, height);
	if (ret != VK_SUCCESS) {
		COMP_ERROR(w->c, "Failed to create surface!");
		return false;
	}

	struct comp_target_swapchain *cts = (struct comp_target_swapchain *)w_wayland;
	cts->display = w_wayland->vk_display;

	return true;
}

static void
comp_window_direct_wayland_flush(struct comp_target *w)
{
	struct comp_window_direct_wayland *w_wayland = (struct comp_window_direct_wayland *)w;

	while (wl_display_prepare_read(w_wayland->display) != 0)
		wl_display_dispatch_pending(w_wayland->display);

	if (wl_display_flush(w_wayland->display) < 0 && errno != EAGAIN) {
		wl_display_cancel_read(w_wayland->display);
		return;
	}

	struct pollfd fds[] = {
	    {
	        .fd = wl_display_get_fd(w_wayland->display),
	        .events = POLLIN,
	    },
	};

	if (poll(fds, 1, 0) > 0) {
		wl_display_read_events(w_wayland->display);
		wl_display_dispatch_pending(w_wayland->display);
	} else {
		wl_display_cancel_read(w_wayland->display);
	}
}

static void
_lease_connector_name(void *data, struct wp_drm_lease_connector_v1 *wp_drm_lease_connector_v1, const char *name)
{
	struct direct_wayland_lease_connector *conn = data;
	conn->name = strdup(name);
}

static void
_lease_connector_description(void *data,
                             struct wp_drm_lease_connector_v1 *wp_drm_lease_connector_v1,
                             const char *description)
{
	struct direct_wayland_lease_connector *conn = data;
	if (conn->description) {
		free(conn->description);
	}
	conn->description = strdup(description);
}

static void
_lease_connector_connector_id(void *data,
                              struct wp_drm_lease_connector_v1 *wp_drm_lease_connector_v1,
                              uint32_t connector_id)
{
	struct direct_wayland_lease_connector *conn = data;
	conn->id = connector_id;
}

static void
_lease_connector_done(void *data, struct wp_drm_lease_connector_v1 *wp_drm_lease_connector_v1)
{
	struct direct_wayland_lease_connector *conn = data;
	COMP_INFO(conn->w->base.base.c, "[%s] connector %s (%s) id: %d", conn->dev->path, conn->name, conn->description,
	          conn->id);
}

static void
_lease_connector_withdrawn(void *data, struct wp_drm_lease_connector_v1 *wp_drm_lease_connector_v1)
{
	struct direct_wayland_lease_connector *conn = data;
	COMP_INFO(conn->w->base.base.c, "Connector %s has been withdrawn by the compositor", conn->name);
	if (conn == conn->w->selected_connector && !conn->w->lease->lease) {
		conn->w->selected_connector = NULL;
	}
}

static const struct wp_drm_lease_connector_v1_listener lease_connector_listener = {
    .name = _lease_connector_name,
    .description = _lease_connector_description,
    .connector_id = _lease_connector_connector_id,
    .done = _lease_connector_done,
    .withdrawn = _lease_connector_withdrawn,
};

static void
_drm_lease_device_drm_fd(void *data, struct wp_drm_lease_device_v1 *drm_lease_device, int fd)
{
	struct direct_wayland_lease_device *dev = data;
	dev->drm_fd = fd;
	dev->path = drmGetDeviceNameFromFd2(fd);
	COMP_INFO(dev->w->base.base.c, "Available DRM lease device: %s", dev->path);
}

static void
_drm_lease_device_connector(void *data,
                            struct wp_drm_lease_device_v1 *wp_drm_lease_device_v1,
                            struct wp_drm_lease_connector_v1 *wp_drm_lease_connector_v1)
{
	struct direct_wayland_lease_device *dev = data;

	struct direct_wayland_lease_connector *conn = calloc(1, sizeof(struct direct_wayland_lease_connector));
	conn->connector = wp_drm_lease_connector_v1;
	conn->dev = dev;
	conn->w = dev->w;
	wp_drm_lease_connector_v1_add_listener(conn->connector, &lease_connector_listener, conn);

	conn->next = dev->connectors;
	dev->connectors = conn;
}

static void
_drm_lease_device_done(void *data, struct wp_drm_lease_device_v1 *wp_drm_lease_device_v1)
{
	struct direct_wayland_lease_device *dev = data;
	dev->done = true;
}

static void
_drm_lease_device_released(void *data, struct wp_drm_lease_device_v1 *wp_drm_lease_device_v1)
{
	struct direct_wayland_lease_device *dev = data;
	COMP_ERROR(dev->w->base.base.c, "Releasing lease device %s", dev->path);
	if (dev == dev->w->selected_device) {
		dev->w->selected_device = NULL;
		dev->w->selected_connector = NULL;
	}
	direct_wayland_lease_device_destroy(dev);
}

static const struct wp_drm_lease_device_v1_listener drm_lease_device_listener = {
    .drm_fd = _drm_lease_device_drm_fd,
    .connector = _drm_lease_device_connector,
    .done = _drm_lease_device_done,
    .released = _drm_lease_device_released,
};

static void
_registry_global_remove_cb(void *data, struct wl_registry *registry, uint32_t name)
{}

static void
_registry_global_cb(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version)
{
	struct comp_window_direct_wayland *w = data;
	if (strcmp(interface, wp_drm_lease_device_v1_interface.name) == 0) {
		struct direct_wayland_lease_device *dev = calloc(1, sizeof(struct direct_wayland_lease_device));
		dev->device = (struct wp_drm_lease_device_v1 *)wl_registry_bind(registry, name,
		                                                                &wp_drm_lease_device_v1_interface, 1);
		dev->w = w;
		dev->drm_fd = -1;
		wp_drm_lease_device_v1_add_listener(dev->device, &drm_lease_device_listener, dev);

		dev->next = w->devices;
		w->devices = dev;
	}
}

static const struct wl_registry_listener registry_listener = {
    .global = _registry_global_cb,
    .global_remove = _registry_global_remove_cb,
};

// Find the connector matching name, return the first available if no match
static bool
get_named_connector_or_first(struct comp_window_direct_wayland *w, const char *name)
{
	struct direct_wayland_lease_device *dev = w->devices;
	struct direct_wayland_lease_connector *conn = NULL;

	while (dev) {
		if (!dev->done) {
			wl_display_dispatch(w->display);
			continue;
		}
		if (dev->connectors) {
			/* Use the first connector available as default */
			if (!w->selected_connector) {
				w->selected_device = dev;
				w->selected_connector = dev->connectors;
				if (!name) {
					return false;
				}
			}

			// If no connector is chosen, we're done.
			conn = dev->connectors;
			while (conn) {
				if (!strcmp(name, conn->name)) {
					w->selected_device = dev;
					w->selected_connector = conn;
					return true;
				}
				conn = conn->next;
			}
		}
		dev = dev->next;
	}
	return false;
}

static bool
comp_window_direct_wayland_init(struct comp_target *w)
{
	struct comp_window_direct_wayland *w_wayland = (struct comp_window_direct_wayland *)w;

	w_wayland->display = wl_display_connect(NULL);
	if (!w_wayland->display) {
		COMP_ERROR(w->c, "Failed to connect to Wayland display");
		return false;
	}

	struct wl_registry *registry = wl_display_get_registry(w_wayland->display);
	wl_registry_add_listener(registry, &registry_listener, w_wayland);
	wl_display_roundtrip(w_wayland->display);
	wl_registry_destroy(registry);

	if (!w_wayland->devices) {
		COMP_ERROR(w->c, "Compositor is missing drm-lease support");
		return false;
	}

	const char *requested_connector = debug_get_option_requested_connector();
	if (requested_connector) {
		COMP_INFO(w->c, "Requesting connector %s", requested_connector);
	} else {
		COMP_INFO(w->c, "No connector was chosen, will use first available connector");
	}

	bool found = get_named_connector_or_first(w_wayland, requested_connector);

	if (!w_wayland->selected_connector) {
		COMP_INFO(w->c, "Found no connectors available for direct mode");
		return false;
	}

	// Inform when chosen connector was not found
	if (requested_connector && !found) {
		COMP_WARN(w->c, "Could not find requested connector %s, selected first available connector %s",
		          requested_connector, w_wayland->selected_connector->name);
	}

	COMP_INFO(w->c, "Using DRM node %s", w_wayland->selected_device->path);
	COMP_INFO(w->c, "Connector id %d %s (%s)", w_wayland->selected_connector->id,
	          w_wayland->selected_connector->name, w_wayland->selected_connector->description);

	struct wp_drm_lease_request_v1 *request =
	    wp_drm_lease_device_v1_create_lease_request(w_wayland->selected_device->device);
	if (!request) {
		COMP_ERROR(w->c, "Failed to create lease request");
		return false;
	}

	wp_drm_lease_request_v1_request_connector(request, w_wayland->selected_connector->connector);

	struct direct_wayland_lease *lease = calloc(1, sizeof(struct direct_wayland_lease));
	lease->w = w_wayland;
	lease->leased_fd = -1;
	lease->finished = false;
	lease->lease = wp_drm_lease_request_v1_submit(request);

	w_wayland->lease = lease;

	wp_drm_lease_v1_add_listener(lease->lease, &lease_listener, lease);

	while (lease->leased_fd < 0 && !lease->finished) {
		if (wl_display_dispatch(w_wayland->display) == -1) {
			COMP_ERROR(w->c, "wl_display roundtrip failed");
			return false;
		}
	}

	if (lease->finished) {
		COMP_ERROR(w->c, "Failed to lease connector");
		return false;
	}

	return true;
}

static void
_update_window_title(struct comp_target *ct, const char *title)
{
	/* Not required in direct mode */
}

struct comp_target *
comp_window_direct_wayland_create(struct comp_compositor *c)
{
	struct comp_window_direct_wayland *w = U_TYPED_CALLOC(struct comp_window_direct_wayland);

	comp_target_swapchain_init_and_set_fnptrs(&w->base, COMP_TARGET_FORCE_FAKE_DISPLAY_TIMING);

	w->base.base.name = "wayland-direct";
	w->base.display = VK_NULL_HANDLE;
	w->base.base.destroy = comp_window_direct_wayland_destroy;
	w->base.base.flush = comp_window_direct_wayland_flush;
	w->base.base.init_pre_vulkan = comp_window_direct_wayland_init;
	w->base.base.init_post_vulkan = comp_window_direct_wayland_init_swapchain;
	w->base.base.set_title = _update_window_title;
	w->base.base.c = c;

	return &w->base.base;
}


/*
 *
 * Factory
 *
 */

static const char *instance_extensions[] = {
    VK_KHR_DISPLAY_EXTENSION_NAME,             //
    VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,     //
    VK_EXT_DIRECT_MODE_DISPLAY_EXTENSION_NAME, //

#ifdef VK_EXT_acquire_drm_display
    VK_EXT_ACQUIRE_DRM_DISPLAY_EXTENSION_NAME,
#endif
};

static bool
detect(const struct comp_target_factory *ctf, struct comp_compositor *c)
{
	return false;
}

static bool
create_target(const struct comp_target_factory *ctf, struct comp_compositor *c, struct comp_target **out_ct)
{
	struct comp_target *ct = comp_window_direct_wayland_create(c);
	if (ct == NULL) {
		return false;
	}

	*out_ct = ct;

	return true;
}

const struct comp_target_factory comp_target_factory_direct_wayland = {
    .name = "Wayland Direct-Mode",
    .identifier = "direct_wayland",
    .requires_vulkan_for_create = false,
    .is_deferred = false,
    .required_instance_version = 0,
    .required_instance_extensions = instance_extensions,
    .required_instance_extension_count = ARRAY_SIZE(instance_extensions),
    .optional_device_extensions = NULL,
    .optional_device_extension_count = 0,
    .detect = detect,
    .create_target = create_target,
};
