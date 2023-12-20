// Copyright 2023, Shawn Wallace
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief SteamVR driver device implementation.
 * @author Shawn Wallace <yungwallace@live.com>
 * @ingroup drv_steamvr_lh
 */

#include <functional>
#include <cstring>
#include <math.h>
#include <thread>
#include <algorithm>

#include "math/m_api.h"
#include "math/m_relation_history.h"
#include "math/m_space.h"
#include "device.hpp"
#include "interfaces/context.hpp"
#include "os/os_time.h"
#include "util/u_debug.h"
#include "util/u_device.h"
#include "util/u_hand_simulation.h"
#include "util/u_hand_tracking.h"
#include "util/u_logging.h"
#include "util/u_json.hpp"
#include "xrt/xrt_defines.h"
#include "xrt/xrt_device.h"

#include "vive/vive_poses.h"

#define DEV_ERR(...) U_LOG_IFL_E(ctx->log_level, __VA_ARGS__)
#define DEV_WARN(...) U_LOG_IFL_W(ctx->log_level, __VA_ARGS__)
#define DEV_INFO(...) U_LOG_IFL_I(ctx->log_level, __VA_ARGS__)
#define DEV_DEBUG(...) U_LOG_IFL_D(ctx->log_level, __VA_ARGS__)

#define DEG_TO_RAD(DEG) (DEG * M_PI / 180.)

DEBUG_GET_ONCE_BOOL_OPTION(lh_emulate_hand, "LH_EMULATE_HAND", true)

// Each device will have its own input class.
struct InputClass
{
	xrt_device_name name;
	std::string description;
	const std::vector<xrt_input_name> poses;
	const std::unordered_map<std::string_view, xrt_input_name> non_poses;
	const std::unordered_map<std::string_view, IndexFinger> finger_curls;
};

