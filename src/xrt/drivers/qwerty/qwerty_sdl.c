// Copyright 2021, Mateo de Mayo.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Connection between user-generated SDL events and qwerty devices.
 * @author Mateo de Mayo <mateodemayo@gmail.com>
 * @ingroup drv_qwerty
 */

#include "qwerty_device.h"
#include "util/u_device.h"
#include "xrt/xrt_device.h"
#include <SDL2/SDL.h>
#include <SDL_scancode.h>
#include <assert.h>
#include <string.h>

// Amount of look_speed units a mouse delta of 1px in screen space will rotate the device
#define SENSITIVITY 0.1f

static struct qwerty_system *
find_qwerty_system(struct xrt_device **xdevs, size_t xdev_count)
{
	struct xrt_device *xdev = NULL;
	for (size_t i = 0; i < xdev_count; i++) {
		if (xdevs[i] == NULL) {
			continue;
		}
		// We check against tracker name instead of device name because the tracking overrides
		// cause the multi device to have the same names even though they are not qwerty devices.
		const char *tracker_name = xdevs[i]->tracking_origin->name;
		if (strcmp(tracker_name, QWERTY_HMD_TRACKER_STR) == 0 ||
		    strcmp(tracker_name, QWERTY_LEFT_TRACKER_STR) == 0 ||
		    strcmp(tracker_name, QWERTY_RIGHT_TRACKER_STR) == 0) {
			xdev = xdevs[i];
			break;
		}
	}

	assert(xdev != NULL && "There is no device in xdevs with the name of a qwerty device");
	struct qwerty_device *qdev = qwerty_device(xdev);
	struct qwerty_system *qsys = qdev->sys;
	assert(qsys != NULL && "The qwerty_system of a qwerty_device was null");
	return qsys;
}

// Determines the default qwerty device based on which devices are in use
static struct qwerty_device *
default_qwerty_device(struct xrt_device **xdevs, size_t xdev_count, struct qwerty_system *qsys)
{
	int head;
	int eyes;
	int face;
	int left;
	int right;
	int gamepad;
	head = eyes = face = left = right = gamepad = XRT_DEVICE_ROLE_UNASSIGNED;
	u_device_assign_xdev_roles(xdevs, xdev_count, &head, &eyes, &face, &left, &right, &gamepad);

	struct xrt_device *xd_hmd = qsys->hmd ? &qsys->hmd->base.base : NULL;
	struct xrt_device *xd_left = &qsys->lctrl->base.base;
	struct xrt_device *xd_right = &qsys->rctrl->base.base;

	struct qwerty_device *default_qdev = NULL;
	if (xdevs[head] == xd_hmd) {
		default_qdev = qwerty_device(xd_hmd);
	} else if (xdevs[right] == xd_right) {
		default_qdev = qwerty_device(xd_right);
	} else if (xdevs[left] == xd_left) {
		default_qdev = qwerty_device(xd_left);
	} else { // Even here, xd_right is allocated and so we can modify it
		default_qdev = qwerty_device(xd_right);
	}

	return default_qdev;
}

// Determines the default qwerty controller based on which devices are in use
static struct qwerty_controller *
default_qwerty_controller(struct xrt_device **xdevs, size_t xdev_count, struct qwerty_system *qsys)
{
	int head;
	int eyes;
	int face;
	int left;
	int right;
	int gamepad;
	head = eyes = face = left = right = gamepad = XRT_DEVICE_ROLE_UNASSIGNED;
	u_device_assign_xdev_roles(xdevs, xdev_count, &head, &eyes, &face, &left, &right, &gamepad);

	struct xrt_device *xd_left = &qsys->lctrl->base.base;
	struct xrt_device *xd_right = &qsys->rctrl->base.base;

	struct qwerty_controller *default_qctrl = NULL;
	if (xdevs[right] == xd_right) {
		default_qctrl = qwerty_controller(xd_right);
	} else if (xdevs[left] == xd_left) {
		default_qctrl = qwerty_controller(xd_left);
	} else { // Even here, xd_right is allocated and so we can modify it
		default_qctrl = qwerty_controller(xd_right);
	}

	return default_qctrl;
}

