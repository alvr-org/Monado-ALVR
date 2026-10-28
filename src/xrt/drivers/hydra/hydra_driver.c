// Copyright 2019, Collabora, Ltd.
// Copyright 2011, Iowa State University
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Razer Hydra prober and driver code
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 *
 * Portions based on the VRPN Razer Hydra driver,
 * originally written by Rylie Pavlik and available under the BSL-1.0.
 */


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xrt/xrt_prober.h"

#include "os/os_hid.h"
#include "os/os_time.h"
#include "os/os_threading.h"
#include "xrt/xrt_byte_order.h"

#include "math/m_api.h"
#include "math/m_filter_one_euro.h"
#include "math/m_relation_history.h"
#include "math/m_space.h"

#include "util/u_debug.h"
#include "util/u_device.h"
#include "util/u_misc.h"
#include "util/u_time.h"
#include "util/u_logging.h"
#include "util/u_linux.h"
#include "util/u_trace_marker.h"
#include "util/u_var.h"

#include "hydra_interface.h"



/*
 *
 * Defines & structs.
 *
 */

#define HD_TRACE(d, ...) U_LOG_XDEV_IFL_T(&d->base, d->sys->log_level, __VA_ARGS__)
#define HD_DEBUG(d, ...) U_LOG_XDEV_IFL_D(&d->base, d->sys->log_level, __VA_ARGS__)
#define HD_INFO(d, ...) U_LOG_XDEV_IFL_I(&d->base, d->sys->log_level, __VA_ARGS__)
#define HD_WARN(d, ...) U_LOG_XDEV_IFL_W(&d->base, d->sys->log_level, __VA_ARGS__)
#define HD_ERROR(d, ...) U_LOG_XDEV_IFL_E(&d->base, d->sys->log_level, __VA_ARGS__)

#define HS_TRACE(d, ...) U_LOG_IFL_T(d->log_level, __VA_ARGS__)
#define HS_DEBUG(d, ...) U_LOG_IFL_D(d->log_level, __VA_ARGS__)
#define HS_INFO(d, ...) U_LOG_IFL_I(d->log_level, __VA_ARGS__)
#define HS_WARN(d, ...) U_LOG_IFL_W(d->log_level, __VA_ARGS__)
#define HS_ERROR(d, ...) U_LOG_IFL_E(d->log_level, __VA_ARGS__)

DEBUG_GET_ONCE_LOG_OPTION(hydra_log, "HYDRA_LOG", U_LOGGING_WARN)

enum hydra_input_index
{
	HYDRA_INDEX_1_CLICK,
	HYDRA_INDEX_2_CLICK,
	HYDRA_INDEX_3_CLICK,
	HYDRA_INDEX_4_CLICK,
	HYDRA_INDEX_MIDDLE_CLICK,
	HYDRA_INDEX_BUMPER_CLICK,
	HYDRA_INDEX_JOYSTICK_CLICK,
	HYDRA_INDEX_JOYSTICK_VALUE,
	HYDRA_INDEX_TRIGGER_VALUE,
	HYDRA_INDEX_GRIP_POSE,
	HYDRA_INDEX_AIM_POSE,
	HYDRA_MAX_CONTROLLER_INDEX
};

/* Yes this is a bizarre bit mask. Mysteries of the Hydra. */
enum hydra_button_bit
{
	HYDRA_BUTTON_BIT_BUMPER = (1 << 0),

	HYDRA_BUTTON_BIT_3 = (1 << 1),
	HYDRA_BUTTON_BIT_1 = (1 << 2),
	HYDRA_BUTTON_BIT_2 = (1 << 3),
	HYDRA_BUTTON_BIT_4 = (1 << 4),

	HYDRA_BUTTON_BIT_MIDDLE = (1 << 5),
	HYDRA_BUTTON_BIT_JOYSTICK = (1 << 6),
};

static const uint8_t HYDRA_REPORT_START_MOTION[] = {

    0x00, // first byte must be report type
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00};

static const uint8_t HYDRA_REPORT_START_GAMEPAD[] = {
    0x00, // first byte must be report type
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00};

