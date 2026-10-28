// Copyright 2019, Philipp Zabel
// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Vector subtraction and normalization, specialized for P3P solving.
 * @author Philipp Zabel <philipp.zabel@gmail.com>
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup xrt_iface
 */

#pragma once

#include <math.h>


static inline void
vec3_normalise(double *v)
{
	double rnorm = 1.0 / sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	v[0] *= rnorm;
	v[1] *= rnorm;
	v[2] *= rnorm;
}

static inline void
vec3_sub(double *ret, const double *a, const double *b)
{
	ret[0] = a[0] - b[0];
	ret[1] = a[1] - b[1];
	ret[2] = a[2] - b[2];
}