namespace {
const std::unordered_map<std::string_view, InputClass> hmd_classes{
    {"vive", InputClass{XRT_DEVICE_GENERIC_HMD, "Vive HMD", {XRT_INPUT_GENERIC_HEAD_POSE}, {}, {}}},
    {"indexhmd", InputClass{XRT_DEVICE_GENERIC_HMD, "Index HMD", {XRT_INPUT_GENERIC_HEAD_POSE}, {}, {}}},
    {"vive_pro", InputClass{XRT_DEVICE_GENERIC_HMD, "Vive Pro HMD", {XRT_INPUT_GENERIC_HEAD_POSE}, {}, {}}},
};

// Adding support for a new controller is a simple as adding it here.
// The key for the map needs to be the name of input profile as indicated by the lighthouse driver.
const std::unordered_map<std::string_view, InputClass> controller_classes{
    {
        "vive_controller",
        InputClass{
            XRT_DEVICE_VIVE_WAND,
            "Vive Wand",
            {
                XRT_INPUT_VIVE_GRIP_POSE,
                XRT_INPUT_VIVE_AIM_POSE,
            },
            {
                {"/input/application_menu/click", XRT_INPUT_VIVE_MENU_CLICK},
                {"/input/trackpad/click", XRT_INPUT_VIVE_TRACKPAD_CLICK},
                {"/input/trackpad/touch", XRT_INPUT_VIVE_TRACKPAD_TOUCH},
                {"/input/system/click", XRT_INPUT_VIVE_SYSTEM_CLICK},
                {"/input/trigger/click", XRT_INPUT_VIVE_TRIGGER_CLICK},
                {"/input/trigger/value", XRT_INPUT_VIVE_TRIGGER_VALUE},
                {"/input/grip/click", XRT_INPUT_VIVE_SQUEEZE_CLICK},
                {"/input/trackpad", XRT_INPUT_VIVE_TRACKPAD},
            },
            {
                // No fingers on this controller type
            },
        },
    },
    {
        "index_controller",
        InputClass{
            XRT_DEVICE_INDEX_CONTROLLER,
            "Valve Index Controller",
            {
                XRT_INPUT_INDEX_GRIP_POSE,
                XRT_INPUT_INDEX_AIM_POSE,
            },
            {
                {"/input/system/click", XRT_INPUT_INDEX_SYSTEM_CLICK},
                {"/input/system/touch", XRT_INPUT_INDEX_SYSTEM_TOUCH},
                {"/input/a/click", XRT_INPUT_INDEX_A_CLICK},
                {"/input/a/touch", XRT_INPUT_INDEX_A_TOUCH},
                {"/input/b/click", XRT_INPUT_INDEX_B_CLICK},
                {"/input/b/touch", XRT_INPUT_INDEX_B_TOUCH},
                {"/input/trigger/click", XRT_INPUT_INDEX_TRIGGER_CLICK},
                {"/input/trigger/touch", XRT_INPUT_INDEX_TRIGGER_TOUCH},
                {"/input/trigger/value", XRT_INPUT_INDEX_TRIGGER_VALUE},
                {"/input/grip/force", XRT_INPUT_INDEX_SQUEEZE_FORCE},
                {"/input/grip/value", XRT_INPUT_INDEX_SQUEEZE_VALUE},
                {"/input/thumbstick/click", XRT_INPUT_INDEX_THUMBSTICK_CLICK},
                {"/input/thumbstick/touch", XRT_INPUT_INDEX_THUMBSTICK_TOUCH},
                {"/input/thumbstick", XRT_INPUT_INDEX_THUMBSTICK},
                {"/input/trackpad/force", XRT_INPUT_INDEX_TRACKPAD_FORCE},
                {"/input/trackpad/touch", XRT_INPUT_INDEX_TRACKPAD_TOUCH},
                {"/input/trackpad", XRT_INPUT_INDEX_TRACKPAD},
            },
            {
                {"/input/finger/index", IndexFinger::Index},
                {"/input/finger/middle", IndexFinger::Middle},
                {"/input/finger/ring", IndexFinger::Ring},
                {"/input/finger/pinky", IndexFinger::Pinky},
            },
        },
    },
    {
        "vive_tracker",
        InputClass{
            XRT_DEVICE_VIVE_TRACKER,
            "HTC Vive Tracker",
            {
                XRT_INPUT_GENERIC_TRACKER_POSE,
            },
            {
                {"/input/power/click", XRT_INPUT_VIVE_TRACKER_SYSTEM_CLICK},
                {"/input/grip/click", XRT_INPUT_VIVE_TRACKER_SQUEEZE_CLICK},
                {"/input/application_menu/click", XRT_INPUT_VIVE_TRACKER_MENU_CLICK},
                {"/input/trigger/click", XRT_INPUT_VIVE_TRACKER_TRIGGER_CLICK},
                {"/input/thumb/click", XRT_INPUT_VIVE_TRACKER_TRACKPAD_CLICK},
            },
            {
                // No fingers on this controller type
            },
        },
    },
    {
        "tundra_tracker",
        InputClass{
            XRT_DEVICE_VIVE_TRACKER,
            "Tundra Tracker",
            {
                XRT_INPUT_GENERIC_TRACKER_POSE,
            },
            {
                {"/input/power/click", XRT_INPUT_VIVE_TRACKER_SYSTEM_CLICK},
                {"/input/grip/click", XRT_INPUT_VIVE_TRACKER_SQUEEZE_CLICK},
                {"/input/application_menu/click", XRT_INPUT_VIVE_TRACKER_MENU_CLICK},
                {"/input/trigger/click", XRT_INPUT_VIVE_TRACKER_TRIGGER_CLICK},
                {"/input/thumb/click", XRT_INPUT_VIVE_TRACKER_TRACKPAD_CLICK},
            },
            {
                // No fingers on this controller type
            },
        },
    },
};

uint64_t
chrono_timestamp_ns()
{
	auto now = std::chrono::steady_clock::now().time_since_epoch();
	uint64_t ts = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
	return ts;
}

// Template for calling a member function of Device from a free function
template <typename DeviceType, auto Func, typename Ret, typename... Args>
inline Ret
device_bouncer(struct xrt_device *xdev, Args... args)
{
	auto *dev = static_cast<DeviceType *>(xdev);
	return std::invoke(Func, dev, args...);
}
} // namespace

HmdDevice::HmdDevice(const DeviceBuilder &builder) : Device(builder)
{
	this->name = XRT_DEVICE_GENERIC_HMD;
	this->device_type = XRT_DEVICE_TYPE_HMD;
	this->container_handle = 0;

#define SETUP_MEMBER_FUNC(name) this->xrt_device::name = &device_bouncer<HmdDevice, &HmdDevice::name>
	SETUP_MEMBER_FUNC(get_view_poses);
	SETUP_MEMBER_FUNC(compute_distortion);
#undef SETUP_MEMBER_FUNC
}

