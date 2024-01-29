// Copyright 2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Visual-Inertial Tracking consumer helper.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Simon Zeni <simon.zeni@collabora.com>
 * @ingroup aux_tracking
 */

#pragma once

#include "vit/vit_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * A bundle of VIT interface functions, used by the tracking interface loader.
 *
 * @ingroup aux_tracking
 */
struct t_vit_bundle
{
	void *handle;

	struct
	{
		uint32_t major;
		uint32_t minor;
		uint32_t patch;
	} version;

	PFN_vit_api_get_version api_get_version;
	PFN_vit_tracker_create tracker_create;
	PFN_vit_tracker_destroy tracker_destroy;
	PFN_vit_tracker_has_image_format tracker_has_image_format;
	PFN_vit_tracker_get_supported_extensions tracker_get_supported_extensions;
	PFN_vit_tracker_get_enabled_extensions tracker_get_enabled_extensions;
	PFN_vit_tracker_enable_extension tracker_enable_extension;
	PFN_vit_tracker_start tracker_start;
	PFN_vit_tracker_stop tracker_stop;
	PFN_vit_tracker_reset tracker_reset;
	PFN_vit_tracker_is_running tracker_is_running;
	PFN_vit_tracker_push_imu_sample tracker_push_imu_sample;
	PFN_vit_tracker_push_img_sample tracker_push_img_sample;
	PFN_vit_tracker_add_imu_calibration tracker_add_imu_calibration;
	PFN_vit_tracker_add_camera_calibration tracker_add_camera_calibration;
	PFN_vit_tracker_pop_pose tracker_pop_pose;
	PFN_vit_tracker_get_timing_titles tracker_get_timing_titles;
	PFN_vit_pose_destroy pose_destroy;
	PFN_vit_pose_get_data pose_get_data;
	PFN_vit_pose_get_timing pose_get_timing;
	PFN_vit_pose_get_features pose_get_features;
};

/*!
 * Load the tracker.
 *
 * @ingroup aux_tracking
 */
bool
t_vit_bundle_load(struct t_vit_bundle *vit, const char *path);

/*!
 * Unload the tracker.
 *
 * @ingroup aux_tracking
 */
void
t_vit_bundle_unload(struct t_vit_bundle *vit);


#ifdef __cplusplus
}
#endif
