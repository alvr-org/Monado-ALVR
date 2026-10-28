// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of math utilities for OpenVR.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_common
 */

#include "openvr_math.hpp"

#include <cstring>


namespace xrt::state_trackers::openvr {

void
xrtPoseToHmdMatrix34(const xrt_pose &pose, vr::HmdMatrix34_t &openvr_mat)
{
	struct xrt_matrix_4x4 mat;
	math_matrix_4x4_isometry_from_pose(&pose, &mat);

	xrtMatrix4x4ToHmdMatrix34(mat, openvr_mat);
}

void
xrtMatrix4x4ToHmdMatrix34(const xrt_matrix_4x4 &mat, vr::HmdMatrix34_t &openvr_mat)
{
	for (int row = 0; row < 3; row++) {
		for (int col = 0; col < 4; col++) {
			// OpenVR expects row-major, but xrt_matrix_4x4 is column-major, so we swap row and col here
			openvr_mat.m[row][col] = mat.v[col * 4 + row];
		}
	}
}

void
hmdMatrix34Identity(vr::HmdMatrix34_t &mat)
{
	std::memset(&mat, 0, sizeof(mat));
	mat.m[0][0] = 1.0f;
	mat.m[1][1] = 1.0f;
	mat.m[2][2] = 1.0f;
}

}; // namespace xrt::state_trackers::openvr
