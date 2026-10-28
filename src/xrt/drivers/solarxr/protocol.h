// Copyright 2025, rcelyte
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Parser for a small subset of the SolarXR Flatbuffers protocol, as defined at
 * https://github.com/SlimeVR/SolarXR-Protocol
 * @ingroup drv_solarxr
 */

#pragma once
#include "xrt/xrt_defines.h"
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t flatbuffers_uoffset_t; // little-endian byte order

enum solarxr_body_part
{
	SOLARXR_BODY_PART_NONE = 0,
	SOLARXR_BODY_PART_HEAD = 1,
	SOLARXR_BODY_PART_NECK = 2,
	SOLARXR_BODY_PART_CHEST = 3,
	SOLARXR_BODY_PART_WAIST = 4,
	SOLARXR_BODY_PART_HIP = 5,
	SOLARXR_BODY_PART_LEFT_UPPER_LEG = 6,
	SOLARXR_BODY_PART_RIGHT_UPPER_LEG = 7,
	SOLARXR_BODY_PART_LEFT_LOWER_LEG = 8,
	SOLARXR_BODY_PART_RIGHT_LOWER_LEG = 9,
	SOLARXR_BODY_PART_LEFT_FOOT = 10,
	SOLARXR_BODY_PART_RIGHT_FOOT = 11,
	SOLARXR_BODY_PART_LEFT_LOWER_ARM = 14,
	SOLARXR_BODY_PART_RIGHT_LOWER_ARM = 15,
	SOLARXR_BODY_PART_LEFT_UPPER_ARM = 16,
	SOLARXR_BODY_PART_RIGHT_UPPER_ARM = 17,
	SOLARXR_BODY_PART_LEFT_HAND = 18,
	SOLARXR_BODY_PART_RIGHT_HAND = 19,
	SOLARXR_BODY_PART_LEFT_SHOULDER = 20,
	SOLARXR_BODY_PART_RIGHT_SHOULDER = 21,
	SOLARXR_BODY_PART_UPPER_CHEST = 22,
	SOLARXR_BODY_PART_LEFT_HIP = 23,
	SOLARXR_BODY_PART_RIGHT_HIP = 24,
	SOLARXR_BODY_PART_LEFT_THUMB_PROXIMAL = 25,
	SOLARXR_BODY_PART_LEFT_THUMB_INTERMEDIATE = 26,
	SOLARXR_BODY_PART_LEFT_THUMB_DISTAL = 27,
	SOLARXR_BODY_PART_LEFT_INDEX_PROXIMAL = 28,
	SOLARXR_BODY_PART_LEFT_INDEX_INTERMEDIATE = 29,
	SOLARXR_BODY_PART_LEFT_INDEX_DISTAL = 30,
	SOLARXR_BODY_PART_LEFT_MIDDLE_PROXIMAL = 31,
	SOLARXR_BODY_PART_LEFT_MIDDLE_INTERMEDIATE = 32,
	SOLARXR_BODY_PART_LEFT_MIDDLE_DISTAL = 33,
	SOLARXR_BODY_PART_LEFT_RING_PROXIMAL = 34,
	SOLARXR_BODY_PART_LEFT_RING_INTERMEDIATE = 35,
	SOLARXR_BODY_PART_LEFT_RING_DISTAL = 36,
	SOLARXR_BODY_PART_LEFT_LITTLE_PROXIMAL = 37,
	SOLARXR_BODY_PART_LEFT_LITTLE_INTERMEDIATE = 38,
	SOLARXR_BODY_PART_LEFT_LITTLE_DISTAL = 39,
	SOLARXR_BODY_PART_RIGHT_THUMB_PROXIMAL = 40,
	SOLARXR_BODY_PART_RIGHT_THUMB_INTERMEDIATE = 41,
	SOLARXR_BODY_PART_RIGHT_THUMB_DISTAL = 42,
	SOLARXR_BODY_PART_RIGHT_INDEX_PROXIMAL = 43,
	SOLARXR_BODY_PART_RIGHT_INDEX_INTERMEDIATE = 44,
	SOLARXR_BODY_PART_RIGHT_INDEX_DISTAL = 45,
	SOLARXR_BODY_PART_RIGHT_MIDDLE_PROXIMAL = 46,
	SOLARXR_BODY_PART_RIGHT_MIDDLE_INTERMEDIATE = 47,
	SOLARXR_BODY_PART_RIGHT_MIDDLE_DISTAL = 48,
	SOLARXR_BODY_PART_RIGHT_RING_PROXIMAL = 49,
	SOLARXR_BODY_PART_RIGHT_RING_INTERMEDIATE = 50,
	SOLARXR_BODY_PART_RIGHT_RING_DISTAL = 51,
	SOLARXR_BODY_PART_RIGHT_LITTLE_PROXIMAL = 52,
	SOLARXR_BODY_PART_RIGHT_LITTLE_INTERMEDIATE = 53,
	SOLARXR_BODY_PART_RIGHT_LITTLE_DISTAL = 54,
	SOLARXR_BODY_PART_MAX_ENUM,
};

