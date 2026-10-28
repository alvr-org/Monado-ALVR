// Copyright 2019-2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Helpers for Windows 10+ direct mode code.
 *
 * Has to implement comp_target instead of comp_target_swapchain because
 * we don't get a VkSurfaceKHR, etc: we manually import images instead.
 *
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @ingroup aux_d3d
 */

#pragma once

#include "math/m_rational.hpp"

#include "xrt/xrt_compiler.h"
#include "xrt/xrt_vulkan_includes.h"

#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Devices.Display.Core.h>

#include <stdint.h>
#include <utility>
#include <tuple>

namespace xrt::auxiliary::d3d::winrt {

static inline xrt::auxiliary::math::Rational<uint32_t>
parseVerticalRefreshRate(const ::winrt::Windows::Devices::Display::Core::DisplayModeInfo &modeInfo)
{
	return xrt::auxiliary::math::Rational<uint32_t>{modeInfo.PresentationRate().VerticalSyncRate.Numerator,
	                                                modeInfo.PresentationRate().VerticalSyncRate.Denominator};
}

bool
isModeAcceptable(const ::winrt::Windows::Devices::Display::Core::DisplayModeInfo &modeInfo);

/*!
 * A comparison function (like std::less) for comparing display modes, such as for
 * sorting or finding the "best" one.
 *
 * This sorts with SRGB modes first, and refresh rate in decreasing order.
 */
bool
modeComparison(const ::winrt::Windows::Devices::Display::Core::DisplayModeInfo &lhs,
               const ::winrt::Windows::Devices::Display::Core::DisplayModeInfo &rhs);

static inline bool
colorSpaceFromVulkan(VkColorSpaceKHR colorSpace, ::winrt::Windows::Graphics::DirectX::DirectXColorSpace &out)
{
	using ::winrt::Windows::Graphics::DirectX::DirectXColorSpace;
	switch (colorSpace) {
	case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR: out = DirectXColorSpace::RgbFullG22NoneP709; return true;
	case VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT: out = DirectXColorSpace::RgbFullG10NoneP709; return true;
	}
	return false;
}

/*!
 * Describes the supported Windows API features for direct mode.
 */
struct SystemApiCapability
{
	//! Supports the minimum WinRT API contract (7.0) for direct mode
	bool supportsBasicDirectMode{};

	//! Supports the WinRT API contract (14.0) for the improved direct mode that debuted in Windows 11
	bool supportsScanoutOptionsAndTryExecuteTask{};

