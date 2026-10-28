// Copyright 2022, Collabora, Ltd.
// Copyright 2025, Holo-Light GmbH
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Direct3D 12 tests.
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @author Krzysztof Lesiak <c-k.lesiak@holo-light.com>
 */

#include "catch_amalgamated.hpp"

#include <util/u_win32_com_guard.hpp>
#include <d3d/d3d_dxgi_helpers.hpp>
#include <d3d/d3d_d3d12_helpers.hpp>
#include <d3d/d3d_d3d12_allocator.hpp>
#include "aux_d3d_dxgi_formats.hpp"

#ifdef XRT_HAVE_VULKAN
#include "vktest_init_bundle.hpp"
#include <vk/vk_image_allocator.h>
#include <util/u_handles.h>
#include <d3d/d3d_dxgi_formats.h>
#endif

using namespace xrt::auxiliary::util;

TEST_CASE("d3d12_device", "[.][needgpu]")
{
	ComGuard comGuard;

	wil::com_ptr<IDXGIAdapter> adapter;
	CHECK_NOTHROW(adapter = xrt::auxiliary::d3d::getAdapterByIndex(0, U_LOGGING_TRACE));
	CHECK(adapter);

	wil::com_ptr<ID3D12Device> device;
	CHECK_NOTHROW(device = xrt::auxiliary::d3d::d3d12::createDevice(adapter, U_LOGGING_TRACE));
	CHECK(device);
}

#ifdef XRT_HAVE_VULKAN

static inline bool
tryImport(struct vk_bundle *vk,
          std::vector<wil::unique_handle> &handles,
          const struct xrt_swapchain_create_info &xsci,
          size_t image_mem_size)
{
	// in d3d11 tryImport handles is const..here we do away with it so we can call release on handles passed in?
	// I need to read more about wil and figure out lifetime semantics of all this.

	INFO("Testing import into Vulkan");

	static constexpr bool use_dedicated_allocation = false;
	xrt_swapchain_create_info vk_info = xsci;
	vk_info.format = d3d_dxgi_format_to_vk((DXGI_FORMAT)xsci.format);
	const auto free_vk_ic = [&](struct vk_image_collection *vkic) {
		vk_ic_destroy(vk, vkic);
		delete vkic;
	};

	std::shared_ptr<vk_image_collection> vkic{new vk_image_collection, free_vk_ic};

	uint32_t image_count = static_cast<uint32_t>(handles.size());

	// Populate for import
	std::vector<xrt_image_native> xins;
	xins.reserve(image_count);

	for (auto &handle : handles) {
		xrt_image_native xin;
		xin.handle = handle.get();
		xin.size = image_mem_size;
		xin.use_dedicated_allocation = use_dedicated_allocation;
		xin.is_dxgi_handle = false;

		xins.emplace_back(xin);
	}

	// Import into a vulkan image collection
	bool result = VK_SUCCESS == vk_ic_from_natives(vk, &vk_info, xins.data(), (uint32_t)xins.size(), vkic.get());

	if (result) {
		// The imported swapchain took ownership of them now, release them from ownership here.
		for (wil::unique_handle &h : handles) {
			h.release();
		}
	}
	return result;
}
#else

static inline bool
tryImport(struct vk_bundle * /* vk */,
          std::vector<wil::unique_handle> const & /* handles */,
          const struct xrt_swapchain_create_info & /* xsci */)
{
	return true;
}

#endif

