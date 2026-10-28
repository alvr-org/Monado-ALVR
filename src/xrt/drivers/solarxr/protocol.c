// Copyright 2025, rcelyte
// SPDX-License-Identifier: BSL-1.0

#include "protocol.h"

#include "math/m_api.h"

#include <endian.h>
#include <string.h>

typedef int32_t flatbuffers_soffset_t;  // little-endian byte order
typedef uint16_t flatbuffers_voffset_t; // little-endian byte order

struct flatbuffers_vtable_t
{
	flatbuffers_voffset_t vtable_size;
	flatbuffers_voffset_t table_size;
	flatbuffers_voffset_t offsets[];
};

struct table_data
{
	uint16_t length;
	const uint8_t *data;
};

static const void *
read_flatbuffers_uoffset(const uint8_t buffer[const],
                         const size_t buffer_len,
                         const flatbuffers_uoffset_t *const ref,
                         const size_t size)
{
	assert((const uint8_t *)ref >= buffer && (const uint8_t *)ref <= &buffer[buffer_len - sizeof(*ref)]);
	const uint32_t offset = le32toh(*ref);
	const size_t capacity = &buffer[buffer_len] - (const uint8_t *)ref;
	if (capacity < size || offset == 0 || offset > capacity - size || (offset % sizeof(uint32_t)) != 0) {
		return NULL;
	}
	return (const uint8_t *)ref + offset;
}

static struct table_data
read_flatbuffers_table(const uint8_t buffer[const],
                       const size_t buffer_len,
                       const flatbuffers_uoffset_t *const ref,
                       uint16_t vtable_out[const],
                       const uint16_t vtable_cap)
{
	memset(vtable_out, 0, vtable_cap * sizeof(*vtable_out));
	const flatbuffers_soffset_t *const table = read_flatbuffers_uoffset(buffer, buffer_len, ref, sizeof(*table));
	if (table == NULL) {
		return (struct table_data){0};
	}

	const int32_t vtable_offset = -(int32_t)le32toh(*table);
	const struct flatbuffers_vtable_t *const vtable =
	    (const struct flatbuffers_vtable_t *)((const uint8_t *)table + vtable_offset);
	if (vtable_offset < buffer - (const uint8_t *)table ||
	    vtable_offset > &buffer[buffer_len - sizeof(*vtable)] - (const uint8_t *)table) {
		return (struct table_data){0};
	}

	const uint16_t vtable_size = le16toh(vtable->vtable_size), table_size = le16toh(vtable->table_size);
	if (vtable_size < sizeof(*vtable) || vtable_size > &buffer[buffer_len] - (const uint8_t *)vtable ||
	    (vtable_size % sizeof(*vtable->offsets)) != 0 || table_size < sizeof(*table) ||
	    table_size > &buffer[buffer_len] - (const uint8_t *)table) {
		return (struct table_data){0};
	}

	for (uint16_t i = 0, length = MIN((vtable_size - sizeof(*vtable)) / sizeof(*vtable->offsets), vtable_cap);
	     i < length; ++i) {
		const uint16_t offset = le16toh(vtable->offsets[i]);
		if (offset < table_size) {
			vtable_out[i] = offset;
		}
	}

	return (struct table_data){table_size, (const uint8_t *)table};
}

static struct flatbuffers_vector
read_flatbuffers_vector(const uint8_t buffer[const],
                        const size_t buffer_len,
                        const flatbuffers_uoffset_t *const ref,
                        const size_t element_size)
{
	const uint32_t *const vector = read_flatbuffers_uoffset(buffer, buffer_len, ref, sizeof(*vector));
	if (vector == NULL) {
		return (struct flatbuffers_vector){0};
	}

	const struct flatbuffers_vector out = {
	    .length = le32toh(*vector),
	    .data = &vector[1],
	};
	if (out.length > (&buffer[buffer_len] - (const uint8_t *)out.data) / element_size) {
		return (struct flatbuffers_vector){0};
	}

	return out;
}

static bool
read_solarxr_quat(struct xrt_quat *const out,
                  const uint8_t buffer[const],
                  const size_t buffer_len,
                  const uint16_t offset)
{
	static_assert(offsetof(struct xrt_quat, x) == 0, "");
	static_assert(offsetof(struct xrt_quat, y) == 4, "");
	static_assert(offsetof(struct xrt_quat, z) == 8, "");
	static_assert(offsetof(struct xrt_quat, w) == 12, "");
	static_assert(sizeof(struct xrt_quat) == 16, "");

	*out = (struct xrt_quat){.w = 1};
	if (offset == 0 || offset + sizeof(*out) > buffer_len) {
		return false;
	}

	memcpy(out, &buffer[offset], sizeof(*out));
	return true;
}