struct hydra_controller_state
{
	struct m_relation_history *relation_history;
	struct
	{
		struct m_filter_euro_vec3 position;
		struct m_filter_euro_quat orientation;
	} motion_vector_filters;

	struct xrt_vec2 js;
	float trigger;
	uint8_t buttons;
};
/*!
 * The states of the finite-state machine controlling the Hydra.
 */
enum hydra_sm_state
{
	HYDRA_SM_LISTENING_AFTER_CONNECT = 0,
	HYDRA_SM_LISTENING_AFTER_SET_FEATURE,
	HYDRA_SM_REPORTING
};

/*!
 * The details of the Hydra state machine in a convenient package.
 */
struct hydra_state_machine
{
	enum hydra_sm_state current_state;

	//! Time of the last (non-trivial) state transition
	timepoint_ns transition_time;
};

struct hydra_device;

/*!
 * A Razer Hydra system containing two controllers.
 *
 * @ingroup drv_hydra
 * @extends xrt_tracking_origin
 */
struct hydra_system
{
	struct xrt_tracking_origin base;
	struct os_hid_device *data_hid;
	struct os_hid_device *command_hid;

	struct os_thread_helper usb_thread;

	struct os_mutex data_mutex;

	struct hydra_state_machine sm;
	struct hydra_device *devs[2];

	int16_t report_counter;

	//! Last time that we received a report
	timepoint_ns report_time;

	/*!
	 * Reference count of the number of devices still alive using this
	 * system
	 */
	uint8_t refs;

	/*!
	 * Was the hydra in gamepad mode at start?
	 *
	 * If it was, we set it back to gamepad on destruction.
	 */
	bool was_in_gamepad_mode;
	int motion_attempt_number;

	enum u_logging_level log_level;
};

/*!
 * A Razer Hydra device, representing just a single controller.
 *
 * @ingroup drv_hydra
 * @implements xrt_device
 */
struct hydra_device
{
	struct xrt_device base;
	struct hydra_system *sys;

	//! Last time that we updated inputs
	timepoint_ns input_time;

	// bool calibration_done;
	// int mirror;
	// int sign_x;

	struct hydra_controller_state state;

	//! Which hydra controller in the system are we?
	size_t index;
};

/*
 *
 * Internal functions.
 *
 */

static void
hydra_device_parse_controller(struct hydra_device *hd, uint8_t *buf, int64_t now);

static inline struct hydra_device *
hydra_device(struct xrt_device *xdev)
{
	assert(xdev);
	struct hydra_device *ret = (struct hydra_device *)xdev;
	assert(ret->sys != NULL);
	return ret;
}

static inline struct hydra_system *
hydra_system(struct xrt_tracking_origin *xtrack)
{
	assert(xtrack);
	struct hydra_system *ret = (struct hydra_system *)xtrack;
	return ret;
}

/*!
 * Reports the number of seconds since the most recent change of state.
 *
 * @relates hydra_sm
 */
static float
hydra_sm_seconds_since_transition(struct hydra_state_machine *hsm, timepoint_ns now)
{

	if (hsm->transition_time == 0) {
		hsm->transition_time = now;
		return 0.f;
	}

	float state_duration_s = time_ns_to_s(now - hsm->transition_time);
	return state_duration_s;
}
/*!
 * Performs a state transition, updating the transition time if the state
 * actually changed.
 *
 * @relates hydra_sm
 */
static void
hydra_sm_transition(struct hydra_state_machine *hsm, enum hydra_sm_state new_state, timepoint_ns now)
{
	if (hsm->transition_time == 0) {
		hsm->transition_time = now;
	}
	if (new_state != hsm->current_state) {
		hsm->current_state = new_state;
		hsm->transition_time = now;
	}
}
static inline uint8_t
hydra_read_uint8(uint8_t **bufptr)
{
	uint8_t ret = **bufptr;
	(*bufptr)++;
	return ret;
}
static inline int16_t
hydra_read_int16_le(uint8_t **bufptr)
{
	uint8_t *buf = *bufptr;
#ifdef XRT_BIG_ENDIAN
	uint8_t bytes[2] = {buf[1], buf[0]};
#else
	uint8_t bytes[2] = {buf[0], buf[1]};
#endif // XRT_BIG_ENDIAN
	(*bufptr) += 2;
	int16_t ret;
	memcpy(&ret, bytes, sizeof(ret));
	return ret;
}

