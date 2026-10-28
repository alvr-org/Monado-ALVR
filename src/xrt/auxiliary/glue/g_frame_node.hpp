// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header for glue classes to wrap xrt frame node interfaces.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup aux_util
 */

#pragma once

#include "xrt/xrt_frame.h"

#include "g_catch_guard.hpp"
#include "g_traits.hpp"

#include <type_traits>


namespace xrt::util {

/*!
 * CRTP glue wrapper for @ref xrt_frame_node. Relies on standard layout to
 * recover the derived object from the C struct, and has some requirements and
 * limitations because of that. See @ref cpp-glue-wrappers for the guide and
 * conventions for these wrappers.
 *
 * Usually inherited alongside another glue base such as @ref FrameServerBase,
 * so that the object is destroyed together with the @ref xrt_frame_context it
 * is added to.
 */
template <class T> class FrameNodeBase
{
public: // Methods
	FrameNodeBase() noexcept
	{
		static_assert(std::is_standard_layout_v<FrameNodeBase>,
		              "glue base must be standard layout for pointer recovery");
		static_assert(is_non_virtual_base_v<FrameNodeBase, T>,
		              "glue base must be a non-virtual base of T for pointer recovery");

		auto &node = *getFrameNode();

		node.break_apart = breakApartWrap;
		node.destroy = destroyFrameNodeWrap;
	}

	~FrameNodeBase() noexcept = default;

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
	fromFrameNode(const xrt_frame_node *node) noexcept
	{
		return &(reinterpret_cast<const FrameNodeBase *>(node)->derived());
	}

	static T *
	fromFrameNode(xrt_frame_node *node) noexcept
	{
		return &(reinterpret_cast<FrameNodeBase *>(node)->derived());
	}

	const xrt_frame_node *
	getFrameNode() const noexcept
	{
		return &mFrameNode;
	}

	xrt_frame_node *
	getFrameNode() noexcept
	{
		return &mFrameNode;
	}


private: // Members
	/*!
	 * Wrapped @ref xrt_frame_node. Must be the first data member: a pointer to
	 * it is then interconvertible with a pointer to this standard-layout base,
	 * which lets the glue cast a C pointer back to the derived C++ class. See
	 * @ref cpp-glue-wrappers.
	 */
	xrt_frame_node mFrameNode = {};


private: // Functions
// Not `->derived()`, that is ambiguous when T has more than one glue base.
#define GET(node) (*fromFrameNode(node))

	static void
	breakApartWrap(struct xrt_frame_node *node) noexcept
	try {
		GET(node).breakApart();
	}
	G_CATCH_GUARDS_VOID

	static void
	destroyFrameNodeWrap(struct xrt_frame_node *node) noexcept
	try {
		T::destroyFrameNode(node);
	}
	G_CATCH_GUARDS_VOID

#undef GET
};

} // namespace xrt::util