ControllerDevice::ControllerDevice(vr::PropertyContainerHandle_t handle, const DeviceBuilder &builder) : Device(builder)
{
	this->device_type = XRT_DEVICE_TYPE_UNKNOWN;
	this->container_handle = handle;
#define SETUP_MEMBER_FUNC(name) this->xrt_device::name = &device_bouncer<ControllerDevice, &ControllerDevice::name>
	SETUP_MEMBER_FUNC(set_output);
	SETUP_MEMBER_FUNC(get_hand_tracking);
#undef SETUP_MEMBER_FUNC
}

Device::~Device()
{
	m_relation_history_destroy(&relation_hist);
}

Device::Device(const DeviceBuilder &builder) : xrt_device({}), ctx(builder.ctx), driver(builder.driver)
{
	m_relation_history_create(&relation_hist);
	std::strncpy(this->serial, builder.serial, XRT_DEVICE_NAME_LEN - 1);
	this->serial[XRT_DEVICE_NAME_LEN - 1] = 0;
	this->tracking_origin = ctx.get();
	this->orientation_tracking_supported = true;
	this->position_tracking_supported = true;
	this->hand_tracking_supported = true;
	this->force_feedback_supported = false;
	this->form_factor_check_supported = false;

#define SETUP_MEMBER_FUNC(name) this->xrt_device::name = &device_bouncer<Device, &Device::name>
	SETUP_MEMBER_FUNC(update_inputs);
	SETUP_MEMBER_FUNC(get_tracked_pose);
#undef SETUP_MEMBER_FUNC

	this->xrt_device::destroy = [](xrt_device *xdev) {
		auto *dev = static_cast<Device *>(xdev);
		dev->driver->Deactivate();
		delete dev;
	};

	init_chaperone(builder.steam_install);
}

void
ControllerDevice::set_hand_tracking_hand(xrt_input_name name)
{
	if (has_index_hand_tracking) {
		inputs_map["HAND"]->name = name;
	}
}

void
Device::set_input_class(const InputClass *input_class)
{
	// this should only be called once
	assert(inputs_vec.empty());
	this->input_class = input_class;

	// reserve to ensure our pointers don't get invalidated
	inputs_vec.reserve(input_class->poses.size() + input_class->non_poses.size() + 1);
	for (xrt_input_name input : input_class->poses) {
		inputs_vec.push_back({true, 0, input, {}});
	}
	for (const auto &[path, input] : input_class->non_poses) {
		assert(inputs_vec.capacity() >= inputs_vec.size() + 1);
		inputs_vec.push_back({true, 0, input, {}});
		inputs_map.insert({path, &inputs_vec.back()});
	}
	this->inputs = inputs_vec.data();
	this->input_count = inputs_vec.size();
}

void
ControllerDevice::set_input_class(const InputClass *input_class)
{
	Device::set_input_class(input_class);
	if (!debug_get_bool_option_lh_emulate_hand()) {
		return;
	}
	has_index_hand_tracking = !input_class->finger_curls.empty();
	if (!has_index_hand_tracking) {
		return;
	}
	finger_inputs_vec.reserve(input_class->finger_curls.size());
	for (const auto &[path, finger] : input_class->finger_curls) {
		finger_inputs_vec.push_back({0, finger, 0.f});
		finger_inputs_map.insert({path, &finger_inputs_vec.back()});
	}
	inputs_vec.push_back({true, 0, XRT_INPUT_GENERIC_HAND_TRACKING_LEFT, {}});
	inputs_map.insert({std::string_view("HAND"), &inputs_vec.back()});
	this->inputs = inputs_vec.data();
	this->input_count = inputs_vec.size();
}

xrt_hand
ControllerDevice::get_xrt_hand()
{
	switch (this->device_type) {
	case XRT_DEVICE_TYPE_LEFT_HAND_CONTROLLER: {
		return xrt_hand::XRT_HAND_LEFT;
	}
	case XRT_DEVICE_TYPE_RIGHT_HAND_CONTROLLER: {
		return xrt_hand::XRT_HAND_RIGHT;
	}
	default: DEV_ERR("Device %s cannot be tracked as a hand!", serial); return xrt_hand::XRT_HAND_LEFT;
	}
}