static bool
read_solarxr_vec3f(struct xrt_vec3 *const out,
                   const uint8_t buffer[const],
                   const size_t buffer_len,
                   const uint16_t offset)
{
	static_assert(offsetof(struct xrt_vec3, x) == 0, "");
	static_assert(offsetof(struct xrt_vec3, y) == 4, "");
	static_assert(offsetof(struct xrt_vec3, z) == 8, "");
	static_assert(sizeof(struct xrt_vec3) == 12, "");

	*out = (struct xrt_vec3){0};
	if (offset == 0 || offset + sizeof(*out) > buffer_len) {
		return false;
	}

	memcpy(out, &buffer[offset], sizeof(*out));
	return true;
}

bool
read_solarxr_message_bundle(struct solarxr_message_bundle *const out,
                            const uint8_t buffer[const],
                            const size_t buffer_len,
                            const solarxr_message_bundle_t *const ref)
{
	*out = (struct solarxr_message_bundle){0};
	uint16_t bundle_vtable[2];
	const struct table_data bundle =
	    read_flatbuffers_table(buffer, buffer_len, &ref->offset, bundle_vtable, ARRAY_SIZE(bundle_vtable));
	if (bundle.length == 0) {
		return false;
	}

	if (bundle_vtable[0] != 0 && bundle_vtable[0] + sizeof(flatbuffers_uoffset_t) <= bundle.length) {
		out->data_feed_msgs = flatbuffers_vector_solarxr_data_feed_message_header(read_flatbuffers_vector(
		    buffer, buffer_len, (const flatbuffers_uoffset_t *)&bundle.data[bundle_vtable[0]],
		    sizeof(*out->data_feed_msgs.data)));
	}

	if (bundle_vtable[1] != 0 && bundle_vtable[1] + sizeof(flatbuffers_uoffset_t) <= bundle.length) {
		out->rpc_msgs = flatbuffers_vector_solarxr_rpc_message_header(read_flatbuffers_vector(
		    buffer, buffer_len, (const flatbuffers_uoffset_t *)&bundle.data[bundle_vtable[1]],
		    sizeof(*out->rpc_msgs.data)));
	}

	return true;
}

bool
read_solarxr_data_feed_message_header(struct solarxr_data_feed_message_header *const out,
                                      const uint8_t buffer[const],
                                      const size_t buffer_len,
                                      const solarxr_data_feed_message_header_t *const ref)
{
	*out = (struct solarxr_data_feed_message_header){0};
	uint16_t header_vtable[2];
	const struct table_data header =
	    read_flatbuffers_table(buffer, buffer_len, &ref->offset, header_vtable, ARRAY_SIZE(header_vtable));
	if (header.length == 0) {
		return false;
	}

	if (header_vtable[0] == 0 || header_vtable[1] == 0 ||
	    header_vtable[1] + sizeof(flatbuffers_uoffset_t) > header.length) {
		return true;
	}

	out->message_type = header.data[header_vtable[0]];

	switch (header.data[header_vtable[0]]) {
	case SOLARXR_DATA_FEED_MESSAGE_DATA_FEED_UPDATE: {
		uint16_t update_vtable[3];
		const struct table_data update = read_flatbuffers_table(
		    buffer, buffer_len, (const flatbuffers_uoffset_t *)&header.data[header_vtable[1]], update_vtable,
		    ARRAY_SIZE(update_vtable));
		if (update.length == 0) {
			break;
		}

		if (update_vtable[0] != 0 && update_vtable[0] + sizeof(flatbuffers_uoffset_t) <= update.length) {
			out->message.data_feed_update.devices =
			    flatbuffers_vector_solarxr_device_data(read_flatbuffers_vector(
			        buffer, buffer_len, (const flatbuffers_uoffset_t *)&update.data[update_vtable[0]],
			        sizeof(*out->message.data_feed_update.devices.data)));
		}

		if (update_vtable[1] != 0 && update_vtable[1] + sizeof(flatbuffers_uoffset_t) <= update.length) {
			out->message.data_feed_update.synthetic_trackers =
			    flatbuffers_vector_solarxr_tracker_data(read_flatbuffers_vector(
			        buffer, buffer_len, (const flatbuffers_uoffset_t *)&update.data[update_vtable[1]],
			        sizeof(*out->message.data_feed_update.synthetic_trackers.data)));
		}

		if (update_vtable[2] != 0 && update_vtable[2] + sizeof(flatbuffers_uoffset_t) <= update.length) {
			out->message.data_feed_update.bones = flatbuffers_vector_solarxr_bone(read_flatbuffers_vector(
			    buffer, buffer_len, (const flatbuffers_uoffset_t *)&update.data[update_vtable[2]],
			    sizeof(*out->message.data_feed_update.bones.data)));
		}

		break;
	}
	default:;
	}

	return true;
}

