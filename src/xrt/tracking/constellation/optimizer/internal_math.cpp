// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Generic math helpers for constellation optimizer code.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup tracking
 */

#include "internal_math.hpp"
#include "imu_preintegration.hpp"


namespace xrt::tracking::constellation::optimizer {

/*
 *
 * Helper functions
 *
 */

//! The matrix which cross-products with v, so skew(v) * u == v.cross(u).
static Matrix3d
skew(const Vector3d &v)
{
	Matrix3d m;
	m << 0.0, -v.z(), v.y(), //
	    v.z(), 0.0, -v.x(),  //
	    -v.y(), v.x(), 0.0;  //
	return m;
}

/*!
 * The right Jacobian of SO(3), which converts a perturbation of a rotation vector into the body-frame rotation it
 * produces: `Exp(phi + delta) ~= Exp(phi) * Exp(rightJacobianSo3(phi) * delta)`.
 *
 * @param phi The rotation vector to evaluate at, in radians.
 */
static Matrix3d
rightJacobianSo3(const Vector3d &phi)
{
	const double theta_squared = phi.squaredNorm();
	const Matrix3d phi_skew = skew(phi);

	double skew_coefficient;
	double skew_squared_coefficient;

	/*
	 * Below a ten thousandth of a radian the closed forms below divide two quantities which are both heading
	 * for zero, losing most of their significant digits to the cancellation. The leading Taylor terms are
	 * exact to well past double precision by the time the angle is this small.
	 */
	constexpr double kSmallAngleSquared = 1e-8;

	if (theta_squared < kSmallAngleSquared) {
		skew_coefficient = 0.5 - (theta_squared / 24.0);
		skew_squared_coefficient = (1.0 / 6.0) - (theta_squared / 120.0);
	} else {
		const double theta = std::sqrt(theta_squared);

		skew_coefficient = (1.0 - std::cos(theta)) / theta_squared;
		skew_squared_coefficient = (theta - std::sin(theta)) / (theta_squared * theta);
	}

	return Matrix3d::Identity()            //
	       - (skew_coefficient * phi_skew) //
	       + (skew_squared_coefficient * phi_skew * phi_skew);
}

template <typename Derived>
static Matrix<double, Derived::RowsAtCompileTime * Derived::ColsAtCompileTime, ImuBias<double>::kNumParameters>
extractJacobianFromJetMatrix(const MatrixBase<Derived> &m)
{
	static_assert(std::is_same_v<typename Derived::Scalar, ImuBiasJet>, "Matrix scalar must be ImuBiasJet");

	constexpr int kRows = Derived::RowsAtCompileTime;
	constexpr int kCols = Derived::ColsAtCompileTime;
	Matrix<double, kRows * kCols, ImuBias<double>::kNumParameters> J;

	for (int r = 0; r < kRows; ++r) {
		for (int c = 0; c < kCols; ++c) {
			for (int j = 0; j < ImuBias<double>::kNumParameters; ++j) {
				J(r * kCols + c, j) = m(r, c).v[j];
			}
		}
	}

	return J;
}

template <int N>
static Matrix3d
quaternionJetToSO3Jacobian(const Quaternion<ceres::Jet<double, N>> &q_jet, int parameter_offset)
{
	Quaterniond q(q_jet.w().a, q_jet.x().a, q_jet.y().a, q_jet.z().a);

	q.normalize();

	Quaterniond q_inv = q.conjugate();

	Matrix3d J;

	for (int j = 0; j < 3; j++) {
		Quaterniond dq(q_jet.w().v[parameter_offset + j], q_jet.x().v[parameter_offset + j],
		               q_jet.y().v[parameter_offset + j], q_jet.z().v[parameter_offset + j]);

		Quaterniond local = q_inv * dq;

		J.col(j) = 2.0 * Vector3d(local.x(), local.y(), local.z());
	}

	return J;
}

/*!
 * Propagates the covariance for this sample.
 *
 * @param[in]  Q_start_cur     Rotation accumulated *before* this sample.
 * @param[in]  corrected_accel Bias and scale-corrected acceleration, in the IMU frame.
 * @param[in]  corrected_gyro  Bias-corrected gyroscope reading.
 * @param[in]  accel_scale     The accelerometer scale.
 * @param      dt              Delta time
 * @param[out] covariance      Out covariance matrix
 */
static void
propagateCovariance(const Quaterniond &Q_start_cur,
                    const Vector3d &corrected_accel,
                    const Vector3d &corrected_gyro,
                    const Vector3d &accel_scale,
                    double dt,
                    PreintegrationCovarianceMatrix &covariance)
{
	if (dt <= 0.0) {
		// A duplicated sample timestamp contributes nothing and would divide by zero below.
		return;
	}

	const Matrix3d dR = Q_start_cur.toRotationMatrix();
	const Matrix3d accel_skew = skew(corrected_accel);
	const Matrix3d scale = accel_scale.asDiagonal();

	/*
	 * How the error already accumulated gets reshuffled by this sample's integration. The off-diagonal
	 * blocks are where a rotation error leaks into velocity and position: point the accelerometer slightly
	 * wrong and you integrate the acceleration slightly sideways.
	 */
	Matrix<double, kNumImuBiasCovIndices, kNumImuBiasCovIndices> A =
	    Matrix<double, kNumImuBiasCovIndices, kNumImuBiasCovIndices>::Identity();
	A.block<3, 3>(kImuBiasRotCovIndex, kImuBiasRotCovIndex) = //
	    quat_exp_so3(Vector3d(-corrected_gyro * dt)).toRotationMatrix();
	A.block<3, 3>(kImuBiasVelCovIndex, kImuBiasRotCovIndex) = //
	    -dR * accel_skew * dt;
	A.block<3, 3>(kImuBiasPosCovIndex, kImuBiasRotCovIndex) = //
	    -0.5 * dR * accel_skew * dt * dt;
	A.block<3, 3>(kImuBiasPosCovIndex, kImuBiasVelCovIndex) = //
	    Matrix3d::Identity() * dt;

	/*
	 * How this sample's sensor noise enters. The gyro noise turns into a rotation through the right Jacobian.
	 * The accel noise is scaled along with the signal, since the scale is applied to the reading.
	 */
	Matrix<double, kNumImuBiasCovIndices, 6> B = Matrix<double, kNumImuBiasCovIndices, 6>::Zero();
	B.block<3, 3>(kImuBiasRotCovIndex, 0) = rightJacobianSo3(corrected_gyro * dt) * dt;
	B.block<3, 3>(kImuBiasVelCovIndex, 3) = dR * scale * dt;
	B.block<3, 3>(kImuBiasPosCovIndex, 3) = 0.5 * dR * scale * dt * dt;

	/*
	 * Density squared over the interval this sample covers, so the partial steps at the keyframe
	 * boundaries contribute proportionally less than a full 1ms one.
	 */
	Matrix<double, 6, 6> noise = Matrix<double, 6, 6>::Zero();
	noise.block<3, 3>(0, 0) = Matrix3d::Identity() * (kGyroNoiseDensity * kGyroNoiseDensity / dt);
	noise.block<3, 3>(3, 3) = Matrix3d::Identity() * (kAccelNoiseDensity * kAccelNoiseDensity / dt);

	covariance = A * covariance * A.transpose() + B * noise * B.transpose();
}

static void
integrateSingleSample(const xrt_imu_sample &sample,
                      double dt,
                      const Vector3<ImuBiasJet> &accel_bias,
                      const Vector3<ImuBiasJet> &gyro_bias,
                      const Vector3<ImuBiasJet> &accel_scale,
                      Quaternion<ImuBiasJet> &Q_start_cur,
                      Vector3<ImuBiasJet> &delta_velocity,
                      Vector3<ImuBiasJet> &delta_position,
                      PreintegrationCovarianceMatrix &covariance)
{
	// Convert the sample to a Jet type
	Vector3<ImuBiasJet> cur_accel = map_vec3_f64(sample.accel_m_s2).cast<ImuBiasJet>();
	Vector3<ImuBiasJet> cur_gyro = map_vec3_f64(sample.gyro_rad_secs).cast<ImuBiasJet>();

	// Apply the bias and scale to the accelerometer and gyroscope readings.
	cur_accel = (cur_accel - accel_bias).cwiseProduct(accel_scale);
	cur_gyro = cur_gyro - gyro_bias;

	{
		/*
		 * Propagate the uncertainty across this sample. Only the real parts matter here.
		 */
		const Vector3d real_accel = {cur_accel.x().a, cur_accel.y().a, cur_accel.z().a};
		const Vector3d real_gyro = {cur_gyro.x().a, cur_gyro.y().a, cur_gyro.z().a};
		const Vector3d real_scale = {accel_scale.x().a, accel_scale.y().a, accel_scale.z().a};
		const Quaterniond real_Q_start_cur = {Q_start_cur.w().a, Q_start_cur.x().a, //
		                                      Q_start_cur.y().a, Q_start_cur.z().a};

		propagateCovariance(real_Q_start_cur, //
		                    real_accel,       //
		                    real_gyro,        //
		                    real_scale,       //
		                    dt,               //
		                    covariance);      //
	}

	// Rotate accel into starting frame, since it's in IMU frame when passed to us.
	const auto start_accel = Q_start_cur * cur_accel;

	// I'm sure we've *never* seen these equations before.
	delta_position += delta_velocity * dt + ImuBiasJet(0.5) * start_accel * dt * dt;
	delta_velocity += start_accel * dt;

	// Integrate gyro into the delta rotation.
	Q_start_cur *= quat_exp_so3(cur_gyro * dt);
}

/*!
 * Decompose a symmetric positive semi-definite @p a into a square root and the transposed pseudo-inverse of that
 * square root, dropping the eigenvalues that are numerically indistinguishable from zero so that a rank-deficient
 * input still gives usable factors instead of infinities.
 *
 * With the eigendecomposition `a = E S E^T`, this hands back `out_j = S^1/2 E^T` and `out_j_pinv_t = S^-1/2 E^T`,
 * so that `out_j^T * out_j == a` and `out_j_pinv_t == (out_j^-1)^T`.
 *
 * This is what OKVIS does at the end of MarginalizationError::updateErrorComputation(),
 * minus its Jacobi preconditioner: @p out_j is its `J_` and @p out_j_pinv_t its `J_pinv_T`.
 */
static void
robustComputeSqrtAndPinvSqrt(const MatrixXd &a, MatrixXd &out_j, MatrixXd &out_j_pinv_t)
{
	// lhs SVD: a = J^T * J = E * S * E^T
	Eigen::SelfAdjointEigenSolver<MatrixXd> saes(a);

	const auto &eigenvectors = saes.eigenvectors();
	const auto &eigenvalues = saes.eigenvalues();

	constexpr double epsilon = std::numeric_limits<double>::epsilon();
	/*
	 * Clamped at zero, because an all-zero or (numerically) negative definite input would otherwise give a
	 * negative tolerance, letting negative eigenvalues through and putting NaNs in the prior.
	 */
	const double tolerance = epsilon * static_cast<double>(a.cols()) * std::max(eigenvalues.maxCoeff(), 0.0);

	const auto keep = eigenvalues.array() > tolerance;

	const VectorXd S = keep.select(eigenvalues.array(), 0);
	const VectorXd S_pinv = keep.select(eigenvalues.array().inverse(), 0);

	const VectorXd S_sqrt = S.cwiseSqrt();
	const VectorXd S_pinv_sqrt = S_pinv.cwiseSqrt();

	out_j = S_sqrt.asDiagonal() * eigenvectors.transpose();
	out_j_pinv_t = S_pinv_sqrt.asDiagonal() * eigenvectors.transpose();
}

/*
 *
 * Exported functions
 *
 */

FactorFitness
evaluateFactorFitness(ceres::Problem &problem, const std::vector<ceres::ResidualBlockId> &residual_blocks)
{
	if (residual_blocks.empty()) {
		return {.num_residuals = 0, .chi_squared = 0.0};
	}

	ceres::Problem::EvaluateOptions evaluate_options{};
	evaluate_options.residual_blocks = residual_blocks;
	evaluate_options.apply_loss_function = false;

	double cost = 0.0;
	std::vector<double> residuals;
	if (!problem.Evaluate(evaluate_options, &cost, &residuals, nullptr, nullptr)) {
		return {.num_residuals = 0, .chi_squared = 0.0};
	}

	// Ceres reports half the sum of squares, matching its cost convention.
	return {.num_residuals = residuals.size(), .chi_squared = cost * 2.0};
}

void
matrixCrsToEigen(const ceres::CRSMatrix &mat_crs, Eigen::MatrixXd &mat)
{
	// Clear out the Eigen matrix
	mat.resize(mat_crs.num_rows, mat_crs.num_cols);
	mat.setZero();

	// Fill with the sparse matrix
	for (int Row = 0; Row < mat_crs.num_rows; Row++) {
		int Start = mat_crs.rows[Row];
		int End = mat_crs.rows[Row + 1];

		for (int n = Start; n < End; n++) {
			int Col = mat_crs.cols[n];

			mat(Row, Col) = mat_crs.values[n];
		}
	}
}

void
marginalize(
    const VectorXd &residual, const MatrixXd &jacobian, const int num_base_states, VectorXd &out_e0, MatrixXd &out_j)
{
	// Inflation disabled for now, but enable if this ends up being an issue later. 1.01 is a good value to try.
	constexpr double kHessianInflation = 1.00;

	const MatrixXd H = jacobian.transpose() * jacobian;

	const VectorXd b0 = -jacobian.transpose() * residual;

	const int num_states = static_cast<int>(H.cols());
	const int num_remain_states = num_states - num_base_states;

	// Split the lhs...
	const MatrixXd V = H.block(0, 0, num_base_states, num_base_states);
	const MatrixXd U = H.block(num_base_states, num_base_states, num_remain_states, num_remain_states);
	const MatrixXd W = H.block(num_base_states, 0, num_remain_states, num_base_states);

	// ...and the rhs
	const VectorXd b_b = b0.head(num_base_states);
	const VectorXd b_a = b0.tail(num_remain_states);

	Eigen::CompleteOrthogonalDecomposition<MatrixXd> V_decomp(V);
	/*
	 * Use a pseudo-inverse for the marginalization block so that rank-deffecient matrices don't produce invalid
	 * inverses.
	 */
	const MatrixXd V_pinv = V_decomp.pseudoInverse();

	// Schur
	MatrixXd H_s = U - W * V_pinv * W.transpose();
	VectorXd b0_s = b_a - W * V_pinv * b_b;

	if (kHessianInflation != 1.00) {
		// Add uncertainty to help prevent accumulation of error
		H_s /= kHessianInflation;
		b0_s /= kHessianInflation;
	}

	/*
	 * The Schur complement is symmetric in exact arithmetic only, and SelfAdjointEigenSolver silently reads
	 * just one triangle, so fold the two halves together instead of letting one of them decide the result.
	 */
	H_s = MatrixXd(0.5 * (H_s + H_s.transpose()));

	/*
	 * Compute the new linear factor:
	 *
	 * J^* = sqrt(H^*)
	 * r^* = -sqrt(H^*)^-T * b^*
	 *
	 * |J^* x + r^*|^2
	 */
	MatrixXd j_pinv_t;
	robustComputeSqrtAndPinvSqrt(H_s, out_j, j_pinv_t);

	// robustComputeSqrtAndPinvSqrt() hands back sqrt(H^*)^-T directly, so no transpose here.
	out_e0 = -j_pinv_t * b0_s;
}

PreintegratedImuSamples
preintegrate(const std::span<const xrt_imu_sample> &imu_samples,
             const xrt_imu_sample &first_sample_after_end_time,
             const ImuBias<ImuBiasJet> &imu_bias,
             timepoint_ns start_time_ns,
             timepoint_ns end_time_ns)
{
	assert(imu_samples.size() >= 1);

	Quaternion<ImuBiasJet> Q_start_cur = Quaternion<ImuBiasJet>::Identity();
	Vector3<ImuBiasJet> delta_velocity = Vector3<ImuBiasJet>::Zero();
	Vector3<ImuBiasJet> delta_position = Vector3<ImuBiasJet>::Zero();

	const auto &accel_bias = imu_bias.accel_bias;
	const auto &gyro_bias = imu_bias.gyro_bias;
	const auto &accel_scale = imu_bias.accel_scale;

	PreintegrationCovarianceMatrix covariance = PreintegrationCovarianceMatrix::Zero();

	auto &first_sample = imu_samples.front();
	auto &last_sample = imu_samples.back();

	// Assert all samples are within the sliding window
	assert(first_sample.timestamp_ns >= start_time_ns);
	assert(last_sample.timestamp_ns <= end_time_ns);

	// Interage the first sample in
	if (first_sample.timestamp_ns > start_time_ns) {
		// Interpolate the first sample to the start time.
		double dt = static_cast<double>(first_sample.timestamp_ns - start_time_ns) * 1e-9;

		integrateSingleSample(first_sample, dt, accel_bias, gyro_bias, accel_scale, Q_start_cur, delta_velocity,
		                      delta_position, covariance);
	}

	// Integrate all samples in the sliding window.
	for (size_t i = 0; i < imu_samples.size() - 1; i++) {
		const auto &sample = imu_samples[i];

		const auto &next_sample = imu_samples[i + 1];
		double dt = static_cast<double>(next_sample.timestamp_ns - sample.timestamp_ns) * 1e-9;

		integrateSingleSample(next_sample, dt, accel_bias, gyro_bias, accel_scale, Q_start_cur, delta_velocity,
		                      delta_position, covariance);
	}

	// Integrate the last sample to end time, if necessary.
	if (last_sample.timestamp_ns < end_time_ns) {
		double dt = static_cast<double>(end_time_ns - last_sample.timestamp_ns) * 1e-9;

		integrateSingleSample(first_sample_after_end_time, dt, accel_bias, gyro_bias, accel_scale, Q_start_cur,
		                      delta_velocity, delta_position, covariance);
	}

	// Extract out the jacobians from Jet states.
	const auto J_v = extractJacobianFromJetMatrix(delta_velocity);
	const auto J_p = extractJacobianFromJetMatrix(delta_position);

	PreintegratedImuSamples preintegrated = {
	    .Q_start_end = {Q_start_cur.w().a, Q_start_cur.x().a, Q_start_cur.y().a, Q_start_cur.z().a},
	    .delta_velocity = {delta_velocity[0].a, delta_velocity[1].a, delta_velocity[2].a},
	    .delta_position = {delta_position[0].a, delta_position[1].a, delta_position[2].a},

	    .whitening = {},

	    .dt = static_cast<double>(end_time_ns - start_time_ns) * 1e-9,

	    .accel_bias_at_integration = {accel_bias[0].a, accel_bias[1].a, accel_bias[2].a},
	    .gyro_bias_at_integration = {gyro_bias[0].a, gyro_bias[1].a, gyro_bias[2].a},

	    .accel_scale_at_integration = {accel_scale[0].a, accel_scale[1].a, accel_scale[2].a},

	    .J_R_bg = quaternionJetToSO3Jacobian(Q_start_cur, ImuBias<double>::kGyroBiasIndex),

	    .J_v_ba = J_v.block<3, 3>(0, ImuBias<double>::kAccelBiasIndex),
	    .J_v_sa = J_v.block<3, 3>(0, ImuBias<double>::kAccelScaleIndex),
	    .J_v_bg = J_v.block<3, 3>(0, ImuBias<double>::kGyroBiasIndex),

	    .J_p_ba = J_p.block<3, 3>(0, ImuBias<double>::kAccelBiasIndex),
	    .J_p_sa = J_p.block<3, 3>(0, ImuBias<double>::kAccelScaleIndex),
	    .J_p_bg = J_p.block<3, 3>(0, ImuBias<double>::kGyroBiasIndex),
	};

	// Sigma = L L^T, so the whitener is L^-1 — a triangular solve, not an inverse.
	Eigen::LLT<PreintegrationCovarianceMatrix> llt(covariance);
	if (llt.info() != Eigen::Success) {
		/*
		 * Nothing sane to weight with. Matching the camera factor's sentinel, make it contribute almost
		 * nothing rather than contribute a full-strength bogus factor.
		 */
		preintegrated.whitening = PreintegrationCovarianceMatrix::Identity() * 1e-3;
	} else {
		preintegrated.whitening = llt.matrixL().solve(PreintegrationCovarianceMatrix::Identity());
	}

	return preintegrated;
}

}; // namespace xrt::tracking::constellation::optimizer