const std::vector<std::string> FACE_BUTTONS = {
    "/input/system/touch", "/input/a/touch", "/input/b/touch", "/input/thumbstick/touch", "/input/trackpad/touch",
};

void
ControllerDevice::update_hand_tracking(struct xrt_hand_joint_set *out)
{
	if (!has_index_hand_tracking)
		return;
	float index = 0.f;
	float middle = 0.f;
	float ring = 0.f;
	float pinky = 0.f;
	float thumb = 0.f;
	for (auto fi : finger_inputs_vec) {
		switch (fi.finger) {
		case IndexFinger::Index: index = fi.value; break;
		case IndexFinger::Middle: middle = fi.value; break;
		case IndexFinger::Ring: ring = fi.value; break;
		case IndexFinger::Pinky: pinky = fi.value; break;
		default: break;
		}
	}
	for (const auto &name : FACE_BUTTONS) {
		auto *input = get_input_from_name(name);
		if (input && input->value.boolean) {
			thumb = 1.f;
			break;
		}
	}
	auto curl_values = u_hand_tracking_curl_values{pinky, ring, middle, index, thumb};

	struct xrt_space_relation hand_relation = {};
	uint64_t ts = chrono_timestamp_ns();
	m_relation_history_get_latest(relation_hist, &ts, &hand_relation);

	u_hand_sim_simulate_for_valve_index_knuckles(&curl_values, get_xrt_hand(), &hand_relation, out);

	struct xrt_relation_chain chain = {};

	struct xrt_pose pose_offset = XRT_POSE_IDENTITY;
	vive_poses_get_pose_offset(name, device_type, inputs_map["HAND"]->name, &pose_offset);

	m_relation_chain_push_pose(&chain, &pose_offset);
	m_relation_chain_push_relation(&chain, &hand_relation);
	m_relation_chain_resolve(&chain, &out->hand_pose);
}

xrt_input *
Device::get_input_from_name(const std::string_view name)
{
	// Return nullptr without any other output to suppress a pile of useless warnings found below.
	if (name == "/input/finger/index" || name == "/input/finger/middle" || name == "/input/finger/ring" ||
	    name == "/input/finger/pinky") {
		return nullptr;
	}
	auto input = inputs_map.find(name);
	if (input == inputs_map.end()) {
		DEV_WARN("requested unknown input name %s for device %s", std::string(name).c_str(), serial);
		return nullptr;
	}
	return input->second;
}

void
ControllerDevice::set_haptic_handle(vr::VRInputComponentHandle_t handle)
{
	// this should only be set once
	assert(output == nullptr);
	DEV_DEBUG("setting haptic handle for %lu", handle);
	haptic_handle = handle;
	xrt_output_name name;
	switch (this->name) {
	case XRT_DEVICE_VIVE_WAND: {
		name = XRT_OUTPUT_NAME_VIVE_HAPTIC;
		break;
	}
	case XRT_DEVICE_INDEX_CONTROLLER: {
		name = XRT_OUTPUT_NAME_INDEX_HAPTIC;
		break;
	}
	case XRT_DEVICE_VIVE_TRACKER: {
		name = XRT_OUTPUT_NAME_VIVE_TRACKER_HAPTIC;
		break;
	}
	default: {
		DEV_WARN("Unknown device name (%u), haptics will not work", this->name);
		return;
	}
	}
	output = std::make_unique<xrt_output>(xrt_output{name});
	this->output_count = 1;
	this->outputs = output.get();
}

void
Device::update_inputs()
{
	std::lock_guard<std::mutex> lock(frame_mutex);
	ctx->maybe_run_frame(++current_frame);
}

IndexFingerInput *
ControllerDevice::get_finger_from_name(const std::string_view name)
{
	auto finger = finger_inputs_map.find(name);
	if (finger == finger_inputs_map.end()) {
		DEV_WARN("requested unknown finger name %s for device %s", std::string(name).c_str(), serial);
		return nullptr;
	}
	return finger->second;
}

