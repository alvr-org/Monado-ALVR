// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of a dynamic OpenVR devices array using the xrt_system_devices interface.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr
 */

#pragma once

#include "xrt/xrt_device.h"
#include "xrt/xrt_system.h"
#include "xrt/xrt_space.h"

#include "util/u_time.h"

#include "common/openvr_logger.hpp"

#include "openvr_events.hpp"

#include "openvr_interfaces_unified.h"

#include <array>
#include <optional>
#include <memory>


namespace xrt::state_trackers::openvr {

struct Devices;

struct Device
{
public: // Fields
	xrt_device *xdev{nullptr};
	vr::TrackedDeviceClass tracked_class{vr::TrackedDeviceClass_Invalid};

	// @todo: Select the correct pose input for the device type
	xrt_space *pose_space{nullptr};

public: // Methods
	Device() = default;
	Device(openvr_logger &logger, Devices &devices, xrt_device *xdev);

	// Delete the move constructor and move assignment operator to prevent accidental moves which would cause the
	// pose_space reference counting to be messed up.
	Device(Device &&) = delete;
	Device &
	operator=(Device &&) = delete;

	~Device()
	{
		xrt_space_reference(&this->pose_space, nullptr);
	}
};

struct Devices
{
public: // Fields
	xrt_space_overseer *xso{nullptr};
	xrt_system_devices *xsysd{nullptr};

	std::shared_ptr<Events> events{};

	std::array<Device, vr::k_unMaxTrackedDeviceCount> device_array{};

	/*!
	 * Mapping of OpenVR tracking universe origins to `xrt_space`s, populated during session creation.
	 * All spaces will fall back to `root` if the specific space best matching a universe is not available.
	 */
	std::array<xrt_space *, vr::ETrackingUniverseOrigin::TrackingUniverseRawAndUncalibrated + 1> space_mappings{};

private: // Methods
	void
	addDevice(openvr_logger &logger, vr::TrackedDeviceIndex_t index, xrt_device *xdev);

public: // Methods
	Devices() = default;
	Devices(openvr_logger &logger,
	        xrt_space_overseer *xso,
	        xrt_system_devices *xsysd,
	        std::shared_ptr<Events> &events);

	/*!
	 * Retrieves a device by its tracked device index.
	 *
	 * @param index The index of the tracked device.
	 * @return An optional containing the device if it exists, or std::nullopt if it does not.
	 */
	std::optional<Device *>
	getDevice(vr::TrackedDeviceIndex_t index);

	void
	trackDevices(openvr_logger &logger,
	             timepoint_ns when_ns,
	             vr::ETrackingUniverseOrigin universe,
	             vr::TrackedDevicePose_t *poses,
	             uint32_t pose_count);

	bool
	getDeviceStringProperty(openvr_logger &logger,
	                        vr::TrackedDeviceIndex_t device_index,
	                        vr::ETrackedDeviceProperty prop,
	                        std::string &out_str,
	                        vr::ETrackedPropertyError *pError);

	bool
	getTrackedDeviceActivityLevel(openvr_logger &logger,
	                              vr::TrackedDeviceIndex_t device_index,
	                              vr::EDeviceActivityLevel &activity_level);
};

}; // namespace xrt::state_trackers::openvr
