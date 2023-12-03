// Copyright 2023, Joseph Albers.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief   Driver for Ultraleap's V5 API for the Leap Motion Controller.
 * @author  Joseph Albers <joseph.albers@outlook.de>
 * @ingroup drv_ulv5
 */

#include "ulv5_interface.h"
#include "util/u_device.h"
#include "util/u_var.h"
#include "util/u_debug.h"
#include "math/m_space.h"
#include "math/m_api.h"
#include "util/u_time.h"
#include "os/os_time.h"
#include "os/os_threading.h"

#include "LeapC.h"

DEBUG_GET_ONCE_LOG_OPTION(ulv5_log, "ULV5_LOG", U_LOGGING_INFO)

#define ULV5_TRACE(ulv5d, ...) U_LOG_XDEV_IFL_T(&ulv5d->base, ulv5d->log_level, __VA_ARGS__)
#define ULV5_DEBUG(ulv5d, ...) U_LOG_XDEV_IFL_D(&ulv5d->base, ulv5d->log_level, __VA_ARGS__)
#define ULV5_INFO(ulv5d, ...) U_LOG_XDEV_IFL_I(&ulv5d->base, ulv5d->log_level, __VA_ARGS__)
#define ULV5_WARN(ulv5d, ...) U_LOG_XDEV_IFL_W(&ulv5d->base, ulv5d->log_level, __VA_ARGS__)
#define ULV5_ERROR(ulv5d, ...) U_LOG_XDEV_IFL_E(&ulv5d->base, ulv5d->log_level, __VA_ARGS__)