void
ControllerDevice::get_hand_tracking(enum xrt_input_name name,
                                    uint64_t desired_timestamp_ns,
                                    struct xrt_hand_joint_set *out_value,
                                    uint64_t *out_timestamp_ns)
{
	if (!has_index_hand_tracking)
		return;
	update_hand_tracking(out_value);
	out_value->is_active = true;
	hand_tracking_timestamp = desired_timestamp_ns;
	*out_timestamp_ns = hand_tracking_timestamp;
}

void
Device::get_pose(uint64_t at_timestamp_ns, xrt_space_relation *out_relation)
{
	m_relation_history_get(this->relation_hist, at_timestamp_ns, out_relation);
}

void
HmdDevice::get_tracked_pose(xrt_input_name name, uint64_t at_timestamp_ns, xrt_space_relation *out_relation)
{
	Device::get_pose(at_timestamp_ns, out_relation);
}

void
ControllerDevice::get_tracked_pose(xrt_input_name name, uint64_t at_timestamp_ns, xrt_space_relation *out_relation)
{
	xrt_space_relation rel = {};
	Device::get_pose(at_timestamp_ns, &rel);

	xrt_pose pose_offset = XRT_POSE_IDENTITY;
	vive_poses_get_pose_offset(input_class->name, device_type, name, &pose_offset);

	xrt_relation_chain relchain = {};

	m_relation_chain_push_pose(&relchain, &pose_offset);
	m_relation_chain_push_relation(&relchain, &rel);
	m_relation_chain_resolve(&relchain, out_relation);

	struct xrt_pose *p = &out_relation->pose;
	DEV_DEBUG("controller %u: GET_POSITION (%f %f %f) GET_ORIENTATION (%f, %f, %f, %f)", name, p->position.x,
	          p->position.y, p->position.z, p->orientation.x, p->orientation.y, p->orientation.z, p->orientation.w);
}

void
ControllerDevice::set_output(xrt_output_name name, const xrt_output_value *value)

{
	const auto &vib = value->vibration;
	if (vib.amplitude == 0.0)
		return;
	vr::VREvent_HapticVibration_t event;
	event.containerHandle = container_handle;
	event.componentHandle = haptic_handle;
	event.fDurationSeconds = (float)vib.duration_ns / 1e9f;
	// 0.0f in OpenXR means let the driver determine a frequency, but
	// in OpenVR means no haptic.
	event.fFrequency = std::max(vib.frequency, 1.0f);
	event.fAmplitude = vib.amplitude;

	ctx->add_haptic_event(event);
}

void
HmdDevice::SetDisplayEyeToHead(uint32_t unWhichDevice,
                               const vr::HmdMatrix34_t &eyeToHeadLeft,
                               const vr::HmdMatrix34_t &eyeToHeadRight)
{
	xrt_matrix_3x3 leftEye_prequat;
	xrt_matrix_3x3 rightEye_prequat;

	xrt_pose leftEye_postquat;
	xrt_pose rightEye_postquat;

	// This is a HmdMatrix34 to xrt_matrix_3x3 copy.
	for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
			leftEye_prequat.v[i * 3 + j] = eyeToHeadLeft.m[i][j];
			rightEye_prequat.v[i * 3 + j] = eyeToHeadRight.m[i][j];
		}
	}

	math_quat_from_matrix_3x3(&leftEye_prequat, &leftEye_postquat.orientation);
	math_quat_from_matrix_3x3(&rightEye_prequat, &rightEye_postquat.orientation);
	leftEye_postquat.position.x = eyeToHeadLeft.m[0][3];
	leftEye_postquat.position.y = eyeToHeadLeft.m[1][3];
	leftEye_postquat.position.z = eyeToHeadLeft.m[2][3];

	rightEye_postquat.position.x = eyeToHeadRight.m[0][3];
	rightEye_postquat.position.y = eyeToHeadRight.m[1][3];
	rightEye_postquat.position.z = eyeToHeadRight.m[2][3];

	this->eye[0].orientation = leftEye_postquat.orientation;
	this->eye[0].position = leftEye_postquat.position;
	this->eye[1].orientation = rightEye_postquat.orientation;
	this->eye[1].position = rightEye_postquat.position;
}

