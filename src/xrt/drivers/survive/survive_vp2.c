// Copyright 2026 Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Vive Pro 2 related code for Libsurvive.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_survive
 */

#include "xrt/xrt_prober.h"

#include "survive_vp2.h"


bool
survive_vp2_init(struct survive_device *survive, struct survive_system *sys)
{
	struct xrt_prober_device **devices = NULL;
	size_t device_count = 0;
	xrt_result_t result = xrt_prober_lock_list(sys->xp, &devices, &device_count);
	if (result != XRT_SUCCESS) {
		SURVIVE_ERROR(survive, "Failed to lock prober devices, cannot check for Vive Pro 2, got err %d",
		              result);
		return false;
	}

	for (size_t i = 0; i < device_count; i++) {
		struct xrt_prober_device *d = devices[i];
		if (d->vendor_id == VP2_VID && d->product_id == VP2_PID) {
			SURVIVE_DEBUG(survive, "Found Vive Pro 2 device in USB list");
			struct os_hid_device *hid;
			result = xrt_prober_open_hid_interface(sys->xp, d, 0, &hid);
			if (result != XRT_SUCCESS) {
				SURVIVE_ERROR(survive, "Failed to open Vive Pro 2 HID interface, got err %d", result);
				survive->hmd.vp2_hid = NULL;
				continue;
			}

			int ret = vp2_hid_open(hid, &survive->hmd.vp2_hid);
			if (ret < 0) {
				SURVIVE_ERROR(survive, "Failed to create Vive Pro 2 HID interface, got err %d", ret);
				continue;
			}

			break;
		}
	}

	result = xrt_prober_unlock_list(sys->xp, &devices);
	if (result != XRT_SUCCESS) {
		SURVIVE_ERROR(survive, "Failed to unlock prober devices after checking for Vive Pro 2, got err %d",
		              result);
		return false;
	}

	if (survive->hmd.vp2_hid == NULL) {
		SURVIVE_WARN(survive, "No Vive Pro 2 device found. This is non-fatal, but some features may not work.");
	}

	return true;
}

void
survive_vp2_setup_hmd(struct survive_device *survive)
{
	struct vp2_config *config = vp2_get_config(survive->hmd.vp2_hid);

	htc_config_get_fov(&config->base, 0, &survive->base.hmd->distortion.fov[0]);
	htc_config_get_fov(&config->base, 1, &survive->base.hmd->distortion.fov[1]);

	enum vp2_resolution resolution = vp2_get_resolution(survive->hmd.vp2_hid);

	int width, height;
	vp2_resolution_get_extents(resolution, &width, &height);

	// Update the refresh rate to the one picked in VP2 config.
	survive->base.hmd->screens[0].nominal_frame_interval_ns =
	    (uint64_t)time_s_to_ns(1.0f / vp2_resolution_get_refresh_rate(resolution));

	// Update the display size to the one picked in VP2 config.
	for (int i = 0; i < 2; i++) {
		survive->base.hmd->views[i].display.w_pixels = width / 2;
		survive->base.hmd->views[i].display.h_pixels = height;

		survive->base.hmd->views[i].viewport.w_pixels = width / 2;
		survive->base.hmd->views[i].viewport.h_pixels = height;
	}

	survive->base.hmd->views[1].viewport.x_pixels = width / 2;

	survive->base.hmd->screens[0].w_pixels = width;
	survive->base.hmd->screens[0].h_pixels = height;

	// With VP2 we can do brightness control.
	survive->base.supported.brightness_control = true;
}

void
survive_vp2_teardown(struct survive_device *survive)
{
	if (survive->hmd.vp2_hid != NULL) {
		vp2_hid_destroy(survive->hmd.vp2_hid);
		survive->hmd.vp2_hid = NULL;
	}
}

xrt_result_t
survive_vp2_compute_distortion(
    struct survive_device *survive, uint32_t view, float u, float v, struct xrt_uv_triplet *result)
{
	struct vp2_config *config = vp2_get_config(survive->hmd.vp2_hid);

	struct xrt_vec2 uv = {.x = u, .y = v};

	// Try VP2 distortion first, fallback if fails.
	if (htc_config_compute_distortion(&config->base, view, &uv, result)) {
		return XRT_SUCCESS;
	}

	return XRT_ERROR_FEATURE_NOT_SUPPORTED;
}

xrt_result_t
survive_vp2_set_brightness(struct survive_device *survive, float brightness, bool relative)
{
	if (relative) {
		float current_brightness;
		xrt_result_t xret = xrt_device_get_brightness(&survive->base, &current_brightness);
		if (xret != XRT_SUCCESS) {
			return xret;
		}

		brightness = current_brightness + brightness;
	}

	int ret = vp2_set_brightness(survive->hmd.vp2_hid, brightness);

	if (ret < 0) {
		SURVIVE_ERROR(survive, "Failed to set brightness to %f, got err %d", brightness, ret);
		return XRT_ERROR_OUTPUT_REQUEST_FAILURE;
	}

	return XRT_SUCCESS;
}
