// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  RANSAC PnP solver
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup tracking
 */

#pragma once

#include "tracking/t_camera_models.hpp"
#include "tracking/t_constellation.h"

#include <Eigen/Core>
#include <Eigen/Geometry>

#include "constellation/camera_model.h"


namespace xrt::tracking::constellation::optimizer {

bool
ransacPose(bool deterministic,
           const camera_model &dist,
           const std::vector<t_blob *> &data_points_2d_f32,
           const std::vector<Eigen::Vector3f> &data_points_3d_f32,
           const std::vector<Eigen::Vector3f> &data_points_3d_normals_f32,
           std::vector<size_t> &inlier_indices,
           xrt_pose &out_pose);

}; // namespace xrt::tracking::constellation::optimizer
