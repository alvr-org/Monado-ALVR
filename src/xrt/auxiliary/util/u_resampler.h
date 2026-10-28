// Copyright 2025, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Simple audio resampler
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup aux_util
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef float sample_t;

struct u_resampler
{
	float sample_rate;

	size_t num_samples;
	sample_t *samples;

	// static scratch buffer for resampling use
	sample_t *scratch;

	size_t read_index;
	size_t write_index;
};

struct u_resampler *
u_resampler_create(size_t num_samples, float sample_rate);

void
u_resampler_destroy(struct u_resampler *resampler);

size_t
u_resampler_read(struct u_resampler *resampler, sample_t *samples, size_t num_samples);

size_t
u_resampler_write(struct u_resampler *resampler, const sample_t *source_samples, size_t num_samples, float sample_rate);

void
u_resampler_reset(struct u_resampler *resampler);

#ifdef __cplusplus
}
#endif