/*!
 * Parse the controller-specific part of a buffer into a hydra device.
 */
static void
hydra_device_parse_controller(struct hydra_device *hd, uint8_t *buf, int64_t now)
{
	struct hydra_controller_state *state = &hd->state;

	static const float SCALE_MM_TO_METER = 0.001f;
	static const float SCALE_INT16_TO_FLOAT_PLUSMINUS_1 = 1.0f / 32768.0f;
	static const float SCALE_UINT8_TO_FLOAT_0_TO_1 = 1.0f / 255.0f;

	struct xrt_pose pose;

	pose.position.x = hydra_read_int16_le(&buf) * SCALE_MM_TO_METER;
	pose.position.z = hydra_read_int16_le(&buf) * SCALE_MM_TO_METER;
	pose.position.y = -hydra_read_int16_le(&buf) * SCALE_MM_TO_METER;

	// the negatives are to fix handedness
	pose.orientation.w = hydra_read_int16_le(&buf) * SCALE_INT16_TO_FLOAT_PLUSMINUS_1;
	pose.orientation.x = hydra_read_int16_le(&buf) * SCALE_INT16_TO_FLOAT_PLUSMINUS_1;
	pose.orientation.y = hydra_read_int16_le(&buf) * SCALE_INT16_TO_FLOAT_PLUSMINUS_1;
	pose.orientation.z = hydra_read_int16_le(&buf) * SCALE_INT16_TO_FLOAT_PLUSMINUS_1;

	//! @todo the presence of this suggest we're not decoding the
	//! orientation right.
	math_quat_normalize(&pose.orientation);

	struct xrt_quat fixed = {
	    .x = pose.orientation.x,
	    .y = -pose.orientation.z,
	    .z = pose.orientation.y,
	    .w = pose.orientation.w,
	};

	struct xrt_quat adjustment = {.x = 0, .y = 1, .z = 0, .w = 0};
	math_quat_rotate(&fixed, &adjustment, &fixed);

	adjustment = (struct xrt_quat){.x = 0, .y = 0, .z = 1, .w = 0};
	math_quat_rotate(&fixed, &adjustment, &fixed);

	pose.orientation = fixed;

	struct xrt_space_relation space_relation = {0};
	m_filter_euro_vec3_run(&state->motion_vector_filters.position, now, &pose.position,
	                       &space_relation.pose.position);
	m_filter_euro_quat_run(&state->motion_vector_filters.orientation, now, &pose.orientation,
	                       &space_relation.pose.orientation);

	space_relation.relation_flags =
	    (XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT | XRT_SPACE_RELATION_ORIENTATION_VALID_BIT) |
	    (XRT_SPACE_RELATION_POSITION_TRACKED_BIT | XRT_SPACE_RELATION_POSITION_VALID_BIT);

	m_relation_history_push_with_motion_estimation(state->relation_history, &space_relation, now);

	state->buttons = hydra_read_uint8(&buf);

	state->js.x = hydra_read_int16_le(&buf) * SCALE_INT16_TO_FLOAT_PLUSMINUS_1;
	state->js.y = hydra_read_int16_le(&buf) * SCALE_INT16_TO_FLOAT_PLUSMINUS_1;

	state->trigger = hydra_read_uint8(&buf) * SCALE_UINT8_TO_FLOAT_0_TO_1;

	HD_TRACE(hd,
	         "\n\t"
	         "controller:  %i\n\t"
	         "position:    (%-1.2f, %-1.2f, %-1.2f)\n\t"
	         "orientation: (%-1.2f, %-1.2f, %-1.2f, %-1.2f)\n\t"
	         "buttons:     %08x\n\t"
	         "joystick:    (%-1.2f, %-1.2f)\n\t"
	         "trigger:     %01.2f\n",
	         (int)hd->index, pose.position.x, pose.position.y, pose.position.z, pose.orientation.x,
	         pose.orientation.y, pose.orientation.z, pose.orientation.w, state->buttons, state->js.x, state->js.y,
	         state->trigger);
}

