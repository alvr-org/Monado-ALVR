// Copyright 2021, Mateo de Mayo.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Internal header for qwerty_device and its friends.
 * @author Mateo de Mayo <mateodemayo@gmail.com>
 * @ingroup drv_qwerty
 */
#pragma once

#include "util/u_logging.h"
#include "xrt/xrt_device.h"

/*!
 * @addtogroup drv_qwerty
 * @{
 */

#define QWERTY_HMD_STR "Qwerty HMD"
#define QWERTY_HMD_TRACKER_STR QWERTY_HMD_STR " Tracker"
#define QWERTY_LEFT_STR "Qwerty Left Controller"
#define QWERTY_LEFT_TRACKER_STR QWERTY_LEFT_STR " Tracker"
#define QWERTY_RIGHT_STR "Qwerty Right Controller"
#define QWERTY_RIGHT_TRACKER_STR QWERTY_RIGHT_STR " Tracker"

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * @brief Container of qwerty devices and driver properties.
 * @see qwerty_hmd, qwerty_controller
 */
struct qwerty_system
{
	struct qwerty_hmd *hmd;          //!< Can be NULL
	struct qwerty_controller *lctrl; //!< Cannot be NULL
	struct qwerty_controller *rctrl; //!< Cannot be NULL
	enum u_logging_level log_level;
	bool process_keys;  //!< If false disable keyboard and mouse input
	bool hmd_focused;   //!< For gui var tracking only, true if hmd is the focused device
	bool lctrl_focused; //!< Same as `hmd_focused` but for the left controller
	bool rctrl_focused; //!< Same as `hmd_focused` but for the right controller
};

/*!
 * Fake device that modifies its tracked pose through its methods.
 * @implements xrt_device
 */
struct qwerty_device
{
	struct xrt_device base;
	struct xrt_pose pose;      //!< Internal pose state
	struct qwerty_system *sys; //!< Reference to the system this device is in.

	float movement_speed; //!< In meters per frame
	bool left_pressed;
	bool right_pressed;
	bool forward_pressed;
	bool backward_pressed;
	bool up_pressed;
	bool down_pressed;

	float look_speed; //!< In radians per frame
	bool look_left_pressed;
	bool look_right_pressed;
	bool look_up_pressed;
	bool look_down_pressed;

	bool sprint_pressed; //!< Movement speed boost
	float yaw_delta;     //!< How much extra yaw to add for the next pose. Then reset to 0.
	float pitch_delta;   //!< Similar to `yaw_delta`
};

/*!
 * @implements qwerty_device
 * @see qwerty_system
 */
struct qwerty_hmd
{
	struct qwerty_device base;
};

/*!
 * Supports input actions and can be attached to the HMD pose.
 * @implements qwerty_device
 * @see qwerty_system
 */
struct qwerty_controller
{
	struct qwerty_device base;

	bool trigger_clicked;
	int64_t trigger_timestamp;
	bool menu_clicked;
	int64_t menu_timestamp;
	bool squeeze_clicked;
	int64_t squeeze_timestamp;
	bool system_clicked;
	int64_t system_timestamp;

	bool thumbstick_left_pressed;
	bool thumbstick_right_pressed;
	bool thumbstick_up_pressed;
	bool thumbstick_down_pressed;
	int64_t thumbstick_timestamp;
	bool thumbstick_clicked;
	int64_t thumbstick_click_timestamp;

	bool trackpad_left_pressed;
	bool trackpad_right_pressed;
	bool trackpad_up_pressed;
	bool trackpad_down_pressed;
	int64_t trackpad_timestamp;
	bool trackpad_clicked;
	int64_t trackpad_click_timestamp;

	/*!
	 * Only used when a qwerty_hmd exists in the system.
	 * Do not modify directly; use qwerty_follow_hmd().
	 * If true, `pose` is relative to the qwerty_hmd.
	 */
	bool follow_hmd; // @todo: Make this work with non-qwerty HMDs.
};

/*!
 * @public @memberof qwerty_system
 */
struct qwerty_system *
qwerty_system_create(struct qwerty_hmd *qhmd,
                     struct qwerty_controller *qleft,
                     struct qwerty_controller *qright,
                     enum u_logging_level log_level);

