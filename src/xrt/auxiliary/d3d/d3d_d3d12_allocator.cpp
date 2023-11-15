// Copyright 2020-2022, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief D3D12 backed image buffer allocator.
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @author Fernando Velazquez Innella <finnella@magicleap.com>
 * @ingroup aux_d3d
 */

#include "d3d_d3d12_allocator.h"
#include "d3d_d3d12_allocator.hpp"

#include "d3d_d3d12_bits.h"
#include "d3d_dxgi_formats.h"

#include "util/u_misc.h"
#include "util/u_logging.h"
#include "util/u_debug.h"
#include "util/u_handles.h"

#include "xrt/xrt_windows.h"

#include <Unknwn.h>
#include <d3d12.h>
#include <wil/com.h>
#include <wil/result.h>

#include <inttypes.h>
#include <memory>

#define DEFAULT_CATCH(...)                                                                                             \
	catch (wil::ResultException const &e)                                                                          \
	{                                                                                                              \
		U_LOG_E("Caught exception: %s", e.what());                                                             \
		return __VA_ARGS__;                                                                                    \
	}                                                                                                              \
	catch (std::exception const &e)                                                                                \
	{                                                                                                              \
		U_LOG_E("Caught exception: %s", e.what());                                                             \
		return __VA_ARGS__;                                                                                    \
	}                                                                                                              \
	catch (...)                                                                                                    \
	{                                                                                                              \
		U_LOG_E("Caught exception");                                                                           \
		return __VA_ARGS__;                                                                                    \
	}


DEBUG_GET_ONCE_LOG_OPTION(d3d12_log, "D3D12_LOG", U_LOGGING_WARN)
#define D3DA_TRACE(...) U_LOG_IFL_T(debug_get_log_option_d3d12_log(), __VA_ARGS__)
#define D3DA_DEBUG(...) U_LOG_IFL_D(debug_get_log_option_d3d12_log(), __VA_ARGS__)
#define D3DA_INFO(...) U_LOG_IFL_I(debug_get_log_option_d3d12_log(), __VA_ARGS__)
#define D3DA_WARN(...) U_LOG_IFL_W(debug_get_log_option_d3d12_log(), __VA_ARGS__)
#define D3DA_ERROR(...) U_LOG_IFL_E(debug_get_log_option_d3d12_log(), __VA_ARGS__)

