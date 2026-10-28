// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of a dynamic OpenVR devices array using the xrt_system_devices interface.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr
 */

#include "xrt/xrt_tracking.h"

#include "util/u_time.h"

#include "common/openvr_logger.hpp"
#include "common/openvr_math.hpp"
#include "common/openvr_error.hpp"

#include "openvr_devices.hpp"

#include <new>


namespace xrt::state_trackers::openvr {

/*
 *
 * Device implementations
 *
 */

Device::Device(openvr_logger &logger, Devices &devices, xrt_device *xdev) : xdev(xdev)
{
	if (xdev != nullptr) {
		std::optional<xrt_input_name> input_name = std::nullopt;

		switch (xdev->device_type) {
		case XRT_DEVICE_TYPE_HMD:
			tracked_class = vr::TrackedDeviceClass_HMD;
			input_name = XRT_INPUT_GENERIC_HEAD_POSE;
			break;
		case XRT_DEVICE_TYPE_GAMEPAD:
		case XRT_DEVICE_TYPE_LEFT_HAND_CONTROLLER:
		case XRT_DEVICE_TYPE_RIGHT_HAND_CONTROLLER:
		case XRT_DEVICE_TYPE_ANY_HAND_CONTROLLER: tracked_class = vr::TrackedDeviceClass_Controller; break;
		case XRT_DEVICE_TYPE_GENERIC_TRACKER: tracked_class = vr::TrackedDeviceClass_GenericTracker; break;
		default: tracked_class = vr::TrackedDeviceClass_Invalid; break;
		}

		if (input_name.has_value()) {
			xrt_result_t xret =
			    xrt_space_overseer_create_pose_space(devices.xso, xdev, input_name.value(), &pose_space);
			if (xret != XRT_SUCCESS) {
				OPENVR_LOG_ERROR_XRET(logger, "Failed to create pose space for device %s", xret,
				                      xdev->str);
			}
		}
	}
}

/*
 *
 * Devices implementations
 *
 */

void
Devices::addDevice(openvr_logger &logger, vr::TrackedDeviceIndex_t index, xrt_device *xdev)
{
	if (index >= vr::k_unMaxTrackedDeviceCount) {
		OPENVR_LOG_ERROR(logger, "addDevice: device index %u is out of range", index);
		return;
	}

	Device &device = this->device_array[index];
	if (device.xdev != nullptr) {
		OPENVR_LOG_ERROR(logger, "addDevice: device index %u is already occupied", index);
		return;
	}

	new (&device) Device(logger, *this, xdev);

	OPENVR_LOG_INFO(logger, "Added device %s at index %u with tracked class %d", xdev->str, index,
	                device.tracked_class);

	this->events->trackedDeviceActivated(index);
	this->events->trackedDeviceUserInteractionStarted(index);
}

Devices::Devices(openvr_logger &logger,
                 xrt_space_overseer *xso,
                 xrt_system_devices *xsysd,
                 std::shared_ptr<Events> &events)
    : xso(xso), xsysd(xsysd), events(events)
{
	// Unconditionally initialize the head device slot to the head static role.
	this->addDevice(logger, vr::k_unTrackedDeviceIndex_Hmd, xsysd->static_roles.head);

	xrt_space *seated = xso->semantic.local;
	if (seated == nullptr) {
		seated = xso->semantic.root;
	}

	xrt_space *standing = xso->semantic.stage;
	if (standing == nullptr) {
		standing = xso->semantic.local_floor;
	}
	if (standing == nullptr) {
		standing = xso->semantic.root;
	}

	xrt_space *raw = xso->semantic.root;

	this->space_mappings[vr::ETrackingUniverseOrigin::TrackingUniverseSeated] = seated;
	this->space_mappings[vr::ETrackingUniverseOrigin::TrackingUniverseStanding] = standing;
	this->space_mappings[vr::ETrackingUniverseOrigin::TrackingUniverseRawAndUncalibrated] = raw;
}

std::optional<Device *>
Devices::getDevice(vr::TrackedDeviceIndex_t index)
{
	if (index >= vr::k_unMaxTrackedDeviceCount) {
		return std::nullopt;
	}

	Device &device = device_array[index];
	if (device.xdev == nullptr || device.tracked_class == vr::TrackedDeviceClass_Invalid) {
		return std::nullopt;
	}

	return &device;
}

void
Devices::trackDevices(openvr_logger &logger,
                      timepoint_ns when_ns,
                      vr::ETrackingUniverseOrigin universe,
                      vr::TrackedDevicePose_t *poses,
                      uint32_t pose_count)
{
	xrt_result_t xret;

	xrt_space *space = this->space_mappings[universe];

	xrt_pose identity_pose = XRT_POSE_IDENTITY;

	vr::TrackedDevicePose_t identity_tracked_pose = {
	    .mDeviceToAbsoluteTracking = {},
	    .vVelocity = {0.0f, 0.0f, 0.0f},
	    .vAngularVelocity = {0.0f, 0.0f, 0.0f},
	    .eTrackingResult = vr::TrackingResult_Uninitialized,
	    .bPoseIsValid = false,
	    .bDeviceIsConnected = false,
	};

	// For each device, if it's connected, update the corresponding device in the array with the new pose.
	for (vr::TrackedDeviceIndex_t i = 0; i < pose_count && i < vr::k_unMaxTrackedDeviceCount; ++i) {
		Device &device = this->device_array[i];

		xrt_device *xdev = device.xdev;
		if (xdev == nullptr || device.tracked_class == vr::TrackedDeviceClass_Invalid ||
		    device.pose_space == nullptr) {
			// Can't track device
			poses[i] = identity_tracked_pose;
			OPENVR_LOG_TRACE(logger, "trackDevices: device %u is not valid for tracking, skipping", i);
			continue;
		}

		xrt_space_relation relation;
		xret = xrt_space_overseer_locate_space(this->xso, space, &identity_pose, when_ns, device.pose_space,
		                                       &identity_pose, &relation);
		if (xret != XRT_SUCCESS) {
			OPENVR_LOG_ERROR(logger, "Failed to locate device %u in space for tracking: error code %d", i,
			                 xret);
			poses[i] = identity_tracked_pose;
			continue;
		}

		vr::ETrackingResult tracking_result = vr::TrackingResult_Running_OutOfRange;
		if (relation.relation_flags & XRT_SPACE_RELATION_POSITION_VALID_BIT &&
		    relation.relation_flags & XRT_SPACE_RELATION_ORIENTATION_VALID_BIT) {
			tracking_result = vr::TrackingResult_Running_OK;
		} else if (relation.relation_flags & XRT_SPACE_RELATION_ORIENTATION_VALID_BIT) {
			tracking_result = vr::TrackingResult_Fallback_RotationOnly;
		}

		poses[i] = {
		    .mDeviceToAbsoluteTracking = {},
		    .vVelocity = {relation.linear_velocity.x, relation.linear_velocity.y, relation.linear_velocity.z},
		    .vAngularVelocity = {relation.angular_velocity.x, relation.angular_velocity.y,
		                         relation.angular_velocity.z},
		    .eTrackingResult = tracking_result,
		    .bPoseIsValid = tracking_result != vr::TrackingResult_Running_OutOfRange,
		    .bDeviceIsConnected = true,
		};
		xrtPoseToHmdMatrix34(relation.pose, poses[i].mDeviceToAbsoluteTracking);

		OPENVR_LOG_TRACE(logger, "trackDevices: device %u tracked with result %d", i, tracking_result);
	}
}

bool
Devices::getDeviceStringProperty(openvr_logger &logger,
                                 vr::TrackedDeviceIndex_t device_index,
                                 vr::ETrackedDeviceProperty prop,
                                 std::string &str,
                                 vr::ETrackedPropertyError *pError)
{
	auto maybe_device = this->getDevice(device_index);
	if (!maybe_device.has_value()) {
		OPENVR_LOG_ERROR(logger, "getDeviceStringProperty: device index %u is out of range or invalid",
		                 device_index);
		SET_ERROR(pError, vr::ETrackedPropertyError::TrackedProp_InvalidDevice);
		return false;
	}

	auto device = maybe_device.value();
	xrt_device *xdev = device->xdev;

	switch (prop) {
	case vr::ETrackedDeviceProperty::Prop_TrackingSystemName_String:
		// @todo hardcode this based on xdev->name
		str = xdev->tracking_origin->name;
		break;
	case vr::ETrackedDeviceProperty::Prop_SerialNumber_String: str = xdev->serial; break;
	case vr::ETrackedDeviceProperty::Prop_ModelNumber_String:
		// @todo hardcode this based on xdev->name
		str = xdev->str;
		break;
	case vr::ETrackedDeviceProperty::Prop_ManufacturerName_String:
		// @todo hardcode this based on xdev->name
		str = "Monado";
		break;
	case vr::ETrackedDeviceProperty::Prop_RenderModelName_String:
		// @todo hardcode this based on xdev->name
		str = "TODO";
		break;
	default: SET_ERROR(pError, vr::ETrackedPropertyError::TrackedProp_UnknownProperty); return false;
	}

	return true;
}

bool
Devices::getTrackedDeviceActivityLevel(openvr_logger &logger,
                                       vr::TrackedDeviceIndex_t device_index,
                                       vr::EDeviceActivityLevel &activity_level)
{
	auto maybe_device = this->getDevice(device_index);
	if (!maybe_device.has_value()) {
		OPENVR_LOG_ERROR(logger, "getTrackedDeviceActivityLevel: device index %u is out of range or invalid",
		                 device_index);
		return false;
	}

	// @todo actually implement this and don't set every device to active user interaction
	activity_level = vr::EDeviceActivityLevel::k_EDeviceActivityLevel_UserInteraction;

	return true;
}

}; // namespace xrt::state_trackers::openvr
