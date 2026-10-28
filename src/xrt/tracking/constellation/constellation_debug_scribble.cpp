// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Implementation of the debug scribble logic for the constellation tracker.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup tracking
 */

#include "t_constellation_tracker_internal.hpp"

#include <string>


namespace xrt::tracking::constellation {

/*
 *
 * Helper methods
 *
 */

static xrt_colour_rgba_u8
device_id_to_color(t_constellation_device_id_t device_id)
{
	// Simple deterministic hash for device ID, ensuring device ID is non-zero
	uint8_t r = (((device_id + 1) * 37) % 127) + 128;
	uint8_t g = (((device_id + 1) * 57) % 127) + 128;
	uint8_t b = (((device_id + 1) * 97) % 127) + 128;

	return {r, g, b, 255};
}

static void
scribble_led_model(xrt_frame *frame,
                   Camera &camera,
                   CameraSample &sample,
                   t_constellation_tracker_led_model *led_model,
                   t_constellation_device_id_t device_id,
                   xrt_pose &Tcv_cam_model,
                   xrt_colour_rgba_u8 &colour)
{
	const xrt_colour_rgba_u8 colour_dim = {
	    .r = (uint8_t)(colour.r * 0.8),
	    .g = (uint8_t)(colour.g * 0.8),
	    .b = (uint8_t)(colour.b * 0.8),
	    .a = colour.a,
	};

	pose_metrics_blob_match_info visible_info = {};
	pose_metrics_match_pose_to_blobs(&Tcv_cam_model, sample.blobs, sample.blob_count, led_model, device_id,
	                                 &camera.model, &visible_info);

	xrt_vec2 projected_model_origin_point;
	if (t_camera_models_project(&camera.model.calib_true, Tcv_cam_model.position.x, Tcv_cam_model.position.y,
	                            Tcv_cam_model.position.z, &projected_model_origin_point.x,
	                            &projected_model_origin_point.y)) {
		// Scribble the origin point of the LED model
		u_frame_scribble_cross(frame, projected_model_origin_point.x, projected_model_origin_point.y, 5,
		                       &colour_dim);
	}

	for (int i = 0; i < visible_info.num_visible_leds; i++) {
		pose_metrics_visible_led_info *led_info = visible_info.visible_leds + i;
		const bool matched = led_info->matched_blob != NULL;

		const xrt_colour_rgba_u8 matched_colour = colour;
		const xrt_colour_rgba_u8 unmatched_colour = {
		    .r = 255,
		    .g = 255,
		    .b = 0,
		    .a = colour.a,
		};

		const xrt_colour_rgba_u8 &led_colour = matched ? matched_colour : unmatched_colour;

		// Scribble where the LED is, @todo this needs to be distorted eventually
		u_frame_scribble_cross(frame, led_info->pos_px_undistorted.x, led_info->pos_px_undistorted.y, 5,
		                       &led_colour);

		std::string led_name = std::to_string(led_info->led->id);
		// Scribble the LED name
		u_frame_scribble_text(frame, led_info->pos_px_undistorted.x + 5, led_info->pos_px_undistorted.y + 5,
		                      led_name.c_str(), 16, &led_colour);
	}
}

static void
scribble_blobs(xrt_frame *frame, Camera &camera, CameraSample &sample)
{
	for (uint32_t i = 0; i < sample.blob_count; i++) {
		auto &blob = sample.blobs[i];

		const xrt_colour_rgba_u8 colour = blob.matched_device_id != XRT_CONSTELLATION_INVALID_DEVICE_ID
		                                      ? device_id_to_color(blob.matched_device_id)
		                                      : xrt_colour_rgba_u8{255, 255, 255, 255};

		u_frame_scribble_cross(frame, blob.center_distorted.x, blob.center_distorted.y, 5, &colour);

		std::string blob_name;
		if (camera.scribble_settings.draw_blob_ids) {
			if (blob_name.length() > 0) {
				blob_name += ' ';
			}

			blob_name += "b:" + std::to_string(blob.blob_id);
		}

		if (camera.scribble_settings.draw_blob_device_ids) {
			if (blob_name.length() > 0) {
				blob_name += ' ';
			}

			if (blob.matched_device_id != XRT_CONSTELLATION_INVALID_DEVICE_ID) {
				blob_name += "d:" + std::to_string(blob.matched_device_id);
			} else {
				blob_name += "d:-";
			}
		}

		if (camera.scribble_settings.draw_blob_led_ids) {
			if (blob_name.length() > 0) {
				blob_name += ' ';
			}

			if (blob.matched_device_led_id != XRT_CONSTELLATION_INVALID_LED_ID) {
				blob_name += "l:" + std::to_string(blob.matched_device_led_id);
			} else {
				blob_name += "l:-";
			}
		}

		u_frame_scribble_text(frame,                       //
		                      blob.center_distorted.x + 5, //
		                      blob.center_distorted.y + 5, //
		                      blob_name.c_str(),           //
		                      16,                          //
		                      &colour);                    //
	}
}

/*
 *
 * Camera methods
 *
 */

void
Camera::debugScribbleSample(CameraSample &sample, bool fast)
{
	ConstellationTracker &tracker = *this->tracker;

	std::shared_ptr<CameraMosaic> mosaic = this->mosaic.lock();
	U_ASSERT_WEAK_PTR_RET(mosaic,
	                      "Camera mosaic was destroyed while preparing debug scribbles, this should never "
	                      "happen since the mosaic owns the camera");

	auto debug_sink =
	    fast ? &this->fast_processing_thread_data.debug_sink : &this->slow_processing_thread_data.debug_sink;

	if (!u_sink_debug_is_active(debug_sink)) {
		return;
	}

	xrt_frame *frame = NULL;
	u_frame_create_one_off(XRT_FORMAT_R8G8B8X8, this->model.width_true, this->model.height_true, &frame);
	memset(frame->data, 0, frame->size);

	{
		std::shared_lock lock(tracker.device_lock);

		// Iterate over the devices
		for (auto &device : tracker.devices) {
			auto maybe_device_state = sample.getDeviceState(device->id);
			if (!maybe_device_state.has_value()) {
				continue; // This device isn't in the sample, skip it
			}
			auto device_state = maybe_device_state.value();

			auto colour = device_id_to_color(device->id);

			// Draw the watched blobs
			if (this->scribble_settings.draw_blobs) {
				scribble_blobs(frame, *this, sample);
			}

			// Draw the prior pose
			if (this->scribble_settings.draw_prior && device_state->Txr_world_device_prior.has_value() &&
			    sample.Txr_world_cam.has_value()) {
				xrt_pose Txr_cam_world;
				math_pose_invert(&sample.Txr_world_cam.value(), &Txr_cam_world);

				xrt_pose Txr_cam_device_prior;
				math_pose_transform(&Txr_cam_world, &device_state->Txr_world_device_prior.value(),
				                    &Txr_cam_device_prior);

				xrt_pose Tcv_cam_device_prior;
				math_pose_convert_from_opencv(&Txr_cam_device_prior, &Tcv_cam_device_prior);

				scribble_led_model(frame,                           //
				                   *this,                           //
				                   sample,                          //
				                   device->search_model->led_model, //
				                   device_state->device_id,         //
				                   Tcv_cam_device_prior,            //
				                   colour);                         //
			}

			// Draw the found pose
			if (this->scribble_settings.draw_found && device_state->found_pose.has_value()) {
				auto &found_pose = device_state->found_pose.value();

				scribble_led_model(frame,                           //
				                   *this,                           //
				                   sample,                          //
				                   device->search_model->led_model, //
				                   device_state->device_id,         //
				                   found_pose.Tcv_cam_device,       //
				                   colour);                         //
			}
		}
	}

	u_sink_debug_push_frame(debug_sink, frame);

	xrt_frame_reference(&frame, nullptr);
}

/*
 *
 * CameraScribbleSettings methods
 *
 */

void
CameraScribbleSettings::setupDebugTracking(void *root)
{
	u_var_add_bool(root, &this->draw_blobs, "Draw watched blobs");
	u_var_add_bool(root, &this->draw_blob_ids, "Draw blob IDs");
	u_var_add_bool(root, &this->draw_blob_device_ids, "Draw blob matched device IDs");
	u_var_add_bool(root, &this->draw_blob_led_ids, "Draw blob matched LED IDs");

	u_var_add_bool(root, &this->draw_prior, "Draw prior device pose");
	u_var_add_bool(root, &this->draw_found, "Draw found device pose");
}

}; // namespace xrt::tracking::constellation
