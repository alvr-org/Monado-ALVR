// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Code to decode and encode COBS (Consistent Overhead Byte Stuffing) data.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup aux_util
 */

#include <util/u_misc.h>

#include "u_cobs.h"

#include <errno.h>
#include <assert.h>


#define PACKET_DELIMITER 0x00

int
u_cobs_decoder_create(size_t buffer_size, cobs_callback_t callback, void *user_data, struct u_cobs_decoder *out_decoder)
{
	assert(out_decoder != NULL);

	uint8_t *buffer = U_TYPED_ARRAY_CALLOC(uint8_t, buffer_size);
	if (buffer == NULL) {
		return -ENOMEM;
	}

	(*out_decoder) = (struct u_cobs_decoder){
	    .buffer = buffer,
	    .buffer_size = buffer_size,
	    .callback = callback,
	    .user_data = user_data,
	};

	return 0;
}

void
u_cobs_decoder_destroy(struct u_cobs_decoder *cobs)
{
	if (cobs->buffer) {
		free(cobs->buffer);
	}
}

int
u_cobs_push_bytes(struct u_cobs_decoder *cobs, const uint8_t *data, size_t length)
{
	assert(cobs != NULL);
	assert(data != NULL);

	for (size_t i = 0; i < length; i++) {
		uint8_t byte = data[i];

		if (byte == PACKET_DELIMITER) {
			if (cobs->error_recovery) {
				cobs->error_recovery = false;
				cobs->read_bytes = 0;
				cobs->bytes_until_next_code = 0;
				continue;
			}

			if (cobs->bytes_until_next_code != 0) {
				cobs->read_bytes = 0;
				return -EINVAL; // Invalid packet: unexpected delimiter
			}

			// Packet complete
			if (cobs->read_bytes > 0) {
				cobs->callback(cobs->user_data, cobs->buffer, cobs->read_bytes);
				cobs->read_bytes = 0;
			}

			continue;
		}

		// If we're trying to recover from an error, just skip all data
		if (cobs->error_recovery) {
			continue;
		}

		if (cobs->read_bytes >= cobs->buffer_size) {
			// Mark that we're trying to do error recovery
			cobs->error_recovery = true;

			for (; i < length; i++) {
				// We are looking for a delimiter to end the error recovery
				if (byte == PACKET_DELIMITER) {
					cobs->error_recovery = false;
					cobs->read_bytes = 0;
					cobs->bytes_until_next_code = 0;
					break;
				}
			}

			// We failed to do an error recovery
			if (cobs->error_recovery) {
				return -ENOSPC; // No space left in buffer
			}
		}

		if (cobs->bytes_until_next_code == 0) {
			// Read code byte
			cobs->bytes_until_next_code = byte - 1;

			// If we're not at the start of a packet, insert the implicit 0x00 byte
			if (cobs->read_bytes > 0) {
				cobs->buffer[cobs->read_bytes++] = 0x00;
			}
		} else {
			// Read data byte
			cobs->buffer[cobs->read_bytes++] = byte;
			cobs->bytes_until_next_code--;
		}
	}

	return 0;
}

int
u_cobs_encode(const uint8_t *input, size_t input_length, uint8_t *output, size_t output_size)
{
	assert(input != NULL);
	assert(output != NULL);

	if ((input_length + 1) > output_size) {
		return -ENOSPC; // There's no space to even fit the smallest possible encoding
	}

	assert(output_size > input_length); // Ensure there's enough space for encoding overhead

	size_t read_index = 0;
	size_t write_index = 0;

	while (read_index < input_length) {
		size_t code_index = write_index++;
		uint8_t code = 1;

		// Write data bytes until we hit a zero byte or reach the maximum code value
		while (read_index < input_length && input[read_index] != 0x00 && code < 0xFF) {
			// Check for output buffer overflow
			if (write_index >= output_size) {
				return -ENOSPC; // No space left in output buffer
			}

			output[write_index++] = input[read_index++];
			// Increment the code (eg. number of non-zero bytes + 1)
			code++;
		}

		// Go back and write the code byte
		output[code_index] = code;

		// If we hit a zero byte, and we're not at the end of input, don't write it, as we just encoded that
		// zero through the code byte
		if (read_index < input_length && input[read_index] == 0x00) {
			read_index++;
		}
	}

	if (write_index >= output_size) {
		return -ENOSPC; // No space left in output buffer
	}

	output[write_index++] = PACKET_DELIMITER; // Append packet delimiter to the end

	return (int)write_index; // Return the size of the encoded output
}
