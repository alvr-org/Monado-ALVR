// Copyright 2019-2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Helper for converting LUIDs between different data types
 *
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @ingroup aux_d3d
 */

#pragma once

#include "xrt/xrt_defines.h"
#include "xrt/xrt_windows.h"
#include "xrt/xrt_compiler.h"

#include <cstring>
#include <winrt/Windows.Graphics.h>

#include <stdint.h>
#include <utility>
#include <tuple>

namespace xrt::auxiliary::d3d {

//! Wrapper/interchange type for LUIDs, which identify display adapters on Windows.
struct ConvertibleLuid
{
	//! Construct from a Windows LUID
	explicit ConvertibleLuid(LUID const &luid) : t(luid.LowPart, luid.HighPart) {}

	//! Construct from a WinRT LUID (Windows.Graphics.DisplayAdapterId)
	explicit ConvertibleLuid(::winrt::Windows::Graphics::DisplayAdapterId const &id) : t(id.LowPart, id.HighPart) {}

	//! Construct from a Monado LUID
	explicit ConvertibleLuid(const xrt_luid_t &luid) : ConvertibleLuid(fromMonadoLuid(luid)) {}

	//! Implicit conversion to a Windows LUID
	operator LUID() const
	{
		LUID ret;
		std::tie(ret.LowPart, ret.HighPart) = t;
		return ret;
	}

	//! Implicit conversion to a WinRT LUID (Windows.Graphics.DisplayAdapterId)
	operator ::winrt::Windows::Graphics::DisplayAdapterId() const
	{
		return ::winrt::Windows::Graphics::DisplayAdapterId{std::get<0>(t), std::get<1>(t)};
	}

	//! Implicit conversion to a Monado LUID
	operator xrt_luid_t() const
	{
		return toMonadoLuid(*this);
	}


	friend XRT_CHECK_RESULT bool
	operator==(ConvertibleLuid const &lhs, ConvertibleLuid const &rhs)
	{
		return lhs.t == rhs.t;
	}
	friend XRT_CHECK_RESULT bool
	operator!=(ConvertibleLuid const &lhs, ConvertibleLuid const &rhs)
	{
		return !(lhs == rhs);
	}
	friend XRT_CHECK_RESULT bool
	operator<(ConvertibleLuid const &lhs, ConvertibleLuid const &rhs)
	{
		return lhs.t < rhs.t;
	}

	//! The actual high, low pair describing the LUID.
	std::pair<DWORD, LONG> t;

	static xrt_luid_t
	toMonadoLuid(LUID luid)
	{
		xrt_luid_t ret{};
		std::memcpy(&ret, &luid, sizeof(xrt_luid_t));
		return ret;
	}
	static LUID
	fromMonadoLuid(xrt_luid_t luid)
	{
		LUID ret{};
		std::memcpy(&ret, &luid, sizeof(xrt_luid_t));
		return ret;
	}
};

} // namespace xrt::auxiliary::d3d
