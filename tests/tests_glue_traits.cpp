// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Tests for glue/g_traits.hpp
 * @author Jakob Bornecrantz <tbornecrantz@nvidia.com>
 */

#include "glue/g_traits.hpp"

#include "catch_amalgamated.hpp"


using xrt::util::is_non_virtual_base_v;

namespace {

struct Base
{
	int a;
};

struct NonVirtual : Base
{
	int b;
};

struct Virtual : virtual Base
{
	int b;
};

//! Grandchild reached through a non-virtual chain.
struct Grandchild : NonVirtual
{
	int c;
};

//! Grandchild where the shared @ref Base is a virtual base further up.
struct VirtualGrandchild : Virtual
{
	int c;
};

struct Unrelated
{
	int x;
};

//! Mirrors how the glue wrappers use the trait, `T` inherits `CrtpBase<T>`.
template <class T> struct CrtpBase
{
	int base;
};

struct CrtpUser : CrtpBase<CrtpUser>
{
	int user;
};

} // namespace


TEST_CASE("is_non_virtual_base_v true for non-virtual bases")
{
	STATIC_REQUIRE(is_non_virtual_base_v<Base, NonVirtual>);
	STATIC_REQUIRE(is_non_virtual_base_v<Base, Grandchild>);
	STATIC_REQUIRE(is_non_virtual_base_v<NonVirtual, Grandchild>);
	STATIC_REQUIRE(is_non_virtual_base_v<CrtpBase<CrtpUser>, CrtpUser>);
}

TEST_CASE("is_non_virtual_base_v false for virtual bases")
{
	STATIC_REQUIRE_FALSE(is_non_virtual_base_v<Base, Virtual>);
	STATIC_REQUIRE_FALSE(is_non_virtual_base_v<Base, VirtualGrandchild>);
}

TEST_CASE("is_non_virtual_base_v false for unrelated types")
{
	STATIC_REQUIRE_FALSE(is_non_virtual_base_v<Base, Unrelated>);
	STATIC_REQUIRE_FALSE(is_non_virtual_base_v<Unrelated, NonVirtual>);
}
