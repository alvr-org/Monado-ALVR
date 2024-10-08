// Copyright 2022, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Integer low pass filter tests.
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 */

#include <math/m_rational.hpp>

#include "catch_amalgamated.hpp"

#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <queue>

using xrt::auxiliary::math::Rational;
namespace Catch {
template <typename T> struct StringMaker<Rational<T>>
{
	static std::string
	convert(Rational<T> const &value)
	{
		return std::to_string(value.numerator) + "/" + std::to_string(value.denominator);
	}
};
} // namespace Catch

TEMPLATE_TEST_CASE("Rational", "", int32_t, uint32_t, int64_t, uint64_t)
{
	using R = Rational<TestType>;
	using T = TestType;
	CHECK(R{1, 1} == R::simplestUnity());
	CHECK((R::simplestUnity() * T{1}) == R::simplestUnity());
	CHECK((T{1} * R::simplestUnity()) == R::simplestUnity());
	CHECK(R::simplestUnity().as_float() == 1.0f);
	CHECK(R::simplestUnity().as_double() == 1.0);

	CHECK(R{5, 8}.reciprocal() == R{8, 5});

	CHECK(R{5, 8}.complement() == R{3, 8});
	CHECK(R{8, 8}.complement() == R{0, 8});

	CHECK(R{0, 8}.as_float() == 0.0f);
	CHECK(R{0, 8}.as_double() == 0.0);

	CHECK(R{5, 8}.withNonNegativeDenominator() == R{5, 8});

	if constexpr (std::is_signed<TestType>::value) {
		CHECK(R{5, -8}.withNonNegativeDenominator() == R{-5, 8});
		CHECK(R{-5, 8}.withNonNegativeDenominator() == R{-5, 8});

		CHECK(R{-5, 8}.reciprocal() == R{-8, 5});
		CHECK(R{5, -8}.complement() == R{8 + 5, 8});
	}

	{
		R val{5, 8};
		CAPTURE(val);
		CHECK((R::simplestUnity() * val) == val);
		CHECK((val * R::simplestUnity()) == val);
		CHECK((val * T{1}) == val);
		CHECK((T{1} * val) == val);

		CHECK((val * val.reciprocal()).numerator == (val * val.reciprocal()).denominator);
		CHECK((val * val.reciprocal()).isUnity());

		CHECK((val / val).numerator == (val / val).denominator);
		CHECK((val / val).isUnity());

		CHECK((val / T{1}) == val);
	}

	if constexpr (std::is_signed<TestType>::value) {
		R val{5, -8};
		R valNonNegativeDenominator = val.withNonNegativeDenominator();

		CAPTURE(val);
		CHECK((R::simplestUnity() * val) == valNonNegativeDenominator);
		CHECK((val * R::simplestUnity()) == valNonNegativeDenominator);
		CHECK((val * T{1}) == valNonNegativeDenominator);
		CHECK((T{1} * val) == valNonNegativeDenominator);

		CHECK((val * val.reciprocal()).numerator == (val * val.reciprocal()).denominator);
		CHECK((val * val.reciprocal()).isUnity());

		CHECK((val / val).numerator == (val / val).denominator);
		CHECK((val / val).isUnity());

		CHECK((val / T{1}) == valNonNegativeDenominator);
	}

	// Check all our predicates, and our float convesions
	{
		// This is divide by zero error, all should be false
		R val{0, 0};
		CAPTURE(val);
		CHECK_FALSE(val.isZero());
		CHECK_FALSE(val.isBetweenZeroAndOne());
		CHECK_FALSE(val.isUnity());
		CHECK_FALSE(val.isOverUnity());
	}
	{
		R val{0, 8};
		CAPTURE(val);
		CHECK(val.isZero());
		CHECK_FALSE(val.isBetweenZeroAndOne());
		CHECK_FALSE(val.isUnity());
		CHECK_FALSE(val.isOverUnity());

		CHECK(val.as_float() == 0.0f);
		CHECK(val.as_double() == 0.0);
	}

	{
		R val{5, 8};
		CAPTURE(val);
		CHECK_FALSE(val.isZero());
		CHECK(val.isBetweenZeroAndOne());
		CHECK_FALSE(val.isUnity());
		CHECK_FALSE(val.isOverUnity());

		CHECK(val.as_float() > 0.0f);
		CHECK(val.as_float() < 1.0f);
		CHECK(val.as_double() > 0.0);
		CHECK(val.as_double() < 1.0);
	}

	{
		R val{8, 8};
		CAPTURE(val);
		CHECK_FALSE(val.isZero());
		CHECK_FALSE(val.isBetweenZeroAndOne());
		CHECK(val.isUnity());
		CHECK_FALSE(val.isOverUnity());

		CHECK(val.as_float() == 1.0f);
		CHECK(val.as_double() == 1.0);
	}
	{
		R val = R::simplestUnity();
		CAPTURE(val);
		CHECK_FALSE(val.isZero());
		CHECK_FALSE(val.isBetweenZeroAndOne());
		CHECK(val.isUnity());
		CHECK_FALSE(val.isOverUnity());

		CHECK(val.as_float() == 1.0f);
		CHECK(val.as_double() == 1.0);
	}
	{
		R val{8, 5};
		CAPTURE(val);
		CHECK_FALSE(val.isZero());
		CHECK_FALSE(val.isBetweenZeroAndOne());
		CHECK_FALSE(val.isUnity());
		CHECK(val.isOverUnity());

		CHECK(val.as_float() > 1.0f);
		CHECK(val.as_double() > 1.0);
	}
}
