// Copyright 2019, Philipp Zabel
// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Lambda Twist P3P solver.
 * @author Philipp Zabel <philipp.zabel@gmail.com>
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup xrt_iface
 */

#pragma once

#include "cubic.h"
#include "quadratic.h"
#include "eig3x3known0.h"
#include "refine_lambda.h"
#include "mat.h"
#include "vec.h"

#include <math.h>
#include <string.h>


// @todo remove when clang-format is updated in CI
// clang-format off
/*
 * This implements algorithm 1 "Lambda Twist P3P" described in the paper
 * "Lambda Twist: An Accurate Fast Robust Perspective Three Point (P3P)
 * Solver." by Mikael Persson and Klas Nordberg. See the paper for details.
 *
 * 1: MIX(n, m) = (n, m, n ⨯ m)
 */
#define MIX(n, m)                                                                                                      \
	{n[0], m[0], n[1] * m[2] - n[2] * m[1], n[1], m[1], n[2] * m[0] - n[0] * m[2],                                 \
	 n[2], m[2], n[0] * m[1] - n[1] * m[0]}

#define VEC3_DOT(x, y) (x[0] * y[0] + x[1] * y[1] + x[2] * y[2])
#define VEC3_SCALE(a, x) {a * x[0], a * x[1], a * x[2]}
#define VEC3_SUB(x, y) {x[0] - y[0], x[1] - y[1], x[2] - y[2]}
// clang-format on

/*
 * 2: function P3P(y₁, y₂, y₃, x₁, x₂, x₃)
 */
