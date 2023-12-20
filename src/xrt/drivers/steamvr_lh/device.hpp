// Copyright 2023, Shawn Wallace
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief SteamVR driver device header - inherits xrt_device.
 * @author Shawn Wallace <yungwallace@live.com>
 * @ingroup drv_steamvr_lh
 */

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#include <condition_variable>
#include <mutex>

#include "interfaces/context.hpp"
#include "math/m_relation_history.h"
#include "xrt/xrt_device.h"
#include "openvr_driver.h"

class Context;
struct InputClass;

struct DeviceBuilder
{
	std::shared_ptr<Context> ctx;
	vr::ITrackedDeviceServerDriver *driver;
	const char *serial;
	const std::string &steam_install;

	// no copies!
	DeviceBuilder(const DeviceBuilder &) = delete;
	DeviceBuilder
	operator=(const DeviceBuilder &) = delete;
};

class Device : public xrt_device
{

public:
	m_relation_history *relation_hist;

	virtual ~Device();

	xrt_input *
	get_input_from_name(std::string_view name);

	void
	update_inputs();

	void
	update_pose(const vr::DriverPose_t &newPose) const;

	//! Helper to use the @ref m_relation_history member.
	void
	get_pose(uint64_t at_timestamp_ns, xrt_space_relation *out_relation);

	void
	handle_properties(const vr::PropertyWrite_t *batch, uint32_t count);

	//! Maps to @ref xrt_device::get_tracked_pose.
	virtual void
	get_tracked_pose(xrt_input_name name, uint64_t at_timestamp_ns, xrt_space_relation *out_relation) = 0;

protected:
	Device(const DeviceBuilder &builder);
	std::shared_ptr<Context> ctx;
	vr::PropertyContainerHandle_t container_handle{0};
	std::unordered_map<std::string_view, xrt_input *> inputs_map;
	std::vector<xrt_input> inputs_vec;
	inline static xrt_pose chaperone = XRT_POSE_IDENTITY;
	const InputClass *input_class;

	float vsync_to_photon_ns{0.f};

	virtual void
	handle_property_write(const vr::PropertyWrite_t &prop) = 0;

	void
	set_input_class(const InputClass *input_class);

private:
	vr::ITrackedDeviceServerDriver *driver;
	std::vector<xrt_binding_profile> binding_profiles_vec;
	uint64_t current_frame{0};

	std::mutex frame_mutex;

	void
	init_chaperone(const std::string &steam_install);
};

class HmdDevice : public Device
{
public:
	xrt_pose eye[2];
	float ipd{0.063}; // meters
	struct Parts
	{
		xrt_hmd_parts base;
		vr::IVRDisplayComponent *display;
	};

	HmdDevice(const DeviceBuilder &builder);

	void
	get_tracked_pose(xrt_input_name name, uint64_t at_timestamp_ns, xrt_space_relation *out_relation) override;

	void
	SetDisplayEyeToHead(uint32_t unWhichDevice,
	                    const vr::HmdMatrix34_t &eyeToHeadLeft,
	                    const vr::HmdMatrix34_t &eyeToHeadRight);

	void
	get_view_poses(const xrt_vec3 *default_eye_relation,
	               uint64_t at_timestamp_ns,
	               uint32_t view_count,
	               xrt_space_relation *out_head_relation,
	               xrt_fov *out_fovs,
	               xrt_pose *out_poses);

	bool
	compute_distortion(uint32_t view, float u, float v, xrt_uv_triplet *out_result);

	void
	set_hmd_parts(std::unique_ptr<Parts> parts);

	inline float
	get_ipd() const
	{
		return ipd;
	}

private:
	std::unique_ptr<Parts> hmd_parts{nullptr};

	void
	handle_property_write(const vr::PropertyWrite_t &prop) override;

	void
	set_nominal_frame_interval(uint64_t interval_ns);

	std::condition_variable hmd_parts_cv;
	std::mutex hmd_parts_mut;
};

class ControllerDevice : public Device
{
protected:
	void
	set_input_class(const InputClass *input_class);

public:
	ControllerDevice(vr::PropertyContainerHandle_t container_handle, const DeviceBuilder &builder);

	void
	set_output(xrt_output_name name, const xrt_output_value *value);

	void
	set_haptic_handle(vr::VRInputComponentHandle_t handle);

	void
	get_tracked_pose(xrt_input_name name, uint64_t at_timestamp_ns, xrt_space_relation *out_relation) override;

	IndexFingerInput *
	get_finger_from_name(std::string_view name);

	void
	get_hand_tracking(enum xrt_input_name name,
	                  uint64_t desired_timestamp_ns,
	                  struct xrt_hand_joint_set *out_value,
	                  uint64_t *out_timestamp_ns);

	xrt_hand
	get_xrt_hand();

	void
	update_hand_tracking(struct xrt_hand_joint_set *out);

private:
	vr::VRInputComponentHandle_t haptic_handle{0};
	std::unique_ptr<xrt_output> output{nullptr};
	bool has_index_hand_tracking{false};
	std::vector<IndexFingerInput> finger_inputs_vec;
	std::unordered_map<std::string_view, IndexFingerInput *> finger_inputs_map;
	uint64_t hand_tracking_timestamp;

	void
	set_hand_tracking_hand(xrt_input_name name);

	void
	handle_property_write(const vr::PropertyWrite_t &prop) override;
};
