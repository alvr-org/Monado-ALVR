// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header for glue classes to wrap xrt input/output cache interfaces.
 * @ingroup aux_util
 */

#pragma once

#include "xrt/xrt_io_cache.h"
#include "g_catch_guard.hpp"
#include "g_traits.hpp"

#include <type_traits>


namespace xrt::util {

/*!
 * CRTP glue wrapper for @ref xrt_io_cache. Relies on standard layout to
 * recover the derived object from the C struct, and has some requirements and
 * limitations because of that. See @ref cpp-glue-wrappers for the guide and
 * conventions for these wrappers.
 */
template <class T> class IoCacheBase
{
public: // Methods
	IoCacheBase() noexcept
	{
		static_assert(std::is_standard_layout_v<IoCacheBase>,
		              "glue base must be standard layout for pointer recovery");
		static_assert(is_non_virtual_base_v<IoCacheBase, T>,
		              "glue base must be a non-virtual base of T for pointer recovery");

		auto &xic = *getXIC();

		xic.get_info = getInfoWrap;
		xic.read = readWrap;
		xic.create_space = createSpaceWrap;
		xic.set_output = setOutputWrap;
		xic.get_output_limits = getOutputLimitsWrap;
		xic.destroy = destroyIoCacheWrap;
	}

	~IoCacheBase() noexcept = default;

	const T &
	derived() const noexcept
	{
		return static_cast<const T &>(*this);
	}

	T &
	derived() noexcept
	{
		return static_cast<T &>(*this);
	}

	static const T *
	fromXIC(const xrt_io_cache *xic) noexcept
	{
		return &(reinterpret_cast<const IoCacheBase *>(xic)->derived());
	}

	static T *
	fromXIC(xrt_io_cache *xic) noexcept
	{
		return &(reinterpret_cast<IoCacheBase *>(xic)->derived());
	}

	const xrt_io_cache *
	getXIC() const noexcept
	{
		return &mIoCache;
	}

	xrt_io_cache *
	getXIC() noexcept
	{
		return &mIoCache;
	}


private: // Members
	/*!
	 * Wrapped @ref xrt_io_cache. Must be the first data member: a pointer to
	 * it is then interconvertible with a pointer to this standard-layout base,
	 * which lets the glue cast a C pointer back to the derived C++ class. See
	 * @ref cpp-glue-wrappers.
	 */
	xrt_io_cache mIoCache = {};


private: // Functions
#define GET(xic) (fromXIC(xic)->derived())

	static xrt_result_t
	getInfoWrap(struct xrt_io_cache *xic, struct xrt_io_cache_info *out_info) noexcept
	try {
		return GET(xic).getInfo(out_info);
	}
	G_CATCH_GUARDS

	static xrt_result_t
	readWrap(struct xrt_io_cache *xic,
	         uint64_t generation_id,
	         uint32_t input_count,
	         struct xrt_input *inputs,
	         uint32_t output_count,
	         struct xrt_output *outputs,
	         bool *out_stale) noexcept
	try {
		return GET(xic).read(generation_id, input_count, inputs, output_count, outputs, out_stale);
	}
	G_CATCH_GUARDS

	static xrt_result_t
	createSpaceWrap(struct xrt_io_cache *xic,
	                struct xrt_device_id id,
	                enum xrt_input_name name,
	                struct xrt_space **out_space) noexcept
	try {
		return GET(xic).createSpace(id, name, out_space);
	}
	G_CATCH_GUARDS

	static xrt_result_t
	setOutputWrap(struct xrt_io_cache *xic,
	              struct xrt_device_id id,
	              enum xrt_output_name name,
	              const struct xrt_output_value *value) noexcept
	try {
		return GET(xic).setOutput(id, name, value);
	}
	G_CATCH_GUARDS

	static xrt_result_t
	getOutputLimitsWrap(struct xrt_io_cache *xic,
	                    struct xrt_device_id id,
	                    struct xrt_output_limits *out_limits) noexcept
	try {
		return GET(xic).getOutputLimits(id, out_limits);
	}
	G_CATCH_GUARDS

	static void
	destroyIoCacheWrap(struct xrt_io_cache *xic) noexcept
	try {
		T::destroyIoCache(xic);
	}
	G_CATCH_GUARDS_VOID

#undef GET
};

} // namespace xrt::util
