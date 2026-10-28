// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Code to decode and encode COBS (Consistent Overhead Byte Stuffing) packet data.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup aux_util
 */

#pragma once

#include <xrt/xrt_compiler.h>


#ifdef __cplusplus
extern "C" {
#endif


// Callback for when packet is finished
typedef void (*cobs_callback_t)(void *user_data, const uint8_t *data, size_t length);

/*!
 * A streaming COBS (Consistent Overhead Byte Stuffing) decoder with automatic error recovery.
 * Prefers to drop incomplete packets instead of attempting to parse them.
 *
 * https://en.wikipedia.org/wiki/Consistent_Overhead_Byte_Stuffing
 *
 * @ingroup aux_util
 */
struct u_cobs_decoder
{
	cobs_callback_t callback;
	void *user_data;

	uint8_t *buffer;
	size_t buffer_size;

	size_t read_bytes;
	size_t bytes_until_next_code;

	//! Whether we're in error recovery mode (trying to reach a 0x00 delimiter)
	bool error_recovery;
};

/*!
 * Creates a new COBS decoder.
 *
 * @param buffer_size The size of the internal buffer for decoding packets into.
 * @param callback    The callback to fire when a packet is completely received.
 * @param user_data   The user data pointer to pass into the callback.
 * @param out_decoder A pointer to store the created COBS decoder.
 * @return 0 on success, -ENOMEM if memory allocation fails.
 *
 * @ingroup aux_util
 */
int
u_cobs_decoder_create(size_t buffer_size,
                      cobs_callback_t callback,
                      void *user_data,
                      struct u_cobs_decoder *out_decoder);

/*!
 * Destroys a COBS decoder previously created by @ref u_cobs_decoder_create
 *
 * @param cobs The COBS decoder to destroy.
 *
 * @ingroup aux_util
 */
void
u_cobs_decoder_destroy(struct u_cobs_decoder *cobs);

/*!
 * Pushes bytes into the COBS decoder. This function will synchronously fire the callback when a packet is completed.
 *
 * @param cobs   The COBS decoder.
 * @param data   The data to decode.
 * @param length The length of the data pointer.
 * @return 0 on success, -EINVAL on invalid packet, -ENOSPC if we run out of space in the packet buffer.
 *
 * @ingroup aux_util
 */
int
u_cobs_push_bytes(struct u_cobs_decoder *cobs, const uint8_t *data, size_t length);

/*!
 * Encodes a single packet using COBS.
 *
 * @param input        The input data buffer to encode.
 * @param input_length The length of the input data buffer.
 * @param output       The output data buffer, where the packet is encoded to.
 * @param output_size  The output size of the data buffer.
 * @return The amount of bytes written to the output buffer. -ENOSPC if we run out of space in the output buffer.
 *
 * @ingroup aux_util
 */
int
u_cobs_encode(const uint8_t *input, size_t input_length, uint8_t *output, size_t output_size);


#ifdef __cplusplus
}
#endif
