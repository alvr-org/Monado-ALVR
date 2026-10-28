// Copyright 2025-2026, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Constellation tracker dataset playback.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup gui
 */

#include "xrt/xrt_config_build.h"
#include "xrt/xrt_defines.h"
#include "xrt/xrt_tracking.h"

#include "os/os_time.h"

#include "math/m_api.h"
#include "math/m_mathinclude.h"

#include "util/u_debug.h"
#include "util/u_logging.h"
#include "util/u_misc.h"
#include "util/u_time.h"
#include "util/u_var.h"

#include "b_system_devices.h"

#include "gui_common.h"
#include "gui_imgui.h"

#ifdef XRT_MODULE_CONSTELLATION_TRACKING

// @note If you're trying to use this as a template for how to interface with the constellation tracker, do not!
//       Please look at target_builder_rift.c instead. This uses internal APIs for debugging.
#include "constellation/t_constellation_tracker_internal.hpp"
#include "constellation/t_constellation_tracker_dataset.hpp"


namespace xrt::tracking::constellation {

DEBUG_GET_ONCE_OPTION(constellation_tracker_test_filename, "CONSTELLATION_TRACKER_DATASET_FILENAME", "")

extern "C" void
stub_push_constellation_tracker_sample(t_constellation_tracker_device *device, t_constellation_tracker_sample *sample)
{
	(void)device;
	(void)sample;
}

static t_constellation_tracker_device stub_device = {
    .push_constellation_tracker_sample = stub_push_constellation_tracker_sample,
};

extern "C" void
stub_mark_blob_device(struct t_blobwatch *tbw,
                      const struct t_blob_observation *tbo,
                      t_constellation_device_id_t device_id)
{
	(void)tbw;
	(void)tbo;
	(void)device_id;
}

extern "C" void
stub_blobwatch_destroy(struct t_blobwatch *tbw)
{
	(void)tbw;
}

static t_blobwatch stub_blobwatch = {
    .mark_blob_device = stub_mark_blob_device,
    .destroy = stub_blobwatch_destroy,
};

extern "C" void
constellation_tracker_test_play_sample_cb(void *ptr);

struct SceneState
{
public: // Fields
	std::string filename;

	DatasetReader dataset;
	uint32_t num_samples;

	std::unique_ptr<ConstellationTracker> tracker;

