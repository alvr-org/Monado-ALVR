// Copyright 2023, Shawn Wallace
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief SteamVR driver device header - inherits xrt_device.
 * @author Shawn Wallace <yungwallace@live.com>
 * @ingroup drv_steamvr_lh
 */

#include "interfaces/context.hpp"

#include "math/m_relation_history.h"

#include "util/u_template_historybuf.hpp"
#include "xrt/xrt_device.h"

#include "vive/vive_common.h"

#include "vp2/vp2_hid.h"

#include "openvr_driver.h"

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <span>
#include <array>
#include <optional>

#include <condition_variable>
#include <mutex>

class Context;
struct InputClass;

struct DeviceBuilder
{
	using ContextPtr = std::shared_ptr<Context>;
	ContextPtr ctx;
	vr::ITrackedDeviceServerDriver *driver;
	const char *serial;
	const std::string &steam_install;

	DeviceBuilder(const ContextPtr &p_ctx,
	              vr::ITrackedDeviceServerDriver *p_driver,
	              const char *p_serial,
	              const std::string &p_stream_install)
	    : ctx{p_ctx}, driver{p_driver}, serial{p_serial}, steam_install{p_stream_install}
	{}

	// no copies!
	DeviceBuilder(const DeviceBuilder &) = delete;
	DeviceBuilder
	operator=(const DeviceBuilder &) = delete;
};

class Property
{
public:
	Property(vr::PropertyTypeTag_t tag, void *buffer, uint32_t bufferSize);

	vr::PropertyTypeTag_t tag;
	std::vector<uint8_t> buffer;
};

class Device : public xrt_device
{
public:
	m_relation_history *relation_hist;

	//! The component handles that are attached to this device.
	std::vector<vr::VRInputComponentHandle_t> handles;

	virtual ~Device();

	xrt_input *
	get_input_from_name(std::string_view name);

	void
	update_pose(const vr::DriverPose_t &newPose) const;

	//! Helper to use the @ref m_relation_history member.
	void
	get_pose(uint64_t at_timestamp_ns, xrt_space_relation *out_relation);

	vr::ETrackedPropertyError
	handle_properties(const vr::PropertyWrite_t *batch, uint32_t count);

	vr::ETrackedPropertyError
	handle_read_properties(vr::PropertyRead_t *batch, uint32_t count);

	//! Maps to @ref xrt_device::get_tracked_pose.
	virtual xrt_result_t
	get_tracked_pose(xrt_input_name name, uint64_t at_timestamp_ns, xrt_space_relation *out_relation) = 0;

	xrt_result_t
	get_battery_status(bool *out_present, bool *out_charging, float *out_charge);

	//! Call with `devices_mut` held.
	inline vr::VRInputComponentHandle_t
	new_input_handle_locked()
	{
		vr::VRInputComponentHandle_t h = ++this->ctx->input.next_handle;
		this->handles.push_back(h);
		return h;
	}

protected:
	Device(const DeviceBuilder &builder);
	std::shared_ptr<Context> ctx;
	vr::PropertyContainerHandle_t container_handle{0};
	std::unordered_map<vr::ETrackedDeviceProperty, Property> properties;
	std::unordered_map<std::string_view, xrt_input *> inputs_map;
	std::vector<xrt_input> inputs_vec;
	inline static xrt_pose chaperone = XRT_POSE_IDENTITY;
	const InputClass *input_class{nullptr};
	std::string manufacturer;
	std::string model;
	float vsync_to_photon_ns{0.f};
	bool provides_battery_status{false};
	bool charging{false};
	float charge{1.0F};

	vr::ETrackedPropertyError
	handle_generic_property_write(const vr::PropertyWrite_t &prop);
	vr::ETrackedPropertyError
	handle_generic_property_read(vr::PropertyRead_t &prop);

	virtual vr::ETrackedPropertyError
	handle_property_write(const vr::PropertyWrite_t &prop);

	void
	set_input_class(const InputClass *input_class);

private:
	vr::ITrackedDeviceServerDriver *driver;

	void
	init_chaperone(const std::string &steam_install);
};

struct VivePro2Data
{
	vp2_hid *hid{nullptr};

	~VivePro2Data()
	{
		if (hid != nullptr) {
			vp2_hid_destroy(hid);
			hid = nullptr;
		}
	}
};

