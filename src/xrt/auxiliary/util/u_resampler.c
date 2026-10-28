// Copyright 2025, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Simple audio resampler
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup aux_util
 */

#include "util/u_misc.h"
#include "math/m_api.h"

#include "u_resampler.h"

#include <assert.h>

struct u_resampler *
u_resampler_create(size_t num_samples, float sample_rate)
{
	struct u_resampler *resampler = U_TYPED_CALLOC(struct u_resampler);
	if (resampler == NULL) {
		return NULL;
	}

	resampler->samples = calloc(num_samples, sizeof(sample_t));
	if (resampler->samples == NULL) {
		u_resampler_destroy(resampler);
		return NULL;
	}

	resampler->scratch = calloc(num_samples, sizeof(sample_t));
	if (resampler->scratch == NULL) {
		u_resampler_destroy(resampler);
		return NULL;
	}

	resampler->num_samples = num_samples;
	resampler->sample_rate = sample_rate;

	resampler->read_index = 0;
	resampler->write_index = 1;

	return resampler;
}

void
u_resampler_destroy(struct u_resampler *resampler)
{
	free(resampler->samples);
	resampler->samples = NULL;
	free(resampler->scratch);
	resampler->scratch = NULL;

	free(resampler);
}

#define WRAP_ADD(resampler, a, b) ((a + b) % resampler->num_samples)
#define BETWEEN(resampler, a, b) ((b > a) ? (b - a) : ((resampler->num_samples - a) + b))

size_t
u_resampler_read(struct u_resampler *resampler, sample_t *samples, size_t num_samples)
{
	size_t total_read = 0;

	// the amount of samples that are in the buffer
	size_t samples_in_buffer = BETWEEN(resampler, resampler->read_index, resampler->write_index) - 1;

	size_t until_end = resampler->num_samples - resampler->read_index;

	size_t read = MIN(samples_in_buffer, MIN(until_end, num_samples));
	total_read += read;
	memcpy(samples, resampler->samples + resampler->read_index, read * sizeof(sample_t));
	num_samples -= read;
	samples += read;
	resampler->read_index = WRAP_ADD(resampler, resampler->read_index, read);
	samples_in_buffer -= read;

	if (num_samples > 0 && samples_in_buffer > 0) {
		read = MIN(samples_in_buffer, num_samples);
		total_read += read;
		memcpy(samples, resampler->samples + resampler->read_index, read * sizeof(sample_t));
		resampler->read_index = WRAP_ADD(resampler, resampler->read_index, read);
	}

	return total_read;
}

// push without resampling
static size_t
resampler_write_raw(struct u_resampler *resampler, const sample_t *samples, size_t num_samples)
{
	// the amount of bytes we can write until we start overwriting the play index
	size_t can_write = BETWEEN(resampler, resampler->write_index, resampler->read_index) - 1;

	if (can_write == 0) {
		return 0;
	}

	size_t written = 0;

	// the amount of bytes until the end of the buffer
	size_t until_end = resampler->num_samples - resampler->write_index;

	written += MIN(can_write, MIN(until_end, num_samples));
	num_samples -= written;
	can_write -= written;

	// copy in the samples until the end of the buffer
	memcpy(resampler->samples + resampler->write_index, samples, written * sizeof(sample_t));
	resampler->write_index = WRAP_ADD(resampler, resampler->write_index, written);
	samples += written;

	if (num_samples > 0 && can_write > 0) {
		assert(resampler->write_index == 0);

		// bytes to write after the start
		size_t written_after_start = MIN(resampler->read_index, num_samples);

		written += written_after_start;

		// copy the data
		memcpy(resampler->samples + resampler->write_index, samples, written_after_start * sizeof(sample_t));
	}

	return written;
}

// converts an index from one sample rate to another
#define TO_RATE(idx, source_rate, target_rate) ((size_t)((float)(idx) * ((target_rate) / (source_rate))))

size_t
u_resampler_write(struct u_resampler *resampler, const sample_t *source_samples, size_t num_samples, float sample_rate)
{
	// no writing needed
	if (num_samples == 0) {
		return 0;
	}

	// short-circuit if the sample rate is already matching, no resampling needed
	if (sample_rate == resampler->sample_rate) {
		return resampler_write_raw(resampler, source_samples, num_samples);
	}

	const float target_sample_rate = resampler->sample_rate;

	// the amount of samples we *can* write
	size_t can_write = BETWEEN(resampler, resampler->write_index, resampler->read_index);

	sample_t *target_samples = resampler->scratch;

	size_t target_idx = 0;
	while (true) {
		size_t source_idx = TO_RATE(target_idx, target_sample_rate, sample_rate);

		// can't read any more samples
		if (source_idx >= num_samples) {
			break;
		}

		// can't write any more samples
		if (can_write == 0) {
			break;
		}

		target_samples[target_idx] = source_samples[source_idx];

		target_idx++;
		can_write--;
	}

	// note: ignoring return value since we should never resample more than we can write
	resampler_write_raw(resampler, target_samples, target_idx);

	return TO_RATE(target_idx - 1, target_sample_rate, sample_rate);
}

void
u_resampler_reset(struct u_resampler *resampler)
{
	resampler->samples[0] = 0;
	resampler->read_index = 0;
	resampler->write_index = 1;
}