namespace xrt::auxiliary::d3d::d3d12 {

static wil::unique_handle
createSharedHandle(ID3D12Device &device, const wil::com_ptr<ID3D12Resource> &image)
{
	wil::unique_handle h;
	THROW_IF_FAILED(device.CreateSharedHandle( //
	    image.get(),                           // pObject
	    nullptr,                               // pAttributes
	    GENERIC_ALL,                           // Access
	    nullptr,                               // Name
	    h.put()));                             // pHandle
	return h;
}


xrt_result_t
allocateSharedImages(ID3D12Device &device,
                     const xrt_swapchain_create_info &xsci,
                     size_t image_count,
                     std::vector<wil::com_ptr<ID3D12Resource>> &out_images,
                     std::vector<wil::unique_handle> &out_handles)
try {
	if (0 != (xsci.create & XRT_SWAPCHAIN_CREATE_PROTECTED_CONTENT)) {
		return XRT_ERROR_SWAPCHAIN_FLAG_VALID_BUT_UNSUPPORTED;
	}

	if (0 != (xsci.create & XRT_SWAPCHAIN_CREATE_STATIC_IMAGE) && image_count > 1) {
		D3DA_ERROR("Got XRT_SWAPCHAIN_CREATE_STATIC_IMAGE but an image count greater than 1!");
		return XRT_ERROR_ALLOCATION;
	}
	if (xsci.array_size == 0) {
		D3DA_ERROR("Array size must not be 0");
		return XRT_ERROR_ALLOCATION;
	}

	// TODO: See if this is still necessary
	DXGI_FORMAT typeless_format = d3d_dxgi_format_to_typeless_dxgi((DXGI_FORMAT)xsci.format);
	if (typeless_format == 0) {
		D3DA_ERROR("Invalid format %04" PRIx64 "!", (uint64_t)xsci.format);
		return XRT_ERROR_SWAPCHAIN_FORMAT_UNSUPPORTED;
	}

	DXGI_SAMPLE_DESC sample_desc{
	    xsci.sample_count, // Count
	    0,                 // Quality
	};

	// Note:
	// To use a cross-adapter heap the following flag must be passed:
	// resource_flags |= D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER;
	// Additionally, only copy operations are allowed with the resource.
	D3D12_RESOURCE_FLAGS resource_flags = d3d_convert_usage_bits_to_d3d12_resource_flags(xsci.bits);

	D3D12_RESOURCE_DESC desc{
	    D3D12_RESOURCE_DIMENSION_TEXTURE2D, // Dimension
	    0,                                  // Alignment
	    xsci.width,                         // Width
	    xsci.height,                        // Height
	    (UINT16)xsci.array_size,            // DepthOrArraySize
	    (UINT16)xsci.mip_count,             // MipLevels
	    typeless_format,                    // Format
	    sample_desc,                        // SampleDesc
	    D3D12_TEXTURE_LAYOUT_UNKNOWN,       // Layout
	    resource_flags                      // Flags;
	};

	// Cubemap
	if (xsci.face_count == 6) {
		desc.DepthOrArraySize *= 6;
	}

	// Create resources and let the driver manage memory
	std::vector<wil::com_ptr<ID3D12Resource>> images;
	D3D12_HEAP_PROPERTIES heap{};
	heap.Type = D3D12_HEAP_TYPE_DEFAULT;
	D3D12_HEAP_FLAGS heap_flags = D3D12_HEAP_FLAG_SHARED;
	D3D12_RESOURCE_STATES initial_resource_state = d3d_convert_usage_bits_to_d3d12_app_resource_state(xsci.bits);

	for (size_t i = 0; i < image_count; ++i) {
		wil::com_ptr<ID3D12Resource> tex;
		HRESULT res = device.CreateCommittedResource( //
		    &heap,                                    // pHeapProperties
		    heap_flags,                               // HeapFlags
		    &desc,                                    // pDesc
		    initial_resource_state,                   // InitialResourceState
		    nullptr,                                  // pOptimizedClearValue
		    IID_PPV_ARGS(tex.put()));                 // riidResource, ppvResource

		if (FAILED(LOG_IF_FAILED(res))) {
			return XRT_ERROR_ALLOCATION;
		}
		images.emplace_back(std::move(tex));
	}

	std::vector<wil::unique_handle> handles;
	handles.reserve(image_count);
	for (const auto &tex : images) {
		handles.emplace_back(createSharedHandle(device, tex));
	}
	out_images = std::move(images);
	out_handles = std::move(handles);
	return XRT_SUCCESS;
}
DEFAULT_CATCH(XRT_ERROR_ALLOCATION)

} // namespace xrt::auxiliary::d3d::d3d12

struct d3d12_allocator
{
	struct xrt_image_native_allocator base;
	wil::com_ptr<ID3D12Device> device;
};


static xrt_result_t
d3d12_images_allocate(struct xrt_image_native_allocator *xina,
                      const struct xrt_swapchain_create_info *xsci,
                      size_t image_count,
                      struct xrt_image_native *out_images)
{
	try {
		d3d12_allocator *d3da = reinterpret_cast<d3d12_allocator *>(xina);

		std::vector<wil::com_ptr<ID3D12Resource>> images;
		std::vector<wil::unique_handle> handles;
		auto result = xrt::auxiliary::d3d::d3d12::allocateSharedImages( //
		    *(d3da->device),                                            // device
		    *xsci,                                                      // xsci
		    image_count,                                                // image_count
		    images,                                                     // out_images
		    handles);                                                   // out_handles

		if (result != XRT_SUCCESS) {
			return result;
		}

		for (size_t i = 0; i < image_count; ++i) {
			out_images[i].handle = handles[i].release();
			out_images[i].is_dxgi_handle = false;
		}

		return XRT_SUCCESS;
	}
	DEFAULT_CATCH(XRT_ERROR_ALLOCATION)
}

static xrt_result_t
d3d12_images_free(struct xrt_image_native_allocator *xina, size_t image_count, struct xrt_image_native *images)
{
	for (size_t i = 0; i < image_count; ++i) {
		u_graphics_buffer_unref(&(images[i].handle));
	}
	return XRT_SUCCESS;
}

static void
d3d12_destroy(struct xrt_image_native_allocator *xina)
{
	d3d12_allocator *d3da = reinterpret_cast<d3d12_allocator *>(xina);
	delete d3da;
}

struct xrt_image_native_allocator *
d3d12_allocator_create(ID3D11Device *device)
try {
	auto ret = std::make_unique<d3d12_allocator>();
	U_ZERO(&(ret->base));
	ret->base.images_allocate = d3d12_images_allocate;
	ret->base.images_free = d3d12_images_free;
	ret->base.destroy = d3d12_destroy;
	return &(ret.release()->base);
}
DEFAULT_CATCH(nullptr)