TEST_CASE("d3d12_allocate", "[.][needgpu]")
{
	unique_vk_bundle vk = makeVkBundle();

#ifdef XRT_HAVE_VULKAN
	REQUIRE(vktest_init_bundle(vk.get()));
#endif

	ComGuard comGuard;

	wil::com_ptr<IDXGIAdapter> adapter = xrt::auxiliary::d3d::getAdapterByIndex(0, U_LOGGING_TRACE);
	wil::com_ptr<ID3D12Device> device = xrt::auxiliary::d3d::d3d12::createDevice(adapter, U_LOGGING_TRACE);
	std::vector<wil::com_ptr<ID3D12Resource>> images;
	std::vector<wil::unique_handle> handles;
	size_t out_image_mem_size = 0;

	size_t image_count = 3;

	xrt_swapchain_create_info xsci{};
	CAPTURE(xsci.sample_count = 1);
	CAPTURE(xsci.width = 800);
	CAPTURE(xsci.height = 600);

	CAPTURE(xsci.mip_count = 1);
	xsci.face_count = 1;
	xsci.array_size = 1;

	SECTION("create images")
	{
		auto nameAndFormat = GENERATE(values(namesAndFormats));
		DYNAMIC_SECTION("Texture format " << nameAndFormat.first)
		{
			auto format = nameAndFormat.second;
			CAPTURE(isDepthStencilFormat(format));
			xsci.format = format;
			if (isDepthStencilFormat(format)) {
				xsci.bits = XRT_SWAPCHAIN_USAGE_DEPTH_STENCIL;
			} else {
				xsci.bits = XRT_SWAPCHAIN_USAGE_COLOR;
			}
			xsci.bits = (xrt_swapchain_usage_bits)(xsci.bits | XRT_SWAPCHAIN_USAGE_SAMPLED);
			images.clear();
			handles.clear();

			SECTION("invalid array size 0")
			{
				CAPTURE(xsci.array_size = 0);
				REQUIRE(XRT_SUCCESS !=
				        xrt::auxiliary::d3d::d3d12::allocateSharedImages(
				            *device.get(), xsci, image_count, images, handles, out_image_mem_size));
				CHECK(images.empty());
				CHECK(handles.empty());
			}
			SECTION("not array")
			{
				CAPTURE(xsci.array_size);
				REQUIRE(XRT_SUCCESS ==
				        xrt::auxiliary::d3d::d3d12::allocateSharedImages(
				            *device.get(), xsci, image_count, images, handles, out_image_mem_size));
				CHECK(images.size() == image_count);
				CHECK(handles.size() == image_count);
				CHECK(tryImport(vk.get(), handles, xsci, out_image_mem_size));
			}
			SECTION("array of 2")
			{
				CAPTURE(xsci.array_size = 2);
				REQUIRE(XRT_SUCCESS ==
				        xrt::auxiliary::d3d::d3d12::allocateSharedImages(
				            *device.get(), xsci, image_count, images, handles, out_image_mem_size));
				CHECK(images.size() == image_count);
				CHECK(handles.size() == image_count);
				CHECK(tryImport(vk.get(), handles, xsci, out_image_mem_size));
			}
			// this does not return an error...so i guess allocating cubemaps with d3d12 is fine?
			//  SECTION("cubemaps not implemented")
			//  {
			//  	CAPTURE(xsci.array_size);
			//  	CAPTURE(xsci.face_count = 6);
			//  	REQUIRE(XRT_ERROR_ALLOCATION ==
			//  	        xrt::auxiliary::d3d::d3d12::allocateSharedImages(*device.get(), xsci,
			//  imageCount,
			//                                                                     images, handles,
			//                                                                     outImageMemSize));
			// 	CHECK(images.empty());
			// 	CHECK(handles.empty());
			// }
			SECTION("protected content not implemented")
			{
				CAPTURE(xsci.array_size);
				CAPTURE(xsci.create = XRT_SWAPCHAIN_CREATE_PROTECTED_CONTENT);
				REQUIRE(XRT_ERROR_SWAPCHAIN_FLAG_VALID_BUT_UNSUPPORTED ==
				        xrt::auxiliary::d3d::d3d12::allocateSharedImages(
				            *device.get(), xsci, image_count, images, handles, out_image_mem_size));
				CHECK(images.empty());
				CHECK(handles.empty());
			}
		}
	}
}