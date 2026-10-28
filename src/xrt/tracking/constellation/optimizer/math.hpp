// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Generic math helpers for constellation optimizer code. Must not include Eigen.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup tracking
 */

#pragma once

#include <cmath>


namespace xrt::tracking::constellation::optimizer {

/*
 *
 * Types
 *
 */

/*!
 * How well one kind of sensor was fit, so that a bad solve can be attributed to the sensor that could not be
 * explained rather than guessed at from the parameter values.
 *
 * Both factors are whitened, so their residuals are in standard deviations and these numbers are comparable to
 * each other and to 1. A `mean_chi_squared` far above 1 means the model cannot explain that sensor's data.
 */
struct FactorFitness
{
public: // Fields
	//! How many residual rows this kind of factor contributes.
	size_t num_residuals;
	//! Sum of the squared whitened residuals, `2 * cost`.
	double chi_squared;

	//! Chi squared per residual row. Around 1 when the data matches the model and its noise model.
	double
	meanChiSquared() const
	{
		return this->num_residuals > 0 ? this->chi_squared / static_cast<double>(this->num_residuals) : 0.0;
	}

	//! The average size of a residual, in standard deviations.
	double
	rmsSigma() const
	{
		return std::sqrt(this->meanChiSquared());
	}
};

}; // namespace xrt::tracking::constellation::optimizer
