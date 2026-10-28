// Copyright 2021, Collabora, Ltd.
// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief A collection of strings, like a list of extensions to enable
 *
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @ingroup aux_util
 *
 */

#pragma once

#include "u_extension_list.h"

#include <memory>
#include <vector>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <algorithm>
#include <unordered_set>

namespace xrt::auxiliary::util {

// Forward declaration
class ExtensionListBuilder;

/*!
 * @brief An immutable collection of strings, like a list of extensions to enable.
 *
 * This class stores copies of strings internally and provides read-only access.
 *
 * Size is limited to one less than the max value of uint32_t which shouldn't be a problem,
 * the size really should be much smaller.
 */
class ExtensionList
{
public:
	//! Default constructor - creates an empty list
	ExtensionList() = default;

	//! Move constructor
	ExtensionList(ExtensionList &&) = default;

	//! Copy constructor, makes sure to do a deep copy.
	ExtensionList(ExtensionList const &other) : ExtensionList({other.strings.begin(), other.strings.end()}) {}

	//! Move assignment
	ExtensionList &
	operator=(ExtensionList &&) = default;

	//! Copy assignment
	ExtensionList &
	operator=(ExtensionList const &other)
	{
		/*
		 * This uses the move assignment to set this to a deep copy of
		 * the other list. We need to do a deep copy of the other list
		 * so that the set and pointers are correctly recalculated using
		 * the new strings.
		 */
		return *this = ExtensionList(other);
	}

	/*!
	 * @brief Get the size of the array (the number of strings)
	 */
	uint32_t
	size() const noexcept
	{
		return static_cast<uint32_t>(ptrs.size());
	}

	/*!
	 * @brief Get the data pointer of the array (array of const char*)
	 */
	const char *const *
	data() const noexcept
	{
		return ptrs.data();
	}

	/*!
	 * @brief Check if the string is in the list.
	 *
	 * (Comparing string contents)
	 *
	 * @param str a string view to search for.
	 *
	 * @return true if the string is in the list.
	 */
	bool
	contains(std::string_view str) const
	{
		return set.find(str) != set.end();
	}


private:
	friend class ExtensionListBuilder;

	//! Main storage of strings.
	std::vector<std::string> strings;

	/*!
	 * Internal set of strings for fast lookup, memory kept alive by the
	 * strings vector. Used to quickly check if a string is in the list.
	 */
	std::unordered_set<std::string_view> set;

	/*!
	 * Internal vector of pointers to the strings, memory kept alive by the
	 * strings vector.
	 */
	std::vector<const char *> ptrs;


private:
	// Internal constructor used by ExtensionListBuilder
	ExtensionList(std::vector<std::string> &&strings_) : strings(std::move(strings_))
	{
		ptrs.reserve(strings.size());
		for (const auto &s : strings) {
			set.insert(s);
			ptrs.push_back(s.c_str());
		}
	}
};

/*!
 * @brief A builder for constructing ExtensionList objects.
 *
 * This class allows write-only operations to build up an extension list,
 * which can then be converted to an immutable ExtensionList.
 */
class ExtensionListBuilder
{
public:
	//! Default constructor
	ExtensionListBuilder() = default;

	//! Construct with capacity hint
	explicit ExtensionListBuilder(uint32_t capacity)
	{
		strings.reserve(capacity);
	}

	//! Construct from an array of strings
	template <uint32_t N> explicit ExtensionListBuilder(const char *(&arr)[N])
	{
		strings.reserve(N);
		for (auto &&elt : arr) {
			append(elt);
		}
	}

	ExtensionListBuilder(ExtensionListBuilder &&) = delete;
	ExtensionListBuilder(ExtensionListBuilder const &) = delete;

	ExtensionListBuilder &
	operator=(ExtensionListBuilder &&) = delete;
	ExtensionListBuilder &
	operator=(ExtensionListBuilder const &) = delete;

	/*!
	 * @brief Append a new string to the builder.
	 *
	 * @param str a string view to append.
	 *
	 * @throws std::out_of_range if you have a ridiculous number of strings in your list already.
	 */
	void
	append(std::string_view str)
	{
		if (strings.size() > (std::numeric_limits<uint32_t>::max)() - 1) {
			throw std::out_of_range("Size limit reached");
		}
		strings.emplace_back(str);
	}

	/*!
	 * @brief Append a new string to the builder if it doesn't match any existing string.
	 *
	 * (Comparing string contents)
	 *
	 * @param str a string view to append.
	 *
	 * @return true if we added it
	 *
	 * @throws std::out_of_range if you have a ridiculous number of strings in your list already.
	 */
	bool
	appendUnique(std::string_view str)
	{
		if (strings.size() > (std::numeric_limits<uint32_t>::max)() - 1) {
			throw std::out_of_range("Size limit reached");
		}
		auto it =
		    std::find_if(strings.begin(), strings.end(), [str](const std::string &elt) { return str == elt; });
		if (it != strings.end()) {
			// already have it
			return false;
		}
		strings.emplace_back(str);
		return true;
	}

	/*!
	 * @brief Sort the strings in the builder suitable for extension lists.
	 *
	 * The list will be sorted first by API (VK, XR, etc.), then all KHR extensions,
	 * then all EXT extensions, then all Vendor extensions, then all experimental
	 * extensions. (Alphabetical within each group.)
	 *
	 * This modifies the builder in-place.
	 */
	void
	sortForExtensions();

	/*!
	 * @brief Build and return an immutable ExtensionList.
	 *
	 * This function consumes the builder by moving its internal data into
	 * the returned ExtensionList. After calling this, the builder is left in a
	 * valid but empty state.
	 *
	 * The && qualifier (rvalue reference qualifier) means this can only be
	 * called on rvalue references, i.e. on:
	 * - Temporaries: `ExtensionListBuilder().build()`
	 * - Moved objects: `std::move(builder).build()`
	 *
	 * This prevents accidentally calling build() twice on the same builder,
	 * which would be a bug.
	 */
	ExtensionList
	build() &&
	{
		return ExtensionList(std::move(strings));
	}


private:
	std::vector<std::string> strings;
};

} // namespace xrt::auxiliary::util