bool
read_solarxr_rpc_message_header(struct solarxr_rpc_message_header *const out,
                                const uint8_t buffer[const],
                                const size_t buffer_len,
                                const solarxr_rpc_message_header_t *const ref)
{
	*out = (struct solarxr_rpc_message_header){0};
	uint16_t header_vtable[3];
	const struct table_data header =
	    read_flatbuffers_table(buffer, buffer_len, &ref->offset, header_vtable, ARRAY_SIZE(header_vtable));
	if (header.length == 0) {
		return false;
	}

	if (header_vtable[1] == 0 || header_vtable[2] == 0 ||
	    header_vtable[2] + sizeof(flatbuffers_uoffset_t) > header.length) {
		return true;
	}

	out->message_type = header.data[header_vtable[1]];

	switch (header.data[header_vtable[1]]) {
	case SOLARXR_RPC_MESSAGE_TYPE_SETTINGS_RESPONSE: {
		uint16_t message_vtable[1];
		const struct table_data message = read_flatbuffers_table(
		    buffer, buffer_len, (const flatbuffers_uoffset_t *)&header.data[header_vtable[2]], message_vtable,
		    ARRAY_SIZE(message_vtable));
		if (message.length == 0 || message_vtable[0] == 0 ||
		    message_vtable[0] + sizeof(flatbuffers_uoffset_t) > message.length) {
			break;
		}

		uint16_t trackers_vtable[15];
		const struct table_data trackers = read_flatbuffers_table(
		    buffer, buffer_len, (const flatbuffers_uoffset_t *)&message.data[message_vtable[0]],
		    trackers_vtable, ARRAY_SIZE(trackers_vtable));
		if (trackers.length == 0) {
			break;
		}

		if (trackers_vtable[0] != 0) {
			out->message.settings_response.steam_vr_trackers.waist = trackers.data[trackers_vtable[0]];
		}

		if (trackers_vtable[1] != 0) {
			out->message.settings_response.steam_vr_trackers.chest = trackers.data[trackers_vtable[1]];
		}

		if (trackers_vtable[7] != 0) {
			out->message.settings_response.steam_vr_trackers.left_foot = trackers.data[trackers_vtable[7]];
		}

		if (trackers_vtable[8] != 0) {
			out->message.settings_response.steam_vr_trackers.right_foot = trackers.data[trackers_vtable[8]];
		}

		if (trackers_vtable[9] != 0) {
			out->message.settings_response.steam_vr_trackers.left_knee = trackers.data[trackers_vtable[9]];
		}

		if (trackers_vtable[10] != 0) {
			out->message.settings_response.steam_vr_trackers.right_knee =
			    trackers.data[trackers_vtable[10]];
		}

		if (trackers_vtable[11] != 0) {
			out->message.settings_response.steam_vr_trackers.left_elbow =
			    trackers.data[trackers_vtable[11]];
		}

		if (trackers_vtable[12] != 0) {
			out->message.settings_response.steam_vr_trackers.right_elbow =
			    trackers.data[trackers_vtable[12]];
		}

		if (trackers_vtable[13] != 0) {
			out->message.settings_response.steam_vr_trackers.left_hand = trackers.data[trackers_vtable[13]];
		}

		if (trackers_vtable[14] != 0) {
			out->message.settings_response.steam_vr_trackers.right_hand =
			    trackers.data[trackers_vtable[14]];
		}

		break;
	}
	default:;
	}

	return true;
}

