// Copyright 2019-2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Common file for the Monado GUI program.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup gui
 */

#pragma once

#include "xrt/xrt_compiler.h"


/*!
 * @defgroup gui GUI Config Interface
 * @ingroup xrt
 *
 * @brief Small GUI interface to configure Monado based on SDL2.
 */

#ifdef __cplusplus
extern "C" {
#endif


struct xrt_device;
struct xrt_prober;
struct xrt_fs;
struct xrt_frame_sink;
struct xrt_frame_context;
struct xrt_settings_tracking;
struct time_state;
struct gui_scene_manager;
struct xrt_system_devices;


/*!
 * A gui program.
 *
 * @ingroup gui
 */
struct gui_program
{
	bool stopped;

	struct gui_scene_manager *gsm;

	struct xrt_system *xsys;
	struct xrt_system_devices *xsysd;
	struct xrt_space_overseer *xso;
	struct xrt_instance *instance;
	struct xrt_prober *xp;

	struct gui_ogl_texture *texs[256];
	size_t num_texs;
};

/*!
 * @interface gui_scene
 * A single currently running scene.
 *
 * @see gui_program
 * @ingroup gui
 */
struct gui_scene
{
	void (*render)(struct gui_scene *, struct gui_program *);
	void (*destroy)(struct gui_scene *, struct gui_program *);
};

/*!
 * A OpenGL texture.
 *
 * @see gui_program
 * @ingroup gui
 */
struct gui_ogl_texture
{
	uint64_t seq;
	uint64_t dropped;
	const char *name;
	uint32_t w, h;
	uint32_t id;
	bool half;
};

/*!
 * @addtogroup gui
 * @{
 */

/*!
 * Initialize the prober and open all devices found.
 *
 * @public @memberof gui_program
 */
int
gui_prober_init(struct gui_program *p);

/*!
 * Create devices.
 *
 * @public @memberof gui_program
 */
int
gui_prober_select(struct gui_program *p);

/*!
 * Update all devices.
 *
 * @public @memberof gui_program
 */
void
gui_prober_update(struct gui_program *p);

/*!
 * Destroy all opened devices and destroy the prober.
 *
 * @public @memberof gui_program
 */
void
gui_prober_teardown(struct gui_program *p);

/*!
 * Create a sink that will turn frames into OpenGL textures, since the frame
 * can come from another thread @ref gui_ogl_sink_update needs to be called.
 *
 * Destruction is handled by the frame context.
 *
 * @public @memberof gui_ogl_texture
 * @ingroup gui
 */
struct gui_ogl_texture *
gui_ogl_sink_create(const char *name, struct xrt_frame_context *xfctx, struct xrt_frame_sink **out_sink);

/*!
 * Update the texture to the latest received frame.
 *
 * @public @memberof gui_ogl_texture
 * @ingroup gui
 */
void
gui_ogl_sink_update(struct gui_ogl_texture * /*tex*/);

/*!
 * Push the scene to the top of the lists.
 *
 * @public @memberof gui_program
 */
void
gui_scene_push_front(struct gui_program *p, struct gui_scene *me);

/*!
 * Put a scene on the delete list, also removes it from any other list.
 *
 * @public @memberof gui_program
 */
void
gui_scene_delete_me(struct gui_program *p, struct gui_scene *me);

/*!
 * Render the scenes.
 *
 * @public @memberof gui_program
 */
void
gui_scene_manager_render(struct gui_program *p);

/*!
 * Initialize the scene manager.
 *
 * @public @memberof gui_program
 */
void
gui_scene_manager_init(struct gui_program *p);

/*!
 * Destroy the scene manager.
 *
 * @public @memberof gui_program
 */
void
gui_scene_manager_destroy(struct gui_program *p);


/*
 *
 * Scene creation functions.
 *
 */

/*!
 * Shows the main menu.
 *
 * @public @memberof gui_program
 */
void
gui_scene_main_menu(struct gui_program *p);

/*!
 * Shows a UI that lets you select a video device and mode for calibration.
 *
 * @public @memberof gui_program
 */
void
gui_scene_select_video_calibrate(struct gui_program *p);

/*!
 * Shows a UI that lets you set up tracking overrides.
 *
 * @public @memberof gui_program
 */
void
gui_scene_tracking_overrides(struct gui_program *p);

/*!
 * Regular debug UI.
 *
 * @public @memberof gui_program
 */
void
gui_scene_debug(struct gui_program *p);

/*!
 * Small hand-tracking demo.
 *
 * @public @memberof gui_program
 */
void
gui_scene_hand_tracking_demo(struct gui_program *p);

/*!
 * EuRoC recorder for DepthAI cameras
 *
 * @public @memberof gui_program
 */
void
gui_scene_record_euroc(struct gui_program *p);

/*!
 * Create a recording view scene.
 *
 * @public @memberof gui_program
 */
void
gui_scene_record(struct gui_program *p, const char *camera);

/*!
 * Remote control debugging UI.
 *
 * @param p self
 * @param address Optional address.
 *
 * @public @memberof gui_program
 */
void
gui_scene_remote(struct gui_program *p, const char *address);

/*!
 * Given the frameserver runs the calibration code on it.
 * Claims ownership of @p s.
 *
 * @public @memberof gui_program
 */
void
gui_scene_calibrate(struct gui_program *p,
                    struct xrt_frame_context *xfctx,
                    struct xrt_fs *xfs,
                    struct xrt_settings_tracking *s);

/*!
 * @}
 */

#ifdef __cplusplus
}
#endif