#define DEFINE_FLATBUFFERS_VECTOR(type_, name_)                                                                        \
	static inline struct name_                                                                                     \
	{                                                                                                              \
		uint32_t length;                                                                                       \
		const type_ *data;                                                                                     \
	} name_(const struct flatbuffers_vector from)                                                                  \
	{                                                                                                              \
		return (struct name_){from.length, (const type_ *)from.data};                                          \
	}

DEFINE_FLATBUFFERS_VECTOR(void, flatbuffers_vector)

struct solarxr_tracker_id
{ // table solarxr_protocol.datatypes.TrackerId
	bool has_device_id;
	uint8_t device_id;
	uint8_t tracker_num;
};

DEFINE_FLATBUFFERS_VECTOR(char, flatbuffers_vector_char)

struct solarxr_tracker_info
{ // table solarxr_protocol.data_feed.tracker.TrackerInfo
	enum solarxr_body_part body_part;
	struct flatbuffers_vector_char display_name;
};

typedef struct
{
	flatbuffers_uoffset_t offset;
} solarxr_tracker_data_t;

struct solarxr_tracker_data
{ // table solarxr_protocol.data_feed.tracker.TrackerData
	bool has_info;
	bool has_rotation;
	bool has_position;
	bool has_raw_angular_velocity;
	bool has_linear_acceleration;
	struct solarxr_tracker_id tracker_id;
	struct solarxr_tracker_info info;
	struct xrt_quat rotation;
	struct xrt_vec3 position;
	struct xrt_vec3 raw_angular_velocity;
	struct xrt_vec3 linear_acceleration;
};

typedef struct
{
	flatbuffers_uoffset_t offset;
} solarxr_device_data_t;

DEFINE_FLATBUFFERS_VECTOR(solarxr_tracker_data_t, flatbuffers_vector_solarxr_tracker_data)

struct solarxr_device_data
{ // table solarxr_protocol.data_feed.device_data.DeviceData
	uint8_t id;
	struct flatbuffers_vector_solarxr_tracker_data trackers; // solarxr_protocol.data_feed.tracker.TrackerData[]
};

typedef struct
{
	flatbuffers_uoffset_t offset;
} solarxr_bone_t;

struct solarxr_bone
{ // table solarxr_protocol.data_feed.Bone
	enum solarxr_body_part body_part;
	struct xrt_quat rotation_g;
	float bone_length;
	struct xrt_vec3 head_position_g;
};

DEFINE_FLATBUFFERS_VECTOR(solarxr_device_data_t, flatbuffers_vector_solarxr_device_data)
DEFINE_FLATBUFFERS_VECTOR(solarxr_bone_t, flatbuffers_vector_solarxr_bone)

struct solarxr_data_feed_update
{                                                              // table solarxr_protocol.data_feed.DataFeedUpdate
	struct flatbuffers_vector_solarxr_device_data devices; // solarxr_protocol.data_feed.device_data.DeviceData[]
	struct flatbuffers_vector_solarxr_tracker_data
	    synthetic_trackers;                       // solarxr_protocol.data_feed.tracker.TrackerData[]
	struct flatbuffers_vector_solarxr_bone bones; // solarxr_protocol.data_feed.Bone[]
};

