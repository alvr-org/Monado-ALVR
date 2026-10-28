// Copyright 2021-2024, Collabora, Ltd.
// Copyright 2025, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief SLAM tracking code.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Mateo de Mayo <mateo.demayo@collabora.com>
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup aux_tracking
 */

#include "xrt/xrt_defines.h"

#include "util/u_logging.h"
#include "util/u_time.h"

#include "math/m_filter_fifo.h"
#include "math/m_vec3.h"
#include "math/m_predict.h"
#include "math/m_api.h"

#include "t_dead_reckoning.h"


bool
t_apply_dead_reckoning(struct m_ff_vec3_f32 *gyro_ff,
                       struct m_ff_vec3_f32 *accel_ff,
                       const struct xrt_vec3 *gravity_correction,
                       timepoint_ns when_ns,
                       const struct xrt_space_relation *base_rel,
                       timepoint_ns base_rel_ts,
                       struct xrt_space_relation *out_relation)
{
	bool using_accel = accel_ff != NULL;

	// @todo Make this determine the correct starting sample for gyro and accelerometer separately
	// Find oldest imu index i that is newer than latest SLAM pose (or -1)
	int i = 0;
	uint64_t imu_ts = UINT64_MAX;
	while (m_ff_vec3_f32_get_timestamp(gyro_ff, i, &imu_ts)) {
		if ((int64_t)imu_ts < base_rel_ts) {
			i--; // Back to the oldest newer-than-SLAM IMU index (or -1)
			break;
		}
		i++;
	}

	if (i == -1) {
		U_LOG_W("No IMU samples received after latest SLAM pose (and frame)");
	}

	struct xrt_space_relation integ_rel = *base_rel;
	timepoint_ns integ_rel_ts = base_rel_ts;
	struct xrt_quat *orient = &integ_rel.pose.orientation;
	struct xrt_vec3 *pos = &integ_rel.pose.position;
	struct xrt_vec3 *ang_vel = &integ_rel.angular_velocity;
	struct xrt_vec3 *lin_vel = &integ_rel.linear_velocity;
	bool clamped = false; // If when_ns is older than the latest IMU ts

	while (i >= 0) { // Decreasing i increases timestamp
		// Get samples
		struct xrt_vec3 gyro = XRT_VEC3_ZERO;
		struct xrt_vec3 accel = XRT_VEC3_ZERO;
		uint64_t gyro_ts = 0;
		uint64_t accel_ts = 0;
		bool got = true;
		got &= m_ff_vec3_f32_get(gyro_ff, i, &gyro, &gyro_ts);
		if (using_accel) {
			got &= m_ff_vec3_f32_get(accel_ff, i, &accel, &accel_ts);
		}
		timepoint_ns ts = gyro_ts;


		// Checks
		if (ts > when_ns) {
			clamped = true;
			//! @todo Instead of using same a and g values, do an interpolated sample like this:
			// a = prev_a + ((when_ns - prev_ts) / (ts - prev_ts)) * (a - prev_a);
			// g = prev_g + ((when_ns - prev_ts) / (ts - prev_ts)) * (g - prev_g);
			ts = when_ns; // clamp ts to when_ns
		}

		// No SLAM samples within the window of the gyro buffer.
		if (!got) {
			U_LOG_T("Failed to get %s sample at index %d for timestamp %" PRIu64,
			        using_accel ? "gyro and accel" : "gyro", i, ts);
			return false;
		}

		if (using_accel && gyro_ts != accel_ts) {
			U_LOG_E("Failure getting synced gyro and accel samples at index %d: gyro ts %" PRIu64
			        " accel ts %" PRIu64,
			        i, gyro_ts, accel_ts);
			return false;
		}

		if (ts < base_rel_ts) {
			U_LOG_E("IMU sample at %" PRIu64 " is older than latest SLAM pose at %" PRIu64, ts,
			        base_rel_ts);
			return false;
		}

		// Update time
		float dt = (float)time_ns_to_s(ts - integ_rel_ts);
		integ_rel_ts = ts;

		// Integrate gyroscope
		struct xrt_quat angvel_delta = XRT_QUAT_IDENTITY;
		struct xrt_vec3 scaled_g = m_vec3_mul_scalar(gyro, dt);
		math_quat_exp_so3(&scaled_g, &angvel_delta);         // Same as using math_quat_from_angle_vector(g/dt)
		math_quat_rotate(orient, &angvel_delta, orient);     // Orientation
		math_quat_rotate_derivative(orient, &gyro, ang_vel); // Angular velocity

		if (using_accel) {
			// Integrate accelerometer
			struct xrt_vec3 world_accel = XRT_VEC3_ZERO;
			math_quat_rotate_vec3(orient, &accel, &world_accel);
			world_accel = m_vec3_add(world_accel, *gravity_correction);
			*lin_vel = m_vec3_add(*lin_vel, m_vec3_mul_scalar(world_accel, dt)); // Linear velocity
			const struct xrt_vec3 accumulated_position_change = m_vec3_add(      //
			    m_vec3_mul_scalar(*lin_vel, dt),                                 //
			    m_vec3_mul_scalar(world_accel, dt * dt * 0.5f));                 //

			math_vec3_accum(&accumulated_position_change, pos);
		}

		if (clamped) {
			break;
		}
		i--;
	}

	// Do the prediction based on the updated relation
	double last_imu_to_now_dt = time_ns_to_s(when_ns - integ_rel_ts);
	struct xrt_space_relation predicted_relation = XRT_SPACE_RELATION_ZERO;
	m_predict_relation(&integ_rel, last_imu_to_now_dt, &predicted_relation);

	*out_relation = predicted_relation;

	return true;
}
