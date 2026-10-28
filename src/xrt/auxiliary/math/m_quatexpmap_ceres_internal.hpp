// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: Apache-2.0
/*!
 * @file
 * @brief  Helpers for quatexpmap math for ceres usage. Internal header to be included by one of the wrappers which
 *         provide the correct Ceres types depending on what the including module is using.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup aux_math
 */
// IWYU pragma: no_include "src/Core/MatrixBase.h"

#pragma once

#include "m_quatexpmap.hpp"


namespace xrt::auxiliary::math {

template <typename T, int N> struct FourthRootMachineEps<ceres::Jet<T, N>>
{
	static constexpr ceres::Jet<T, N>
	get()
	{
		return ceres::Jet<T, N>(FourthRootMachineEps<T>::get());
	}
};

} // namespace xrt::auxiliary::math