static int
hydra_system_read_data_hid(struct hydra_system *hs)
{
	assert(hs);
	uint8_t buffer[128];

	int ret = os_hid_read(hs->data_hid, buffer, sizeof(buffer),
	                      20); // 20ms is a generous number above the 16.66ms we expect to receive reports at (60hz)

	timepoint_ns now = os_monotonic_get_ns();

	// we dont care if we get no data
	if (ret <= 0) {
		return ret;
	}

	if (ret != 52) {
		HS_ERROR(hs, "Unexpected data report of size %d", ret);
		return -1;
	}

	os_mutex_lock(&hs->data_mutex);

	uint8_t new_counter = buffer[7];
	bool missed = false;
	if (hs->report_counter != -1) {
		uint8_t expected_counter = ((hs->report_counter + 1) & 0xff);
		missed = new_counter != expected_counter;
	}
	hs->report_counter = new_counter;


	if (hs->devs[0] != NULL) {
		hydra_device_parse_controller(hs->devs[0], buffer + 8, now);
	}
	if (hs->devs[1] != NULL) {
		hydra_device_parse_controller(hs->devs[1], buffer + 30, now);
	}

	hs->report_time = now;

	os_mutex_unlock(&hs->data_mutex);

	HS_TRACE(hs,
	         "\n\t"
	         "missed: %s\n\t"
	         "seq_no: %x\n",
	         missed ? "yes" : "no", new_counter);

	return ret;
}


/*!
 * Switch to motion controller mode.
 */
static void
hydra_system_enter_motion_control(struct hydra_system *hs, timepoint_ns now)
{
	assert(hs);

	hs->was_in_gamepad_mode = true;
	hs->motion_attempt_number++;
	HS_DEBUG(hs,
	         "Setting feature report to start motion-controller mode, "
	         "attempt %d",
	         hs->motion_attempt_number);

	os_hid_set_feature(hs->command_hid, HYDRA_REPORT_START_MOTION, sizeof(HYDRA_REPORT_START_MOTION));

	// Doing a throwaway get-feature now.
	uint8_t buf[91] = {0};
	os_hid_get_feature(hs->command_hid, 0, buf, sizeof(buf));

	hydra_sm_transition(&hs->sm, HYDRA_SM_LISTENING_AFTER_SET_FEATURE, now);
}
/*!
 * Update the internal state of the Hydra driver.
 *
 * Reads devices, checks the state machine and timeouts, etc.
 *
 */
static int
hydra_system_update(struct hydra_system *hs)
{
	assert(hs);

	// In all states of the state machine:
	// Try reading a report: will only return >0 if we get a full motion
	// report.
	int received = hydra_system_read_data_hid(hs);

	// we got an error
	if (received < 0) {
		return received;
	}

	os_mutex_lock(&hs->data_mutex);

	timepoint_ns now = os_monotonic_get_ns();

	// if we got data, transition to "reporting" mode
	if (received > 0) {
		hydra_sm_transition(&hs->sm, HYDRA_SM_REPORTING, now);
	}

	switch (hs->sm.current_state) {
	case HYDRA_SM_LISTENING_AFTER_CONNECT: {
		float state_duration_s = hydra_sm_seconds_since_transition(&hs->sm, now);
		if (state_duration_s > 1.0f) {
			// only waiting 1 second for the initial report after
			// connect
			hydra_system_enter_motion_control(hs, now);
		}
	} break;
	case HYDRA_SM_LISTENING_AFTER_SET_FEATURE: {
		float state_duration_s = hydra_sm_seconds_since_transition(&hs->sm, now);
		if (state_duration_s > 5.0f) {
			// giving each motion control attempt 5 seconds to work.
			hydra_system_enter_motion_control(hs, now);
		}
	} break;
	default: break;
	}

	os_mutex_unlock(&hs->data_mutex);

	return 0;
}