void
qwerty_process_event(struct xrt_device **xdevs, size_t xdev_count, SDL_Event event)
{
	static struct qwerty_system *qsys = NULL;

	static bool alt_pressed = false;
	static bool ctrl_pressed = false;

	// Default focused device: the one focused when CTRL and ALT are not pressed
	static struct qwerty_device *default_qdev;
	// Default focused controller: the one used for qwerty_controller specific methods
	static struct qwerty_controller *default_qctrl;

	// We can cache the devices as they don't get destroyed during runtime
	static bool cached = false;
	if (!cached) {
		qsys = find_qwerty_system(xdevs, xdev_count);
		default_qdev = default_qwerty_device(xdevs, xdev_count, qsys);
		default_qctrl = default_qwerty_controller(xdevs, xdev_count, qsys);
		cached = true;
	}

	if (!qsys->process_keys) {
		return;
	}

	// Initialize different views of the same pointers.

	struct qwerty_controller *qleft = qsys->lctrl;
	struct qwerty_device *qd_left = &qleft->base;

	struct qwerty_controller *qright = qsys->rctrl;
	struct qwerty_device *qd_right = &qright->base;

	bool using_qhmd = qsys->hmd != NULL;
	struct qwerty_hmd *qhmd = using_qhmd ? qsys->hmd : NULL;
	struct qwerty_device *qd_hmd = using_qhmd ? &qhmd->base : NULL;

	// clang-format off
	// CTRL/ALT keys logic
	bool alt_down = event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_LALT;
	bool alt_up = event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_LALT;
	bool ctrl_down = event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_LCTRL;
	bool ctrl_up = event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_LCTRL;
	if (alt_down) alt_pressed = true;
	if (alt_up) alt_pressed = false;
	if (ctrl_down) ctrl_pressed = true;
	if (ctrl_up) ctrl_pressed = false;

	bool change_focus = alt_down || alt_up || ctrl_down || ctrl_up;
	if (change_focus) {
		if (using_qhmd) qwerty_release_all(qd_hmd);
		qwerty_release_all(qd_right);
		qwerty_release_all(qd_left);
	}

	// Determine focused device
	struct qwerty_device *qdev;
	if (ctrl_pressed) qdev = qd_left;
	else if (alt_pressed) qdev = qd_right;
	else qdev = default_qdev;

	// Determine focused controller for qwerty_controller specific methods
	struct qwerty_controller *qctrl = qdev != qd_hmd ? qwerty_controller(&qdev->base) : default_qctrl;

	// Update gui tracked variables
	qsys->hmd_focused = qdev == qd_hmd;
	qsys->lctrl_focused = qdev == qd_left;
	qsys->rctrl_focused = qdev == qd_right;

	// WASDQE Movement
	if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_A) qwerty_press_left(qdev);
	if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_A) qwerty_release_left(qdev);
	if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_D) qwerty_press_right(qdev);
	if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_D) qwerty_release_right(qdev);
	if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_W) qwerty_press_forward(qdev);
	if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_W) qwerty_release_forward(qdev);
	if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_S) qwerty_press_backward(qdev);
	if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_S) qwerty_release_backward(qdev);
	if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_E) qwerty_press_up(qdev);
	if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_E) qwerty_release_up(qdev);
	if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_Q) qwerty_press_down(qdev);
	if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_Q) qwerty_release_down(qdev);

	// Arrow keys rotation
	if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_LEFT) qwerty_press_look_left(qdev);
	if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_LEFT) qwerty_release_look_left(qdev);
	if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_RIGHT) qwerty_press_look_right(qdev);
	if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_RIGHT) qwerty_release_look_right(qdev);
	if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_UP) qwerty_press_look_up(qdev);
	if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_UP) qwerty_release_look_up(qdev);
	if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_DOWN) qwerty_press_look_down(qdev);
	if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_DOWN) qwerty_release_look_down(qdev);

	// Movement speed
	if (event.type == SDL_MOUSEWHEEL) qwerty_change_movement_speed(qdev, event.wheel.y);
	if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_KP_PLUS) qwerty_change_movement_speed(qdev, 1);
	if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_KP_MINUS) qwerty_change_movement_speed(qdev, -1);

	// Sprinting
	if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_LSHIFT) qwerty_press_sprint(qdev);
	if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_LSHIFT) qwerty_release_sprint(qdev);

	// Mouse rotation
	if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_RIGHT) {
		SDL_SetRelativeMouseMode(false);
	}
	if (event.type == SDL_MOUSEMOTION && event.motion.state & SDL_BUTTON_RMASK) {
		SDL_SetRelativeMouseMode(true);
		float yaw = -event.motion.xrel * SENSITIVITY;
		float pitch = -event.motion.yrel * SENSITIVITY;
		qwerty_add_look_delta(qdev, yaw, pitch);
	}

	// Trigger, squeeze and menu clicks only for controllers.
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) qwerty_press_trigger(qctrl);
	if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) qwerty_release_trigger(qctrl);
	if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_MIDDLE) qwerty_press_squeeze(qctrl);
	if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_MIDDLE) qwerty_release_squeeze(qctrl);

	if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_N) qwerty_press_menu(qctrl);
	if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_N) qwerty_release_menu(qctrl);
	if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_B) qwerty_press_system(qctrl);
	if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_B) qwerty_release_system(qctrl);

	// Thumbstick
	if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_F) qwerty_press_thumbstick_left(qctrl);
	if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_F) qwerty_release_thumbstick_left(qctrl);
	if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_H) qwerty_press_thumbstick_right(qctrl);
	if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_H) qwerty_release_thumbstick_right(qctrl);
	if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_T) qwerty_press_thumbstick_up(qctrl);
	if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_T) qwerty_release_thumbstick_up(qctrl);
	if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_G) qwerty_press_thumbstick_down(qctrl);
	if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_G) qwerty_release_thumbstick_down(qctrl);
	if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_V) qwerty_press_thumbstick_click(qctrl);
	if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_V) qwerty_release_thumbstick_click(qctrl);

	// Trackpad
	if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_J) qwerty_press_trackpad_left(qctrl);
	if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_J) qwerty_release_trackpad_left(qctrl);
	if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_L) qwerty_press_trackpad_right(qctrl);
	if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_L) qwerty_release_trackpad_right(qctrl);
	if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_I) qwerty_press_trackpad_up(qctrl);
	if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_I) qwerty_release_trackpad_up(qctrl);
	if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_K) qwerty_press_trackpad_down(qctrl);
	if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_K) qwerty_release_trackpad_down(qctrl);
	if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_M) qwerty_press_trackpad_click(qctrl);
	if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_M) qwerty_release_trackpad_click(qctrl);

	// clang-format on

	// Controllers follow/unfollow HMD
	if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_C && event.key.repeat == 0) {
		if (qdev != qd_hmd) {
			qwerty_follow_hmd(qctrl, !qctrl->follow_hmd);
		} else { // If no controller is focused, set both to the same state
			bool both_not_following = !qleft->follow_hmd && !qright->follow_hmd;
			qwerty_follow_hmd(qleft, both_not_following);
			qwerty_follow_hmd(qright, both_not_following);
		}
	}

	// Reset controller poses
	if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_R && event.key.repeat == 0) {
		if (qdev != qd_hmd) {
			qwerty_reset_controller_pose(qctrl);
		} else { // If no controller is focused, reset both
			qwerty_reset_controller_pose(qleft);
			qwerty_reset_controller_pose(qright);
		}
	}
}
