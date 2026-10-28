// Copyright 2025, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Internal Blubur S1 driver definitions.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_blubur_s1
 */

#pragma once

#include "os/os_hid.h"
#include "os/os_threading.h"

#include "util/u_distortion_mesh.h"
#include "util/u_logging.h"

#include "math/m_imu_3dof.h"
#include "math/m_relation_history.h"

#include "blubur_s1_interface.h"
#include "blubur_s1_protocol.h"


struct blubur_s1_hmd
{
	struct xrt_device base;

	enum u_logging_level log_level;

	struct u_poly_3k_eye_values poly_3k_values[2];

	struct os_hid_device *dev;
	struct os_thread_helper thread;

	struct m_imu_3dof fusion_3dof;

	uint16_t last_remote_timestamp_ms;
	timepoint_ns last_remote_timestamp_ns;

	time_duration_ns hw2mono;
	int hw2mono_samples;

	struct m_relation_history *relation_history;

	struct os_mutex input_mutex;

	struct
	{
		enum blubur_s1_status_bits status;
	} input;
};