/*
 *
 * qwerty_device methods
 *
 */

/*!
 * @brief Cast to qwerty_device. Ensures returning a valid device or crashing.
 * @public @memberof qwerty_device
 */
struct qwerty_device *
qwerty_device(struct xrt_device *xd);

//! @public @memberof qwerty_device
void
qwerty_press_left(struct qwerty_device *qd);
//! @public @memberof qwerty_device
void
qwerty_release_left(struct qwerty_device *qd);
//! @public @memberof qwerty_device
void
qwerty_press_right(struct qwerty_device *qd);
//! @public @memberof qwerty_device
void
qwerty_release_right(struct qwerty_device *qd);
//! @public @memberof qwerty_device
void
qwerty_press_forward(struct qwerty_device *qd);
//! @public @memberof qwerty_device
void
qwerty_release_forward(struct qwerty_device *qd);
//! @public @memberof qwerty_device
void
qwerty_press_backward(struct qwerty_device *qd);
//! @public @memberof qwerty_device
void
qwerty_release_backward(struct qwerty_device *qd);
//! @public @memberof qwerty_device
void
qwerty_press_up(struct qwerty_device *qd);
//! @public @memberof qwerty_device
void
qwerty_release_up(struct qwerty_device *qd);
//! @public @memberof qwerty_device
void
qwerty_press_down(struct qwerty_device *qd);
//! @public @memberof qwerty_device
void
qwerty_release_down(struct qwerty_device *qd);

//! @public @memberof qwerty_device
void
qwerty_press_look_left(struct qwerty_device *qd);
//! @public @memberof qwerty_device
void
qwerty_release_look_left(struct qwerty_device *qd);
//! @public @memberof qwerty_device
void
qwerty_press_look_right(struct qwerty_device *qd);
//! @public @memberof qwerty_device
void
qwerty_release_look_right(struct qwerty_device *qd);
//! @public @memberof qwerty_device
void
qwerty_press_look_up(struct qwerty_device *qd);
//! @public @memberof qwerty_device
void
qwerty_release_look_up(struct qwerty_device *qd);
//! @public @memberof qwerty_device
void
qwerty_press_look_down(struct qwerty_device *qd);
//! @public @memberof qwerty_device
void
qwerty_release_look_down(struct qwerty_device *qd);

/*!
 * Momentarily increase `movement_speed` until `qwerty_release_sprint()`
 * @public @memberof qwerty_device
 */
void
qwerty_press_sprint(struct qwerty_device *qd);

/*!
 * Stop doing what @ref qwerty_press_sprint started.
 * @public @memberof qwerty_device
 */
void
qwerty_release_sprint(struct qwerty_device *qd);

/*!
 * Add yaw and pitch movement for the next frame
 * @public @memberof qwerty_device
 */
void
qwerty_add_look_delta(struct qwerty_device *qd, float yaw, float pitch);

/*!
 * Change movement speed in exponential steps (usually integers, but any float allowed)
 * @public @memberof qwerty_device
 */
void
qwerty_change_movement_speed(struct qwerty_device *qd, float steps);

/*!
 * Release all movement input
 * @public @memberof qwerty_device
 */
void
qwerty_release_all(struct qwerty_device *qd);

/*!
 * Create qwerty_hmd. Crash on failure.
 * @public @memberof qwerty_hmd
 */
struct qwerty_hmd *
qwerty_hmd_create(void);

/*!
 * Cast to qwerty_hmd. Ensures returning a valid HMD or crashing.
 * @public @memberof qwerty_hmd
 */
struct qwerty_hmd *
qwerty_hmd(struct xrt_device *xd);

/*
 *
 * qwerty_controller methods
 *
 */
/*!
 * Create qwerty_controller. Crash on failure.
 * @public @memberof qwerty_controller
 */
struct qwerty_controller *
qwerty_controller_create(bool is_left, struct qwerty_hmd *qhmd);

/*!
 * Cast to qwerty_controller. Ensures returning a valid controller or crashing.
 * @public @memberof qwerty_controller
 */
struct qwerty_controller *
qwerty_controller(struct xrt_device *xd);

