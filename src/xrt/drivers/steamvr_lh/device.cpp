// Copyright 2023, Shawn Wallace
// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief SteamVR driver device implementation.
 * @author Shawn Wallace <yungwallace@live.com>
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_steamvr_lh
 */

#include "math/m_api.h"
#include "math/m_predict.h"
#include "math/m_relation_history.h"
#include "math/m_space.h"

#include "os/os_time.h"

#include "interfaces/context.hpp"

#include "util/u_debug.h"
#include "util/u_device.h"
#include "util/u_device_id.h"
#include "util/u_hand_tracking.h"
#include "util/u_logging.h"
#include "util/u_json.hpp"

#include "util/u_time.h"
#include "xrt/xrt_defines.h"
#include "xrt/xrt_device.h"
#include "xrt/xrt_prober.h"

#include "vive/vive_poses.h"

#include "vp2/vp2_config.h"

#include "openvr_driver.h"

#include "device.hpp"

#include <cmath>
#include <functional>
#include <cstring>
#include <numbers>
#include <openvr_driver.h>
#include <thread>
#include <algorithm>
#include <map>

#define DEV_ERR(...) U_LOG_IFL_E(ctx->log_level, __VA_ARGS__)
#define DEV_WARN(...) U_LOG_IFL_W(ctx->log_level, __VA_ARGS__)
#define DEV_INFO(...) U_LOG_IFL_I(ctx->log_level, __VA_ARGS__)
#define DEV_DEBUG(...) U_LOG_IFL_D(ctx->log_level, __VA_ARGS__)

DEBUG_GET_ONCE_BOOL_OPTION(lh_emulate_hand, "LH_EMULATE_HAND", true)
DEBUG_GET_ONCE_FLOAT_OPTION(lh_override_ipd, "LH_OVERRIDE_IPD_MM", -1.0f)
DEBUG_GET_ONCE_BOOL_OPTION(lh_standby_on_exit, "LH_STANDBY_ON_EXIT", false)

// Each device will have its own input class.
struct InputClass
{
	xrt_device_name name;
	const std::vector<xrt_input_name> poses;
	const std::unordered_map<std::string_view, xrt_input_name> non_poses;
};