	//! Populate the fields based on the system we are running on
	void
	populate();
};

/*!
 * Create a "scanout" object for scanning out a surface to a direct mode display.
 *
 * The underlying function CreateSimpleScanout is prone to spurious IllegalArgument failures, so
 * this wrapper tries twice.
 *
 * See https://github.com/MicrosoftDocs/winrt-api/issues/1942
 *
 * @param capability A populated @ref SystemApiCapability describing the features available and allowed for use.
 * @param maxAttempts The maximum number of times to try creating a scanout: consider at least 2, since this is prone to
 * spurious failure
 * @param device Display device
 * @param source Display source
 * @param primary Primary display surface
 * @param subResourceIndex as in CreateSimpleScanout. Usually 0 unless you are using hardware stereo (like 3D TV)
 * @param allowTearing determines the sync interval value in Win10 and Win11, and the flags to create the scanout in
 * Win11.
 * @return ::winrt::Windows::Devices::Display::Core::DisplayScanout
 */
::winrt::Windows::Devices::Display::Core::DisplayScanout
createScanout(SystemApiCapability const &capability,
              int maxAttempts,
              ::winrt::Windows::Devices::Display::Core::DisplayDevice const &device,
              ::winrt::Windows::Devices::Display::Core::DisplaySource const &source,
              ::winrt::Windows::Devices::Display::Core::DisplaySurface const &primary,
              uint32_t subResourceIndex,
              bool allowTearing);


#define D3D_WINRT_MAKE_STRINGIFY_CASE(WDDC_ENUM)                                                                       \
	case ::winrt::Windows::Devices::Display::Core::WDDC_ENUM: return #WDDC_ENUM

//! Convert Windows.Devices.Display.Core.DisplayManagerResult enum values to string
XRT_CHECK_RESULT constexpr const char *
to_string(::winrt::Windows::Devices::Display::Core::DisplayManagerResult e)
{
	switch (e) {
		D3D_WINRT_MAKE_STRINGIFY_CASE(DisplayManagerResult::Success);
		D3D_WINRT_MAKE_STRINGIFY_CASE(DisplayManagerResult::UnknownFailure);
		D3D_WINRT_MAKE_STRINGIFY_CASE(DisplayManagerResult::TargetAccessDenied);
		D3D_WINRT_MAKE_STRINGIFY_CASE(DisplayManagerResult::TargetStale);
		D3D_WINRT_MAKE_STRINGIFY_CASE(DisplayManagerResult::RemoteSessionNotSupported);
	}
	return "DisplayManagerResult::UNKNOWN";
}

//! Convert Windows.Devices.Display.Core.DisplayStateOperationStatus enum values to string
XRT_CHECK_RESULT constexpr const char *
to_string(::winrt::Windows::Devices::Display::Core::DisplayStateOperationStatus e)
{
	switch (e) {
		D3D_WINRT_MAKE_STRINGIFY_CASE(DisplayStateOperationStatus::Success);
		D3D_WINRT_MAKE_STRINGIFY_CASE(DisplayStateOperationStatus::PartialFailure);
		D3D_WINRT_MAKE_STRINGIFY_CASE(DisplayStateOperationStatus::UnknownFailure);
		D3D_WINRT_MAKE_STRINGIFY_CASE(DisplayStateOperationStatus::TargetOwnershipLost);
		D3D_WINRT_MAKE_STRINGIFY_CASE(DisplayStateOperationStatus::SystemStateChanged);
		D3D_WINRT_MAKE_STRINGIFY_CASE(DisplayStateOperationStatus::TooManyPathsForAdapter);
		D3D_WINRT_MAKE_STRINGIFY_CASE(DisplayStateOperationStatus::ModesNotSupported);
		D3D_WINRT_MAKE_STRINGIFY_CASE(DisplayStateOperationStatus::RemoteSessionNotSupported);
	}
	return "DisplayStateOperationStatus::UNKNOWN";
}

//! Convert Windows.Devices.Display.Core.DisplayPathStatus enum values to string
XRT_CHECK_RESULT constexpr const char *
to_string(::winrt::Windows::Devices::Display::Core::DisplayPathStatus e)
{
	switch (e) {
		D3D_WINRT_MAKE_STRINGIFY_CASE(DisplayPathStatus::Unknown);
		D3D_WINRT_MAKE_STRINGIFY_CASE(DisplayPathStatus::Succeeded);
		D3D_WINRT_MAKE_STRINGIFY_CASE(DisplayPathStatus::Pending);
		D3D_WINRT_MAKE_STRINGIFY_CASE(DisplayPathStatus::Failed);
		D3D_WINRT_MAKE_STRINGIFY_CASE(DisplayPathStatus::FailedAsync);
		D3D_WINRT_MAKE_STRINGIFY_CASE(DisplayPathStatus::InvalidatedAsync);
	}
	return "DisplayPathStatus::UNKNOWN";
}

//! Convert Windows.Devices.Display.Core.DisplayPresentStatus enum values to string
XRT_CHECK_RESULT constexpr const char *
to_string(::winrt::Windows::Devices::Display::Core::DisplayPresentStatus e)
{
	switch (e) {
		D3D_WINRT_MAKE_STRINGIFY_CASE(DisplayPresentStatus::Success);
		D3D_WINRT_MAKE_STRINGIFY_CASE(DisplayPresentStatus::SourceStatusPreventedPresent);
		D3D_WINRT_MAKE_STRINGIFY_CASE(DisplayPresentStatus::ScanoutInvalid);
		D3D_WINRT_MAKE_STRINGIFY_CASE(DisplayPresentStatus::SourceInvalid);
		D3D_WINRT_MAKE_STRINGIFY_CASE(DisplayPresentStatus::DeviceInvalid);
		D3D_WINRT_MAKE_STRINGIFY_CASE(DisplayPresentStatus::UnknownFailure);
	}
	return "DisplayPresentStatus::UNKNOWN";
}
#undef D3D_WINRT_MAKE_STRINGIFY_CASE
} // namespace xrt::auxiliary::d3d::winrt