static void
hydra_device_update_input_click(struct hydra_device *hd, timepoint_ns now, int index, uint32_t bit)
{
	assert(hd);
	hd->base.inputs[index].timestamp = now;
	hd->base.inputs[index].value.boolean = (hd->state.buttons & bit) != 0;
}

static void *
hydra_usb_thread_run(void *user_data)
{
	struct hydra_system *hs = (struct hydra_system *)user_data;

	const char *thread_name = "Hydra USB";

	U_TRACE_SET_THREAD_NAME(thread_name);
	os_thread_name_self(thread_name);

#ifdef XRT_OS_LINUX
	// Try to raise priority of this thread.
	u_linux_try_to_set_realtime_priority_on_thread(hs->log_level, thread_name);
#endif

	os_thread_helper_lock(&hs->usb_thread);

	int result = 0;
#if 0
	int ticks = 0;
#endif

	while (os_thread_helper_is_running_locked(&hs->usb_thread) && result >= 0) {
		os_thread_helper_unlock(&hs->usb_thread);

		result = hydra_system_update(hs);

		os_thread_helper_lock(&hs->usb_thread);
#if 0
		ticks += 1;
#endif
	}

	os_thread_helper_unlock(&hs->usb_thread);

	return NULL;
}

/*
 *
 * Device functions.
 *
 */

static xrt_result_t
hydra_device_update_inputs(struct xrt_device *xdev)
{
	struct hydra_device *hd = hydra_device(xdev);
	struct hydra_system *hs = hydra_system(xdev->tracking_origin);

	os_mutex_lock(&hs->data_mutex);

	if (hd->input_time != hs->report_time) {
		timepoint_ns now = hs->report_time;
		hd->input_time = now;

		hydra_device_update_input_click(hd, now, HYDRA_INDEX_1_CLICK, HYDRA_BUTTON_BIT_1);
		hydra_device_update_input_click(hd, now, HYDRA_INDEX_2_CLICK, HYDRA_BUTTON_BIT_2);
		hydra_device_update_input_click(hd, now, HYDRA_INDEX_3_CLICK, HYDRA_BUTTON_BIT_3);
		hydra_device_update_input_click(hd, now, HYDRA_INDEX_4_CLICK, HYDRA_BUTTON_BIT_4);

		hydra_device_update_input_click(hd, now, HYDRA_INDEX_MIDDLE_CLICK, HYDRA_BUTTON_BIT_MIDDLE);
		hydra_device_update_input_click(hd, now, HYDRA_INDEX_BUMPER_CLICK, HYDRA_BUTTON_BIT_BUMPER);
		hydra_device_update_input_click(hd, now, HYDRA_INDEX_JOYSTICK_CLICK, HYDRA_INDEX_JOYSTICK_CLICK);

		struct xrt_input *inputs = hd->base.inputs;
		struct hydra_controller_state *state = &(hd->state);

		inputs[HYDRA_INDEX_JOYSTICK_VALUE].timestamp = now;
		inputs[HYDRA_INDEX_JOYSTICK_VALUE].value.vec2 = state->js;

		inputs[HYDRA_INDEX_TRIGGER_VALUE].timestamp = now;
		inputs[HYDRA_INDEX_TRIGGER_VALUE].value.vec1.x = state->trigger;
	}

	os_mutex_unlock(&hs->data_mutex);

	return XRT_SUCCESS;
}

static xrt_result_t
hydra_device_get_tracked_pose(struct xrt_device *xdev,
                              enum xrt_input_name name,
                              int64_t at_timestamp_ns,
                              struct xrt_space_relation *out_relation)
{
	struct hydra_device *hd = hydra_device(xdev);

	struct xrt_relation_chain xrc = {0};

