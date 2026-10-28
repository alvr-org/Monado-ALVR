// Copyright 2025, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Audio resampler test.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup gui
 */

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>

#include "os/os_time.h"

#include "math/m_api.h"
#include "math/m_mathinclude.h"

#include "util/u_debug.h"
#include "util/u_logging.h"
#include "util/u_misc.h"
#include "util/u_time.h"
#include "util/u_resampler.h"

#include "xrt/xrt_defines.h"
#include "xrt/xrt_tracking.h"

#include "gui_common.h"
#include "gui_imgui.h"

#define CSV_EOL "\r\n"
// #define CSV_PRECISION 10
#define FMT_F32_GENERAL "%.4f"
#define FMT_F32_TIME "%.6f"
#define FMT_F32_VARIANCE "%.6f"

static ImVec2 button_dims = {256, 0};

struct resampler_test
{
	struct gui_scene base;
	struct u_resampler *resampler;

	int to_write;
	int last_written;
	int to_read;
	int last_read;
	float frequency;
	float sample_rate;
};

#define BUFFER_SIZE 4000
#define SAMPLE_RATE 3000

static inline void
scene_render(struct gui_scene *scene, struct gui_program *p)
{
	struct resampler_test *test_scene = (struct resampler_test *)scene;

	// Draw the controls first, and decide whether to update.
	igBegin("Resampler", NULL, 0);

	igSeparatorText("Controls");

	float temp[BUFFER_SIZE] = {0};

	igDragInt("Samples To Push", &test_scene->to_write, 1, 1, BUFFER_SIZE, "%d samples", 0);
	igDragInt("Samples To Read", &test_scene->to_read, 1, 1, BUFFER_SIZE, "%d samples", 0);
	igDragFloat("Frequency", &test_scene->frequency, 0.1, 50, 8000, "%f hz", 0);
	igDragFloat("Sample Rate", &test_scene->sample_rate, 1, 50, 8000, "%f hz", 0);

	igText("Last Written %d", test_scene->last_written);
	igText("Last Read %d", test_scene->last_read);

	if (igButton("Push Samples", button_dims)) {
		for (int i = 0; i < test_scene->to_write; i++) {
			float t = i / test_scene->sample_rate;

			temp[i] = sinf((M_PI * 2) * test_scene->frequency * t);
		}

		test_scene->last_written =
		    u_resampler_write(test_scene->resampler, temp, test_scene->to_write, test_scene->sample_rate);
	}
	if (igButton("Read Samples", button_dims)) {
		// discard
		test_scene->last_read = u_resampler_read(test_scene->resampler, temp, test_scene->to_read);
	}

	igSeparatorText("State");

	igText("Read Index: %d", test_scene->resampler->read_index);
	igText("Write Index: %d", test_scene->resampler->write_index);

	ImPlot_BeginPlot("Ring Buffer", (ImVec2){800, 400}, 0);
	ImPlot_PlotLine_FloatPtrInt("Sample", test_scene->resampler->samples, test_scene->resampler->num_samples, 1, 0,
	                            0, 0, sizeof(sample_t));

	ImPlot_Annotation_Str(test_scene->resampler->read_index, 0, (ImVec4){0, 0, 1, 1}, (ImVec2){0}, true,
	                      "Read Index");
	ImPlot_Annotation_Str(test_scene->resampler->write_index, 0, (ImVec4){1, 0, 0, 1}, (ImVec2){0}, true,
	                      "Write Index");

	ImPlot_EndPlot();

	igEnd();
}

static void
scene_destroy(struct gui_scene *scene, struct gui_program *p)
{
	struct resampler_test *test_scene = (struct resampler_test *)scene;

	u_resampler_destroy(test_scene->resampler);

	free(test_scene);
}

void
gui_scene_resampler_test(struct gui_program *p)
{
	struct resampler_test *test_scene = U_TYPED_CALLOC(struct resampler_test);

	test_scene->base.render = scene_render;
	test_scene->base.destroy = scene_destroy;

	test_scene->resampler = u_resampler_create(BUFFER_SIZE, SAMPLE_RATE);
	test_scene->to_write = 1024;
	test_scene->to_read = 1024;
	test_scene->sample_rate = SAMPLE_RATE;
	test_scene->frequency = 300;

	gui_scene_push_front(p, &test_scene->base);
}
