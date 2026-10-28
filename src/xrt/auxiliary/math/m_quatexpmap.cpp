// Copyright 2019, Collabora, Ltd.
// Copyright 2016, Sensics, Inc.
// SPDX-License-Identifier: Apache-2.0
/*!
 * @file
 * @brief  Base implementations for math library.
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @ingroup aux_math
 *
 * Based in part on inc/osvr/Util/EigenQuatExponentialMap.h in OSVR-Core
 */
// IWYU pragma: no_include "src/Core/MatrixBase.h"

#include "math/m_api.h"
#include "math/m_eigen_interop.hpp"

#include "m_quatexpmap.hpp"

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <assert.h>


using namespace xrt::auxiliary::math;

extern "C" void
math_quat_integrate_velocity(const struct xrt_quat *quat,
                             const struct xrt_vec3 *ang_vel,
                             const float dt,
                             struct xrt_quat *result)
{
	assert(quat != NULL);
	assert(ang_vel != NULL);
	assert(result != NULL);
	assert(dt != 0);


	Eigen::Quaternionf q = map_quat(*quat);
	Eigen::Quaternionf incremental_rotation = quat_exp_so3(map_vec3(*ang_vel) * dt).normalized();
	map_quat(*result) = q * incremental_rotation;
}

extern "C" void
math_quat_finite_difference(const struct xrt_quat *quat0,
                            const struct xrt_quat *quat1,
                            const float dt,
                            struct xrt_vec3 *out_ang_vel)
{
	assert(quat0 != NULL);
	assert(quat1 != NULL);
	assert(out_ang_vel != NULL);
	assert(dt != 0);


	Eigen::Quaternionf inc_quat = map_quat(*quat1) * map_quat(*quat0).conjugate();
	map_vec3(*out_ang_vel) = quat_ln_so3(inc_quat) / dt;
}

extern "C" void
math_quat_exp_so3(const struct xrt_vec3 *axis_angle, struct xrt_quat *out_quat)
{
	map_quat(*out_quat) = quat_exp_so3(map_vec3(*axis_angle));
}

extern "C" void
math_quat_ln_so3(const struct xrt_quat *quat, struct xrt_vec3 *out_axis_angle)
{
	Eigen::Quaternionf eigen_quat = map_quat(*quat);
	map_vec3(*out_axis_angle) = quat_ln_so3(eigen_quat);
}