/*!
 * Simulate pressing input/trigger/value to 1.0
 * @public @memberof qwerty_controller
 */
void
qwerty_press_trigger(struct qwerty_controller *qc);

/*!
 * Simulate releasing input/trigger/value to 0.0
 * @public @memberof qwerty_controller
 */
void
qwerty_release_trigger(struct qwerty_controller *qc);

/*!
 * Simulate pressing input/menu/click
 * @public @memberof qwerty_controller
 */
void
qwerty_press_menu(struct qwerty_controller *qc);

/*!
 * Simulate releasing input/menu/click
 * @public @memberof qwerty_controller
 */
void
qwerty_release_menu(struct qwerty_controller *qc);

/*!
 * Simulate pressing input/squeeze/click
 * @public @memberof qwerty_controller
 */
void
qwerty_press_squeeze(struct qwerty_controller *qc);

/*!
 * Simulate releasing input/squeeze/click
 * @public @memberof qwerty_controller
 */
void
qwerty_release_squeeze(struct qwerty_controller *qc);

/*!
 * Simulate pressing input/system/click
 * @public @memberof qwerty_controller
 */
void
qwerty_press_system(struct qwerty_controller *qc);

/*!
 * Simulate releasing input/system/click
 * @public @memberof qwerty_controller
 */
void
qwerty_release_system(struct qwerty_controller *qc);

//! @public @memberof qwerty_controller
void
qwerty_press_thumbstick_left(struct qwerty_controller *qc);
//! @public @memberof qwerty_controller
void
qwerty_release_thumbstick_left(struct qwerty_controller *qc);
//! @public @memberof qwerty_controller
void
qwerty_press_thumbstick_right(struct qwerty_controller *qc);
//! @public @memberof qwerty_controller
void
qwerty_release_thumbstick_right(struct qwerty_controller *qc);
//! @public @memberof qwerty_controller
void
qwerty_press_thumbstick_up(struct qwerty_controller *qc);
//! @public @memberof qwerty_controller
void
qwerty_release_thumbstick_up(struct qwerty_controller *qc);
//! @public @memberof qwerty_controller
void
qwerty_press_thumbstick_down(struct qwerty_controller *qc);
//! @public @memberof qwerty_controller
void
qwerty_release_thumbstick_down(struct qwerty_controller *qc);
//! @public @memberof qwerty_controller
void
qwerty_press_thumbstick_click(struct qwerty_controller *qc);
//! @public @memberof qwerty_controller
void
qwerty_release_thumbstick_click(struct qwerty_controller *qc);

//! @public @memberof qwerty_controller
void
qwerty_press_trackpad_left(struct qwerty_controller *qc);
//! @public @memberof qwerty_controller
void
qwerty_release_trackpad_left(struct qwerty_controller *qc);
//! @public @memberof qwerty_controller
void
qwerty_press_trackpad_right(struct qwerty_controller *qc);
//! @public @memberof qwerty_controller
void
qwerty_release_trackpad_right(struct qwerty_controller *qc);
//! @public @memberof qwerty_controller
void
qwerty_press_trackpad_up(struct qwerty_controller *qc);
//! @public @memberof qwerty_controller
void
qwerty_release_trackpad_up(struct qwerty_controller *qc);
//! @public @memberof qwerty_controller
void
qwerty_press_trackpad_down(struct qwerty_controller *qc);
//! @public @memberof qwerty_controller
void
qwerty_release_trackpad_down(struct qwerty_controller *qc);
//! @public @memberof qwerty_controller
void
qwerty_press_trackpad_click(struct qwerty_controller *qc);
//! @public @memberof qwerty_controller
void
qwerty_release_trackpad_click(struct qwerty_controller *qc);

/*!
 * Attach/detach the pose of `qc` to its HMD. Only works when a qwerty_hmd is present.
 * @public @memberof qwerty_controller
 */
void
qwerty_follow_hmd(struct qwerty_controller *qc, bool follow);

/*!
 * Reset controller to initial pose and makes it follow the HMD
 * @public @memberof qwerty_controller
 */
void
qwerty_reset_controller_pose(struct qwerty_controller *qc);


/*!
 * @}
 */

#ifdef __cplusplus
}
#endif