extern "C" {

const char *
leap_result_to_string(eLeapRS result)
{
	switch (result) {
	case eLeapRS_Success: return "eLeapRS_Success";
	case eLeapRS_UnknownError: return "eLeapRS_UnknownError";
	case eLeapRS_InvalidArgument: return "eLeapRS_InvalidArgument";
	case eLeapRS_InsufficientResources: return "eLeapRS_InsufficientResources";
	case eLeapRS_InsufficientBuffer: return "eLeapRS_InsufficientBuffer";
	case eLeapRS_Timeout: return "eLeapRS_Timeout";
	case eLeapRS_NotConnected: return "eLeapRS_NotConnected";
	case eLeapRS_HandshakeIncomplete: return "eLeapRS_HandshakeIncomplete";
	case eLeapRS_BufferSizeOverflow: return "eLeapRS_BufferSizeOverflow";
	case eLeapRS_ProtocolError: return "eLeapRS_ProtocolError";
	case eLeapRS_InvalidClientID: return "eLeapRS_InvalidClientID";
	case eLeapRS_UnexpectedClosed: return "eLeapRS_UnexpectedClosed";
	case eLeapRS_UnknownImageFrameRequest: return "eLeapRS_UnknownImageFrameRequest";
	case eLeapRS_UnknownTrackingFrameID: return "eLeapRS_UnknownTrackingFrameID";
	case eLeapRS_RoutineIsNotSeer: return "eLeapRS_RoutineIsNotSeer";
	case eLeapRS_TimestampTooEarly: return "eLeapRS_TimestampTooEarly";
	case eLeapRS_ConcurrentPoll: return "eLeapRS_ConcurrentPoll";
	case eLeapRS_NotAvailable: return "eLeapRS_NotAvailable";
	case eLeapRS_NotStreaming: return "eLeapRS_NotStreaming";
	case eLeapRS_CannotOpenDevice: return "eLeapRS_CannotOpenDevice";
	default: return "unknown result type.";
	}
}

static const enum xrt_space_relation_flags valid_flags = (enum xrt_space_relation_flags)(
    XRT_SPACE_RELATION_ORIENTATION_VALID_BIT | XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT |
    XRT_SPACE_RELATION_POSITION_VALID_BIT | XRT_SPACE_RELATION_POSITION_TRACKED_BIT);


struct ulv5_device
{
	struct xrt_device base;

	struct xrt_tracking_origin tracking_origin;

	enum u_logging_level log_level;

	bool stop_frame_polling_thread;

	struct os_thread_helper oth;

	struct xrt_hand_joint_set joint_set[2];

	bool hand_exists[2];

	// LEAP_CONNECTION *leap_connection;
};

inline struct ulv5_device *
ulv5_device(struct xrt_device *xdev)
{
	return (struct ulv5_device *)xdev;
}

static void
ulv5_device_update_inputs(struct xrt_device *xdev)
{
	// Empty
}

static void
ulv5_device_get_hand_tracking(struct xrt_device *xdev,
                              enum xrt_input_name name,
                              uint64_t at_timestamp_ns,
                              struct xrt_hand_joint_set *out_value,
                              uint64_t *out_timestamp_ns)
{
	struct ulv5_device *ulv5d = ulv5_device(xdev);

	if (name != XRT_INPUT_GENERIC_HAND_TRACKING_LEFT && name != XRT_INPUT_GENERIC_HAND_TRACKING_RIGHT) {
		ULV5_ERROR(ulv5d, "unknown input name for hand tracker");
		return;
	}

	bool hand_index = (name == XRT_INPUT_GENERIC_HAND_TRACKING_RIGHT); // 0 if left, 1 if right.

	bool hand_valid = ulv5d->hand_exists[hand_index];

	os_thread_helper_lock(&ulv5d->oth);
	memcpy(out_value, &ulv5d->joint_set[hand_index], sizeof(struct xrt_hand_joint_set));
	hand_valid = ulv5d->hand_exists[hand_index];
	os_thread_helper_unlock(&ulv5d->oth);
	m_space_relation_ident(&out_value->hand_pose);

	if (hand_valid) {
		out_value->is_active = true;
		out_value->hand_pose.relation_flags = valid_flags;
	} else {
		out_value->is_active = false;
	}
	// still a lie...
	*out_timestamp_ns = at_timestamp_ns;
}

// todo: cleanly shutdown the LEAP_CONNECTION
static void
ulv5_device_destroy(struct xrt_device *xdev)
{
	struct ulv5_device *ulv5d = ulv5_device(xdev);

	ulv5d->stop_frame_polling_thread = true;

	// Destroy also stops the thread.
	os_thread_helper_destroy(&ulv5d->oth);

	// Remove the variable tracking.
	u_var_remove_root(ulv5d);

	u_device_free(&ulv5d->base);
}

static void
ulv5_process_joint(LEAP_VECTOR joint_pos,
                   LEAP_QUATERNION joint_orientation,
                   float width,
                   struct xrt_hand_joint_value *joint)
{
	joint->radius = (width / 1000) / 2;

	struct xrt_space_relation *relation = &joint->relation;
	relation->pose.position.x = joint_pos.x * -1 / 1000.0;
	relation->pose.position.y = joint_pos.z * -1 / 1000.0;
	relation->pose.position.z = joint_pos.y * -1 / 1000.0;
	relation->relation_flags = valid_flags;
}

void
ulv5_process_hand(LEAP_HAND hand, struct ulv5_device *ulv5d, int handedness)
{
	struct xrt_hand_joint_set dummy_joint_set;
// gives access to individual joints of the joint_set
#define dummy_joint_set(y) &dummy_joint_set.values.hand_joint_set_default[XRT_HAND_JOINT_##y]

	ulv5_process_joint(hand.palm.position, hand.palm.orientation, hand.palm.width, dummy_joint_set(PALM));
	// wrist is the next_joint of the arm
	ulv5_process_joint(hand.arm.next_joint, hand.arm.rotation, hand.arm.width, dummy_joint_set(WRIST));
	// thumb
	ulv5_process_joint(hand.thumb.proximal.prev_joint, hand.thumb.proximal.rotation, hand.thumb.proximal.width,
	                   dummy_joint_set(THUMB_METACARPAL));
	ulv5_process_joint(hand.thumb.intermediate.prev_joint, hand.thumb.intermediate.rotation,
	                   hand.thumb.intermediate.width, dummy_joint_set(THUMB_PROXIMAL));
	ulv5_process_joint(hand.thumb.distal.prev_joint, hand.thumb.distal.rotation, hand.thumb.distal.width,
	                   dummy_joint_set(THUMB_DISTAL));
	ulv5_process_joint(hand.thumb.distal.next_joint, hand.thumb.distal.rotation, hand.thumb.distal.width,
	                   dummy_joint_set(THUMB_TIP));
	// index
	ulv5_process_joint(hand.index.metacarpal.prev_joint, hand.index.metacarpal.rotation,
	                   hand.index.metacarpal.width, dummy_joint_set(INDEX_METACARPAL));
	ulv5_process_joint(hand.index.proximal.prev_joint, hand.index.proximal.rotation, hand.index.proximal.width,
	                   dummy_joint_set(INDEX_PROXIMAL));
	ulv5_process_joint(hand.index.intermediate.prev_joint, hand.index.intermediate.rotation,
	                   hand.index.intermediate.width, dummy_joint_set(INDEX_INTERMEDIATE));
	ulv5_process_joint(hand.index.distal.prev_joint, hand.index.distal.rotation, hand.index.distal.width,
	                   dummy_joint_set(INDEX_DISTAL));
	ulv5_process_joint(hand.index.distal.next_joint, hand.index.distal.rotation, hand.index.distal.width,
	                   dummy_joint_set(INDEX_TIP));
	// middle
	ulv5_process_joint(hand.middle.metacarpal.prev_joint, hand.middle.metacarpal.rotation,
	                   hand.middle.metacarpal.width, dummy_joint_set(MIDDLE_METACARPAL));
	ulv5_process_joint(hand.middle.proximal.prev_joint, hand.middle.proximal.rotation, hand.middle.proximal.width,
	                   dummy_joint_set(MIDDLE_PROXIMAL));
	ulv5_process_joint(hand.middle.intermediate.prev_joint, hand.middle.intermediate.rotation,
	                   hand.middle.intermediate.width, dummy_joint_set(MIDDLE_INTERMEDIATE));
	ulv5_process_joint(hand.middle.distal.prev_joint, hand.middle.distal.rotation, hand.middle.distal.width,
	                   dummy_joint_set(MIDDLE_DISTAL));
	ulv5_process_joint(hand.middle.distal.next_joint, hand.middle.distal.rotation, hand.middle.distal.width,
	                   dummy_joint_set(MIDDLE_TIP));
	// ring
	ulv5_process_joint(hand.ring.metacarpal.prev_joint, hand.ring.metacarpal.rotation, hand.ring.metacarpal.width,
	                   dummy_joint_set(RING_METACARPAL));
	ulv5_process_joint(hand.ring.proximal.prev_joint, hand.ring.proximal.rotation, hand.ring.proximal.width,
	                   dummy_joint_set(RING_PROXIMAL));
	ulv5_process_joint(hand.ring.intermediate.prev_joint, hand.ring.intermediate.rotation,
	                   hand.ring.intermediate.width, dummy_joint_set(RING_INTERMEDIATE));
	ulv5_process_joint(hand.ring.distal.prev_joint, hand.ring.distal.rotation, hand.ring.distal.width,
	                   dummy_joint_set(RING_DISTAL));
	ulv5_process_joint(hand.ring.distal.next_joint, hand.ring.distal.rotation, hand.ring.distal.width,
	                   dummy_joint_set(RING_TIP));
	// pinky
	ulv5_process_joint(hand.pinky.metacarpal.prev_joint, hand.pinky.metacarpal.rotation,
	                   hand.pinky.metacarpal.width, dummy_joint_set(LITTLE_METACARPAL));
	ulv5_process_joint(hand.pinky.proximal.prev_joint, hand.pinky.proximal.rotation, hand.pinky.proximal.width,
	                   dummy_joint_set(LITTLE_PROXIMAL));
	ulv5_process_joint(hand.pinky.intermediate.prev_joint, hand.pinky.intermediate.rotation,
	                   hand.pinky.intermediate.width, dummy_joint_set(LITTLE_INTERMEDIATE));
	ulv5_process_joint(hand.pinky.distal.prev_joint, hand.pinky.distal.rotation, hand.pinky.distal.width,
	                   dummy_joint_set(LITTLE_DISTAL));
	ulv5_process_joint(hand.pinky.distal.next_joint, hand.pinky.distal.rotation, hand.pinky.distal.width,
	                   dummy_joint_set(LITTLE_TIP));

	// copy over to ulv5d joint set
	os_thread_helper_lock(&ulv5d->oth);
	memcpy(&ulv5d->joint_set[handedness], &dummy_joint_set, sizeof(struct xrt_hand_joint_set));
	os_thread_helper_unlock(&ulv5d->oth);
}

void *
leap_input_loop(void *ptr_to_xdev)
{
	struct xrt_device *xdev = (struct xrt_device *)ptr_to_xdev;
	struct ulv5_device *ulv5d = ulv5_device(xdev);

	LEAP_CONNECTION connection;
	if (LeapCreateConnection(NULL, &connection) == eLeapRS_Success)
		ULV5_INFO(ulv5d, "created leap connection.");
	if (LeapOpenConnection(connection) == eLeapRS_Success)
		ULV5_INFO(ulv5d, "opened leap connection to background service.");
	if (LeapSetTrackingMode(connection, eLeapTrackingMode_HMD) == eLeapRS_Success)
		ULV5_INFO(ulv5d, "set tracking mode to HMD use.");

	// check if the leap hardware is connected
	uint32_t num_connected_devices = 0;
	for (int i = 0; i < 5; i++) {
		LEAP_CONNECTION_MESSAGE msg;
		LeapPollConnection(connection, 1000, &msg);
		LeapGetDeviceList(connection, NULL, &num_connected_devices);

		if (num_connected_devices > 0) {
			break;
		}

		ULV5_INFO(ulv5d, "waiting for leap USB connection...");
	}
	if (num_connected_devices == 0)
		ULV5_ERROR(ulv5d, "leap hardware is physically not connected.");

	// main loop, polling the message queue of leap background service
	LEAP_CONNECTION_MESSAGE msg;
	const LEAP_TRACKING_EVENT *tracking_event;
	unsigned int timeout = 1000;
	eLeapRS result;
	while (!ulv5d->stop_frame_polling_thread) {
		result = LeapPollConnection(connection, timeout, &msg);

		if (result != eLeapRS_Success)
			ULV5_ERROR(ulv5d,
			           "LeapPollConnection returned %s\n"
			           "TIP: make sure you are connected to a full USB2.0 bandwidth port"
			           "(not a HUB with multiple devices connected)",
			           leap_result_to_string(result));

		// only care about hand tracking data
		if (msg.type == eLeapEventType_Tracking) {
			tracking_event = msg.tracking_event;

			uint32_t num_hands = tracking_event->nHands;
			LEAP_HAND *hands = tracking_event->pHands;

			for (uint32_t i = 0; i < num_hands; i++) {
				int handedness = hands[i].type;
				ulv5_process_hand(hands[i], ulv5d, handedness);
				ulv5d->hand_exists[handedness] = true;
			}
		}
	}
}

xrt_result_t
ulv5_create_device(struct xrt_device **out_xdev)
{
	enum u_device_alloc_flags flags = U_DEVICE_ALLOC_NO_FLAGS;
	int num_hands = 2;

	struct ulv5_device *ulv5d = U_DEVICE_ALLOCATE(struct ulv5_device, flags, num_hands, 0);

	os_thread_helper_init(&ulv5d->oth);
	os_thread_helper_start(&ulv5d->oth, (&leap_input_loop), (void *)&ulv5d->base);

	ulv5d->base.tracking_origin = &ulv5d->tracking_origin;
	ulv5d->base.tracking_origin->type = XRT_TRACKING_TYPE_OTHER;

	math_pose_identity(&ulv5d->base.tracking_origin->offset);

	ulv5d->log_level = debug_get_log_option_ulv5_log();

	ulv5d->base.update_inputs = ulv5_device_update_inputs;
	ulv5d->base.get_hand_tracking = ulv5_device_get_hand_tracking;
	ulv5d->base.destroy = ulv5_device_destroy;

	strncpy(ulv5d->base.str, "Leap Motion v5 driver", XRT_DEVICE_NAME_LEN);
	strncpy(ulv5d->base.serial, "Leap Motion v5 driver", XRT_DEVICE_NAME_LEN);

	ulv5d->base.inputs[0].name = XRT_INPUT_GENERIC_HAND_TRACKING_LEFT;
	ulv5d->base.inputs[1].name = XRT_INPUT_GENERIC_HAND_TRACKING_RIGHT;

	ulv5d->base.name = XRT_DEVICE_HAND_TRACKER;

	ulv5d->base.device_type = XRT_DEVICE_TYPE_HAND_TRACKER;
	ulv5d->base.hand_tracking_supported = true;

	u_var_add_root(ulv5d, "Leap Motion v5 driver", true);
	u_var_add_ro_text(ulv5d, ulv5d->base.str, "Name");

	ULV5_INFO(ulv5d, "Hand Tracker initialized!");

	out_xdev[0] = &ulv5d->base;

	return XRT_SUCCESS;
}

} // extern "C"