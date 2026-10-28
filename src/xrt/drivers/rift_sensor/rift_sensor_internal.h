// Copyright 2025-2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Internal structures for Oculus Rift sensor probing/initialization
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_rift_sensor
 */

#include "xrt/xrt_frame.h"
#include "xrt/xrt_byte_order.h"

#include "os/os_threading.h"

#include "rift_sensor_interface.h"

#include <libusb.h>


struct rift_sensor_context
{
	struct xrt_frame_node node;

	enum u_logging_level log_level;

	struct libusb_context *usb_ctx;

	struct rift_sensor *sensors;
	uint32_t sensor_count;

	struct os_thread_helper usb_thread;
};

struct rift_sensor
{
	enum rift_variant variant;
	struct xrt_fs *frame_server;
	struct t_camera_calibration calibration;
	struct libusb_device_handle *hid_dev;
	bool usb2;

	time_duration_ns frame_interval;
};

#define SIZE_ASSERT(type, size)                                                                                        \
	static_assert(sizeof(type) == (size), "Size of " #type " is not " #size " bytes as was expected")

#pragma pack(push, 1)

struct rift_sensor_dk2_calib
{
	uint8_t unk1[18]; // 00
	__lef64 fx;       // 18
	uint8_t unk2[4];  // 26
	__lef64 fy;       // 30
	uint8_t unk3[4];  // 38
	__lef64 cx;       // 42
	uint8_t unk4[4];  // 50
	__lef64 cy;       // 54
	uint8_t unk5[4];  // 62
	__lef64 k1;       // 66
	uint8_t unk6[4];  // 74
	__lef64 k2;       // 78
	uint8_t unk7[4];  // 86
	__lef64 p1;       // 90
	uint8_t unk8[4];  // 98
	__lef64 p2;       // 102
	uint8_t unk9[4];  // 110
	__lef64 k3;       // 114
	uint8_t pad[6];   // 122
};
SIZE_ASSERT(struct rift_sensor_dk2_calib, 128);

#define CV1_CALIB_MAGIC 0xBAADF00DDEADD00D
#define CV1_CALIB_SIZE 160

enum rift_sensor_cv1_lens_type
{
	RIFT_SENSOR_CV1_LENS_TYPE_RADTAN = 0x0,
	RIFT_SENSOR_CV1_LENS_TYPE_KB_LEGACY = 0x1,
	RIFT_SENSOR_CV1_LENS_TYPE_OLD_BA_REJECTED = 0x2,
	RIFT_SENSOR_CV1_LENS_TYPE_RADTAN_RECIPROCAL = 0x3,
	RIFT_SENSOR_CV1_LENS_TYPE_IDENTITY = 0x4,
	RIFT_SENSOR_CV1_LENS_TYPE_KB_SPLINE = 0x5,
	RIFT_SENSOR_CV1_LENS_TYPE_CV1 = 0x6,
	RIFT_SENSOR_CV1_LENS_TYPE_OPAQUE_BLOB = 0x7,
};

struct rift_sensor_cv1_calib
{
	__le64 magic;                        // 0x00
	__le64 version;                      // 0x08
	uint8_t header_unk1[0x8];            // 0x10
	__le32 crc;                          // 0x18
	__le32 payload_length;               // 0x1c
	uint8_t body[CV1_CALIB_SIZE - 0x20]; // 0x20
};
SIZE_ASSERT(struct rift_sensor_cv1_calib, CV1_CALIB_SIZE);

#pragma pack(pop)

static struct rift_sensor_context *
rift_sensor_context(struct xrt_frame_node *node)
{
	return container_of(node, struct rift_sensor_context, node);
}