	switch (name) {
	case XRT_INPUT_HYDRA_AIM_POSE: {
		const struct xrt_pose aim_offset = {
		    .position = {0, 0.045, -0.08},
		    .orientation = {-0.258819, 0, 0, 0.9659258},
		};
		m_relation_chain_push_pose(&xrc, &aim_offset);
		break;
	}
	case XRT_INPUT_HYDRA_GRIP_POSE:
	default: break;
	}

	struct xrt_space_relation device_relation = {0};
	m_relation_history_get(hd->state.relation_history, at_timestamp_ns, &device_relation);

	m_relation_chain_push_relation(&xrc, &device_relation);

	m_relation_chain_resolve(&xrc, out_relation);

	return XRT_SUCCESS;
}

static void
hydra_system_remove_child(struct hydra_system *hs, struct hydra_device *hd)
{
	assert(hydra_system(hd->base.tracking_origin) == hs);
	assert(hd->index == 0 || hd->index == 1);

	// Make the device not point to the system
	hd->sys = NULL;

	// Make the system not point to the device
	assert(hs->devs[hd->index] == hd);
	hs->devs[hd->index] = NULL;

	// Decrease ref count of system
	hs->refs--;

	if (hs->refs == 0) {
		os_thread_helper_destroy(&hs->usb_thread);

		os_mutex_destroy(&hs->data_mutex);

		// No more children, destroy system.
		if (hs->data_hid != NULL && hs->command_hid != NULL && hs->sm.current_state == HYDRA_SM_REPORTING &&
		    hs->was_in_gamepad_mode) {

			HS_DEBUG(hs,
			         "Sending command to re-enter gamepad mode "
			         "and pausing while it takes effect.");

			os_hid_set_feature(hs->command_hid, HYDRA_REPORT_START_GAMEPAD,
			                   sizeof(HYDRA_REPORT_START_GAMEPAD));
			os_nanosleep(2 * 1000 * 1000 * 1000);
		}
		if (hs->data_hid != NULL) {
			os_hid_destroy(hs->data_hid);
			hs->data_hid = NULL;
		}
		if (hs->command_hid != NULL) {
			os_hid_destroy(hs->command_hid);
			hs->command_hid = NULL;
		}
		free(hs);
	}
}

static void
hydra_device_destroy(struct xrt_device *xdev)
{
	struct hydra_device *hd = hydra_device(xdev);
	struct hydra_system *hs = hydra_system(xdev->tracking_origin);

	m_relation_history_destroy(&hd->state.relation_history);

	hydra_system_remove_child(hs, hd);

	free(hd);
}

/*
 *
 * Prober functions.
 *
 */