class HmdDevice : public Device
{
public:
	VIVE_VARIANT variant{VIVE_UNKNOWN};
	xrt_pose eye[2] = {XRT_POSE_IDENTITY, XRT_POSE_IDENTITY};
	float lh_ipd{0.063}; // meters
	struct Parts
	{
		xrt_hmd_parts base;
		vr::IVRDisplayComponent *display;
	};

	struct AnalogGainRange
	{
		float min{0.1f};
		float max{1.0f};
	};

	// @todo Remove when clang-format is updated in CI
	// clang-format off
	struct VivePro2Data vp2{};
	// clang-format on

	HmdDevice(const DeviceBuilder &builder);

	float
	getIpdM();

	xrt_result_t
	update_inputs();

	xrt_result_t
	get_tracked_pose(xrt_input_name name, uint64_t at_timestamp_ns, xrt_space_relation *out_relation) override;

	void
	SetDisplayEyeToHead(uint32_t unWhichDevice,
	                    const vr::HmdMatrix34_t &eyeToHeadLeft,
	                    const vr::HmdMatrix34_t &eyeToHeadRight);

	xrt_result_t
	get_view_poses(const xrt_vec3 *default_eye_relation,
	               uint64_t at_timestamp_ns,
	               xrt_view_type view_type,
	               uint32_t view_count,
	               xrt_space_relation *out_head_relation,
	               xrt_fov *out_fovs,
	               xrt_pose *out_poses);

	xrt_result_t
	compute_distortion(uint32_t view, float u, float v, xrt_uv_triplet *out_result);

	void
	set_hmd_parts(std::unique_ptr<Parts> parts);

	xrt_result_t
	get_brightness(float *out_brightness);
	xrt_result_t
	set_brightness(float brightness, bool relative);

	xrt_result_t
	get_compositor_info(const struct xrt_device_compositor_mode *mode, struct xrt_device_compositor_info *out_info);

	bool
	init_vive_pro_2(struct xrt_prober *xp);

private:
	std::unique_ptr<Parts> hmd_parts{nullptr};

	void
	set_hmd_class(const InputClass *hmd_class);

	vr::ETrackedPropertyError
	handle_property_write(const vr::PropertyWrite_t &prop) override;

	void
	set_nominal_frame_interval(uint64_t interval_ns);

	std::condition_variable hmd_parts_cv;
	std::mutex hmd_parts_mut;
	float brightness{1.0f};
	AnalogGainRange analog_gain_range{};
};

class ControllerDevice : public Device
{
public:
	ControllerDevice(vr::PropertyContainerHandle_t container_handle, const DeviceBuilder &builder);

	xrt_result_t
	set_output(xrt_output_name name, const xrt_output_value *value);

	void
	set_haptic_handle(vr::VRInputComponentHandle_t handle);

	xrt_result_t
	get_tracked_pose(xrt_input_name name, uint64_t at_timestamp_ns, xrt_space_relation *out_relation) override;

	xrt_result_t
	get_hand_tracking(enum xrt_input_name name,
	                  int64_t desired_timestamp_ns,
	                  struct xrt_hand_joint_set *out_value,
	                  int64_t *out_timestamp_ns);

	xrt_hand
	get_xrt_hand();

	void
	update_skeleton_transforms(std::span<const vr::VRBoneTransform_t> bones);

	void
	set_skeleton(std::span<const vr::VRBoneTransform_t> bones, xrt_hand hand, bool is_simulated, const char *path);

	xrt_result_t
	notify_chirality(bool has_chirality, xrt_hand chirality);

protected:
	void
	generate_palm_pose_offset(std::span<const vr::VRBoneTransform_t> bones, xrt_hand hand);

private:
	vr::VRInputComponentHandle_t haptic_handle{0};
	size_t latest_haptic_event{0};
	std::unique_ptr<xrt_output> output{nullptr};
	bool has_hand_tracking{false};
	xrt_hand skeleton_hand = XRT_HAND_LEFT;
	std::array<std::optional<xrt_pose>, 2> palm_offsets;
	std::array<xrt_input, 2> hand_tracking_inputs{};

	struct JointsWithTimestamp
	{
		xrt_hand_joint_set joint_set;
		int64_t timestamp{0};
	};
	xrt::auxiliary::util::HistoryBuffer<JointsWithTimestamp, 5> joint_history;

	vr::ETrackedPropertyError
	handle_property_write(const vr::PropertyWrite_t &prop) override;
};