enum solarxr_data_feed_message_type
{
	SOLARXR_DATA_FEED_MESSAGE_POLL_DATA_FEED = 1,
	SOLARXR_DATA_FEED_MESSAGE_START_DATA_FEED = 2,
	SOLARXR_DATA_FEED_MESSAGE_DATA_FEED_UPDATE = 3,
};

union solarxr_data_feed_message { // union solarxr_protocol.data_feed.DataFeedMessage
	struct solarxr_data_feed_update data_feed_update;
};

typedef struct
{
	flatbuffers_uoffset_t offset;
} solarxr_data_feed_message_header_t;

struct solarxr_data_feed_message_header
{ // table solarxr_protocol.data_feed.DataFeedMessageHeader
	enum solarxr_data_feed_message_type message_type;
	union solarxr_data_feed_message message;
};

struct solarxr_steamvr_trackers_setting
{ // table solarxr_protocol.rpc.SteamVRTrackersSetting
	bool waist;
	bool chest;
	bool left_foot;
	bool right_foot;
	bool left_knee;
	bool right_knee;
	bool left_elbow;
	bool right_elbow;
	bool left_hand;
	bool right_hand;
};

struct solarxr_settings_response
{ // table solarxr_protocol.rpc.SettingsResponse
	struct solarxr_steamvr_trackers_setting steam_vr_trackers;
};

enum solarxr_rpc_message_type
{
	SOLARXR_RPC_MESSAGE_TYPE_SETTINGS_REQUEST = 6,
	SOLARXR_RPC_MESSAGE_TYPE_SETTINGS_RESPONSE = 7,
};

union solarxr_rpc_message { // union solarxr_protocol.rpc.RpcMessage
	struct solarxr_settings_response settings_response;
};

typedef struct
{
	flatbuffers_uoffset_t offset;
} solarxr_rpc_message_header_t;

struct solarxr_rpc_message_header
{ // table solarxr_protocol.rpc.RpcMessageHeader
	enum solarxr_rpc_message_type message_type;
	union solarxr_rpc_message message;
};

typedef struct
{
	flatbuffers_uoffset_t offset;
} solarxr_message_bundle_t;

DEFINE_FLATBUFFERS_VECTOR(solarxr_data_feed_message_header_t, flatbuffers_vector_solarxr_data_feed_message_header)
DEFINE_FLATBUFFERS_VECTOR(solarxr_rpc_message_header_t, flatbuffers_vector_solarxr_rpc_message_header)

struct solarxr_message_bundle
{ // table solarxr_protocol.MessageBundle
	struct flatbuffers_vector_solarxr_data_feed_message_header
	    data_feed_msgs; // solarxr_protocol.data_feed.DataFeedMessageHeader[]
	struct flatbuffers_vector_solarxr_rpc_message_header rpc_msgs; // solarxr_protocol.rpc.RpcMessageHeader[]
};

#undef DEFINE_FLATBUFFERS_VECTOR

bool
read_solarxr_message_bundle(struct solarxr_message_bundle *out,
                            const uint8_t buffer[],
                            size_t buffer_len,
                            const solarxr_message_bundle_t *ref);
bool
read_solarxr_data_feed_message_header(struct solarxr_data_feed_message_header *out,
                                      const uint8_t buffer[],
                                      size_t buffer_len,
                                      const solarxr_data_feed_message_header_t *ref);
bool
read_solarxr_rpc_message_header(struct solarxr_rpc_message_header *out,
                                const uint8_t buffer[],
                                size_t buffer_len,
                                const solarxr_rpc_message_header_t *ref);
bool
read_solarxr_device_data(struct solarxr_device_data *out,
                         const uint8_t buffer[],
                         size_t buffer_len,
                         const solarxr_device_data_t *ref);
bool
read_solarxr_tracker_data(struct solarxr_tracker_data *out,
                          const uint8_t buffer[],
                          size_t buffer_len,
                          const solarxr_tracker_data_t *ref);
bool
read_solarxr_bone(struct solarxr_bone *out, const uint8_t buffer[], size_t buffer_len, const solarxr_bone_t *ref);

#ifdef __cplusplus
}
#endif
