// Copyright 2021, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Test C++ quatexpmap interface.
 * @author Mateo de Mayo <mateo.demayo@collabora.com>
 */

#include "catch_amalgamated.hpp"

#include "math/m_api.h"
#include "math/m_vec3.h"

#include "tinyceres/jet.hpp"

#include "math/m_quatexpmap.hpp"
#include "math/m_quatexpmap_tinyceres.hpp"

#include <vector>

using std::vector;
using namespace xrt::auxiliary::math;

TEST_CASE("m_quatexpmap")
{
	xrt_vec3 axis1 = m_vec3_normalize({4, -7, 3});
	xrt_vec3 axis2 = m_vec3_normalize({-1, -2, -3});
	xrt_vec3 axis3 = m_vec3_normalize({1, -1, 1});
	xrt_vec3 axis4 = m_vec3_normalize({-11, 23, 91});
	SECTION("Test integrate velocity and finite difference mappings")
	{
		vector<xrt_vec3> q1_axes{{axis1, axis2}};
		float q1_angle = (float)GENERATE(M_PI, -M_PI / 6);
		vector<xrt_vec3> vel_axes{{axis3, axis4}};
		float vel_angle = (float)GENERATE(-M_PI, M_PI / 5);
		float dt = (float)GENERATE(0.01, 0.1, 1);

		for (xrt_vec3 q1_axis : q1_axes) {
			for (xrt_vec3 vel_axis : vel_axes) {
				// First orientation q1
				xrt_quat q1{};
				math_quat_from_angle_vector(q1_angle, &q1_axis, &q1);

				// Second orientation q2: q1 rotated by vel_angle*dt radians around its local vel_axis
				xrt_quat q2{};
				xrt_vec3 vel = vel_axis * vel_angle;
				math_quat_integrate_velocity(&q1, &vel, dt, &q2);

				// Global velocity vector from q1 to q2
				xrt_vec3 new_global_vel{};
				math_quat_finite_difference(&q1, &q2, dt, &new_global_vel);

				// Adjust global velocity back to local (w.r.t. q1)
				xrt_quat inv_q1{};
				xrt_vec3 new_vel{};
				math_quat_invert(&q1, &inv_q1);
				math_quat_rotate_derivative(&inv_q1, &new_global_vel, &new_vel);

				INFO("vel=" << vel.x << ", " << vel.y << ", " << vel.z);
				INFO("new_vel=" << new_vel.x << ", " << new_vel.y << ", " << new_vel.z);
				CHECK(m_vec3_len(new_vel - vel) <= 0.001);
			}
		}
	}

	SECTION("Test quat_ln_so3 with antipodal identity rotation returns zero vector")
	{
		Eigen::Quaterniond quat = {-1, 0, 0, 0};
		auto result = quat_ln_so3(quat);
		CHECK(result == Eigen::Vector3d::Zero());
	}

	SECTION("Test quat_exp_so3 and quat_ln_so3 are inverses")
	{
		// We use rotations with less than PI radians as quat_ln will return the negative rotation otherwise
		auto aa = GENERATE_COPY(xrt_vec3{0, 0, 0},            //
		                        axis1 * (float)M_PI * 0.01f,  //
		                        axis2 * (float)M_PI * 0.5f,   //
		                        axis3 * (float)M_PI * 0.99f); //

		xrt_quat quat{};
		math_quat_exp_so3(&aa, &quat);

		xrt_vec3 expected_aa{};
		math_quat_ln_so3(&quat, &expected_aa);

		CHECK(m_vec3_len(expected_aa - aa) <= 0.001);

		map_quat(quat) = quat_exp_so3(map_vec3(aa));
		map_vec3(expected_aa) = quat_ln_so3(Eigen::Quaternionf(map_quat(quat)));

		CHECK(m_vec3_len(expected_aa - aa) <= 0.001);
	}

	SECTION("quat_ln_so3 produces sane derivatives")
	{
		// Identity is a special case that we need to be sure not to produce NaN derivatives for
		Eigen::Quaternion<ceres::Jet<double, 4>> identity_quat = {{1, 0}, {0, 1}, {0, 2}, {0, 3}};
		// A simple 180 degree rotation
		Eigen::Quaternion<ceres::Jet<double, 4>> rotation_around_y = {{0, 0}, {0, 1}, {1, 2}, {0, 3}};

		auto quats = {identity_quat, rotation_around_y};

		for (const auto &quat : quats) {
			auto aa = xrt::auxiliary::math::quat_ln_so3(quat);

			CHECK(aa.allFinite());

			CHECK(aa.x().v.allFinite());
			CHECK(aa.y().v.allFinite());
			CHECK(aa.z().v.allFinite());
		}
	}

	SECTION("quat_exp_so3 produces sane derivatives")
	{
		Eigen::Vector3<ceres::Jet<double, 3>> identity_rot = {{0, 0}, {0, 1}, {0, 2}};
		Eigen::Vector3<ceres::Jet<double, 3>> some_rot = {{0.3, 0}, {0.1, 1}, {0.2, 2}};

		auto vecs = {identity_rot, some_rot};

		for (const auto &vec : vecs) {
			auto quat = quat_exp_so3(vec);

			CHECK(quat.coeffs().allFinite());

			CHECK(quat.w().v.allFinite());
			CHECK(quat.x().v.allFinite());
			CHECK(quat.y().v.allFinite());
			CHECK(quat.z().v.allFinite());
		}
	}

	SECTION("Test quat_exp_so3(angle_axis) returns the appropriate quaternion")
	{
		float angle = M_PI_2;
		xrt_vec3 axis = axis4;
		xrt_vec3 aa = axis * angle;
		xrt_quat q{};
		// @note: Since quaternions store half-rotations, we use quat_exp_so3 here to store the exact physical
		//        rotation from the axisangle. `quat_exp` would produce a quaternion with 2x real rotation.
		map_quat(q) = quat_exp_so3(map_vec3(aa));

		CHECK(fabsf(q.x - (axis.x * sin(angle / 2))) <= 0.001);
		CHECK(fabsf(q.y - (axis.y * sin(angle / 2))) <= 0.001);
		CHECK(fabsf(q.z - (axis.z * sin(angle / 2))) <= 0.001);
		CHECK(fabsf(q.w - (cos(angle / 2))) <= 0.001);
	}

	SECTION(
	    "Test quat_exp_so3<double>(angle_axis) with norm less than quat_small_sqr_vecnorm returns the appropriate "
	    "quaternion")
	{
		/*
		 * Angles small enough that theta^2/4 stays under quat_small_sqr_vecnorm, so the cosine comes from
		 * its series expansion instead of the direct computation. The smallest one also takes the sinc
		 * series in double precision.
		 */
		double angle = GENERATE(0.0, 1.e-14, 1.e-6, 1.e-4);
		xrt_vec3 axisf = GENERATE_COPY(axis1, axis4);
		Eigen::Vector3d axis = map_vec3(axisf).cast<double>();
		Eigen::Vector3d aa = axis * angle;
		INFO("angle=" << angle);

		// Assert the squared norm / 4 is actually less than the cutoff.
		REQUIRE(0.25 * aa.squaredNorm() < quat_small_sqr_vecnorm);

		Eigen::Quaterniond q = quat_exp_so3(aa);

		auto approx = [](double expected) { return Catch::Approx(expected).epsilon(1.e-12); };

		// The direct computation is still exact at this magnitude, so it is fine to compare against.
		double half_sin = sin(angle / 2);

		CHECK(q.x() == approx(axis.x() * half_sin));
		CHECK(q.y() == approx(axis.y() * half_sin));
		CHECK(q.z() == approx(axis.z() * half_sin));
		CHECK(q.w() == approx(cos(angle / 2)));

		// Assert it output a unity quaternion
		CHECK(q.norm() == approx(1));

		// Assert the logarithm gets it back
		Eigen::Vector3d recovered = quat_ln_so3(q);
		CHECK((recovered - aa).norm() <= 1.e-12 * angle);
	}

	SECTION(
	    "Test quat_exp_so3<float>(angle_axis) with norm less than quat_small_sqr_vecnorm returns the appropriate "
	    "quaternion")
	{
		/*
		 * Angles small enough that theta^2/4 stays under quat_small_sqr_vecnorm, so the cosine comes from
		 * its series expansion instead of the direct computation. The smallest one also takes the sinc
		 * series in double precision.
		 */
		float angle = GENERATE(0.0, 1.e-14, 1.e-6, 1.e-4);
		xrt_vec3 axisf = GENERATE_COPY(axis1, axis4);
		Eigen::Vector3f axis = map_vec3(axisf);
		Eigen::Vector3f aa = axis * angle;
		INFO("angle=" << angle);

		// Assert the squared norm / 4 is actually less than the cutoff.
		REQUIRE(0.25 * aa.squaredNorm() < quat_small_sqr_vecnorm);

		Eigen::Quaternionf q = quat_exp_so3(aa);

		auto approx = [](double expected) { return Catch::Approx(expected).epsilon(1.e-5); };

		// The direct computation is still exact at this magnitude, so it is fine to compare against.
		float half_sin = sinf(angle / 2);

		CHECK(q.x() == approx(axis.x() * half_sin));
		CHECK(q.y() == approx(axis.y() * half_sin));
		CHECK(q.z() == approx(axis.z() * half_sin));
		CHECK(q.w() == approx(cosf(angle / 2)));

		// Assert it output a unity quaternion
		CHECK(q.norm() == approx(1));

		// Assert the logarithm gets it back
		Eigen::Vector3f recovered = quat_ln_so3(q);
		CHECK((recovered - aa).norm() <= 1.e-5 * angle);
	}
}