#define SET_INPUT(NAME)                                                                                                \
	do {                                                                                                           \
		(hd->base.inputs[HYDRA_INDEX_##NAME].name = XRT_INPUT_HYDRA_##NAME);                                   \
	} while (0)

int
hydra_found(struct xrt_prober *xp,
            struct xrt_prober_device **devices,
            size_t device_count,
            size_t index,
            cJSON *attached_data,
            struct xrt_device **out_xdevs)
{
	struct xrt_prober_device *dev = devices[index];
	int ret;

	struct os_hid_device *data_hid = NULL;
	ret = xrt_prober_open_hid_interface(xp, dev, 0, &data_hid);
	if (ret != 0) {
		return -1;
	}
	struct os_hid_device *command_hid = NULL;
	ret = xrt_prober_open_hid_interface(xp, dev, 1, &command_hid);
	if (ret != 0) {
		os_hid_destroy(data_hid);
		return -1;
	}

	// Create the system
	struct hydra_system *hs = U_TYPED_CALLOC(struct hydra_system);
	hs->base.type = XRT_TRACKING_TYPE_MAGNETIC;
	snprintf(hs->base.name, XRT_TRACKING_NAME_LEN, "%s", "Sixense Magnetic Tracking");
	// Arbitrary static transform from local space to base.
	hs->base.initial_offset.position.y = 1.0f;
	hs->base.initial_offset.position.z = -0.25f;
	hs->base.initial_offset.orientation.w = 1.0f;

	ret = os_thread_helper_init(&hs->usb_thread);
	if (ret < 0) {
		HS_ERROR(hs, "Failed to init USB thread.");
	after_system_err:
		free(hs);
		os_hid_destroy(command_hid);
		os_hid_destroy(data_hid);
		return -1;
	}

	ret = os_mutex_init(&hs->data_mutex);
	if (ret < 0) {
		HS_ERROR(hs, "Failed to init data mutex.");
		os_thread_helper_destroy(&hs->usb_thread);
		goto after_system_err;
	}

	hs->data_hid = data_hid;
	hs->command_hid = command_hid;

	enum u_device_alloc_flags flags = (enum u_device_alloc_flags)0;
	hs->devs[0] = U_DEVICE_ALLOCATE(struct hydra_device, flags, HYDRA_MAX_CONTROLLER_INDEX, 0);
	hs->devs[1] = U_DEVICE_ALLOCATE(struct hydra_device, flags, HYDRA_MAX_CONTROLLER_INDEX, 0);

	hs->report_counter = -1;
	hs->refs = 2;

	hs->log_level = debug_get_log_option_hydra_log();

	u_var_add_root(hs, "Razer Hydra System", false);
	u_var_add_log_level(hs, &hs->log_level, "Log Level");
	u_var_add_bool(hs, &hs->was_in_gamepad_mode, "Was In Gamepad Mode");
	u_var_add_i32(hs, &hs->motion_attempt_number, "Motion Attempt Number");
	u_var_add_ro_i16(hs, &hs->report_counter, "Report Counter");

	// Populate the individual devices
	for (size_t i = 0; i < 2; ++i) {
		struct hydra_device *hd = hs->devs[i];

		u_device_populate_function_pointers(&hd->base, hydra_device_get_tracked_pose, hydra_device_destroy);
		hd->base.update_inputs = hydra_device_update_inputs;
		hd->base.name = XRT_DEVICE_HYDRA;
		snprintf(hd->base.str, XRT_DEVICE_NAME_LEN, "%s %i", "Razer Hydra Controller", (int)(i + 1));
		snprintf(hd->base.serial, XRT_DEVICE_NAME_LEN, "%s%i", "RZRHDRC", (int)(i + 1));
		SET_INPUT(1_CLICK);
		SET_INPUT(2_CLICK);
		SET_INPUT(3_CLICK);
		SET_INPUT(4_CLICK);
		SET_INPUT(MIDDLE_CLICK);
		SET_INPUT(BUMPER_CLICK);
		SET_INPUT(JOYSTICK_CLICK);
		SET_INPUT(JOYSTICK_VALUE);
		SET_INPUT(TRIGGER_VALUE);
		SET_INPUT(GRIP_POSE);
		SET_INPUT(AIM_POSE);
		hd->index = i;
		hd->sys = hs;

		const float fc_min = 9.f;
		const float fc_min_d = 9.f;
		const float beta = 0.1f;

		m_filter_euro_vec3_init(&hd->state.motion_vector_filters.position, fc_min, fc_min_d, beta);
		m_filter_euro_quat_init(&hd->state.motion_vector_filters.orientation, fc_min, fc_min_d, beta);

		m_relation_history_create(&hd->state.relation_history);

		hd->base.tracking_origin = &hs->base;

		hd->base.supported.position_tracking = true;
		hd->base.supported.orientation_tracking = true;
		hd->base.device_type = XRT_DEVICE_TYPE_ANY_HAND_CONTROLLER;

		out_xdevs[i] = &(hd->base);
	}

	ret = os_thread_helper_start(&hs->usb_thread, hydra_usb_thread_run, hs);
	if (ret < 0) {
		HS_ERROR(hs, "Failed to start USB thread.");

		// doing this will destroy the system as well
		xrt_device_destroy((struct xrt_device **)&hs->devs[0]);
		xrt_device_destroy((struct xrt_device **)&hs->devs[1]);

		return ret;
	}

	HS_INFO(hs, "Opened Razer Hydra!");
	return 2;
}
