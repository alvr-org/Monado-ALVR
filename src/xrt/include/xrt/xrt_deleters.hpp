// Copyright 2019-2026, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Generic unique_ptr deleters for Monado types
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @ingroup xrt_iface
 */

#pragma once

#include <memory>

namespace xrt {

/*!
 * Generic deleter functors for the variety of interface/object types in Monado.
 *
 * Use these with std::unique_ptr to make per-interface type aliases for unique ownership.
 * These are stateless deleters whose function pointer is statically specified as a template argument.
 */
namespace deleters {
	/*!
	 * Deleter type for interfaces with destroy functions that take pointers to interface pointers (so they may be
	 * zeroed).
	 * Wraps the type and the destroy function for use with `std::unique_ptr`
	 */
	template <typename T, void (*DeleterFn)(T **)> struct ptr_ptr_deleter
	{
		void
		operator()(T *obj) const noexcept
		{
			if (obj == nullptr) {
				return;
			}
			DeleterFn(&obj);
		}
	};

	/*!
	 * Deleter type for interfaces with destroy functions that take just pointers.
	 * Wraps the type and the destroy function for use with `std::unique_ptr`
	 */
	template <typename T, void (*DeleterFn)(T *)> struct ptr_deleter
	{
		void
		operator()(T *obj) const noexcept
		{
			if (obj == nullptr) {
				return;
			}
			DeleterFn(obj);
		}
	};

	/*!
	 * Deleter type for ref-counted interfaces with two-parameter `reference(dest, src)` functions.
	 * Wraps the type and the interface reference function for use with `std::unique_ptr`
	 */
	template <typename T, void (*ReferenceFn)(T **, T *)> struct reference_deleter
	{
		void
		operator()(T *obj) const noexcept
		{
			if (obj == nullptr) {
				return;
			}
			ReferenceFn(&obj, nullptr);
		}
	};


	/*!
	 * Deleter type for opaque object types for interfaces with destroy functions that take pointers to
	 * interface pointers (so they may be zeroed).
	 * Wraps the types and the interface destroy function for use with `std::unique_ptr`
	 */
	template <typename Derived, typename T, void (*DeleterFn)(T **)> struct cast_ptr_ptr_deleter
	{
		void
		operator()(Derived *obj) const noexcept
		{
			if (obj == nullptr) {
				return;
			}
			T *base = reinterpret_cast<T *>(obj);
			DeleterFn(&base);
		}
	};

	namespace detail {
		/// Get the base type of a non-opaque derived type.
		template <typename Derived> using BaseType_t = decltype(std::declval<Derived>().base);

		/// Get the base of the base type of a non-opaque derived type.
		template <typename Derived> using BaseBaseType_t = decltype(std::declval<Derived>().base.base);
	} // namespace detail

	/*!
	 * Deleter type for non-opaque twice-derived object types from interfaces with destroy functions that take
	 * pointers to interface pointers (so they may be zeroed). Wraps the type and the interface destroy function for
	 * use with `std::unique_ptr` (interface type deduced)
	 */
	template <typename Derived, void (*DeleterFn)(detail::BaseBaseType_t<Derived> **)>
	struct base_base_ptr_ptr_deleter
	{
		void
		operator()(Derived *obj) const noexcept
		{
			if (obj == nullptr) {
				return;
			}
			detail::BaseBaseType_t<Derived> *base = &obj->base.base;
			DeleterFn(&base);
		}
	};

	/*!
	 * Deleter type for non-opaque once-derived object types from interfaces with destroy functions that take
	 * pointers to interface pointers (so they may be zeroed). Wraps the type and the interface destroy function for
	 * use with `std::unique_ptr` (interface type deduced)
	 */
	template <typename Derived, void (*DeleterFn)(detail::BaseType_t<Derived> **)> struct base_ptr_ptr_deleter
	{
		void
		operator()(Derived *obj) const noexcept
		{
			if (obj == nullptr) {
				return;
			}
			detail::BaseType_t<Derived> *base = &obj->base;
			DeleterFn(&base);
		}
	};

} // namespace deleters

} // namespace xrt
