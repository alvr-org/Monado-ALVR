// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  RANSAC PnP solver
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup tracking
 */

#include "math/m_api.h"
#include "math/m_eigen_interop.hpp"

#include "tracking/t_camera_models.hpp"
#include "tracking/t_constellation.h"

#include "constellation/lambdatwist/lambdatwist_p3p.h"
#include "constellation/pose_metrics.h"
#include "constellation/correspondence_search.h"

#include "ransac.hpp"

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <random>
#include <stdexcept>
#include <numeric>
#include <cmath>


using namespace xrt::auxiliary::math;

namespace {

struct Score
{
public: // Fields
	float sum_sq_error;
	uint32_t inliers;
	xrt_pose pose;

public: // Methods
	Score(float sum_sq_error_, uint32_t inliers_, const xrt_pose &pose_)
	    : sum_sq_error(sum_sq_error_), inliers(inliers_), pose(pose_)
	{}

	bool
	betterThan(const Score &other) const
	{
		if (this->inliers > other.inliers) {
			return true;
		} else if (this->inliers == other.inliers) {
			return this->sum_sq_error < other.sum_sq_error;
		} else {
			return false;
		}
	}
};

}; // namespace

namespace xrt::tracking::constellation::optimizer {

bool
ransacPose(bool deterministic,
           const camera_model &dist,
           const std::vector<t_blob *> &data_points_2d_f32,
           const std::vector<Eigen::Vector3f> &data_points_3d_f32,
           const std::vector<Eigen::Vector3f> &data_points_3d_normals_f32,
           std::vector<size_t> &inlier_indices,
           xrt_pose &out_pose)
{
	if (data_points_2d_f32.size() != data_points_3d_f32.size()) {
		throw std::invalid_argument("data_points_2d_f32 and data_points_3d_f32 must have the same size");
	}

	std::mt19937 rng(deterministic ? 88888888 : std::random_device{}());

	uint32_t num_data_points = data_points_2d_f32.size();

	// Convert into homogeneous coordinates and undistort the 2D points
	std::vector<Eigen::Vector3d> data_points_homog_2d_f64(data_points_2d_f32.size());
	for (size_t i = 0; i < num_data_points; ++i) {
		data_points_homog_2d_f64[i] = map_vec3(data_points_2d_f32[i]->center_homogenized).cast<double>();
	}
	std::vector<float> data_points_homog_max_dist_sq(data_points_2d_f32.size());
	for (size_t i = 0; i < num_data_points; ++i) {
		// @note The size here is technically the size of the blob in *distorted* pixels, but we are using it as
		// a rough estimate of the size in undistorted pixels. This is not perfect, but it should be good enough
		// for RANSAC.
		Eigen::Vector2f size = {data_points_2d_f32[i]->size.x / dist.calib_pinhole.fx,  //
		                        data_points_2d_f32[i]->size.y / dist.calib_pinhole.fy}; //

		data_points_homog_max_dist_sq[i] = size.squaredNorm();
	}

	// Convert 3D points to double precision for lambdatwist
	std::vector<Eigen::Vector3d> data_points_3d_f64(num_data_points);
	for (size_t i = 0; i < num_data_points; ++i) {
		data_points_3d_f64[i] = data_points_3d_f32[i].cast<double>();
	}

	constexpr uint32_t max_k = 1000;    // Absolute maximum number of iterations.
	constexpr float confidence = 0.99f; // We want to be 99% sure we've found a good model.

	// 4 points needed to estimate a model. Fixed to how the algorithm works, don't change.
	constexpr uint32_t n = 4;
	// Amount of iterations, variable, updated iteratively as the model runs.
	uint32_t k = max_k;
	// 3px threshold for inliers on top of the blob size, variable.
	const float t = 3.0f / std::max(dist.calib_pinhole.fx, dist.calib_pinhole.fy);
	// 4 inliers needed to assert that a model fits well to data, variable, but probably don't change.
	constexpr uint32_t d = 4;

	std::vector<size_t> indices;
	indices.resize(num_data_points);
	std::iota(indices.begin(), indices.end(), 0);

	std::array<Eigen::Vector3d, 3> lambda_homog_points_f64;
	std::array<Eigen::Vector3d, 3> lambda_model_positions_f64;
	double Rs[4][9], Ts[4][3];

	Score best_score(std::numeric_limits<float>::max(), 0, XRT_POSE_IDENTITY);
	std::vector<size_t> best_inlier_indices;
	best_inlier_indices.reserve(num_data_points);
	bool found_valid_pose = false;

	std::vector<size_t> current_inlier_indices;
	current_inlier_indices.reserve(num_data_points);

	std::array<size_t, n> random_indices;
	for (uint32_t iteration = 0; iteration < k; iteration++) {
		// Randomly select n data points
		std::sample(indices.begin(), indices.end(), random_indices.begin(), n, rng);

		for (uint32_t i = 0; i < 3; i++) {
			lambda_homog_points_f64[i] = data_points_homog_2d_f64[random_indices[i]];
			lambda_model_positions_f64[i] = data_points_3d_f64[random_indices[i]];
		}

		// Run lambdatwistp3p to get up to 4 possible solutions for R and T
		int valid = lambdatwist_p3p(              //
		    lambda_homog_points_f64[0].data(),    //
		    lambda_homog_points_f64[1].data(),    //
		    lambda_homog_points_f64[2].data(),    //
		    lambda_model_positions_f64[0].data(), //
		    lambda_model_positions_f64[1].data(), //
		    lambda_model_positions_f64[2].data(), //
		    Rs,                                   //
		    Ts);                                  //

		for (int sol = 0; sol < valid; sol++) {
			struct xrt_pose pose;

			// Get the rotation matrix as an Eigen matrix
			Eigen::Map<const Eigen::Matrix<double, 3, 3, Eigen::RowMajor>> rot(Rs[sol]);

			// Convert the rotation matrix to a quaternion and store it in the pose
			map_quat(pose.orientation) = Eigen::Quaterniond(rot).cast<float>();
			// Normalize the orientation
			map_quat(pose.orientation).normalize();

			// Convert the position
			map_vec3(pose.position) = Eigen::Map<Eigen::Vector3d>(Ts[sol]).cast<float>();

			// Skip this solution if it's out of range
			if (pose.position.z < CLOSEST_ACCEPTABLE_POSE_M ||
			    pose.position.z > FARTHEST_ACCEPTABLE_POSE_M) {
				continue;
			}

			bool checks_failed = false;
			for (int p = 0; p < 3; p++) {
				// This pose must yield a projection of the anchor point, or something is really wrong
				Eigen::Vector3f checkpos =
				    map_quat(pose.orientation) * data_points_3d_f32[random_indices[p]] +
				    map_vec3(pose.position);

				// And should be camera facing in this pose
				Eigen::Vector3f checkdir =
				    map_quat(pose.orientation) * data_points_3d_normals_f32[random_indices[p]];

				Eigen::Vector3f checkpos_normalized = checkpos.normalized();
				double facing_dot = checkpos_normalized.dot(checkdir);

				// Require only that the anchor LED not be actively facing away from the camera
				// (that it's at worst perpendicular). Controller LEDs can be visible at that angle
				if (facing_dot > 0.0) {
					// LED not facing the camera -> invalid pose
					checks_failed = true;
					break;
				}

				Eigen::Vector3f tmpblob = lambda_homog_points_f64[p].cast<float>();

				// Calculate the image plane projection of the anchor LED position and check it's
				// within 2.5mm of where it should be, to catch spurious failures in lambdatwist
				Eigen::Vector3f projected_checkpos = checkpos / checkpos.z();
				float l_sq = (projected_checkpos - tmpblob).squaredNorm();
				if (l_sq > ANCHOR_LED_MAX_DISTANCE_M_SQ) {
					checks_failed = true;
					break; // @todo: Figure out why this happened
				}

#if !CHECK_ALL_PROJECTIONS
				break;
#endif
			}

			if (checks_failed) {
				continue;
			}

			// Check against the 4th point to check the proposed P3P solution
			Eigen::Vector3f checkpos_eigen =
			    map_quat(pose.orientation) * data_points_3d_f32[random_indices[3]] +
			    map_vec3(pose.position);
			Eigen::Vector3f projected_checkpos = checkpos_eigen / checkpos_eigen.z();

			// Subtract projected point from undistorted 4th reference point to check error
			const Eigen::Vector3f checkblob = data_points_homog_2d_f64[random_indices[3]].cast<float>();

			float distance_sq = (projected_checkpos - checkblob).squaredNorm();

			// Check that the 4th point projected to within its blob
			if (distance_sq <= data_points_homog_max_dist_sq[random_indices[3]]) {
				// Valid solve! Let's count the inliers.
				Score score(0.0f, 0, pose);
				current_inlier_indices.clear();
				for (size_t datapoint = 0; datapoint < num_data_points; datapoint++) {
					// Transform the 3D point into the camera frame using the estimated pose
					Eigen::Vector3f checkpos =
					    map_quat(pose.orientation) * data_points_3d_f32[datapoint] +
					    map_vec3(pose.position);

					/*!
					 * LEDs can be closer than the device's origin (gated by @ref
					 * CLOSEST_ACCEPTABLE_POSE_M), but we also want to punish LEDs that are *really*
					 * close, since that's almost certainly invalid too.
					 *
					 * We also want to punish LEDs that are *behind* the camera, this does both at
					 * once.
					 */
					const float behind_camera_region_m = 0.015f;

					// behind camera check
					if (checkpos.z() <= behind_camera_region_m) {
						continue;
					}

					// Project the 3D point into the image plane
					Eigen::Vector3f projected_checkpos = checkpos / checkpos.z();

					// Subtract projected point from undistorted reference point to check error
					const Eigen::Vector3f checkblob =
					    data_points_homog_2d_f64[datapoint].cast<float>();
					float distance_sq = (projected_checkpos - checkblob).squaredNorm();
					// Add small margin for blob size for inliers specifically
					if (distance_sq > (data_points_homog_max_dist_sq[datapoint] * 1.5f) ||
					    distance_sq > (t * t)) {
						// Not an inlier
						continue;
					}

					// This is an inlier
					score.inliers++;
					score.sum_sq_error += distance_sq;
					current_inlier_indices.push_back(datapoint);
				}

				// If this solution has more inliers than the best one so far, update the best solution
				if (score.betterThan(best_score) && score.inliers >= d) {
					best_score = score;
					best_inlier_indices = current_inlier_indices;
					found_valid_pose = true;

					if (score.inliers == num_data_points) {
						// All points are inliers, we can stop early
						goto out;
					}

					// Update K with our current best inlier ratio, to reduce the number of
					// iterations needed
					if (best_score.inliers > 0) {
						const float epsilon = 1e-6f;

						// Compute our inlier ratio, avoiding 0 and 1 cus log.
						float w = std::clamp(static_cast<float>(best_score.inliers) /
						                         static_cast<float>(num_data_points),
						                     epsilon, 1.0f - epsilon);

						// Update K to the new computed value
						k = static_cast<uint32_t>(
						    std::ceil(std::log(1.0f - confidence) /
						              std::log(1.0f - std::pow(w, static_cast<float>(n)))));
						// Clamp K to the maximum number of iterations
						k = std::clamp(k, 1u, max_k);
					}
				}
			} else {
				// Not a valid solve
				continue;
			}
		}
	}
out:

	// Update the output parameters with the best solution.
	inlier_indices = std::move(best_inlier_indices);
	out_pose = best_score.pose;

	return found_valid_pose;
}

}; // namespace xrt::tracking::constellation::optimizer