bool
read_solarxr_device_data(struct solarxr_device_data *const out,
                         const uint8_t buffer[const],
                         const size_t buffer_len,
                         const solarxr_device_data_t *const ref)
{
	*out = (struct solarxr_device_data){0};
	uint16_t data_vtable[5];
	const struct table_data data =
	    read_flatbuffers_table(buffer, buffer_len, &ref->offset, data_vtable, ARRAY_SIZE(data_vtable));
	if (data.length == 0) {
		return false;
	}

	if (data_vtable[0] != 0) {
		out->id = data.data[data_vtable[0]];
	}

	if (data_vtable[4] != 0 && data_vtable[4] + sizeof(flatbuffers_uoffset_t) <= data.length) {
		out->trackers = flatbuffers_vector_solarxr_tracker_data(read_flatbuffers_vector(
		    buffer, buffer_len, (const flatbuffers_uoffset_t *)&data.data[data_vtable[4]],
		    sizeof(*out->trackers.data)));
	}

	return true;
}

bool
read_solarxr_tracker_data(struct solarxr_tracker_data *const out,
                          const uint8_t buffer[const],
                          const size_t buffer_len,
                          const solarxr_tracker_data_t *const ref)
{
	*out = (struct solarxr_tracker_data){0};
	uint16_t data_vtable[9];
	const struct table_data data =
	    read_flatbuffers_table(buffer, buffer_len, &ref->offset, data_vtable, ARRAY_SIZE(data_vtable));
	if (data.length == 0) {
		return false;
	}

	if (data_vtable[0] != 0 && data_vtable[0] + sizeof(flatbuffers_uoffset_t) <= data.length) {
		uint16_t id_vtable[2];
		const struct table_data id = read_flatbuffers_table(
		    buffer, buffer_len, (const flatbuffers_uoffset_t *)&data.data[data_vtable[0]], id_vtable,
		    ARRAY_SIZE(id_vtable));
		if (id_vtable[0] != 0) {
			out->tracker_id.has_device_id = true;
			out->tracker_id.device_id = id.data[id_vtable[0]];
		}

		if (id_vtable[1] != 0) {
			out->tracker_id.tracker_num = id.data[id_vtable[1]];
		}
	}

	if (data_vtable[1] != 0 && data_vtable[1] + sizeof(flatbuffers_uoffset_t) <= data.length) {
		uint16_t info_vtable[8];
		const struct table_data info = read_flatbuffers_table(
		    buffer, buffer_len, (const flatbuffers_uoffset_t *)&data.data[data_vtable[1]], info_vtable,
		    ARRAY_SIZE(info_vtable));
		out->has_info = true;
		if (info_vtable[1] != 0) {
			out->info.body_part = info.data[info_vtable[1]];
		}

		if (info_vtable[7] != 0) {
			out->info.display_name = flatbuffers_vector_char(read_flatbuffers_vector(
			    buffer, buffer_len, (const flatbuffers_uoffset_t *)&info.data[info_vtable[7]],
			    sizeof(*out->info.display_name.data)));
		}
	}

	out->has_rotation = read_solarxr_quat(&out->rotation, data.data, data.length, data_vtable[3]);
	out->has_position = read_solarxr_vec3f(&out->position, data.data, data.length, data_vtable[4]);
	out->has_raw_angular_velocity =
	    read_solarxr_vec3f(&out->raw_angular_velocity, data.data, data.length, data_vtable[5]);
	out->has_linear_acceleration =
	    read_solarxr_vec3f(&out->linear_acceleration, data.data, data.length, data_vtable[8]);

	return true;
}

bool
read_solarxr_bone(struct solarxr_bone *const out,
                  const uint8_t buffer[const],
                  const size_t buffer_len,
                  const solarxr_bone_t *const ref)
{
	*out = (struct solarxr_bone){0};
	uint16_t bone_vtable[4];
	const struct table_data bone =
	    read_flatbuffers_table(buffer, buffer_len, &ref->offset, bone_vtable, ARRAY_SIZE(bone_vtable));
	if (bone.length == 0) {
		return false;
	}

	if (bone_vtable[0] != 0) {
		out->body_part = bone.data[bone_vtable[0]];
	}

	read_solarxr_quat(&out->rotation_g, bone.data, bone.length, bone_vtable[1]);
	if (bone_vtable[2] != 0 && bone_vtable[2] + sizeof(out->bone_length) <= bone.length) {
		memcpy(&out->bone_length, &bone.data[bone_vtable[2]], sizeof(out->bone_length));
	}

	read_solarxr_vec3f(&out->head_position_g, bone.data, bone.length, bone_vtable[3]);
	return true;
}
