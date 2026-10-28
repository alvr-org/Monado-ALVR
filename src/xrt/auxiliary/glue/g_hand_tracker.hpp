// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header for glue classes to wrap xrt hand tracker interfaces.
 * @ingroup aux_util
 */

#pragma once

#include "xrt/xrt_hand_tracker.h"
#include "g_catch_guard.hpp"
#include "g_traits.hpp"

#include <type_traits>


namespace xrt::util {

/*!
 * CRTP glue wrapper for @ref xrt_hand_tracker. Relies on standard layout to
 * recover the derived object from the C struct, and has some requirements and
 * limitations because of that. See @ref cpp-glue-wrappers for the guide and
 * conventions for these wrappers.
 */
template <class T> class HandTrackerBase
{
public: // Methods
	HandTrackerBase() noexcept
	{
		static_assert(std::is_standard_layout_v<HandTrackerBase>,
		              "glue base must be standard layout for pointer recovery");
		static_assert(is_non_virtual_base_v<HandTrackerBase, T>,
		              "glue base must be a non-virtual base of T for pointer recovery");

		auto &xht = *getXHT();

		xht.locate = locateWrap;
		xht.set_output = setOutputWrap;
		xht.destroy = destroyHandTrackerWrap;
	}

	~HandTrackerBase() noexcept = default;

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
	fromXHT(const xrt_hand_tracker *xht) noexcept
	{
		return &(reinterpret_cast<const HandTrackerBase *>(xht)->derived());
	}

	static T *
	fromXHT(xrt_hand_tracker *xht) noexcept
	{
		return &(reinterpret_cast<HandTrackerBase *>(xht)->derived());
	}

	const xrt_hand_tracker *
	getXHT() const noexcept
	{
		return &mHandTracker;
	}

	xrt_hand_tracker *
	getXHT() noexcept
	{
		return &mHandTracker;
	}


private: // Members
	/*!
	 * Wrapped @ref xrt_hand_tracker. Must be the first data member: a pointer to
	 * it is then interconvertible with a pointer to this standard-layout base,
	 * which lets the glue cast a C pointer back to the derived C++ class. See
	 * @ref cpp-glue-wrappers.
	 */
	xrt_hand_tracker mHandTracker = {};


private: // Functions
#define GET(xht) (fromXHT(xht)->derived())

	static xrt_result_t
	locateWrap(struct xrt_hand_tracker *xht,
	           struct xrt_space_overseer *xso,
	           struct xrt_space *base_space,
	           const struct xrt_pose *base_offset,
	           int64_t at_timestamp_ns,
	           struct xrt_hand_tracker_location *out_location) noexcept
	try {
		return GET(xht).locate(xso, base_space, base_offset, at_timestamp_ns, out_location);
	}
	G_CATCH_GUARDS

	static xrt_result_t
	setOutputWrap(struct xrt_hand_tracker *xht,
	              enum xrt_output_name name,
	              const struct xrt_output_value *value) noexcept
	try {
		return GET(xht).setOutput(name, value);
	}
	G_CATCH_GUARDS

	static void
	destroyHandTrackerWrap(struct xrt_hand_tracker *xht) noexcept
	try {
		T::destroyHandTracker(xht);
	}
	G_CATCH_GUARDS_VOID

#undef GET
};

} // namespace xrt::util