void
HmdDevice::get_view_poses(const xrt_vec3 *default_eye_relation,
                          uint64_t at_timestamp_ns,
                          uint32_t view_count,
                          xrt_space_relation *out_head_relation,
                          xrt_fov *out_fovs,
                          xrt_pose *out_poses)
{
	struct xrt_vec3 eye_relation = *default_eye_relation;
	eye_relation.x = ipd;

	u_device_get_view_poses( //
	    this,                //
	    &eye_relation,       //
	    at_timestamp_ns,     //
	    view_count,          //
	    out_head_relation,   //
	    out_fovs,            //
	    out_poses);          //

	out_poses[0].orientation = this->eye[0].orientation;
	out_poses[0].position.z = this->eye[0].position.z;
	out_poses[0].position.y = this->eye[0].position.y;
	out_poses[1].orientation = this->eye[1].orientation;
	out_poses[1].position.z = this->eye[1].position.z;
	out_poses[1].position.y = this->eye[1].position.y;
}

bool
HmdDevice::compute_distortion(uint32_t view, float u, float v, xrt_uv_triplet *out_result)
{
	vr::EVREye eye = (view == 0) ? vr::Eye_Left : vr::Eye_Right;
	vr::DistortionCoordinates_t coords = this->hmd_parts->display->ComputeDistortion(eye, u, v);
	out_result->r = {coords.rfRed[0], coords.rfRed[1]};
	out_result->g = {coords.rfGreen[0], coords.rfGreen[1]};
	out_result->b = {coords.rfBlue[0], coords.rfBlue[1]};
	return true;
}

void
HmdDevice::set_hmd_parts(std::unique_ptr<Parts> parts)
{
	{
		std::lock_guard lk(hmd_parts_mut);
		hmd_parts = std::move(parts);
		this->hmd = &hmd_parts->base;
	}
	hmd_parts_cv.notify_all();
}

namespace {
xrt_quat
copy_quat(const vr::HmdQuaternion_t &quat)
{
	return xrt_quat{(float)quat.x, (float)quat.y, (float)quat.z, (float)quat.w};
}

xrt_vec3
copy_vec3(const double (&vec)[3])
{
	return xrt_vec3{(float)vec[0], (float)vec[1], (float)vec[2]};
}

xrt_pose
copy_pose(const vr::HmdQuaternion_t &orientation, const double (&position)[3])
{
	return xrt_pose{copy_quat(orientation), copy_vec3(position)};
}
} // namespace

void
Device::init_chaperone(const std::string &steam_install)
{
	static bool initialized = false;
	if (initialized)
		return;

	initialized = true;

	// Lighthouse driver seems to create a lighthousedb.json and a chaperone_info.vrchap (which is json)
	// We will use the known_universes from the lighthousedb.json to match to a universe from chaperone_info.vrchap

	using xrt::auxiliary::util::json::JSONNode;
	auto lighthousedb = JSONNode::loadFromFile(steam_install + "/config/lighthouse/lighthousedb.json");
	if (lighthousedb.isInvalid()) {
		DEV_ERR("Couldn't load lighthousedb file, playspace center will be off - was Room Setup run?");
		return;
	}
	auto chap_info = JSONNode::loadFromFile(steam_install + "/config/chaperone_info.vrchap");
	if (chap_info.isInvalid()) {
		DEV_ERR("Couldn't load chaperone info, playspace center will be off - was Room Setup run?");
		return;
	}

	// XXX: This may be broken if there are multiple known universes - how do we determine which to use then?
	const JSONNode universe = lighthousedb["known_universes"][0];
	const std::string id = universe["id"].asString();
	JSONNode info = {};
	for (const JSONNode &u : chap_info["universes"].asArray()) {
		if (u["universeID"].asString() == id) {
			DEV_INFO("Found info for universe %s", id.c_str());
			info = u;
			break;
		}
	}

	if (info.isInvalid()) {
		DEV_ERR("Couldn't find chaperone info for universe %s, playspace center will be off", id.c_str());
		return;
	}

	std::vector<JSONNode> translation_arr = info["standing"]["translation"].asArray();

	// If the array is missing elements, add zero.
	for (size_t i = translation_arr.size(); i < 3; i++) {
		translation_arr.push_back(JSONNode("0.0"));
	}

	const double yaw = info["standing"]["yaw"].asDouble();
	const xrt_vec3 yaw_axis{0.0, -1.0, 0.0};
	math_quat_from_angle_vector(static_cast<float>(yaw), &yaw_axis, &chaperone.orientation);
	chaperone.position = copy_vec3({
	    translation_arr[0].asDouble(),
	    translation_arr[1].asDouble(),
	    translation_arr[2].asDouble(),
	});
	math_quat_rotate_vec3(&chaperone.orientation, &chaperone.position, &chaperone.position);
	DEV_INFO("Initialized chaperone data.");
}