namespace {
using namespace std::string_view_literals;
// From https://github.com/ValveSoftware/openvr/blob/master/docs/Driver_API_Documentation.md#bone-structure
enum HandSkeletonBone : std::uint8_t
{
	eBone_Root = 0,
	eBone_Wrist,
	eBone_Thumb0,
	eBone_Thumb1,
	eBone_Thumb2,
	eBone_Thumb3,
	eBone_IndexFinger0,
	eBone_IndexFinger1,
	eBone_IndexFinger2,
	eBone_IndexFinger3,
	eBone_IndexFinger4,
	eBone_MiddleFinger0,
	eBone_MiddleFinger1,
	eBone_MiddleFinger2,
	eBone_MiddleFinger3,
	eBone_MiddleFinger4,
	eBone_RingFinger0,
	eBone_RingFinger1,
	eBone_RingFinger2,
	eBone_RingFinger3,
	eBone_RingFinger4,
	eBone_PinkyFinger0,
	eBone_PinkyFinger1,
	eBone_PinkyFinger2,
	eBone_PinkyFinger3,
	eBone_PinkyFinger4,
	eBone_Aux_Thumb,
	eBone_Aux_IndexFinger,
	eBone_Aux_MiddleFinger,
	eBone_Aux_RingFinger,
	eBone_Aux_PinkyFinger,
	eBone_Count
};

// Adding support for a new controller is a simple as adding it here.
// The key for the map needs to be the name of input profile as indicated by the lighthouse driver.
const std::unordered_map<std::string_view, InputClass> controller_classes{
    {
        "vive_controller",
        InputClass{
            XRT_DEVICE_VIVE_WAND,
            {
                XRT_INPUT_VIVE_GRIP_POSE,
                XRT_INPUT_VIVE_AIM_POSE,
                XRT_INPUT_GENERIC_PALM_POSE,
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
        },
    },
    {
        "index_controller",
        InputClass{
            XRT_DEVICE_INDEX_CONTROLLER,
            {
                XRT_INPUT_INDEX_GRIP_POSE,
                XRT_INPUT_INDEX_AIM_POSE,
                XRT_INPUT_GENERIC_PALM_POSE,
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
        },
    },
    {
        "flipvr_controller_vc1b",
        InputClass{
            XRT_DEVICE_FLIPVR,
            {
                XRT_INPUT_FLIPVR_GRIP_POSE,
                XRT_INPUT_FLIPVR_AIM_POSE,
            },
            {
                {"/input/system/click", XRT_INPUT_FLIPVR_SYSTEM_CLICK},
                {"/input/a/click", XRT_INPUT_FLIPVR_A_CLICK},
                {"/input/a/touch", XRT_INPUT_FLIPVR_A_TOUCH},
                {"/input/b/click", XRT_INPUT_FLIPVR_B_CLICK},
                {"/input/b/touch", XRT_INPUT_FLIPVR_B_TOUCH},
                {"/input/x/click", XRT_INPUT_FLIPVR_X_CLICK},
                {"/input/x/touch", XRT_INPUT_FLIPVR_X_TOUCH},
                {"/input/y/click", XRT_INPUT_FLIPVR_Y_CLICK},
                {"/input/y/touch", XRT_INPUT_FLIPVR_Y_TOUCH},
                {"/input/trigger/click", XRT_INPUT_FLIPVR_TRIGGER_CLICK},
                {"/input/trigger/touch", XRT_INPUT_FLIPVR_TRIGGER_TOUCH},
                {"/input/trigger/value", XRT_INPUT_FLIPVR_TRIGGER_VALUE},
                {"/input/grip/value", XRT_INPUT_FLIPVR_SQUEEZE_VALUE},
                {"/input/grip/click", XRT_INPUT_FLIPVR_SQUEEZE_CLICK},
                {"/input/joystick/click", XRT_INPUT_FLIPVR_THUMBSTICK_CLICK},
                {"/input/joystick/touch", XRT_INPUT_FLIPVR_THUMBSTICK_TOUCH},
                {"/input/joystick", XRT_INPUT_FLIPVR_THUMBSTICK},
            },
        },
    },
    {
        "vive_tracker",
        InputClass{
            XRT_DEVICE_VIVE_TRACKER,
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
        },
    },
    {
        "tundra_tracker",
        InputClass{
            XRT_DEVICE_VIVE_TRACKER,
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
        },
    },
};

// Used for any HMD that isn't in hmd_classes.
const InputClass generic_hmd_class{
    XRT_DEVICE_GENERIC_HMD,
    {
        XRT_INPUT_GENERIC_HEAD_POSE,
    },
    {
        {"/proximity", XRT_INPUT_GENERIC_HEAD_DETECT},
    },
};

// Adding support for a new HMD is as simple as adding it here.
// The key for the map is the variant determined from the model number given by the lighthouse driver, as the
// lighthouse driver gives the same input profile to multiple HMDs (e.g. the Vive Pro and Vive Pro 2).
const std::unordered_map<VIVE_VARIANT, InputClass> hmd_classes{
    {
        VIVE_VARIANT_PRO2,
        InputClass{
            XRT_DEVICE_VIVE_PRO2,
            {
                XRT_INPUT_GENERIC_HEAD_POSE,
            },
            {
                {"/proximity", XRT_INPUT_GENERIC_HEAD_DETECT},
                // Paths from the lighthouse vive_pro profile, on VP2 these are fed from the HID device.
                {"/input/system/click", XRT_INPUT_VIVE_PRO_SYSTEM_CLICK},
                {"/input/volume_up/click", XRT_INPUT_VIVE_PRO_VOLUP_CLICK},
                {"/input/volume_down/click", XRT_INPUT_VIVE_PRO_VOLDN_CLICK},
                {"/input/mute_mic/click", XRT_INPUT_VIVE_PRO_MUTE_MIC_CLICK},
            },
        },
    },
};

// Template for calling a member function of Device from a free function
template <typename DeviceType, auto Func, typename Ret, typename... Args>
inline Ret
device_bouncer(struct xrt_device *xdev, Args... args)
{
	auto *dev = static_cast<DeviceType *>(xdev);
	return std::invoke(Func, dev, args...);
}

// Setting used for brightness in the steamvr section. This isn't defined by the openvr header.
static const char *analog_gain_settings_key = "analogGain";

/**
 * Map a 0-1 (or > 1) brightness value into the analogGain value.
 */
float
brightness_to_analog_gain(float brightness)
{
	if (brightness >= 1.f)
		return brightness;
	if (brightness <= 0.f)
		return 0.f;

	return powf(brightness, 2.2);
}
xrt_pose
bone_to_pose(const vr::VRBoneTransform_t &bone)
{
	return xrt_pose{xrt_quat{bone.orientation.x, bone.orientation.y, bone.orientation.z, bone.orientation.w},
	                xrt_vec3{bone.position.v[0], bone.position.v[1], bone.position.v[2]}};
}
} // namespace

Property::Property(vr::PropertyTypeTag_t tag, void *buffer, uint32_t bufferSize)
{
	this->tag = tag;
	this->buffer.resize(bufferSize);
	std::memcpy(this->buffer.data(), buffer, bufferSize);
}

HmdDevice::HmdDevice(const DeviceBuilder &builder) : Device(builder)
{
	this->name = XRT_DEVICE_GENERIC_HMD;
	this->device_type = XRT_DEVICE_TYPE_HMD;
	this->container_handle = 0;

	this->supported.compositor_info = true;

	// Refined once lighthouse gives us the model number, see handle_property_write.
	set_hmd_class(&generic_hmd_class);

#define SETUP_MEMBER_FUNC(name) this->xrt_device::name = &device_bouncer<HmdDevice, &HmdDevice::name, xrt_result_t>
	SETUP_MEMBER_FUNC(update_inputs);
	SETUP_MEMBER_FUNC(get_view_poses);
	SETUP_MEMBER_FUNC(compute_distortion);
	SETUP_MEMBER_FUNC(set_brightness);
	SETUP_MEMBER_FUNC(get_brightness);
	SETUP_MEMBER_FUNC(get_compositor_info);
#undef SETUP_MEMBER_FUNC
}

ControllerDevice::ControllerDevice(vr::PropertyContainerHandle_t handle, const DeviceBuilder &builder) : Device(builder)
{
	this->device_type = XRT_DEVICE_TYPE_UNKNOWN;
	this->container_handle = handle;

	this->xrt_device::get_hand_tracking =
	    &device_bouncer<ControllerDevice, &ControllerDevice::get_hand_tracking, xrt_result_t>;
	this->xrt_device::set_output = &device_bouncer<ControllerDevice, &ControllerDevice::set_output, xrt_result_t>;
	this->xrt_device::notify_chirality =
	    &device_bouncer<ControllerDevice, &ControllerDevice::notify_chirality, xrt_result_t>;

	this->supported.notify_chirality = true;

	this->inputs_map["/skeleton/hand/left"] = &hand_tracking_inputs[XRT_HAND_LEFT];
	this->inputs_map["/skeleton/hand/right"] = &hand_tracking_inputs[XRT_HAND_RIGHT];
}

Device::~Device()
{
	m_relation_history_destroy(&relation_hist);
}

Device::Device(const DeviceBuilder &builder) : xrt_device({}), ctx(builder.ctx), driver(builder.driver)
{
	u_device_id_assign(this);

	m_relation_history_create(&relation_hist);
	std::strncpy(this->serial, builder.serial, XRT_DEVICE_NAME_LEN - 1);
	this->serial[XRT_DEVICE_NAME_LEN - 1] = 0;
	this->tracking_origin = ctx.get();
	this->supported.orientation_tracking = true;
	this->supported.position_tracking = true;
	this->supported.hand_tracking = true;
	this->supported.force_feedback = false;
	this->supported.form_factor_check = false;
	this->supported.battery_status = true;
	this->supported.brightness_control = true;

	this->xrt_device::update_inputs = u_device_noop_update_inputs;
#define SETUP_MEMBER_FUNC(name) this->xrt_device::name = &device_bouncer<Device, &Device::name>
	SETUP_MEMBER_FUNC(get_tracked_pose);
	SETUP_MEMBER_FUNC(get_battery_status);
#undef SETUP_MEMBER_FUNC

	this->xrt_device::destroy = [](xrt_device *xdev) {
		auto *dev = static_cast<Device *>(xdev);
		auto &ctx = dev->ctx;

		if (debug_get_bool_option_lh_standby_on_exit()) {
			dev->driver->EnterStandby();
		}
		dev->driver->Deactivate();

		// The context outlives us until the last device releases it, and its frame thread keeps
		// running that whole time - unregister before tearing anything down so it cannot reach us.
		{
			std::lock_guard lk(ctx->devices_mut);

			for (auto handle : dev->handles) {
				// Remove all the handles this device has from the global tables
				ctx->input.handle_to_input.erase(handle);
				ctx->input.vec2_inputs.erase(handle);
				ctx->input.skeleton_to_controller.erase(handle);
			}

			for (auto &input : dev->inputs_vec) {
				// Remove all our vec2 inputs from the global table
				ctx->input.vec2_input_to_components.erase(&input);
			}

			if (ctx->hmd == dev) {
				ctx->hmd = nullptr;
			}
			for (ControllerDevice *&controller : ctx->controller) {
				if (controller == dev) {
					controller = nullptr;
				}
			}
		}

		delete dev;
	};

	init_chaperone(builder.steam_install);
}

// NOTE: No operations that would force inputs_vec or finger_inputs_vec to reallocate (such as insertion)
// should be done after this function is called, otherwise the pointers in inputs_map/finger_inputs_map
// would be invalidated.
void
Device::set_input_class(const InputClass *input_class)
{
	// this should only be called once, or after clearing the inputs (see HmdDevice::set_hmd_class)
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

xrt_result_t
ControllerDevice::notify_chirality(bool has_chirality, xrt_hand chirality)
{
	// Just assume left hand if no chirality is specified
	this->skeleton_hand = has_chirality ? chirality : XRT_HAND_LEFT;

	return XRT_SUCCESS;
}

namespace {
xrt_quat
from_euler_angles(float x, float y, float z)
{
	const xrt_vec3 v{x, y, z};
	xrt_quat out;
	math_quat_from_euler_angles(&v, &out);
	return out;
}

constexpr float pi = std::numbers::pi_v<float>;
constexpr float frac_pi_2 = pi / 2.0f;

// OpenVR skeletal poses are defined with the palms facing each other, but OpenXR
// hand tracking is defined with the palms facing down. These per hand rotations
// are necessary to translate to what OpenXR expects.
const xrt_quat right_hand_rotate = from_euler_angles(0.0f, frac_pi_2, 0.0f);
const xrt_quat left_hand_rotate = from_euler_angles(0.0f, frac_pi_2, pi);

xrt_quat
right_wrist_rotate_init()
{
	const xrt_quat rot1 = from_euler_angles(0.0f, 0.0f, frac_pi_2);
	const xrt_quat rot2 = from_euler_angles(0.0f, pi, 0.0f);
	xrt_quat ret;
	math_quat_rotate(&rot1, &rot2, &ret);
	return ret;
}
xrt_quat
left_wrist_rotate_init()
{
	const xrt_quat rot1 = from_euler_angles(pi, 0.0f, 0.0f);
	const xrt_quat rot2 = from_euler_angles(0.0f, 0.0f, -frac_pi_2);
	xrt_quat ret;
	math_quat_rotate(&rot1, &rot2, &ret);
	return ret;
}

const xrt_quat right_wrist_rotate = right_wrist_rotate_init();
const xrt_quat left_wrist_rotate = left_wrist_rotate_init();

xrt_pose
generate_palm_pose(const xrt_pose &metacarpal_pose, const xrt_pose &proximal_pose)
{
	// OpenVR doesn't provide a palm joint, but the OpenXR palm is in the middle of
	// the metacarpal and proximal bones of the middle finger,
	// so we'll interpolate between them to generate it.
	xrt_pose pose;
	math_pose_interpolate(&metacarpal_pose, &proximal_pose, 0.5, &pose);
	// Use metacarpal orientation, because the palm shouldn't really rotate
	pose.orientation = metacarpal_pose.orientation;
	return pose;
}

xrt_pose
palm_offset_index(xrt_hand hand)
{
	// Taken from:
	// https://github.com/ValveSoftware/OpenXR-Canonical-Pose-Tool/blob/5e6f3f6db584d58483058ff3262e7eef02c3acfd/dist/steamvr/cpt_SteamVR-valve_index_controller.xml#L1
	switch (hand) {
	case XRT_HAND_LEFT:
		return xrt_pose{.orientation = xrt_quat{.x = -0.46, .y = -0.02, .z = -0.01, .w = 0.89},
		                .position = xrt_vec3{.x = -0.015, .y = 0.0, .z = 0.001}};
	case XRT_HAND_RIGHT:
		return xrt_pose{.orientation = xrt_quat{.x = -0.46, .y = 0.02, .z = 0.01, .w = 0.89},
		                .position = xrt_vec3{.x = 0.015, .y = 0.0, .z = 0.001}};
	}

	return {};
}

} // namespace

void
ControllerDevice::set_skeleton(std::span<const vr::VRBoneTransform_t> bones,
                               xrt_hand hand,
                               bool is_simulated,
                               const char *path)
{
	assert(bones.size() == eBone_Count);
	generate_palm_pose_offset(bones, hand);
	if (!is_simulated && debug_get_bool_option_lh_emulate_hand()) {
		assert(inputs_vec.capacity() >= inputs_vec.size() + 1);
		const xrt_input_name tracker_name =
		    (hand == XRT_HAND_RIGHT) ? XRT_INPUT_HT_CONFORMING_RIGHT : XRT_INPUT_HT_CONFORMING_LEFT;
		inputs_vec.push_back({true, 0, tracker_name, {}});
		inputs_map.insert({path, &inputs_vec.back()});
		this->input_count = inputs_vec.size();
		has_hand_tracking = true;
	}
}

void
ControllerDevice::generate_palm_pose_offset(std::span<const vr::VRBoneTransform_t> bones, xrt_hand hand)
{
	if (this->input_class->name == XRT_DEVICE_INDEX_CONTROLLER) {
		xrt_pose grip_offset;
		vive_poses_get_pose_offset(this->input_class->name, this->device_type, XRT_INPUT_INDEX_GRIP_POSE,
		                           &grip_offset);
		auto offset = palm_offset_index(hand);
		math_pose_transform(&grip_offset, &offset, &offset);
		palm_offsets[hand] = offset;
		return;
	}
	// The palm pose offset is generated from the OpenVR provided skeleton.
	// https://github.com/ValveSoftware/openvr/blob/master/docs/Driver_API_Documentation.md#notes-on-the-skeleton

	xrt_pose root = bone_to_pose(bones[eBone_Root]);
	xrt_pose wrist = bone_to_pose(bones[eBone_Wrist]);
	xrt_pose metacarpal = bone_to_pose(bones[eBone_MiddleFinger0]);
	xrt_pose proximal = bone_to_pose(bones[eBone_MiddleFinger1]);

	// The skeleton pose is given with the Root bone as origin.
	// To convert from this, according to OpenVR docs we transform the wrist
	// and then counter-transform the metacarpals
	xrt_pose root_inv;
	math_pose_invert(&root, &root_inv);
	math_pose_transform(&root_inv, &wrist, &wrist);
	math_pose_transform(&root, &metacarpal, &metacarpal);
	math_pose_transform(&wrist, &metacarpal, &metacarpal);
	math_pose_transform(&metacarpal, &proximal, &proximal);

	xrt_pose palm_offset = generate_palm_pose(metacarpal, proximal);
	xrt_quat palm_rotate = from_euler_angles(0.0f, 0.0f, frac_pi_2);

	switch (hand) {
	case XRT_HAND_LEFT: {
		math_quat_rotate(&palm_offset.orientation, &left_hand_rotate, &palm_offset.orientation);
		math_quat_invert(&palm_rotate, &palm_rotate);
		break;
	}
	case XRT_HAND_RIGHT: {
		math_quat_rotate(&palm_offset.orientation, &right_hand_rotate, &palm_offset.orientation);
		break;
	}
	}
	math_quat_rotate(&palm_offset.orientation, &palm_rotate, &palm_offset.orientation);

	// For controllers like the Vive Wands which can be in any hand, it will store both the left hand
	// and the right hand skeletons, so we need to store both.
	palm_offsets[hand] = palm_offset;
}

void
ControllerDevice::update_skeleton_transforms(std::span<const vr::VRBoneTransform_t> bones)
{
	if (!has_hand_tracking) {
		return;
	}

	assert(bones.size() == eBone_Count);

	xrt_hand_joint_set joint_set;
	int64_t timestamp;
	if (!m_relation_history_get_latest(relation_hist, &timestamp, &joint_set.hand_pose)) {
		return;
	}
	joint_set.is_active = true;
	auto &joints = joint_set.values.hand_joint_set_default;

	xrt_pose root = bone_to_pose(bones[eBone_Root]);
	xrt_pose wrist = bone_to_pose(bones[eBone_Wrist]);

	// Here we're doing the same transformation as seen in generate_palm_pose_offset.
	xrt_pose root_inv;
	math_pose_invert(&root, &root_inv);
	math_pose_transform(&root_inv, &wrist, &wrist);

	constexpr auto valid_flags = (enum xrt_space_relation_flags)(
	    XRT_SPACE_RELATION_POSITION_VALID_BIT | XRT_SPACE_RELATION_ORIENTATION_VALID_BIT |
	    XRT_SPACE_RELATION_POSITION_TRACKED_BIT | XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT);

	xrt_pose wrist_xr = wrist;

	switch (skeleton_hand) {
	case XRT_HAND_LEFT: {
		math_quat_rotate(&wrist_xr.orientation, &left_wrist_rotate, &wrist_xr.orientation);
		break;
	}
	case XRT_HAND_RIGHT: {
		math_quat_rotate(&wrist_xr.orientation, &right_wrist_rotate, &wrist_xr.orientation);
		break;
	}
	}

	joints[XRT_HAND_JOINT_WRIST].relation.pose = wrist_xr;
	joints[XRT_HAND_JOINT_WRIST].relation.relation_flags = valid_flags;

	xrt_pose parent_pose;
	for (int joint = XRT_HAND_JOINT_THUMB_METACARPAL; joint <= XRT_HAND_JOINT_LITTLE_TIP; ++joint) {
		// Luckily openvr and openxr joint values match
		xrt_pose pose = bone_to_pose(bones[joint]);
		joints[joint].relation.relation_flags = valid_flags;

		if (u_hand_joint_is_metacarpal((xrt_hand_joint)joint)) {
			// Counter transform metacarpals
			math_pose_transform(&root, &pose, &pose);
			math_pose_transform(&wrist, &pose, &pose);
		} else {
			math_pose_transform(&parent_pose, &pose, &pose);
		}

		parent_pose = pose;

		// Rotate joint to OpenXR orientation
		switch (skeleton_hand) {
		case XRT_HAND_LEFT: math_quat_rotate(&pose.orientation, &left_hand_rotate, &pose.orientation); break;
		case XRT_HAND_RIGHT: math_quat_rotate(&pose.orientation, &right_hand_rotate, &pose.orientation); break;
		}
		joints[joint].relation.pose = pose;
	}

	joints[XRT_HAND_JOINT_PALM].relation.relation_flags = valid_flags;
	joints[XRT_HAND_JOINT_PALM].relation.pose =
	    generate_palm_pose(joints[XRT_HAND_JOINT_MIDDLE_METACARPAL].relation.pose,
	                       joints[XRT_HAND_JOINT_MIDDLE_PROXIMAL].relation.pose);

	u_hand_joints_apply_joint_width(&joint_set);
	this->joint_history.push_back(JointsWithTimestamp{joint_set, timestamp});
}

xrt_input *
Device::get_input_from_name(const std::string_view name)
{
	static const std::array ignore_inputs = {"/input/finger/index"sv, "/input/finger/middle"sv,
	                                         "/input/finger/ring"sv, "/input/finger/pinky"sv,
	                                         "/input/grip/touch"sv};

	// Return nullptr without any other output to suppress a pile of useless warnings found below.
	if (std::ranges::find(ignore_inputs, name) != std::ranges::end(ignore_inputs)) {
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
	DEV_DEBUG("setting haptic handle for %" PRIu64, handle);
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
	case XRT_DEVICE_FLIPVR: {
		name = XRT_OUTPUT_NAME_FLIPVR_HAPTIC;
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

xrt_result_t
ControllerDevice::get_hand_tracking(enum xrt_input_name name,
                                    int64_t desired_timestamp_ns,
                                    struct xrt_hand_joint_set *out_value,
                                    int64_t *out_timestamp_ns)
{
	if (!has_hand_tracking) {
		return XRT_ERROR_NOT_IMPLEMENTED;
	}

	// No handtracking data?
	if (joint_history.empty()) {
		out_value->is_active = false;
		return XRT_SUCCESS;
	}

	const auto it = std::ranges::lower_bound(joint_history, desired_timestamp_ns, {},
	                                         [](const JointsWithTimestamp &joint) { return joint.timestamp; });

	auto predict_joint_set = [out_value, out_timestamp_ns,
	                          desired_timestamp_ns](const JointsWithTimestamp &joints) {
		out_value->is_active = joints.joint_set.is_active;
		int64_t delta_ns = desired_timestamp_ns - joints.timestamp;
		double delta_s = time_ns_to_s(delta_ns);

		*out_timestamp_ns = desired_timestamp_ns;
		m_predict_relation(&joints.joint_set.hand_pose, delta_s, &out_value->hand_pose);
		for (int i = 0; i < XRT_HAND_JOINT_COUNT; i++) {
			auto &new_joint = joints.joint_set.values.hand_joint_set_default[i];
			auto &interp_joint = out_value->values.hand_joint_set_default[i];

			m_predict_relation(&new_joint.relation, delta_s, &interp_joint.relation);
			interp_joint.radius = new_joint.radius;
		}
	};

	if (it == joint_history.end()) {
		// Timestamp is newer than anything in history
		predict_joint_set(joint_history.back());
	} else if (desired_timestamp_ns == it->timestamp) {
		*out_value = it->joint_set;
		*out_timestamp_ns = it->timestamp;
	} else if (it == joint_history.begin()) {
		// Timestamp is older than anything in history
		predict_joint_set(joint_history.front());
	} else {
		// Interpolate
		auto it_previous = it - 1;

		auto delta_before = desired_timestamp_ns - it_previous->timestamp;
		auto delta_after = it->timestamp - desired_timestamp_ns;
		float lerp_amt = (float)delta_before / (float)(delta_after - delta_before);

		auto interpolate = [lerp_amt](xrt_space_relation &previous, xrt_space_relation &next,
		                              xrt_space_relation &out) {
			auto flags = (xrt_space_relation_flags)(previous.relation_flags & next.relation_flags);
			m_space_relation_interpolate(&previous, &next, lerp_amt, flags, &out);
		};

		interpolate(it_previous->joint_set.hand_pose, it->joint_set.hand_pose, out_value->hand_pose);

		for (int i = 0; i < XRT_HAND_JOINT_COUNT; i++) {
			auto &prev = it_previous->joint_set.values.hand_joint_set_default[i];
			auto &next = it->joint_set.values.hand_joint_set_default[i];
			auto &out = out_value->values.hand_joint_set_default[i];
			interpolate(prev.relation, next.relation, out.relation);
			out.radius = next.radius;
		}

		*out_timestamp_ns = desired_timestamp_ns;
	}

	return XRT_SUCCESS;
}

void
Device::get_pose(uint64_t at_timestamp_ns, xrt_space_relation *out_relation)
{
	m_relation_history_get(this->relation_hist, at_timestamp_ns, out_relation);
}

xrt_result_t
Device::get_battery_status(bool *out_present, bool *out_charging, float *out_charge)
{
	*out_present = this->provides_battery_status;
	*out_charging = this->charging;
	*out_charge = this->charge;
	return XRT_SUCCESS;
}

xrt_result_t
HmdDevice::get_brightness(float *out_brightness)
{
	if (variant == VIVE_VARIANT_PRO2 && vp2.hid != nullptr) {
		this->brightness = vp2_get_brightness(vp2.hid);
	}

	*out_brightness = this->brightness;
	return XRT_SUCCESS;
}

xrt_result_t
HmdDevice::set_brightness(float brightness, bool relative)
{
	// Vive Pro 2 has a special command for setting brightness directly on the headset
	if (variant == VIVE_VARIANT_PRO2 && vp2.hid != nullptr) {
		brightness = relative ? vp2_get_brightness(vp2.hid) + brightness : brightness;

		int ret = vp2_set_brightness(vp2.hid, brightness);
		if (ret < 0) {
			DEV_ERR("Failed to set VP2 brightness, got error %d", ret);
			return XRT_ERROR_OUTPUT_REQUEST_FAILURE;
		}

		this->brightness = std::clamp(brightness, 0.0f, 1.3f);

		return XRT_SUCCESS;
	}

	constexpr auto min_brightness = 0.2f;
	constexpr auto max_brightness = 1.5f;

	const auto target_brightness = relative ? (this->brightness + brightness) : brightness;
	this->brightness = std::clamp(target_brightness, min_brightness, max_brightness);
	const auto analog_gain =
	    std::clamp(brightness_to_analog_gain(this->brightness), analog_gain_range.min, analog_gain_range.max);
	ctx->settings.SetFloat(vr::k_pch_SteamVR_Section, analog_gain_settings_key, analog_gain);
	return XRT_SUCCESS;
}

xrt_result_t
HmdDevice::get_compositor_info(const struct xrt_device_compositor_mode *mode,
                               struct xrt_device_compositor_info *out_info)
{
	time_duration_ns scanout_time_ns;
	enum xrt_panel_refresh_type panel_refresh_type;
	enum xrt_scanout_direction scanout_direction;

	vive_variant_scanout_info(this->variant, mode->frame_interval_ns, &scanout_time_ns, &scanout_direction,
	                          &panel_refresh_type);

	(*out_info) = {
	    .panel_refresh_type = panel_refresh_type,
	    .scanout_direction = scanout_direction,
	    .scanout_time_ns = scanout_time_ns,
	};

	return XRT_SUCCESS;
}

float
HmdDevice::getIpdM()
{
	// On Vive Pro 2, we get this from the VP2 HID device.
	if (this->vp2.hid) {
		float lens_separation;
		bool bret = vp2_get_status( //
		    this->vp2.hid,          //
		    nullptr,                //
		    &lens_separation,       //
		    nullptr,                //
		    nullptr,                //
		    nullptr,                //
		    nullptr);               //
		if (bret) {
			return lens_separation;
		}
	}

	return this->lh_ipd;
}

xrt_result_t
HmdDevice::update_inputs()
{
	// On Vive Pro 2, the proximity sensor and buttons are read from the VP2 HID device rather than lighthouse.
	if (this->vp2.hid) {
		bool proximity, system, volume_up, volume_down, mute_mic;
		bool bret = vp2_get_status( //
		    this->vp2.hid,          //
		    &proximity,             //
		    nullptr,                //
		    &system,                //
		    &volume_up,             //
		    &volume_down,           //
		    &mute_mic);             //
		if (bret) {
			int64_t now = os_monotonic_get_ns();
			auto set_bool = [&](std::string_view path, bool value) {
				auto input = inputs_map.find(path);
				if (input == inputs_map.end()) {
					return;
				}
				input->second->active = true;
				input->second->value.boolean = value;
				input->second->timestamp = now;
			};

			// Paths match the input class in hmd_classes.
			set_bool("/proximity", proximity);
			set_bool("/input/system/click", system);
			set_bool("/input/volume_up/click", volume_up);
			set_bool("/input/volume_down/click", volume_down);
			set_bool("/input/mute_mic/click", mute_mic);
		}
	}

	return XRT_SUCCESS;
}

xrt_result_t
HmdDevice::get_tracked_pose(xrt_input_name name, uint64_t at_timestamp_ns, xrt_space_relation *out_relation)
{
	switch (name) {
	case XRT_INPUT_GENERIC_HEAD_POSE: Device::get_pose(at_timestamp_ns, out_relation); break;
	default: U_LOG_XDEV_UNSUPPORTED_INPUT(this, ctx->log_level, name); return XRT_ERROR_INPUT_UNSUPPORTED;
	}

	return XRT_SUCCESS;
}

xrt_result_t
ControllerDevice::get_tracked_pose(xrt_input_name name, uint64_t at_timestamp_ns, xrt_space_relation *out_relation)
{
	xrt_space_relation rel = {};
	Device::get_pose(at_timestamp_ns, &rel);

	xrt_pose pose_offset = XRT_POSE_IDENTITY;

	if (name == XRT_INPUT_GENERIC_PALM_POSE) {
		if (!palm_offsets[skeleton_hand].has_value()) {
			DEV_ERR("%s hand skeleton has not been initialized",
			        skeleton_hand == XRT_HAND_LEFT ? "left" : "right");
			*out_relation = XRT_SPACE_RELATION_ZERO;
			return XRT_SUCCESS;
		}
		pose_offset = *palm_offsets[skeleton_hand];
	} else {
		vive_poses_get_pose_offset(input_class->name, device_type, name, &pose_offset);
	}
	xrt_relation_chain relchain = {};

	m_relation_chain_push_pose(&relchain, &pose_offset);
	m_relation_chain_push_relation(&relchain, &rel);
	m_relation_chain_resolve(&relchain, out_relation);

	struct xrt_pose *p = &out_relation->pose;
	DEV_DEBUG("controller %u: GET_POSITION (%f %f %f) GET_ORIENTATION (%f, %f, %f, %f)", name, p->position.x,
	          p->position.y, p->position.z, p->orientation.x, p->orientation.y, p->orientation.z, p->orientation.w);

	return XRT_SUCCESS;
}

xrt_result_t
ControllerDevice::set_output(xrt_output_name name, const xrt_output_value *value)

{
	const auto &vib = value->vibration;
	if (vib.amplitude == 0.0)
		return XRT_SUCCESS;
	vr::VREvent_HapticVibration_t event;
	event.containerHandle = container_handle;
	event.componentHandle = haptic_handle;
	event.fDurationSeconds = (float)vib.duration_ns / 1e9f;
	// 0.0f in OpenXR means let the driver determine a frequency, but
	// in OpenVR means no haptic, so let's set a reasonable default.
	float frequency = vib.frequency;
	if (frequency == 0.0) {
		frequency = 200.0f;
	}

	event.fFrequency = frequency;
	event.fAmplitude = vib.amplitude;

	// `latest_haptic_event` assumes single haptic component per device
	this->latest_haptic_event = ctx->add_haptic_event(event, this->latest_haptic_event);
	return XRT_SUCCESS;
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

xrt_result_t
HmdDevice::get_view_poses(const xrt_vec3 *default_eye_relation,
                          uint64_t at_timestamp_ns,
                          xrt_view_type view_type,
                          uint32_t view_count,
                          xrt_space_relation *out_head_relation,
                          xrt_fov *out_fovs,
                          xrt_pose *out_poses)
{
	struct xrt_vec3 eye_relation = *default_eye_relation;

	eye_relation.x = this->getIpdM();

	float override_ipd_mm = debug_get_float_option_lh_override_ipd();
	if (override_ipd_mm != -1.0f) {
		// Convert from mm -> m
		eye_relation.x = override_ipd_mm / 1000.0f;
	}

	xrt_result_t xret = u_device_get_view_poses( //
	    this,                                    //
	    &eye_relation,                           //
	    at_timestamp_ns,                         //
	    view_type,                               //
	    view_count,                              //
	    out_head_relation,                       //
	    out_fovs,                                //
	    out_poses);                              //
	if (xret != XRT_SUCCESS) {
		return xret;
	}

	out_poses[0].orientation = this->eye[0].orientation;
	out_poses[0].position.z = this->eye[0].position.z;
	out_poses[0].position.y = this->eye[0].position.y;
	out_poses[1].orientation = this->eye[1].orientation;
	out_poses[1].position.z = this->eye[1].position.z;
	out_poses[1].position.y = this->eye[1].position.y;

	return XRT_SUCCESS;
}

xrt_result_t
HmdDevice::compute_distortion(uint32_t view, float u, float v, xrt_uv_triplet *out_result)
{
	// Vive Pro 2 has it's own special distortion model
	if (this->variant == VIVE_VARIANT_PRO2 && this->vp2.hid != nullptr) {
		vp2_config *config = vp2_get_config(this->vp2.hid);

		struct xrt_vec2 uv = {u, v};

		// If computing through VP2 method fails, fall back to the standard method.
		if (htc_config_compute_distortion(&config->base, view, &uv, out_result)) {
			return XRT_SUCCESS;
		}
	}

	// Vive Pro 2 has a vertically flipped distortion map.
	if (this->variant == VIVE_VARIANT_PRO2) {
		v = 1.0f - v;
	}

	vr::EVREye eye = (view == 0) ? vr::Eye_Left : vr::Eye_Right;
	vr::DistortionCoordinates_t coords = this->hmd_parts->display->ComputeDistortion(eye, u, v);
	out_result->r = {coords.rfRed[0], coords.rfRed[1]};
	out_result->g = {coords.rfGreen[0], coords.rfGreen[1]};
	out_result->b = {coords.rfBlue[0], coords.rfBlue[1]};
	return XRT_SUCCESS;
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

	JSONNode info = {};
	bool universe_found = false;

	// XXX: This may be broken if there are multiple known universes - how do we determine which to use then?
	auto known_universes = lighthousedb["known_universes"].asArray();
	for (auto &universe : known_universes) {
		const std::string id = universe["id"].asString();
		for (const JSONNode &u : chap_info["universes"].asArray()) {
			if (u["universeID"].asString() == id) {
				DEV_INFO("Found info for universe %s", id.c_str());
				info = u;
				universe_found = true;
				break;
			}
		}
		if (universe_found) {
			break;
		}
	}

	if (info.isInvalid()) {
		DEV_ERR("Couldn't find chaperone info for any known universe, playspace center will be off");
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

inline xrt_space_relation_flags
operator|(xrt_space_relation_flags a, xrt_space_relation_flags b)
{
	return static_cast<xrt_space_relation_flags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline xrt_space_relation_flags &
operator|=(xrt_space_relation_flags &a, xrt_space_relation_flags b)
{
	a = a | b;
	return a;
}

void
Device::update_pose(const vr::DriverPose_t &newPose) const
{
	xrt_space_relation relation = {};

	if (newPose.poseIsValid) {
		// The pose is known to be valid but that alone is not enough to say whether the data comes from
		// inference or if it represents actively tracked position and orientation data. Furthermore we avoid
		// assumptions regarding the validity of time-derivatives until we know that they're based on tracked
		// data. This is a conservative strategy that should reduce concerns regarding drift. see
		// https://registry.khronos.org/OpenXR/specs/1.1/man/html/XrSpaceLocationFlagBits.html
		relation.relation_flags |= xrt_space_relation_flags::XRT_SPACE_RELATION_ORIENTATION_VALID_BIT |
		                           xrt_space_relation_flags::XRT_SPACE_RELATION_POSITION_VALID_BIT;

		switch (newPose.result) {
		// see https://github.com/ValveSoftware/openvr/blob/master/docs/Driver_API_Documentation.md
		case vr::TrackingResult_Running_OK:
			// If the tracker is running ok then we have actively tracked 6DoF data
			relation.relation_flags |=
			    xrt_space_relation_flags::XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT |
			    xrt_space_relation_flags::XRT_SPACE_RELATION_POSITION_TRACKED_BIT |
			    xrt_space_relation_flags::XRT_SPACE_RELATION_ANGULAR_VELOCITY_VALID_BIT |
			    xrt_space_relation_flags::XRT_SPACE_RELATION_LINEAR_VELOCITY_VALID_BIT;
			break;
		case vr::TrackingResult_Fallback_RotationOnly:
		case vr::TrackingResult_Running_OutOfRange:
			// If the tracking is degraded we should still be able to assume that we still have tracked 3DoF
			// data
			relation.relation_flags |=
			    xrt_space_relation_flags::XRT_SPACE_RELATION_ORIENTATION_TRACKED_BIT |
			    xrt_space_relation_flags::XRT_SPACE_RELATION_ANGULAR_VELOCITY_VALID_BIT;
			break;
		default: break;
		}
	}

	// The driver still outputs good pose data regardless of the pose results above
	relation.pose = copy_pose(newPose.qRotation, newPose.vecPosition);
	relation.linear_velocity = copy_vec3(newPose.vecVelocity);
	relation.angular_velocity = copy_vec3(newPose.vecAngularVelocity);

	// local transform (head to driver offset)
	const xrt_pose local = copy_pose(newPose.qDriverFromHeadRotation, newPose.vecDriverFromHeadTranslation);

	// IMU linear velocity contribution due to rotation around driver origin (tangential velocity)
	xrt_vec3 tangential_velocity;
	math_vec3_cross(&relation.angular_velocity, &local.position, &tangential_velocity);
	math_quat_rotate_vec3(&relation.pose.orientation, &tangential_velocity, &tangential_velocity);
	math_vec3_accum(&tangential_velocity, &relation.linear_velocity);

	// apply local transform
	math_quat_rotate_vec3(&relation.pose.orientation, &relation.angular_velocity, &relation.angular_velocity);
	math_pose_transform(&relation.pose, &local, &relation.pose);

	// apply world transform
	const xrt_pose world = copy_pose(newPose.qWorldFromDriverRotation, newPose.vecWorldFromDriverTranslation);
	math_pose_transform(&world, &relation.pose, &relation.pose);
	math_quat_rotate_vec3(&world.orientation, &relation.linear_velocity, &relation.linear_velocity);
	math_quat_rotate_vec3(&world.orientation, &relation.angular_velocity, &relation.angular_velocity);

	// apply chaperone transform
	math_pose_transform(&chaperone, &relation.pose, &relation.pose);
	math_quat_rotate_vec3(&chaperone.orientation, &relation.linear_velocity, &relation.linear_velocity);
	math_quat_rotate_vec3(&chaperone.orientation, &relation.angular_velocity, &relation.angular_velocity);

	const int64_t ts = os_monotonic_get_ns() + time_s_to_ns(newPose.poseTimeOffset);

	m_relation_history_push(relation_hist, &relation, ts);
}

vr::ETrackedPropertyError
Device::handle_properties(const vr::PropertyWrite_t *batch, uint32_t count)
{
	for (uint32_t i = 0; i < count; ++i) {
		vr::ETrackedPropertyError err = handle_property_write(batch[i]);
		if (err != vr::ETrackedPropertyError::TrackedProp_Success) {
			return err;
		}
	}
	return vr::ETrackedPropertyError::TrackedProp_Success;
}

vr::ETrackedPropertyError
Device::handle_read_properties(vr::PropertyRead_t *batch, uint32_t count)
{
	for (uint32_t i = 0; i < count; ++i) {
		vr::ETrackedPropertyError err = handle_generic_property_read(batch[i]);
		if (err != vr::ETrackedPropertyError::TrackedProp_Success) {
			return err;
		}
	}
	return vr::ETrackedPropertyError::TrackedProp_Success;
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

vr::ETrackedPropertyError
Device::handle_generic_property_write(const vr::PropertyWrite_t &prop)
{
	switch (prop.writeType) {
	case vr::EPropertyWriteType::PropertyWrite_Set:
		if (properties.count(prop.prop) > 0) {
			Property &p = properties.at(prop.prop);
			if (p.tag != prop.unTag) {
				return vr::ETrackedPropertyError::TrackedProp_WrongDataType;
			}
			p.buffer.resize(prop.unBufferSize);
			std::memcpy(p.buffer.data(), prop.pvBuffer, prop.unBufferSize);
			return vr::ETrackedPropertyError::TrackedProp_Success;
		} else {
			properties.emplace(std::piecewise_construct, std::forward_as_tuple(prop.prop),
			                   std::forward_as_tuple(prop.unTag, prop.pvBuffer, prop.unBufferSize));
		}
		break;
	case vr::EPropertyWriteType::PropertyWrite_Erase: properties.erase(prop.prop); break;
	case vr::EPropertyWriteType::PropertyWrite_SetError:
		DEV_DEBUG("Property write type SetError not supported! (property %d)", prop.prop);
		break;
	}
	return vr::ETrackedPropertyError::TrackedProp_Success;
}

vr::ETrackedPropertyError
Device::handle_generic_property_read(vr::PropertyRead_t &prop)
{
	if (properties.count(prop.prop) == 0) {
		// not verified if this is the correct error
		return vr::ETrackedPropertyError::TrackedProp_UnknownProperty;
	}
	Property &p = properties.at(prop.prop);
	prop.unTag = p.tag;
	prop.unRequiredBufferSize = p.buffer.size();
	if (prop.pvBuffer == nullptr || prop.unBufferSize < p.buffer.size()) {
		prop.eError = vr::ETrackedPropertyError::TrackedProp_BufferTooSmall;
		return prop.eError;
	}
	std::memcpy(prop.pvBuffer, p.buffer.data(), p.buffer.size());
	prop.eError = vr::ETrackedPropertyError::TrackedProp_Success;
	return prop.eError;
}

vr::ETrackedPropertyError
Device::handle_property_write(const vr::PropertyWrite_t &prop)
{
	switch (prop.prop) {
	case vr::Prop_ManufacturerName_String: {
		this->manufacturer = std::string(static_cast<char *>(prop.pvBuffer), prop.unBufferSize);
		if (!this->model.empty()) {
			std::snprintf(this->str, std::size(this->str), "%s %s", this->manufacturer.c_str(),
			              this->model.c_str());
		}
		break;
	}
	case vr::Prop_ModelNumber_String: {
		this->model = std::string(static_cast<char *>(prop.pvBuffer), prop.unBufferSize);
		if (!this->manufacturer.empty()) {
			std::snprintf(this->str, std::size(this->str), "%s %s", this->manufacturer.c_str(),
			              this->model.c_str());
		}
		break;
	}
	default: {
		DEV_DEBUG("Unhandled property: %i", prop.prop);
		break;
	}
	}
	return handle_generic_property_write(prop);
}

void
HmdDevice::set_hmd_class(const InputClass *hmd_class)
{
	if (this->input_class == hmd_class) {
		return;
	}

	// Input components hold pointers into inputs_vec, so the inputs can't be rebuilt once any exist.
	if (!this->handles.empty()) {
		DEV_WARN("HMD input components already created, keeping current input class");
		return;
	}

	inputs_vec.clear();
	inputs_map.clear();

	this->name = hmd_class->name;
	set_input_class(hmd_class);
}

bool
HmdDevice::init_vive_pro_2(struct xrt_prober *xp)
{
	xrt_result_t xret;
	int ret = 0;

	struct xrt_prober_device **devices = nullptr;
	size_t device_count;

	xret = xrt_prober_lock_list(xp, &devices, &device_count);
	if (xret != XRT_SUCCESS) {
		DEV_ERR("Failed to lock prober device list");
		return false;
	}

	for (size_t i = 0; i < device_count; i++) {
		struct xrt_prober_device *dev = devices[i];

		if (dev->vendor_id == VP2_VID && dev->product_id == VP2_PID) {
			DEV_INFO("Found Vive Pro 2 HID device");
			struct os_hid_device *hid_dev = nullptr;
			ret = xrt_prober_open_hid_interface(xp, dev, 0, &hid_dev);
			if (ret != 0) {
				DEV_ERR("Failed to open Vive Pro 2 HID interface");
				break;
			}

			ret = vp2_hid_open(hid_dev, &this->vp2.hid);
			if (ret != 0) {
				DEV_ERR("Failed to open Vive Pro 2 HID device");
				break;
			}

			break;
		}
	}

	xrt_prober_unlock_list(xp, &devices);

	if (this->vp2.hid == nullptr) {
		if (ret == 0) {
			DEV_ERR("Vive Pro 2 HID device (%04x:%04x) not found", VP2_VID, VP2_PID);
		}
		return false;
	}

	int width, height;
	vp2_resolution_get_extents(vp2_get_resolution(this->vp2.hid), &width, &height);

	for (int i = 0; i < 2; i++) {
		this->hmd_parts->base.views[i].display.w_pixels = width / 2;
		this->hmd_parts->base.views[i].display.h_pixels = height;

		this->hmd_parts->base.views[i].viewport.w_pixels = width / 2;
		this->hmd_parts->base.views[i].viewport.h_pixels = height;
	}

	this->hmd_parts->base.views[1].viewport.x_pixels = width / 2;

	this->hmd_parts->base.screens[0].w_pixels = width;
	this->hmd_parts->base.screens[0].h_pixels = height;

	vp2_config *config = vp2_get_config(this->vp2.hid);

	htc_config_get_fov(&config->base, 0, &this->hmd_parts->base.distortion.fov[0]);
	htc_config_get_fov(&config->base, 1, &this->hmd_parts->base.distortion.fov[1]);

	this->supported.brightness_control = true;
	this->supported.presence = true;

	return ret == 0;
}

vr::ETrackedPropertyError
HmdDevice::handle_property_write(const vr::PropertyWrite_t &prop)
{
	switch (prop.prop) {
	case vr::Prop_ModelNumber_String: {
		std::string model_number(static_cast<char *>(prop.pvBuffer), prop.unBufferSize);

		this->variant = vive_determine_variant(model_number.c_str());

		auto hmd_class = hmd_classes.find(this->variant);
		set_hmd_class(hmd_class != hmd_classes.end() ? &hmd_class->second : &generic_hmd_class);

		return Device::handle_property_write(prop);
	}
	case vr::Prop_DisplayFrequency_Float: {
		assert(prop.unBufferSize == sizeof(float));
		float freq = *static_cast<float *>(prop.pvBuffer);
		int64_t interval_ns = (1.f / freq) * 1e9f;
		set_nominal_frame_interval(interval_ns);
		break;
	}
	case vr::Prop_UserIpdMeters_Float: {
		if (*static_cast<float *>(prop.pvBuffer) != 0) {
			lh_ipd = *static_cast<float *>(prop.pvBuffer);
		}
		break;
	}
	case vr::Prop_SecondsFromVsyncToPhotons_Float: {
		vsync_to_photon_ns = *static_cast<float *>(prop.pvBuffer) * 1e9f;
		break;
	}
	case vr::Prop_DeviceProvidesBatteryStatus_Bool: {
		float supported = *static_cast<bool *>(prop.pvBuffer);
		this->provides_battery_status = supported;
		DEV_DEBUG("Has battery status: HMD: %s", supported ? "true" : "false");
		break;
	}
	case vr::Prop_DeviceIsCharging_Bool: {
		float charging = *static_cast<bool *>(prop.pvBuffer);
		this->charging = charging;
		DEV_DEBUG("Charging: HMD: %s", charging ? "true" : "false");
		break;
	}
	case vr::Prop_DeviceBatteryPercentage_Float: {
		float bat = *static_cast<float *>(prop.pvBuffer);
		this->charge = bat;
		DEV_DEBUG("Battery: HMD: %f", bat);
		break;
	}
	case vr::Prop_ContainsProximitySensor_Bool: {
		// Ignored on VP2
		if (this->variant == VIVE_VARIANT_PRO2) {
			break;
		}

		this->supported.presence = *static_cast<bool *>(prop.pvBuffer);
		break;
	}
	case vr::Prop_DisplaySupportsAnalogGain_Bool: {
		this->supported.brightness_control = *static_cast<bool *>(prop.pvBuffer);
		break;
	}
	case vr::Prop_DisplayMinAnalogGain_Float: {
		this->analog_gain_range.min = *static_cast<float *>(prop.pvBuffer);
		break;
	}
	case vr::Prop_DisplayMaxAnalogGain_Float: {
		this->analog_gain_range.max = *static_cast<float *>(prop.pvBuffer);
		break;
	}
	default: {
		return Device::handle_property_write(prop);
	}
	}
	return handle_generic_property_write(prop);
}

vr::ETrackedPropertyError
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
			this->name = input_class->second.name;
			set_input_class(&input_class->second);
		}
		break;
	}
	case vr::Prop_ModelNumber_String: {
		using namespace std::literals::string_view_literals;
		vr::PropertyWrite_t fixedProp = prop;
		const std::string_view name = {static_cast<char *>(prop.pvBuffer), prop.unBufferSize};
		if (name == "SlimeVR Virtual Tracker\0"sv) {
			static const InputClass input_class = {
			    XRT_DEVICE_VIVE_TRACKER, {XRT_INPUT_GENERIC_TRACKER_POSE}, {}};
			this->name = input_class.name;
			set_input_class(&input_class);
			this->manufacturer = name.substr(0, name.find_first_of(' '));
			fixedProp.pvBuffer = (char *)fixedProp.pvBuffer + this->manufacturer.size() +
			                     (this->manufacturer.size() != name.size());
			fixedProp.unBufferSize = name.end() - (char *)fixedProp.pvBuffer;
		}
		return Device::handle_property_write(fixedProp);
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
			this->skeleton_hand = XRT_HAND_RIGHT;
			break;
		}
		case vr::TrackedControllerRole_LeftHand: {
			this->device_type = XRT_DEVICE_TYPE_LEFT_HAND_CONTROLLER;
			this->skeleton_hand = XRT_HAND_LEFT;
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
	case vr::Prop_DeviceProvidesBatteryStatus_Bool: {
		float supported = *static_cast<bool *>(prop.pvBuffer);
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
			break;
		}
		}
		this->provides_battery_status = supported;
		DEV_DEBUG("Has battery status: %s: %s", name, supported ? "true" : "false");
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
		this->charging = charging;
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
		this->charge = bat;
		DEV_DEBUG("Battery: %s: %f", name, bat);
		break;
	}
	default: {
		return Device::handle_property_write(prop);
	}
	}
	return handle_generic_property_write(prop);
}
