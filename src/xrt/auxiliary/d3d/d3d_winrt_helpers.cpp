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
#include "d3d_winrt_helpers.hpp"

#include <dxgiformat.h>

#include <winrt/base.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.Metadata.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Devices.Display.Core.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <windows.devices.display.core.interop.h>

#include <d3d11_1.h>
#include <d3d11_4.h>

#include <wil/com.h> // must be after winrt
#include <wil/resource.h>
#include <wil/result_macros.h>

#include <array>
#include <tuple>
#include <algorithm>
#include <utility>
#include <winrt/impl/Windows.Devices.Display.Core.0.h>
#include <winrt/impl/Windows.Devices.Display.Core.2.h>

namespace xrt::auxiliary::d3d::winrt {

namespace winrtWDDC = ::winrt::Windows::Devices::Display::Core;
namespace winrtWDD = ::winrt::Windows::Devices::Display;
namespace Collections = ::winrt::Windows::Foundation::Collections;
using ::winrt::Windows::Graphics::DirectX::DirectXPixelFormat;

static inline std::tuple<bool, uint16_t, DXGI_FORMAT>
lookupFormat(DirectXPixelFormat format)
{
	switch (format) {
	// These are all first-tier
	case DirectXPixelFormat::B8G8R8A8UIntNormalizedSrgb: return {true, 0, DXGI_FORMAT_B8G8R8A8_UNORM_SRGB};
	case DirectXPixelFormat::B8G8R8X8UIntNormalizedSrgb: return {true, 0, DXGI_FORMAT_B8G8R8X8_UNORM_SRGB};
	case DirectXPixelFormat::R8G8B8A8UIntNormalizedSrgb: return {true, 0, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB};
	// These are second tier options
	case DirectXPixelFormat::B8G8R8A8UIntNormalized: return {true, 5, DXGI_FORMAT_B8G8R8A8_UNORM};
	case DirectXPixelFormat::B8G8R8X8UIntNormalized: return {true, 5, DXGI_FORMAT_B8G8R8X8_UNORM};
	case DirectXPixelFormat::R8G8B8A8UIntNormalized: return {true, 5, DXGI_FORMAT_R8G8B8A8_UNORM};
	default: return {false, 1000, (DXGI_FORMAT)0};
	}
}

//! Make something sortable out of a display mode
static inline std::pair<uint16_t, float>
makeModeKey(const winrtWDDC::DisplayModeInfo &modeInfo)
{
	auto format = modeInfo.SourcePixelFormat();
	uint16_t preference = 5000;

	std::tie(std::ignore, preference, std::ignore) = lookupFormat(modeInfo.SourcePixelFormat());

	// negate the fps so we get the largest at the top, not the smallest
	return std::make_pair(preference, -parseVerticalRefreshRate(modeInfo).as_float());
}

bool
isModeAcceptable(const ::winrt::Windows::Devices::Display::Core::DisplayModeInfo &modeInfo)
{
	bool ret = false;
	std::tie(ret, std::ignore, std::ignore) = lookupFormat(modeInfo.SourcePixelFormat());
	return ret;
}
bool
modeComparison(const ::winrt::Windows::Devices::Display::Core::DisplayModeInfo &lhs,
               const ::winrt::Windows::Devices::Display::Core::DisplayModeInfo &rhs)
{
	return makeModeKey(lhs) < makeModeKey(rhs);
}

void
sortModes(std::vector<::winrt::Windows::Devices::Display::Core::DisplayModeInfo> &acceptableModes)
{
	std::sort(begin(acceptableModes), end(acceptableModes), modeComparison);
}

static bool
checkForBasicAPI()
{
	constexpr uint16_t ContractVersionForBasicAPI = 7;
	return ::winrt::Windows::Foundation::Metadata::ApiInformation::IsApiContractPresent(
	    L"Windows.Foundation.UniversalApiContract", ContractVersionForBasicAPI);
}

static bool
checkForEnhancedApi()
{
	constexpr uint16_t ContractVersionForWin11 = 14;
	return ::winrt::Windows::Foundation::Metadata::ApiInformation::IsApiContractPresent(
	    L"Windows.Foundation.UniversalApiContract", ContractVersionForWin11);
}

void
SystemApiCapability::populate()
{
	supportsBasicDirectMode = checkForBasicAPI();
	supportsScanoutOptionsAndTryExecuteTask = checkForEnhancedApi();
}

::winrt::Windows::Devices::Display::Core::DisplayScanout
createScanout(SystemApiCapability const &capability,
              int maxAttempts,
              ::winrt::Windows::Devices::Display::Core::DisplayDevice const &device,
              ::winrt::Windows::Devices::Display::Core::DisplaySource const &source,
              ::winrt::Windows::Devices::Display::Core::DisplaySurface const &primary,
              uint32_t subResourceIndex,
              bool allowTearing)
{
	using ::winrt::Windows::Devices::Display::Core::DisplayScanout;
	using ::winrt::Windows::Devices::Display::Core::DisplayScanoutOptions;
	DisplayScanout ret{nullptr};
	auto haveWin11 = capability.supportsScanoutOptionsAndTryExecuteTask;

	auto TryCreateScanout = [&] {
		try {
			if (haveWin11) {
				// Can always use syncinterval 0 when we have API 14 (win 11) or newer because we can
				// explicitly choose tearing or not.
				const uint32_t syncInterval = 0;
				DisplayScanoutOptions options =
				    allowTearing ? DisplayScanoutOptions::AllowTearing : DisplayScanoutOptions::None;
				ret = device.CreateSimpleScanoutWithDirtyRectsAndOptions(
				    source, primary, subResourceIndex, syncInterval, nullptr, options);

			} else {
				// On Win10, sync internal of 0 has tearing, unexpectedly.
				const uint32_t syncInterval = allowTearing ? 0 : 1;

				ret = device.CreateSimpleScanout(source, primary, subResourceIndex, syncInterval);
			}
		} catch (::winrt::hresult_invalid_argument const &) {
			// ignore
		}
		return ret;
	};
	for (int i = 0; i < maxAttempts && ret == nullptr; ++i) {
		TryCreateScanout();
	}
	if (ret == nullptr) {
		throw std::runtime_error("Couldn't construct a scanout even after repeated tries.");
	}
	return ret;
}

} // namespace xrt::auxiliary::d3d::winrt
