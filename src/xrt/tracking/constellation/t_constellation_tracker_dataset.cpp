// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Implementation of the data recorder logic for the constellation tracker.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup tracking
 */

#include "xrt/xrt_byte_order.h"

#include "t_constellation_tracker_dataset.hpp"

#include <string>


namespace xrt::tracking::constellation {

#define CT_DATA_MAGIC_VALUE 0x066A4C50

enum PacketType
{
	PACKET_TYPE_CAMERA_SAMPLE = 1,
	PACKET_TYPE_DEVICE_INFO = 2,
};

namespace {

	void
	throw_if_stream_failed(std::fstream &file, const char *operation)
	{
		if (!file) {
			throw std::runtime_error(operation);
		}
	}

} // namespace

/*
 *
 * DataSerializer implementations
 *
 */

DataSerializer::DataSerializer(std::string file, bool write)
{
	this->file = std::fstream(file, std::ios::binary | (write ? std::ios::out : std::ios::in));
	if (!this->file.is_open()) {
		throw std::runtime_error("Failed to open file for data serialization.");
	}
}

DataSerializer::~DataSerializer()
{
	if (this->file.is_open()) {
		this->file.close();
	}
}

// uint8_t

void
DataSerializer::write(uint8_t value)
{
	this->file.write(reinterpret_cast<const char *>(&value), sizeof(value));
	throw_if_stream_failed(this->file, "Failed to write dataset file.");
}

void
DataSerializer::read(uint8_t &value)
{
	this->file.read(reinterpret_cast<char *>(&value), sizeof(value));
	throw_if_stream_failed(this->file, "Failed to read dataset file.");
}

// uint32_t

void
DataSerializer::write(uint32_t value)
{
	__le32 le_value = __cpu_to_le32(value);
	this->file.write(reinterpret_cast<const char *>(&le_value), sizeof(le_value));
	throw_if_stream_failed(this->file, "Failed to write dataset file.");
}

void
DataSerializer::read(uint32_t &value)
{
	__le32 le_value;
	this->file.read(reinterpret_cast<char *>(&le_value), sizeof(le_value));
	throw_if_stream_failed(this->file, "Failed to read dataset file.");
	value = __le32_to_cpu(le_value);
}

// float

void
DataSerializer::write(float value)
{
	__lef32 le_value = __cpu_to_lef32(value);
	this->file.write(reinterpret_cast<const char *>(&le_value), sizeof(le_value));
	throw_if_stream_failed(this->file, "Failed to write dataset file.");
}

void
DataSerializer::read(float &value)
{
	__lef32 le_value;
	this->file.read(reinterpret_cast<char *>(&le_value), sizeof(le_value));
	throw_if_stream_failed(this->file, "Failed to read dataset file.");
	value = __lef32_to_cpu(le_value);
}

// uint64_t

void
DataSerializer::write(uint64_t value)
{
	__le64 le_value = __cpu_to_le64(value);
	this->file.write(reinterpret_cast<const char *>(&le_value), sizeof(le_value));
	throw_if_stream_failed(this->file, "Failed to write dataset file.");
}

void
DataSerializer::read(uint64_t &value)
{
	__le64 le_value;
	this->file.read(reinterpret_cast<char *>(&le_value), sizeof(le_value));
	throw_if_stream_failed(this->file, "Failed to read dataset file.");
	value = __le64_to_cpu(le_value);
}

// double

void
DataSerializer::write(double value)
{
	__lef64 le_value = __cpu_to_lef64(value);
	this->file.write(reinterpret_cast<const char *>(&le_value), sizeof(le_value));
	throw_if_stream_failed(this->file, "Failed to write dataset file.");
}

void
DataSerializer::read(double &value)
{
	__lef64 le_value;
	this->file.read(reinterpret_cast<char *>(&le_value), sizeof(le_value));
	throw_if_stream_failed(this->file, "Failed to read dataset file.");
	value = __lef64_to_cpu(le_value);
}

// xrt_pose

void
DataSerializer::write(const xrt_pose &value)
{
	this->write(value.position.x);
	this->write(value.position.y);
	this->write(value.position.z);
	this->write(value.orientation.x);
	this->write(value.orientation.y);
	this->write(value.orientation.z);
	this->write(value.orientation.w);
}

void
DataSerializer::read(xrt_pose &value)
{
	this->read(value.position.x);
	this->read(value.position.y);
	this->read(value.position.z);
	this->read(value.orientation.x);
	this->read(value.orientation.y);
	this->read(value.orientation.z);
	this->read(value.orientation.w);
}

// FoundDevicePose

void
DataSerializer::write(const FoundDevicePose &value)
{
	this->write(value.Tcv_cam_device);
}

void
DataSerializer::read(FoundDevicePose &value)
{
	this->read(value.Tcv_cam_device);
}

// DeviceState

void
DataSerializer::write(const DeviceState &value)
{
	this->write(static_cast<uint8_t>(value.device_id));

	this->write(value.Txr_world_device_prior);
	this->write(value.found_pose);
}

void
DataSerializer::read(DeviceState &value)
{
	uint8_t device_id;
	this->read(device_id);
	value.device_id = static_cast<t_constellation_device_id_t>(device_id);

	this->read(value.Txr_world_device_prior);
	this->read(value.found_pose);
}

// t_blob

void
DataSerializer::write(const t_blob &value)
{
	this->write(value.blob_id);
	this->write(static_cast<uint8_t>(value.matched_device_id));
	this->write(static_cast<uint8_t>(value.matched_device_led_id));
	this->write(value.center_distorted.x);
	this->write(value.center_distorted.y);
	this->write(value.center_undistorted.x);
	this->write(value.center_undistorted.y);
	this->write(value.center_homogenized.x);
	this->write(value.center_homogenized.y);
	this->write(value.motion_vector.x);
	this->write(value.motion_vector.y);
	this->write(static_cast<uint32_t>(value.bounding_box.offset.w));
	this->write(static_cast<uint32_t>(value.bounding_box.offset.h));
	this->write(static_cast<uint32_t>(value.bounding_box.extent.w));
	this->write(static_cast<uint32_t>(value.bounding_box.extent.h));
	this->write(value.size.x);
	this->write(value.size.y);
	this->write(value.brightness);
}

void
DataSerializer::read(t_blob &value)
{
	this->read(value.blob_id);

	uint8_t matched_device_id;
	this->read(matched_device_id);
	value.matched_device_id = static_cast<t_constellation_device_id_t>(matched_device_id);

	uint8_t matched_device_led_id;
	this->read(matched_device_led_id);
	value.matched_device_led_id = static_cast<t_constellation_device_id_t>(matched_device_led_id);

	this->read(value.center_distorted.x);
	this->read(value.center_distorted.y);
	this->read(value.center_undistorted.x);
	this->read(value.center_undistorted.y);
	this->read(value.center_homogenized.x);
	this->read(value.center_homogenized.y);
	this->read(value.motion_vector.x);
	this->read(value.motion_vector.y);

	uint32_t bounding_box_offset_w;
	this->read(bounding_box_offset_w);
	value.bounding_box.offset.w = static_cast<int>(bounding_box_offset_w);

	uint32_t bounding_box_offset_h;
	this->read(bounding_box_offset_h);
	value.bounding_box.offset.h = static_cast<int>(bounding_box_offset_h);

	uint32_t bounding_box_extent_w;
	this->read(bounding_box_extent_w);
	value.bounding_box.extent.w = static_cast<int>(bounding_box_extent_w);

	uint32_t bounding_box_extent_h;
	this->read(bounding_box_extent_h);
	value.bounding_box.extent.h = static_cast<int>(bounding_box_extent_h);

	this->read(value.size.x);
	this->read(value.size.y);
	this->read(value.brightness);
}

// CameraSample

void
DataSerializer::write(const CameraSample &value)
{
	this->write(static_cast<uint64_t>(value.id));
	this->write(static_cast<uint64_t>(value.sequence_id));
	this->write(static_cast<uint64_t>(value.timestamp_ns));

	this->write(static_cast<uint32_t>(value.blob_count));
	for (size_t i = 0; i < value.blob_count; i++) {
		this->write(value.blobs[i]);
	}

	this->write(value.Txr_world_cam);

	this->write(value.device_count);
	for (size_t i = 0; i < value.device_count; i++) {
		this->write(value.device_states[i]);
	}

	this->write(value.mosaic_index);
	this->write(value.camera_index);
}

void
DataSerializer::read(CameraSample &value)
{
	this->read(value.id);
	this->read(value.sequence_id);

	uint64_t timestamp_ns;
	this->read(timestamp_ns);
	value.timestamp_ns = timestamp_ns;

	this->read(value.blob_count);
	if (value.blob_count > ARRAY_SIZE(value.blobs)) {
		throw std::runtime_error("Dataset sample blob_count exceeds CameraSample storage.");
	}

	for (size_t i = 0; i < value.blob_count; i++) {
		this->read(value.blobs[i]);
	}

	this->read(value.Txr_world_cam);

	uint32_t device_count;
	this->read(device_count);
	value.device_count = device_count;

	if (device_count > value.device_states.max_size()) {
		throw std::runtime_error("Dataset sample device_count exceeds CameraSample storage.");
	}

	for (size_t i = 0; i < value.device_count; i++) {
		this->read(value.device_states[i]);
	}

	this->read(value.mosaic_index);
	this->read(value.camera_index);
}

// t_camera_calibration

void
DataSerializer::write(const t_camera_calibration &value)
{
	this->write(static_cast<uint32_t>(value.image_size_pixels.w));
	this->write(static_cast<uint32_t>(value.image_size_pixels.h));

	// @todo remove when clang-format is updated in CI
	// clang-format off
	double *intrinsics_as_array = reinterpret_cast<double *>(const_cast<double (*)[3][3]>(&value.intrinsics));
	// clang-format on

	for (size_t i = 0; i < 9; i++) {
		this->write(intrinsics_as_array[i]);
	}

	for (size_t i = 0; i < ARRAY_SIZE(value.distortion_parameters_as_array); i++) {
		this->write(value.distortion_parameters_as_array[i]);
	}

	this->write(static_cast<uint8_t>(value.distortion_model));
}

void
DataSerializer::read(t_camera_calibration &value)
{
	uint32_t image_size_w;
	this->read(image_size_w);
	value.image_size_pixels.w = static_cast<int>(image_size_w);

	uint32_t image_size_h;
	this->read(image_size_h);
	value.image_size_pixels.h = static_cast<int>(image_size_h);

	// @todo remove when clang-format is updated in CI
	// clang-format off
	double *intrinsics_as_array = reinterpret_cast<double *>(const_cast<double (*)[3][3]>(&value.intrinsics));
	// clang-format on

	for (size_t i = 0; i < 9; i++) {
		this->read(intrinsics_as_array[i]);
	}

	for (size_t i = 0; i < ARRAY_SIZE(value.distortion_parameters_as_array); i++) {
		this->read(value.distortion_parameters_as_array[i]);
	}

	uint8_t distortion_model;
	this->read(distortion_model);
	value.distortion_model = static_cast<t_camera_distortion_model>(distortion_model);
}

// t_constellation_tracker_led

void
DataSerializer::write(t_constellation_tracker_led &value)
{
	this->write(value.position.x);
	this->write(value.position.y);
	this->write(value.position.z);

	this->write(value.normal.x);
	this->write(value.normal.y);
	this->write(value.normal.z);

	this->write(value.radius_m);
	this->write(value.visibility_angle);

	this->write(static_cast<uint8_t>(value.id));
}

void
DataSerializer::read(t_constellation_tracker_led &value)
{
	this->read(value.position.x);
	this->read(value.position.y);
	this->read(value.position.z);

	this->read(value.normal.x);
	this->read(value.normal.y);
	this->read(value.normal.z);

	this->read(value.radius_m);
	this->read(value.visibility_angle);

	uint8_t id;
	this->read(id);
	value.id = static_cast<t_constellation_led_id_it>(id);
}

// t_constellation_tracker_led_model

void
DataSerializer::write(const t_constellation_tracker_led_model &value)
{
	this->write(static_cast<uint32_t>(value.led_count));
	for (size_t i = 0; i < value.led_count; i++) {
		this->write(value.leds[i]);
	}
}

void
DataSerializer::read(std::vector<t_constellation_tracker_led> &led_storage, t_constellation_tracker_led_model &value)
{
	uint32_t led_count;
	this->read(led_count);
	value.led_count = led_count;

	led_storage.resize(led_count);
	value.leds = led_storage.data();

	for (size_t i = 0; i < value.led_count; i++) {
		this->read(value.leds[i]);
	}
}

// Flush

void
DataSerializer::flush()
{
	this->file.flush();
}

/*
 *
 * DataRecorder implementations
 *
 */

DataRecorder::DataRecorder(ConstellationTracker *tracker, std::string out_file) : serializer(out_file, true)
{
	// Magic value to identify the file so we can prevent loading invalid files. Written in BE to preserve the
	// match with the source code.
	this->serializer.write(static_cast<uint32_t>(__be32_to_cpu(CT_DATA_MAGIC_VALUE)));

	this->serializer.write(static_cast<uint32_t>(tracker->mosaics.size()));
	for (auto &mosaic : tracker->mosaics) {
		this->serializer.write(static_cast<uint32_t>(mosaic->cameras.size()));
		for (auto &camera : mosaic->cameras) {
			this->serializer.write(camera->calibration);
		}
	}

	this->serializer.flush();
}

void
DataRecorder::recordSample(const CameraSample &sample)
{
	std::lock_guard<std::mutex> guard(this->lock);

	this->serializer.write(static_cast<uint8_t>(PACKET_TYPE_CAMERA_SAMPLE));
	this->serializer.write(sample);
	this->serializer.flush();
}

void
DataRecorder::recordDeviceInfo(const Device &device)
{
	std::lock_guard<std::mutex> guard(this->lock);

	this->serializer.write(static_cast<uint8_t>(PACKET_TYPE_DEVICE_INFO));
	this->serializer.write(static_cast<uint8_t>(device.id));
	this->serializer.write(device.params.led_model);
	this->serializer.flush();
}

/*
 *
 * DatasetReader implementations
 *
 */

DatasetReader::DatasetReader(std::string filename) : serializer(filename, false)
{
	uint32_t magic_value;
	this->serializer.read(magic_value);
	if (magic_value != static_cast<uint32_t>(__be32_to_cpu(CT_DATA_MAGIC_VALUE))) {
		throw std::runtime_error(
		    "Invalid magic value in dataset file, not a valid constellation tracker dataset.");
	}

	uint32_t mosaic_count;
	this->serializer.read(mosaic_count);

	for (size_t i = 0; i < mosaic_count; i++) {
		DatasetMosaic mosaic;

		uint32_t camera_count;
		this->serializer.read(camera_count);

		for (size_t j = 0; j < camera_count; j++) {
			t_camera_calibration calibration;
			this->serializer.read(calibration);
			mosaic.camera_calibrations.push_back(calibration);
		}

		this->mosaics.push_back(mosaic);
	}

	while (true) {
		try {
			uint8_t packet_type;
			this->serializer.read(packet_type);
			switch (packet_type) {
			case PACKET_TYPE_CAMERA_SAMPLE: {
				CameraSample sample;
				this->serializer.read(sample);
				this->samples.push_back(sample);
				break;
			}
			case PACKET_TYPE_DEVICE_INFO: {
				DatasetDevice &device = this->devices.emplace_back();

				uint8_t device_id;
				this->serializer.read(device_id);
				device.id = static_cast<t_constellation_device_id_t>(device_id);

				this->serializer.read(device.leds, device.led_model);

				break;
			}
			default: {
				throw std::runtime_error("Invalid packet type in dataset file. Got ID " +
				                         std::to_string(packet_type));
			}
			}
		} catch (const std::exception &e) {
			break;
		}
	}
}

}; // namespace xrt::tracking::constellation
