// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Implementation of math utilities for OpenVR.
 *
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_common
 */

#pragma once

#include "math/m_api.h"

#include "openvr_interfaces_unified.h"


namespace xrt::state_trackers::openvr {

/*!
 * Converts an xrt_pose to an OpenVR HmdMatrix34, which is used for representing poses in OpenVR. This involves
 * converting the xrt_pose to a 4x4 isometry matrix, and then dropping the last row to fit it into the 3x4 OpenVR
 * format.
 *
 * @param pose The xrt_pose to convert.
 * @param openvr_mat The output OpenVR HmdMatrix34_t to write the result into.
 */
void
xrtPoseToHmdMatrix34(const xrt_pose &pose, vr::HmdMatrix34_t &openvr_mat);

/*!
 * Converts a 4x4 matrix in column-major format (as used by xrt_matrix_4x4) to an OpenVR HmdMatrix34, which is a
 * 3x4 matrix in row-major format. This involves dropping the last row of the 4x4 matrix, and transposing the data to
 * convert from column-major to row-major.
 *
 * @param mat The xrt_matrix_4x4 to convert.
 * @param openvr_mat The output OpenVR HmdMatrix34_t to write the result into.
 */
void
xrtMatrix4x4ToHmdMatrix34(const xrt_matrix_4x4 &mat, vr::HmdMatrix34_t &openvr_mat);

/*!
 * Sets an OpenVR HmdMatrix34_t to the identity matrix. This is a 3x4 matrix where the upper-left 3x3 portion is the
 * identity rotation, and the rightmost column is the zero translation.
 *
 * @param mat The OpenVR HmdMatrix34_t to set to the identity matrix.
 */
void
hmdMatrix34Identity(vr::HmdMatrix34_t &mat);

}; // namespace xrt::state_trackers::openvr
