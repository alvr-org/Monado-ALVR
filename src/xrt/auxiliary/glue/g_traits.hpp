// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Shared type traits for glue classes.
 * @ingroup aux_util
 */

#pragma once

#include <type_traits>


namespace xrt::util {

/*!
 * True when `Base` is an accessible, non-virtual base of `Derived`, that is a
 * `Base *` can be `static_cast` down to `Derived *`. That cast is ill-formed
 * for a virtual base, so this is false for virtual inheritance. The glue
 * wrappers use it to reject virtual inheritance of the glue base, which would
 * break pointer recovery.
 *
 * @todo C++17 has no direct trait for "is virtual base"; revisit with a
 * cleaner formulation if reflection (C++26) makes one available.
 */
template <class Base, class Derived, class = void> inline constexpr bool is_non_virtual_base_v = false;

template <class Base, class Derived>
inline constexpr bool
    is_non_virtual_base_v<Base, Derived, std::void_t<decltype(static_cast<Derived *>(static_cast<Base *>(nullptr)))>> =
        true;

} // namespace xrt::util