void
Device::update_pose(const vr::DriverPose_t &newPose) const
{
	xrt_space_relation relation = {};
	if (newPose.poseIsValid && newPose.deviceIsConnected) {
		relation.relation_flags = XRT_SPACE_RELATION_BITMASK_ALL;
		relation.pose = copy_pose(newPose.qRotation, newPose.vecPosition);
		relation.linear_velocity = copy_vec3(newPose.vecVelocity);
		relation.angular_velocity = copy_vec3(newPose.vecAngularVelocity);

		math_quat_rotate_vec3(&relation.pose.orientation, &relation.angular_velocity,
		                      &relation.angular_velocity);

		// apply over local transform
		const xrt_pose local = copy_pose(newPose.qDriverFromHeadRotation, newPose.vecDriverFromHeadTranslation);
		math_pose_transform(&relation.pose, &local, &relation.pose);

		// apply world transform
		const xrt_pose world =
		    copy_pose(newPose.qWorldFromDriverRotation, newPose.vecWorldFromDriverTranslation);
		math_pose_transform(&world, &relation.pose, &relation.pose);
		math_quat_rotate_vec3(&world.orientation, &relation.linear_velocity, &relation.linear_velocity);
		math_quat_rotate_vec3(&world.orientation, &relation.angular_velocity, &relation.angular_velocity);

		// apply chaperone transform
		math_pose_transform(&chaperone, &relation.pose, &relation.pose);
		math_quat_rotate_vec3(&chaperone.orientation, &relation.linear_velocity, &relation.linear_velocity);
		math_quat_rotate_vec3(&chaperone.orientation, &relation.angular_velocity, &relation.angular_velocity);
	}

	const uint64_t ts = chrono_timestamp_ns() + static_cast<uint64_t>(newPose.poseTimeOffset * 1000000.0);

	m_relation_history_push(relation_hist, &relation, ts);
}

void
Device::handle_properties(const vr::PropertyWrite_t *batch, uint32_t count)
{
	for (uint32_t i = 0; i < count; ++i) {
		handle_property_write(batch[i]);
	}
}

void
HmdDevice::set_nominal_frame_interval(uint64_t interval_ns)
{
	auto set = [this, interval_ns] { hmd_parts->base.screens[0].nominal_frame_interval_ns = interval_ns; };

	if (hmd_parts) {
		set();
	} else {
		std::thread t([this, set] {
			std::unique_lock lk(hmd_parts_mut);
			hmd_parts_cv.wait(lk, [this] { return hmd_parts != nullptr; });
			set();
		});
		t.detach();
	}
}

namespace {
// From openvr driver documentation
// (https://github.com/ValveSoftware/openvr/blob/master/docs/Driver_API_Documentation.md#Input-Profiles):
// "Input profiles are expected to be a valid JSON file,
// and should be located: <driver_name>/resources/input/<device_name>_profile.json"
// So we will just parse the file name to get the device name.
std::string_view
parse_profile(std::string_view path)
{
	size_t name_start_idx = path.find_last_of('/') + 1;
	size_t name_end_idx = path.find_last_of('_');
	return path.substr(name_start_idx, name_end_idx - name_start_idx);
}
} // namespace