static inline int
lambdatwist_p3p(const double *iny1,
                const double *iny2,
                const double *iny3, /* vec3 */
                const double *x1,
                const double *x2,
                const double *x3, /* vec3 */
                double (*R)[9],
                double (*t)[3]) /* mat3[4], vec3[4] */
{
	double y1[3], y2[3], y3[3];

	memcpy(y1, iny1, sizeof(y1));
	memcpy(y2, iny2, sizeof(y2));
	memcpy(y3, iny3, sizeof(y3));

	/* 3: Normalize yᵢ */
	vec3_normalise(y1);
	vec3_normalise(y2);
	vec3_normalise(y3);

	/* 4: Compute aᵢⱼ and bᵢⱼ according to (3) */

	/* reusable intermediate values: Δxᵢⱼ = xᵢ - xⱼ */
	const double dx12[3] = VEC3_SUB(x1, x2);
	const double dx13[3] = VEC3_SUB(x1, x3);
	const double dx23[3] = VEC3_SUB(x2, x3);

	/* aᵢⱼ = |xᵢ - xⱼ|² (2) */
	const double a12 = VEC3_DOT(dx12, dx12);
	const double a13 = VEC3_DOT(dx13, dx13);
	const double a23 = VEC3_DOT(dx23, dx23);

	/* bᵢⱼ = yᵢ^T yⱼ (3) */
	const double b12 = VEC3_DOT(y1, y2);
	const double b13 = VEC3_DOT(y1, y3);
	const double b23 = VEC3_DOT(y2, y3);

	/*
	 * 5: Construct D₁ and D₂ from (5) and (6)
	 *
	 *   D₁ = M₁₂a₂₃ - M₂₃a₁₂ = ( d₁₁ d₁₂ d₁₃ )  (5)
	 *   D₂ = M₁₃a₂₃ - M₂₃a₁₃ = ( d₂₁ d₂₂ d₂₃ )  (6)
	 *
	 * with
	 *
	 *         ⎛ 1  -b₁₂ 0 ⎞        ⎛ 1   0 -b₁₃⎞        ⎛ 0   0   0 ⎞
	 *   M₁₂ = ⎜-b₁₂ 1   0 ⎟, M₁₃ = ⎜ 0   0   0 ⎟, M₂₃ = ⎜ 0   1 -b₂₃⎟
	 *         ⎝ 0   0   0 ⎠        ⎝-b₁₃ 0   1 ⎠        ⎝ 0 -b₂₃  1 ⎠
	 *
	 * thus
	 *
	 *        ⎛ a₂₃   -a₂₃b₁₂   0     ⎞       ⎛ a₂₃    0     -a₂₃b₁₃  ⎞
	 *   D₁ = ⎜-a₂₃b₁₂ a₂₃-a₁₂  a₁₂b₂₃⎟, D₂ = ⎜ 0      -a₁₃   a₁₃b₂₃  ⎟
	 *        ⎝ 0      a₁₂b₂₃  -a₁₂   ⎠       ⎝-a₂₃b₁₃ a₁₃b₂₃ a₂₃-a₁₃ ⎠
	 */

	/*
	 * 6: Compute a real root γ to (8)-(10) of the cubic equation
	 *
	 * to find D₀ = D₁ + γD₂, by solving 0 = det(D₁ + λD₂)  (8)
	 *
	 *         ⎛ a₂₃ + γa₂₃  -a₂₃b₁₂            -γa₂₃b₁₃           ⎞
	 *   = det ⎜-a₂₃b₁₂       a₂₃ - a₁₂ - γa₁₃   a₁₂b₂₃ + γa₁₃b₂₃  ⎟
	 *         ⎝-γa₂₃b₁₃      a₁₂b₂₃+γa₁₃b₂₃    -a₁₂ + γ(a₂₃ - a₁₃)⎠
	 *
	 * which is equivalent to
	 *
	 *   c₃γ³ + c₂γ² + c₁γ + c₀ (9)
	 *
	 * with
	 *
	 *   c₃ = det(D₂)
	 *   c₂ = d₂₁^T(d₁₂ ⨯ d₁₃) + d₂₂^T(d₁₃ ⨯ d₁₁) + d₂₃^T(d₁₁ ⨯ d₁₂)
	 *   c₃ = d₁₁^T(d₂₂ ⨯ d₂₃) + d₁₂^T(d₂₃ ⨯ d₂₁) + d₁₃^T(d₂₁ ⨯ d₂₂)
	 *   c₀ = det(D₁)
	 */
	const double a23b123 = 2.0 * a23 * (b12 * b13 * b23 - 1.0);
	const double s12_sq = 1.0 - b12 * b12;
	const double s13_sq = 1.0 - b13 * b13;
	const double s23_sq = 1.0 - b23 * b23;

	const double c3 = a13 * (a13 * s23_sq - a23 * s13_sq);
	const double c2 = a23 * (a23 - a12) * s13_sq + a13 * (a23b123 + (2.0 * a12 + a13) * s23_sq);
	const double c1 = a23 * (a23 - a13) * s12_sq + a12 * (a23b123 + (2.0 * a13 + a12) * s23_sq);
	const double c0 = a12 * (a12 * s23_sq - a23 * s12_sq);

	/*
	 * Since det(D₂) > 0 we can reduce the cubic equation to:
	 *   γ³ + (c₂/c₃)γ² + (c₁/c₃)γ + c₀/c₃
	 */
	const double gamma = reduced_cubic_real_root(c2 / c3, c1 / c3, c0 / c3);

	/*
	 * 7: D₀ = D₁ + γ D₂
	 *
	 *     ⎛ a₂₃ + γa₂₃  -a₂₃b₁₂            -γa₂₃b₁₃           ⎞
	 *   = ⎜-a₂₃b₁₂       a₂₃ - a₁₂ - γa₁₃   a₁₂b₂₃ + γa₁₃b₂₃  ⎟
	 *     ⎝-γa₂₃b₁₃      a₁₂b₂₃ + γa₁₃b₂₃  -a₁₂ + γ(a₂₃ - a₁₃)⎠
	 *
	 * D₀ is symmetric, we only use its upper triangular part.
	 */
	const double D0_00 = a23 + gamma * a23;
	const double D0_01 = -a23 * b12;
	const double D0_02 = -gamma * a23 * b13;
	const double D0_11 = a23 - a12 - gamma * a13;
	const double D0_12 = (a12 + gamma * a13) * b23;
	const double D0_22 = -a12 + gamma * (a23 - a13);
	const double D0[6] = {
	    D0_00, D0_01, D0_02, D0_11, D0_12, D0_22,
	};

	/* 8: [E, σ₁, σ₂] = EIG3X3KNOWN0(D₀). */
	double E[9];
	double sigma[2];
	/* this function does not write E[2], E[5], nor E[8]*/
	eig3x3known0(D0, E, sigma);

	/* 9: s = ±√(-σ₂ / σ₁) */
	const double v = sqrt(-sigma[1] / sigma[0]);

	int k = 0;
	double L[4][3];

	/*
	 * 10: Compute the τₖ > 0 for each s using Eqn (14) with
	 *     coefficients in Eqn (15)
	 */
	double s, tmp, w0, w1, a, b, c, tau1, tau2;
	/* s = +√(-σ₂ / σ₁) */
	s = +v;
	tmp = 1.0 / (s * E[1] - E[0]);
	w0 = (E[3] - s * E[4]) * tmp; /* ω₀ = (e₃ - se₄) / (se₁ - e₀) */
	w1 = (E[6] - s * E[7]) * tmp; /* ω₁ = (e₆ - se₇) / (se₁ - e₀) */
	/*
	 * a = (a₁₃ - a₁₂)ω₁² + 2a₁₂b₁₃ω₁ - a₁₂
	 * b = 2a₁₂b₁₃ω₀ - 2a₁₃b₁₂ω₁ - 2ω₀ω₁(a₁₂ - a₁₃)   (15)
	 * c = a₁₃ + (a₁₃ - a₁₂)ω₀² - 2a₁₃b₁₂ω₀
	 *           ^ this parenthesis was misplaced in the paper
	 */
	a = (a13 - a12) * w1 * w1 + 2.0 * a12 * b13 * w1 - a12;
	b = -2.0 * (a13 * b12 * w1 - a12 * b13 * w0 + w0 * w1 * (a12 - a13));
	c = a13 + (a13 - a12) * w0 * w0 - 2.0 * a13 * b12 * w0;

	// @todo remove when clang-format is updated in CI
	// clang-format off
	/* calculate real roots τ₁, τ₂ of aτ² + bτ + c = 0 */
	if (quadratic_real_roots(a, b, c, &tau1, &tau2)) {
		/*
		 * 11: Compute Λₖ according to Eqn (16), λ₃ₖ = τₖ λ₂ₖ
		 *     and Eqn (13), λ₁ₖ > 0
		 */
/* Macro instead of inline function, gcc produces faster code this way */
#define COMPUTE_LAMBDA(tau)                                                                                            \
	if ((tau) > 0.0)                                                                                               \
		do {                                                                                                   \
			/*                                                                                             \
			 * The solution to equation (16) for λ₂ₖ is:                                              \
			 * λ₂ₖ = √(a₂₃ / (τₖ(-2b₂₃ + τₖ) + 1))                                    \
			 *                   ^^ missing in the paper                                                   \
			 */                                                                                            \
			const double tmp = (tau) * (-2.0 * b23 + (tau)) + 1.0;                                         \
			const double l2 = sqrt(a23 / tmp);                                                             \
			/* λ₃ₖ = τₖλ₂ₖ */                                                                              \
			const double l3 = (tau) * l2;                                                                  \
			/* λ₁ₖ = ω₀λ₂ₖ + ω₁λ₃ₖ (13) */                                                                 \
			const double l1 = w0 * l2 + w1 * l3;                                                           \
			if (l1 > 0.0) {                                                                                \
				L[k][0] = l1;                                                                          \
				L[k][1] = l2;                                                                          \
				L[k][2] = l3;                                                                          \
				k++;                                                                                   \
			}                                                                                              \
	} while (0)
		COMPUTE_LAMBDA(tau1);
		COMPUTE_LAMBDA(tau2);
	}
	// clang-format on

	/* s = -√(-σ₂ / σ₁) */
	s = -v;
	tmp = 1.0 / (s * E[1] - E[0]);
	w0 = (E[3] - s * E[4]) * tmp;
	w1 = (E[6] - s * E[7]) * tmp;
	a = (a13 - a12) * w1 * w1 + 2.0 * a12 * b13 * w1 - a12;
	b = -2.0 * (a13 * b12 * w1 - a12 * b13 * w0 + w0 * w1 * (a12 - a13));
	c = a13 + (a13 - a12) * w0 * w0 - 2.0 * a13 * b12 * w0;
	if (quadratic_real_roots(a, b, c, &tau1, &tau2)) {
		COMPUTE_LAMBDA(tau1);
		COMPUTE_LAMBDA(tau2);
	}

	/* 12: X⁻¹ = (mix(x₁ - x₂, x₁ - x₃))⁻¹ */
	double X[9] = MIX(dx12, dx13);
	mat3_invert(X);

	/* 13: for each valid Λₖ do */
	const int valid = k;
	for (k = 0; k < valid; k++) {
		/* 14: Gauss-Newton-Refine(Λₖ), see Section 3.8 */
		gauss_newton_refineL(L[k], a12, a13, a23, b12, b13, b23);

		const double l1 = L[k][0], l2 = L[k][1], l3 = L[k][2];
		const double l1y1[3] = VEC3_SCALE(l1, y1); /* λ₁ₖ y₁ */
		const double dly12[3] = VEC3_SUB(l1y1, l2 * y2);
		const double dly13[3] = VEC3_SUB(l1y1, l3 * y3);

		/* 15: Yₖ = MIX(λ₁ₖ y₁ - λ₂ₖ y₂, λ₁ₖ y₁ - λ₃ₖ y₃) */
		const double Y[9] = MIX(dly12, dly13);

		/* 16: Rₖ = Yₖ X⁻¹ */
		mat3_mul(R[k], Y, X);

		/* 17: tₖ = λ₁ₖ y₁ - Rₖ x₁ */
		double Rx1[3];
		mat3vec3_mul(Rx1, R[k], x1);
		vec3_sub(t[k], l1y1, Rx1);
	}

	/* 18: Return all Rₖ, tₖ */
	return valid;
}
