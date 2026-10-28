// Copyright 2023, Collabora, Ltd.
// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Camera (un)projection C++ API for various camera models.
 * @author Moshi Turner <moshiturner@protonmail.com>
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup aux_tracking
 */

#pragma once

#include "math/m_vec2.h"
#include "math/m_matrix_2x2.h"
#include "math/m_mathinclude.h"

#include "t_tracking.h"

#include "tracking/t_camera_models.h"


namespace xrt::auxiliary::tracking::camera_models {

static constexpr double kSqrtEpsilon = 0.00316; // sqrt(1e-05)

// We're doing a lot of these, so let's have a macro.
#define CAST(x) static_cast<T>(x)

// We could use Eigen here, but custom types keeps this file lean and quick to compile,
// along with giving us more control.
template <typename T> struct Vector2
{
public: // Fields
	T x;
	T y;

public: // Methods
	inline Vector2<T>
	sub(const Vector2<T> &other) const
	{
		return {this->x - other.x, this->y - other.y};
	}

	inline T
	length() const
	{
		return sqrt(this->x * this->x + this->y * this->y);
	}
};

template <typename T> struct Matrix2x2
{
public: // Fields
	T v[4];

public: // Methods
	inline void
	invert(Matrix2x2<T> &invertedMatrix) const
	{
		T determinant = v[0] * v[3] - v[1] * v[2];
		invertedMatrix.v[0] = v[3] / determinant;
		invertedMatrix.v[1] = -v[1] / determinant;
		invertedMatrix.v[2] = -v[2] / determinant;
		invertedMatrix.v[3] = v[0] / determinant;
	}

	inline void
	transformVector2(const Vector2<T> &vec, Vector2<T> &result_out) const
	{
		result_out.x = v[0] * vec.x + v[1] * vec.y;
		result_out.y = v[2] * vec.x + v[3] * vec.y;
	}
};

/*
 * Functions for @ref T_DISTORTION_PINHOLE
 */

template <typename T>
static inline bool
pinhole_project(const t_camera_model_params &dist, //
                const T x,                         //
                const T y,                         //
                const T z,                         //
                T &out_x,                          //
                T &out_y)
{
	out_x = ((CAST(dist.fx) * x / z) + CAST(dist.cx));
	out_y = ((CAST(dist.fy) * y / z) + CAST(dist.cy));

	bool is_valid = z >= kSqrtEpsilon;
	return is_valid;
}

template <typename T>
static inline bool
pinhole_unproject(const t_camera_model_params &dist, //
                  const T x,                         //
                  const T y,                         //
                  T &out_x,                          //
                  T &out_y,                          //
                  T &out_z)
{
	const T mx = (x - CAST(dist.cx)) / CAST(dist.fx);
	const T my = (y - CAST(dist.cy)) / CAST(dist.fy);

	const T r2 = mx * mx + my * my;

	const T norm = sqrt(CAST(1.0) + r2);

	const T norm_inv = CAST(1.0) / norm;

	out_x = mx * norm_inv;
	out_y = my * norm_inv;
	out_z = norm_inv;

	// Pinhole unprojection is always valid :)
	return true;
}

/*
 * Functions for @ref T_DISTORTION_FISHEYE_KB4 (un)projections
 */

template <typename T>
static inline T
kb4_calc_r_theta(const t_camera_model_params &dist, //
                 const T &theta,                    //
                 const T &theta2)
{
	T r_theta = CAST(dist.fisheye.k4) * theta2;
	r_theta += CAST(dist.fisheye.k3);
	r_theta *= theta2;
	r_theta += CAST(dist.fisheye.k2);
	r_theta *= theta2;
	r_theta += CAST(dist.fisheye.k1);
	r_theta *= theta2;
	r_theta += CAST(1.0);
	r_theta *= theta;

	return r_theta;
}

template <typename T>
static inline bool
kb4_project(const t_camera_model_params &dist, //
            const T &x,                        //
            const T &y,                        //
            const T &z,                        //
            T &out_x,                          //
            T &out_y)
{
	const T r2 = x * x + y * y;
	const T r = sqrt(r2);

	if (r > kSqrtEpsilon) {
		const T theta = atan2(r, z);
		const T theta2 = theta * theta;

		T r_theta = kb4_calc_r_theta(dist, theta, theta2);

		const T mx = x * r_theta / r;
		const T my = y * r_theta / r;

		out_x = CAST(dist.fx) * mx + CAST(dist.cx);
		out_y = CAST(dist.fy) * my + CAST(dist.cy);

		return true;
	} else {
		out_x = CAST(dist.fx) * x / z + CAST(dist.cx);
		out_y = CAST(dist.fy) * y / z + CAST(dist.cy);

		// The projection is only valid if the point is not close to the zero norm.
		return z >= kSqrtEpsilon;
	}

	assert(!"Unreachable");
}

template <typename T>
static inline T
kb4_solve_theta(const t_camera_model_params &dist, const T &r_theta, T *d_func_d_theta)
{
	T theta = r_theta;
	for (int i = 4; i > 0; i--) {
		T theta2 = theta * theta;

		T func = CAST(dist.fisheye.k4) * theta2;
		func += CAST(dist.fisheye.k3);
		func *= theta2;
		func += CAST(dist.fisheye.k2);
		func *= theta2;
		func += CAST(dist.fisheye.k1);
		func *= theta2;
		func += CAST(1.0);
		func *= theta;

		(*d_func_d_theta) = CAST(9.0 * dist.fisheye.k4) * theta2;
		(*d_func_d_theta) += CAST(7.0 * dist.fisheye.k3);
		(*d_func_d_theta) *= theta2;
		(*d_func_d_theta) += CAST(5.0 * dist.fisheye.k2);
		(*d_func_d_theta) *= theta2;
		(*d_func_d_theta) += CAST(3.0 * dist.fisheye.k1);
		(*d_func_d_theta) *= theta2;
		(*d_func_d_theta) += CAST(1.0);

		// Iteration of Newton method
		theta += (r_theta - func) / (*d_func_d_theta);
	}

	return theta;
}

template <typename T>
static inline bool
kb4_unproject(const t_camera_model_params &dist, //
              const T &x,                        //
              const T &y,                        //
              T &out_x,                          //
              T &out_y,                          //
              T &out_z)
{
	const T mx = (x - CAST(dist.cx)) / CAST(dist.fx);
	const T my = (y - CAST(dist.cy)) / CAST(dist.fy);

	T theta = CAST(0.0);
	T sin_theta = CAST(0.0);
	T cos_theta = CAST(1.0);
	T thetad = sqrt(mx * mx + my * my);
	T scaling = CAST(1.0);
	T d_func_d_theta = CAST(0.0);

	if (thetad > kSqrtEpsilon) {
		theta = kb4_solve_theta(dist, thetad, &d_func_d_theta);

		sin_theta = sin(theta);
		cos_theta = cos(theta);
		scaling = sin_theta / thetad;
	}

	out_x = mx * scaling;
	out_y = my * scaling;
	out_z = cos_theta;

	//! @todo I'm not 100% sure if kb4 is always non-injective. basalt-headers always returns true here,
	//!       so it might be wrong too.
	return true;
}

template <typename T>
static inline void
kb4_undistort(const t_camera_model_params &dist, const T &x, const T &y, T &out_x, T &out_y)
{
	T xp, yp, zp;

	kb4_unproject(dist, x, y, xp, yp, zp);

	out_x = xp / zp;
	out_y = yp / zp;
}

/*
 * Functions for radial-tangential (un)projections
 */

template <typename T>
static inline bool
rt8_project(const t_camera_model_params &dist, //
            const T &x,                        //
            const T &y,                        //
            const T &z,                        //
            T &out_x,                          //
            T &out_y)
{
	const T xp = x / z;
	const T yp = y / z;
	const T rp2 = xp * xp + yp * yp;
	const T cdist = (CAST(1.0) + rp2 * (CAST(dist.rt8.k1) + rp2 * (CAST(dist.rt8.k2) + rp2 * CAST(dist.rt8.k3)))) /
	                (CAST(1.0) + rp2 * (CAST(dist.rt8.k4) + rp2 * (CAST(dist.rt8.k5) + rp2 * CAST(dist.rt8.k6))));
	const T deltaX = CAST(2.0f * dist.rt8.p1) * xp * yp + CAST(dist.rt8.p2) * (rp2 + CAST(2.0) * xp * xp);
	const T deltaY = CAST(2.0f * dist.rt8.p2) * xp * yp + CAST(dist.rt8.p1) * (rp2 + CAST(2.0) * yp * yp);
	const T xpp = xp * cdist + deltaX;
	const T ypp = yp * cdist + deltaY;
	const T u = CAST(dist.fx) * xpp + CAST(dist.cx);
	const T v = CAST(dist.fy) * ypp + CAST(dist.cy);

	out_x = u;
	out_y = v;

	const float rpmax = dist.rt8.metric_radius;

	bool positive_z = z >= kSqrtEpsilon; // Sophus::Constants<Scalar>::epsilonSqrt();
	bool in_injective_area = rpmax == 0.0 ? true : rp2 <= rpmax * rpmax;
	bool is_valid = positive_z && in_injective_area;

	return is_valid;
}

template <typename T>
static inline void
rt8_distort(const t_camera_model_params &params,
            const Vector2<T> &undist,
            Vector2<T> &out_dist,
            Matrix2x2<T> &out_d_dist_d_undist)
{
	const T k1 = CAST(params.rt8.k1);
	const T k2 = CAST(params.rt8.k2);
	const T p1 = CAST(params.rt8.p1);
	const T p2 = CAST(params.rt8.p2);
	const T k3 = CAST(params.rt8.k3);
	const T k4 = CAST(params.rt8.k4);
	const T k5 = CAST(params.rt8.k5);
	const T k6 = CAST(params.rt8.k6);

	const T xp = undist.x;
	const T yp = undist.y;
	const T rp2 = xp * xp + yp * yp;
	const T cdist = (CAST(1.0) + rp2 * (k1 + rp2 * (k2 + rp2 * k3))) / //
	                (CAST(1.0) + rp2 * (k4 + rp2 * (k5 + rp2 * k6)));  //
	const T deltaX = CAST(2.0) * p1 * xp * yp + p2 * (rp2 + CAST(2.0) * xp * xp);
	const T deltaY = CAST(2.0) * p2 * xp * yp + p1 * (rp2 + CAST(2.0) * yp * yp);
	const T xpp = xp * cdist + deltaX;
	const T ypp = yp * cdist + deltaY;
	out_dist.x = xpp;
	out_dist.y = ypp;

	// Jacobian part!
	// Expressions derived with sympy
	const T v0 = xp * xp;
	const T v1 = yp * yp;
	const T v2 = v0 + v1;
	const T v3 = k6 * v2;
	const T v4 = k4 + v2 * (k5 + v3);
	const T v5 = v2 * v4 + CAST(1.0);
	const T v6 = v5 * v5;
	const T v7 = CAST(1.0) / v6;
	const T v8 = p1 * yp;
	const T v9 = p2 * xp;
	const T v10 = CAST(2.0) * v6;
	const T v11 = k3 * v2;
	const T v12 = k1 + v2 * (k2 + v11);
	const T v13 = v12 * v2 + CAST(1.0);
	const T v14 = v13 * (v2 * (k5 + CAST(2.0) * v3) + v4);
	const T v15 = CAST(2.0) * v14;
	const T v16 = v12 + v2 * (k2 + CAST(2.0) * v11);
	const T v17 = CAST(2.0) * v16;
	const T v18 = xp * yp;
	const T v19 = CAST(2.0) * v7 * (-v14 * v18 + v16 * v18 * v5 + v6 * (p1 * xp + p2 * yp));

	const T dxpp_dxp = v7 * (-v0 * v15 + v10 * (v8 + CAST(3.0) * v9) + v5 * (v0 * v17 + v13));
	const T dxpp_dyp = v19;
	const T dypp_dxp = v19;
	const T dypp_dyp = v7 * (-v1 * v15 + v10 * (CAST(3.0) * v8 + v9) + v5 * (v1 * v17 + v13));

	out_d_dist_d_undist.v[0] = dxpp_dxp;
	out_d_dist_d_undist.v[1] = dxpp_dyp;
	out_d_dist_d_undist.v[2] = dypp_dxp;
	out_d_dist_d_undist.v[3] = dypp_dyp;
}

template <typename T>
static inline void
rt8_undistort(const t_camera_model_params &params, const T &u, const T &v, T &out_x, T &out_y)
{
	const T x0 = (u - CAST(params.cx)) / CAST(params.fx);
	const T y0 = (v - CAST(params.cy)) / CAST(params.fy);

	//! @todo Decide if besides rpmax, it could be useful to have an rppmax
	//! field. A good starting point to having this would be using the sqrt of
	//! the max rpp2 value computed in the optimization of `computeRpmax()`.

	// Newton solver
	Vector2<T> dist = {x0, y0};
	Vector2<T> undist = dist;

	const int N = 5; // Max iterations
	for (int i = 0; i < N; i++) {
		Matrix2x2<T> J;
		Vector2<T> fundist;

		rt8_distort(params, undist, fundist, J);
		Vector2<T> residual = fundist.sub(dist);

		// fundist - dist;
		Matrix2x2<T> J_inverse;

		J.invert(J_inverse);

		Vector2<T> undist_sub;

		J_inverse.transformVector2(residual, undist_sub);

		undist = undist.sub(undist_sub);
		if (residual.length() < kSqrtEpsilon) {
			break;
		}
	}

	out_x = undist.x;
	out_y = undist.y;
}

template <typename T>
static inline bool
rt8_unproject(const t_camera_model_params &params, const T &u, const T &v, T &out_x, T &out_y, T &out_z)
{
	T xp, yp;
	rt8_undistort(params, u, v, xp, yp);

	const T norm_inv = CAST(1.0) / sqrt(xp * xp + yp * yp + CAST(1.0));
	out_x = xp * norm_inv;
	out_y = yp * norm_inv;
	out_z = norm_inv;

	const T rp2 = xp * xp + yp * yp;
	bool in_injective_area =
	    params.rt8.metric_radius == 0.0f ? true : rp2 <= CAST(params.rt8.metric_radius * params.rt8.metric_radius);
	bool is_valid = in_injective_area;

	return is_valid;
}

/*
 * Functions for @ref T_DISTORTION_RIFT_CV1 (un)projections
 */

//! Radial undistortion scale s = |ray| / |p| for a distorted normalized image point of radius @p r.
template <typename T>
static inline T
cv1_radial_undistort_scale(const t_camera_model_params &dist, const T &r)
{
	if (r == CAST(0.0)) {
		return CAST(1.0);
	}

	// The image radius r is used directly as the field angle: t = tan(r). (KB4 would use atan(r).)
	const T t = tan(r);
	const T t2 = t * t;

	// P(t) = 1 + k1 t^2 + k2 t^4 + k3 t^6 + k4 t^8 (Oculus d1..d4), Horner form.
	T poly = CAST(dist.cv1.k4) * t2;
	poly += CAST(dist.cv1.k3);
	poly *= t2;
	poly += CAST(dist.cv1.k2);
	poly *= t2;
	poly += CAST(dist.cv1.k1);
	poly *= t2;
	poly += CAST(1.0);

	return (t / r) / poly;
}

//! Tangential ("decentering") delta with the CV1 4th-order affine gain (p1, p2, g3, g4), for a scaled point @p q.
template <typename T>
static inline void
cv1_decentering_delta(const t_camera_model_params &dist, const T &qx, const T &qy, T &out_dx, T &out_dy)
{
	const T p1 = CAST(dist.cv1.p1);
	const T p2 = CAST(dist.cv1.p2);
	const T rq2 = qx * qx + qy * qy;

	T dx = (CAST(2.0) * qx * qx + rq2) * p1 + CAST(2.0) * p2 * qx * qy;
	T dy = (CAST(2.0) * qy * qy + rq2) * p2 + CAST(2.0) * p1 * qx * qy;

	// Affine gain g = 1 + g3 rq^2 + g4 rq^4, applied to the decentering delta.
	const T gain = CAST(1.0) + rq2 * (CAST(dist.cv1.g3) + rq2 * CAST(dist.cv1.g4));

	out_dx = dx * gain;
	out_dy = dy * gain;
}

//! Maps a distorted image-space point (@p x, @p y) to a pinhole ray tangent (x/z, y/z).
template <typename T>
static inline void
cv1_undistort(const t_camera_model_params &dist, const T &x, const T &y, T &out_x, T &out_y)
{
	// Normalize the distorted pixel onto the sensor plane. CV1 uses a single focal length, so
	// fx == fy here.
	const T px = (x - CAST(dist.cx)) / CAST(dist.fx);
	const T py = (y - CAST(dist.cy)) / CAST(dist.fy);

	const T r = sqrt(px * px + py * py);
	const T scale = cv1_radial_undistort_scale(dist, r);

	const T qx = scale * px;
	const T qy = scale * py;

	T dx, dy;
	cv1_decentering_delta(dist, qx, qy, dx, dy);

	out_x = qx + dx;
	out_y = qy + dy;
}

// This is a very common name, so make sure to undef it.
#undef CAST

/*
 * Exported C++ functions.
 */

template <typename T>
bool
project(const t_camera_model_params &dist, //
        const T &x,                        //
        const T &y,                        //
        const T &z,                        //
        T &out_x,                          //
        T &out_y)
{
	switch (dist.model) {
	case T_DISTORTION_PINHOLE: {
		return pinhole_project(dist, x, y, z, out_x, out_y);
	} break;
	case T_DISTORTION_OPENCV_RADTAN_8: {
		return rt8_project(dist, x, y, z, out_x, out_y);
	}; break;
	case T_DISTORTION_FISHEYE_KB4: {
		return kb4_project(dist, x, y, z, out_x, out_y);
	}; break;
	case T_DISTORTION_RIFT_CV1: {
		// Dummy value so we aren't returning uninitialized values.
		out_x = T(dist.fx) * x / z + T(dist.cx);
		out_y = T(dist.fy) * y / z + T(dist.cy);

		// No projection path is present for CV1
		return false;
	}; break;
	// Return false so we don't get warnings on Release builds.
	default: assert(false); return false;
	}
}

template <typename T>
void
undistort(const t_camera_model_params &dist, const T &x, const T &y, T &out_x, T &out_y)
{
	switch (dist.model) {
	case T_DISTORTION_PINHOLE: {
		// Like the other models, output normalized image coordinates.
		out_x = (x - T(dist.cx)) / T(dist.fx);
		out_y = (y - T(dist.cy)) / T(dist.fy);
	}; break;
	case T_DISTORTION_OPENCV_RADTAN_8: {
		rt8_undistort(dist, x, y, out_x, out_y);
	}; break;
	case T_DISTORTION_FISHEYE_KB4: {
		kb4_undistort(dist, x, y, out_x, out_y);
	}; break;
	case T_DISTORTION_RIFT_CV1: {
		cv1_undistort(dist, x, y, out_x, out_y);
	}; break;
	// Return false so we don't get warnings on Release builds.
	default: assert(false);
	}
}

}; // namespace xrt::auxiliary::tracking::camera_models
