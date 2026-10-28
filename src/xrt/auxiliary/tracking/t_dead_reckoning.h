// Copyright 2021-2024, Collabora, Ltd.
// Copyright 2025, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief SLAM tracking code.
 * @author Mateo de Mayo <mateo.demayo@collabora.com>
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup aux_tracking
 */

#include "xrt/xrt_defines.h"

#include "util/u_logging.h"
#include "util/u_time.h"

#include "math/m_filter_fifo.h"


#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Apply dead reckoning to a base relation using IMU data from filter fifos. The filter fifos should be exclusively
 * locked and unmodified during the runtime of this function.
 *
 * @param gyro_ff               The gyro filter fifo.
 * @param accel_ff              The accelerometer filter fifo. Can be NULL to only use gyro.
 * @param gravity_correction    Gravity correction to apply to accelerometer data. Can be NULL if
 *                              accel_ff is NULL.
 * @param when_ns               The timestamp to predict to.
 * @param base_rel              The base relation to start dead reckoning from.
 * @param base_rel_ts           The timestamp of the base relation.
 * @param out_relation          The predicted relation output.
 *
 * @returns Whether the dead reckoning succeeded, we might fail if the base relation is older than the oldest IMU
 *          sample.
 */
bool
t_apply_dead_reckoning(struct m_ff_vec3_f32 *gyro_ff,
                       struct m_ff_vec3_f32 *accel_ff,
                       const struct xrt_vec3 *gravity_correction,
                       timepoint_ns when_ns,
                       const struct xrt_space_relation *base_rel,
                       timepoint_ns base_rel_ts,
                       struct xrt_space_relation *out_relation);

#ifdef __cplusplus
}
#endif