void
HmdDevice::handle_property_write(const vr::PropertyWrite_t &prop)
{
	switch (prop.prop) {
	case vr::Prop_DisplayFrequency_Float: {
		assert(prop.unBufferSize == sizeof(float));
		float freq = *static_cast<float *>(prop.pvBuffer);
		set_nominal_frame_interval((1.f / freq) * 1e9f);
		break;
	}
	case vr::Prop_InputProfilePath_String: {
		std::string_view profile =
		    parse_profile(std::string_view(static_cast<char *>(prop.pvBuffer), prop.unBufferSize));
		auto input_class = hmd_classes.find(profile);
		if (input_class == hmd_classes.end()) {
			DEV_ERR("Could not find input class for hmd profile %s", std::string(profile).c_str());
		} else {
			std::strcpy(this->str, input_class->second.description.c_str());
			this->name = input_class->second.name;
			set_input_class(&input_class->second);
		}
		break;
	}
	case vr::Prop_UserIpdMeters_Float: {
		if (*static_cast<float *>(prop.pvBuffer) != 0) {
			ipd = *static_cast<float *>(prop.pvBuffer);
		}
		break;
	}
	case vr::Prop_SecondsFromVsyncToPhotons_Float: {
		vsync_to_photon_ns = *static_cast<float *>(prop.pvBuffer) * 1e9f;
		break;
	}
	default: DEV_DEBUG("Unassigned HMD property: %i", prop.prop); break;
	}
}

void
ControllerDevice::handle_property_write(const vr::PropertyWrite_t &prop)
{
	switch (prop.prop) {
	case vr::Prop_InputProfilePath_String: {
		std::string_view profile =
		    parse_profile(std::string_view(static_cast<char *>(prop.pvBuffer), prop.unBufferSize));
		auto input_class = controller_classes.find(profile);
		if (input_class == controller_classes.end()) {
			DEV_ERR("Could not find input class for controller profile %s", std::string(profile).c_str());
		} else {
			std::strcpy(this->str, input_class->second.description.c_str());
			this->name = input_class->second.name;
			set_input_class(&input_class->second);
		}
		break;
	}
	case vr::Prop_ControllerRoleHint_Int32: {
		vr::ETrackedControllerRole role = *static_cast<vr::ETrackedControllerRole *>(prop.pvBuffer);
		switch (role) {
		case vr::TrackedControllerRole_Invalid: {
			this->device_type = XRT_DEVICE_TYPE_ANY_HAND_CONTROLLER;
			break;
		}
		case vr::TrackedControllerRole_RightHand: {
			this->device_type = XRT_DEVICE_TYPE_RIGHT_HAND_CONTROLLER;
			set_hand_tracking_hand(XRT_INPUT_GENERIC_HAND_TRACKING_RIGHT);
			break;
		}
		case vr::TrackedControllerRole_LeftHand: {
			this->device_type = XRT_DEVICE_TYPE_LEFT_HAND_CONTROLLER;
			set_hand_tracking_hand(XRT_INPUT_GENERIC_HAND_TRACKING_LEFT);
			break;
		}
		case vr::TrackedControllerRole_OptOut: {
			this->device_type = XRT_DEVICE_TYPE_GENERIC_TRACKER;
			break;
		}
		default: {
			this->device_type = XRT_DEVICE_TYPE_UNKNOWN;
			DEV_WARN("requested unimplemented role hint %i", this->device_type);
			break;
		}
		}
		break;
	}
	case vr::Prop_DeviceIsCharging_Bool: {
		float charging = *static_cast<bool *>(prop.pvBuffer);
		const char *name;
		switch (this->device_type) {
		case XRT_DEVICE_TYPE_LEFT_HAND_CONTROLLER: {
			name = "Left";
			break;
		}
		case XRT_DEVICE_TYPE_RIGHT_HAND_CONTROLLER: {
			name = "Right";
			break;
		}
		default: {
			name = "Unknown";
		}
		}
		DEV_DEBUG("Charging: %s: %s", name, charging ? "true" : "false");
		break;
	}
	case vr::Prop_DeviceBatteryPercentage_Float: {
		float bat = *static_cast<float *>(prop.pvBuffer);
		const char *name;
		switch (this->device_type) {
		case XRT_DEVICE_TYPE_LEFT_HAND_CONTROLLER: {
			name = "Left";
			break;
		}
		case XRT_DEVICE_TYPE_RIGHT_HAND_CONTROLLER: {
			name = "Right";
			break;
		}
		default: {
			name = "Unknown";
		}
		}
		DEV_DEBUG("Battery: %s: %f", name, bat);
		break;
	}
	default: DEV_DEBUG("Unassigned controller property: %i", prop.prop); break;
	}
}
