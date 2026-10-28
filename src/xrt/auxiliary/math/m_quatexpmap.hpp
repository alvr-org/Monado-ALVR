// Copyright 2019, Collabora, Ltd.
// Copyright 2016, Sensics, Inc.
// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: Apache-2.0
/*!
 * @file
 * @brief  Base implementations for math library.
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup aux_math
 *
 * Based in part on inc/osvr/Util/EigenQuatExponentialMap.h in OSVR-Core
 */
// IWYU pragma: no_include "src/Core/MatrixBase.h"

#pragma once

#include "math/m_api.h"
#include "math/m_eigen_interop.hpp"

#include <Eigen/Core>
#include <Eigen/Geometry>

#include <assert.h>
#include <cmath>


namespace xrt::auxiliary::math {

template <typename Scalar> struct FourthRootMachineEps;

template <> struct FourthRootMachineEps<double>
{
	//! Machine epsilon is 1e-53, so double-precision fourth root is roughly 1e-13
	static double
	get()
	{
		return 1.e-13;
	}
};
template <> struct FourthRootMachineEps<float>
{
	//! Machine epsilon is 1e-24, so double-precision fourth root is roughly 1e-6
	static float
	get()
	{
		return 1.e-6f;
	}
};

/*!
 * Computes the "historical" (un-normalized) sinc(Theta),
 * using `theta^2` to allow us to handle sqrt(0) better for differentiation.
 *
 * (sine(theta)/theta for theta != 0, defined as the limit value of 0 at theta = 0)
 */
template <typename Scalar>
inline Scalar
sinc_sq(Scalar theta_sq)
{
	/*
	 * fourth root of machine epsilon is recommended cutoff for taylor
	 * series expansion vs. direct computation per
	 * Grassia, F. S. (1998). Practical Parameterization of Rotations
	 * Using the Exponential Map. Journal of Graphics Tools, 3(3),
	 * 29-48. http://doi.org/10.1080/10867651.1998.10487493
	 */
	Scalar ret;

	// @note: This should *not* be used in the less than epsilon case,
	//        or else NaNs will leak through autodiff!
	Scalar theta = sqrt(theta_sq);

	if (theta < FourthRootMachineEps<Scalar>::get()) {
		// taylor series expansion.
		ret = Scalar(1.f) - theta_sq / Scalar(6.f);
		return ret;
	}

	// direct computation.
	ret = sin(theta) / theta;
	return ret;
}

/*!
 * Squared vector norm below which the quat maps switch to a series expansion, i.e. a vector norm of 1e-4.
 *
 * The truncated series is good to O(theta^6) there, well inside float precision, and the direct computation is still
 * exact at that magnitude, so one constant serves both float and double.
 */
constexpr double quat_small_sqr_vecnorm = 1.e-8;

/*!
 * Computes cos(theta) given theta^2, staying differentiable at theta = 0.
 */
template <typename Scalar>
inline Scalar
cos_sq(Scalar theta_sq)
{
	if (theta_sq < Scalar(quat_small_sqr_vecnorm)) {
		/*
		 * Differentiating sqrt(0) produces NaN, so expand in theta^2 instead. Truncating after theta^4
		 * leaves an O(theta^6) error, below 1e-24 at this cutoff.
		 */
		return Scalar(1) - theta_sq / Scalar(2) + theta_sq * theta_sq / Scalar(24);
	}

	return cos(sqrt(theta_sq));
}

/*!
 * Fully-templated free function for quaternion exponentiation, SO(3) version as described by Grassia.
 *
 * Implementation inspired by Grassia, F. S. (1998). Practical Parameterization of Rotations Using the Exponential Map.
 * Journal of Graphics Tools, 3(3), 29–48. http://doi.org/10.1080/10867651.1998.10487493
 */
template <typename Derived>
inline Eigen::Quaternion<typename Derived::Scalar>
quat_exp_so3(Eigen::MatrixBase<Derived> const &vec)
{
	EIGEN_STATIC_ASSERT_VECTOR_SPECIFIC_SIZE(Derived, 3);
	using Scalar = typename Derived::Scalar;
	const Scalar theta_sq = vec.squaredNorm();

	// Computing the squared number which is half of squared theta is `theta^2/4`
	const Scalar sq_half_theta = Scalar(0.25) * theta_sq;

	const Scalar vecscale = Scalar(0.5) * sinc_sq(sq_half_theta);
	Eigen::Quaternion<Scalar> ret;
	ret.vec() = vecscale * vec;
	ret.w() = cos_sq(sq_half_theta);
	// @note We don't normalize here since with all valid inputs, the output should be normalized already.
	return ret;
}

/*!
 * Fully-templated free function for quaternion log map, SO(3) version.
 *
 * Assumes a unit quaternion.
 *
 * @note This is the log of the quaternion as given, not of the shortest rotation it represents: a quaternion with
 *       a negative w takes the long way round, returning a rotation vector whose norm exceeds pi. Negate the
 *       coefficients before calling if you want the minimal result, as is usually wanted when the output feeds
 *       an optimizer residual.
 */
template <typename Scalar>
inline Eigen::Matrix<Scalar, 3, 1>
quat_ln_so3(Eigen::Quaternion<Scalar> const &quat)
{
	/*
	 * ln q = ( (phi)/(norm of vec) vec, ln(norm of quat))
	 * When we assume a unit quaternion, ln(norm of quat) = 0
	 * so then we scale the vector part by phi/sin(phi) to get the
	 * result (i.e., ln(qv, qw) = (phi/sin(phi)) * qv )
	 */
	const Scalar sqr_vecnorm = quat.vec().squaredNorm();

	/*
	 * Near phi = 0 the coefficient is 0/0, so expand it as a series instead. Note that this is only the small
	 * angle case when w is positive: a small vector part alongside a negative w means phi is near pi, which the
	 * direct computation below handles exactly.
	 */
	if (sqr_vecnorm < Scalar(quat_small_sqr_vecnorm) && quat.w() > Scalar(0)) {
		/*
		 * atan(s/w)/s = (1/w)(1 - x^2/3 + x^4/5 - ...) for x = s/w. Kept in terms of s^2 so that no square
		 * root is taken: sqrt has an infinite derivative at zero, which would otherwise poison the Jacobian
		 * for autodiff scalars whenever this is handed the identity quaternion.
		 */
		const Scalar x2 = sqr_vecnorm / (quat.w() * quat.w());
		const Scalar phiOverSin = (Scalar(1) - x2 / Scalar(3) + x2 * x2 / Scalar(5)) / quat.w();

		// 2.0 term to produce the physical rotation vector
		return Scalar(2) * (quat.vec() * phiOverSin);
	}

	/*
	 * A zero vector part with a negative w is the identity rotation written antipodally. Every vector of norm 2pi
	 * is an equally valid log of it, so there is no axis to recover; return the identity's log rather than an
	 * arbitrary 360 degree rotation.
	 */
	if (sqr_vecnorm == Scalar(0)) {
		return Eigen::Matrix<Scalar, 3, 1>::Zero();
	}

	const Scalar vecnorm = sqrt(sqr_vecnorm);

	// "best for numerical stability" vs asin or acos
	const Scalar phi = atan2(vecnorm, quat.w());

	/*
	 * The coefficient is nominally phi / sin(phi), but for a unit quaternion sin(phi) is exactly the vector norm
	 * we already have. Dividing by that is cheaper than evaluating sin(atan2(...)), and it stays accurate as phi
	 * approaches pi, where sin(phi) loses its significant digits.
	 *
	 * 2.0 term to produce the physical rotation vector.
	 */
	return Scalar(2) * (quat.vec() * (phi / vecnorm));
}

} // namespace xrt::auxiliary::math
