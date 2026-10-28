// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Implementation of the data recorder logic for the constellation tracker.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup tracking
 */

#pragma once

#include "t_constellation_tracker_internal.hpp"

#include <mutex>


namespace xrt::tracking::constellation {

struct DataSerializer
{
private: // Fields
	std::fstream file;

public: // Methods
	DataSerializer(std::string file, bool write);

	~DataSerializer();

	void
	write(uint8_t value);
	void
	read(uint8_t &value);

	void
	write(uint32_t value);
	void
	read(uint32_t &value);

	void
	write(float value);
	void
	read(float &value);

	void
	write(uint64_t value);
	void
	read(uint64_t &value);

	void
	write(double value);
	void
	read(double &value);

	void
	write(const xrt_pose &value);
	void
	read(xrt_pose &value);

	void
	write(const FoundDevicePose &value);
	void
	read(FoundDevicePose &value);

	void
	write(const DeviceState &value);
	void
	read(DeviceState &value);

	void
	write(const t_blob &value);
	void
	read(t_blob &value);

	void
	write(const CameraSample &value);
	void
	read(CameraSample &value);

	void
	write(const t_camera_calibration &value);
	void
	read(t_camera_calibration &value);

	void
	write(t_constellation_tracker_led &value);
	void
	read(t_constellation_tracker_led &value);

	void
	write(const t_constellation_tracker_led_model &value);
	void
	read(std::vector<t_constellation_tracker_led> &led_storage, t_constellation_tracker_led_model &value);

	template <typename T>
	void
	write(const std::optional<T> &opt_value)
	{
		this->write(static_cast<uint8_t>(opt_value.has_value() ? 1 : 0));
		if (opt_value.has_value()) {
			this->write(opt_value.value());
		}
	}

	template <typename T>
	void
	read(std::optional<T> &opt_value)
	{
		uint8_t has_value;
		this->read(has_value);
		if (has_value) {
			T value;
			this->read(value);
			opt_value = value;
		} else {
			opt_value = std::nullopt;
		}
	}

	void
	flush();
};

struct DataRecorder
{
private: // Fields
	DataSerializer serializer;

	/*!
	 * Serializes writes to @ref serializer. Every camera has a fast processing thread of its own and
	 * records from it, so without this two cameras' packets interleave and the file is unreadable
	 * from the first race onwards.
	 */
	std::mutex lock;

public: // Methods
	DataRecorder(ConstellationTracker *tracker, std::string out_file);

	~DataRecorder() = default;

	void
	recordSample(const CameraSample &sample);

	void
	recordDeviceInfo(const Device &device);
};

struct DatasetMosaic
{
	std::vector<t_camera_calibration> camera_calibrations;
};

struct DatasetDevice
{
	t_constellation_device_id_t id;

	std::vector<t_constellation_tracker_led> leds;
	t_constellation_tracker_led_model led_model;
};

struct DatasetReader
{
private: // Fields
	DataSerializer serializer;

public: // Fields
	std::vector<DatasetMosaic> mosaics;
	std::vector<DatasetDevice> devices;

	std::vector<CameraSample> samples;

public: // Methods
	DatasetReader(std::string filename);
};

}; // namespace xrt::tracking::constellation