	uint16_t sample_index{0};
	u_var_draggable_u16 draggable_sample_index;
	bool slow{true};
	bool wipe_blob_associations{true};
	u_var_button play_sample_button;

public: // Methods
	SceneState(std::string filename) : filename(filename), dataset(filename)
	{
		this->num_samples = static_cast<uint32_t>(this->dataset.samples.size());

		this->draggable_sample_index = {
		    .val = &this->sample_index,
		    .step = 1,
		    .min = 0,
		    .max = static_cast<uint16_t>(this->num_samples > 0 ? this->num_samples - 1 : 0),
		};

		this->play_sample_button = {
		    .cb = constellation_tracker_test_play_sample_cb,
		    .ptr = this,
		    .downed = 0,
		    .label = {},
		    .width = 0.0f,
		    .height = 0.0f,
		    .disabled = false,
		};

		t_constellation_tracker_params params = {
		    .flags = T_CONSTELLATION_TRACKER_FLAGS_DETERMINISTIC,
		    .mosaics = {},
		    .num_mosaics = static_cast<size_t>(this->dataset.mosaics.size()),
		};

		for (size_t i = 0; i < this->dataset.mosaics.size(); i++) {
			params.mosaics[i] = {
			    // @todo support tracking origins in dataset files by interpolating the camera poses in the
			    //       samples.
			    .tracking_origin = nullptr,
			    .cameras = {},
			    .num_cameras = this->dataset.mosaics[i].camera_calibrations.size(),
			};

			for (size_t j = 0; j < this->dataset.mosaics[i].camera_calibrations.size(); j++) {
				params.mosaics[i].cameras[j] = {
				    .calibration = this->dataset.mosaics[i].camera_calibrations[j],
				    // @todo support camera poses in dataset files by interpolating the camera poses in
				    //       the samples.
				    .pose_in_origin = XRT_POSE_IDENTITY,
				    .has_concrete_pose = false,
				    .blob_sink = nullptr,
				};
			}
		}

		// Just grab the latest pose of the camera, good enough for testing
		for (auto &sample : this->dataset.samples) {
			params.mosaics[sample.mosaic_index].cameras[sample.camera_index].has_concrete_pose =
			    sample.Txr_world_cam.has_value();
			params.mosaics[sample.mosaic_index].cameras[sample.camera_index].pose_in_origin =
			    sample.Txr_world_cam.value_or(xrt_pose(XRT_POSE_IDENTITY));
		}

		this->tracker = std::make_unique<ConstellationTracker>(&params);

		for (auto &device : this->dataset.devices) {
			t_constellation_tracker_device_params device_params = {
			    .led_model = device.led_model,
			    // @todo support tracking sources in dataset files by interpolating the tracking source pose
			    //       in the samples.
			    .tracking_source = nullptr,
			    .imu_sink = nullptr,
			};

			this->tracker->addDevice(&device_params, &stub_device);
		}

		tracker->setupVariableTracking();
	};
};

void
constellation_tracker_test_play_sample_cb(void *ptr)
{
	SceneState *scene = (SceneState *)ptr;

	if (scene->dataset.samples.empty()) {
		CT_WARN(scene->tracker.get(), "No samples in dataset to play");
		return;
	}

	CameraSample sample = scene->dataset.samples[scene->sample_index];
	sample.source = &stub_blobwatch;

	if (scene->wipe_blob_associations) {
		for (uint32_t i = 0; i < sample.blob_count; i++) {
			sample.blobs[i].matched_device_id = XRT_CONSTELLATION_INVALID_DEVICE_ID;
			sample.blobs[i].matched_device_led_id = XRT_CONSTELLATION_INVALID_LED_ID;
		}
	}

	if (scene->slow) {
		for (uint32_t i = 0; i < sample.device_count; i++) {
			// Force a slow process
			sample.device_states[i].needs_slow_processing = true;
		}

		scene->tracker->mosaics[sample.mosaic_index]->cameras[sample.camera_index]->processSampleSlow(sample);
	} else {
		sample.device_count = 0; // always filled in by fast process
		scene->tracker->mosaics[sample.mosaic_index]->cameras[sample.camera_index]->processSampleFast(sample);
	}
}

}; // namespace xrt::tracking::constellation

using namespace xrt::tracking::constellation;

static void
do_setup(gui_program *p)
{
	SceneState *test_scene = new SceneState(debug_get_option_constellation_tracker_test_filename());

	u_var_add_root(test_scene, "Constellation Tracker Test", true);

	u_var_add_ro_text(test_scene, test_scene->filename.c_str(), "Filename");
	u_var_add_ro_u32(test_scene, &test_scene->num_samples, "Number of Samples");

	u_var_add_draggable_u16(test_scene, &test_scene->draggable_sample_index, "Sample Index");
	u_var_add_button(test_scene, &test_scene->play_sample_button, "Play Sample");
	u_var_add_bool(test_scene, &test_scene->slow, "Slow Processing");
	u_var_add_bool(test_scene, &test_scene->wipe_blob_associations, "Wipe Blob Associations");
}

void
gui_scene_constellation_tracker_test(gui_program *p)
{
	try {
		do_setup(p);
	} catch (const std::exception &e) {
		U_LOG_E("Failed to set up constellation tracker test scene: %s", e.what());
		return;
	}

	gui_scene_debug(p);
}

#else

void
gui_scene_constellation_tracker_test(gui_program *p)
{
	(void)p;
}

#endif // XRT_MODULE_CONSTELLATION_TRACKING
