// Copyright 2025, rcelyte
// SPDX-License-Identifier: BSL-1.0

#pragma once
#include <endian.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

struct solarxr_ipc_message
{
	uint32_t length;
	uint8_t body[];
};

static inline struct solarxr_ipc_message *
solarxr_ipc_message_start(uint8_t head[const], const uint8_t *const end)
{
	if ((size_t)(end - head) < sizeof(struct solarxr_ipc_message)) {
		return NULL;
	}

	struct solarxr_ipc_message *const message = (struct solarxr_ipc_message *)head;
	message->length = 0;
	return message;
}

static inline bool
solarxr_ipc_message_write(struct solarxr_ipc_message *const message,
                          const uint8_t *const end,
                          const uint8_t data[const],
                          const uint32_t data_len)
{
	if (message == NULL) {
		return false;
	}

	if ((size_t)(end - message->body) < message->length ||
	    (size_t)(end - &message->body[message->length]) < data_len) {
		message->length = UINT32_MAX;
		return false;
	}

	memcpy(&message->body[message->length], data, data_len);
	message->length += data_len;
	return true;
}

static inline uint32_t
solarxr_ipc_message_end(struct solarxr_ipc_message *const message, uint8_t **const end_out)
{
	if (message == NULL || message->length >= UINT32_MAX - sizeof(*message)) {
		return 0;
	}

	*end_out = &message->body[message->length];
	const uint32_t length = sizeof(*message) + message->length;
	message->length = htole32(length);
	return length;
}

static inline uint32_t
solarxr_ipc_message_write_single(uint8_t **const head,
                                 const uint8_t *const end,
                                 const uint8_t data[const],
                                 const uint32_t data_len)
{
	struct solarxr_ipc_message *const message = solarxr_ipc_message_start(*head, end);
	solarxr_ipc_message_write(message, end, data, data_len);
	return solarxr_ipc_message_end(message, head);
}

static inline uint32_t
solarxr_ipc_message_inline(uint8_t data[const], const uint32_t data_len)
{
	struct solarxr_ipc_message *const message = solarxr_ipc_message_start(data, &data[data_len]);
	if (message != NULL) {
		message->length = &data[data_len] - message->body;
	}
	return solarxr_ipc_message_end(message, &(uint8_t *){NULL});
}

#ifdef __cplusplus
}
#endif
