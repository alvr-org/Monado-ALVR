// Copyright 2021-2026, Collabora, Ltd.
// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief A collection of strings for extensions.
 *
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @ingroup aux_util
 *
 */

#include "u_extension_list.h"
#include "u_extension_list.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>


/*
 *
 * Defines and structs.
 *
 */

using xrt::auxiliary::util::ExtensionList;
using xrt::auxiliary::util::ExtensionListBuilder;

struct u_extension_list
{
	u_extension_list() = default;
	u_extension_list(ExtensionList &&extension_list) : list(std::move(extension_list)) {}

	ExtensionList list;
};

struct u_extension_list_builder
{
	u_extension_list_builder() = default;
	u_extension_list_builder(size_t capacity) : builder(capacity) {}

	u_extension_list_builder(u_extension_list_builder &&) = delete;
	u_extension_list_builder(u_extension_list_builder const &) = delete;

	u_extension_list_builder &
	operator=(u_extension_list_builder &&) = delete;
	u_extension_list_builder &
	operator=(u_extension_list_builder const &) = delete;

	ExtensionListBuilder builder;
};


/*
 *
 * Helpers
 *
 */

enum class ExtensionType : std::uint8_t
{
	KHR = 0,         // Khronos extensions
	EXT = 1,         // Multi-vendor extensions
	VENDOR = 2,      // Vendor-specific extensions (AMD, NV, INTEL, etc.)
	EXPERIMENTAL = 3 // Experimental extensions
};

/*!
 * @brief Helper class for sorting extension names.
 *
 * Encapsulates the sort key with comparison operators for cleaner sorting.
 */
struct ExtensionSortKey
{
	std::string_view api_prefix;
	ExtensionType type;
	std::string_view name;

	/*!
	 * Sorts in field declaration order:
	 *
	 * 1. API prefix (VK, XR, etc.)
	 * 2. Extension type (KHR < EXT < VENDOR < EXPERIMENTAL)
	 * 3. Alphabetically by full name
	 */
	auto
	operator<=>(const ExtensionSortKey &other) const = default;
};

/*!
 * @brief Check if a vendor string represents an experimental extension.
 *
 * Experimental extensions have vendor codes ending with 'X' optionally followed
 * by digits.
 *
 * Examples: NVX, AMDX, NVX1, NVX2, INTELX
 *
 * Special case: QNX is a legitimate vendor (QNX operating system), not
 * experimental.
 */
static bool
is_experimental_vendor(std::string_view vendor)
{
	// Special case: QNX is a legitimate vendor, not experimental
	if (vendor == "QNX") {
		return false;
	}

	// Check if vendor ends with 'X' optionally followed by digits
	if (vendor.empty()) {
		return false;
	}

	size_t len = vendor.length();
	size_t i = len - 1;

	// Skip trailing digits
	while (i > 0 && std::isdigit(vendor[i])) {
		i--;
	}

	// Check if we found an 'X' and there's something before it
	return (vendor[i] == 'X' && i > 0);
}

/*!
 * @brief Get the extension type and API prefix for sorting purposes.
 *
 * Returns an ExtensionSortKey for comparison.
 */
static ExtensionSortKey
get_extension_sort_key(const std::string_view name)
{
	// Find the first underscore to separate API prefix (e.g., "VK", "XR")
	size_t first_underscore = name.find('_');
	if (first_underscore == std::string::npos) {
		// No underscore, treat as is
		return {name, ExtensionType::VENDOR, name};
	}

	std::string_view api_prefix = name.substr(0, first_underscore);

	// Find the second underscore to get the vendor/type part
	size_t second_underscore = name.find('_', first_underscore + 1);
	if (second_underscore == std::string::npos) {
		// Only one underscore, treat as vendor
		return {api_prefix, ExtensionType::VENDOR, name};
	}

	std::string_view vendor = name.substr(first_underscore + 1, second_underscore - first_underscore - 1);

	// Determine extension type based on vendor string
	ExtensionType type;
	if (vendor == "KHR") {
		type = ExtensionType::KHR;
	} else if (vendor == "EXT") {
		type = ExtensionType::EXT;
	} else if (is_experimental_vendor(vendor)) {
		type = ExtensionType::EXPERIMENTAL;
	} else {
		type = ExtensionType::VENDOR;
	}

	return {api_prefix, type, name};
}


/*
 *
 * 'Exported' functions - Immutable Extension List
 *
 */

struct u_extension_list *
u_extension_list_create()
{
	try {
		auto ret = std::make_unique<u_extension_list>();
		return ret.release();
	} catch (std::exception const &) {
		return nullptr;
	}
}

struct u_extension_list *
u_extension_list_create_from_array(const char *const *arr, uint32_t size)
{
	if (arr == nullptr || size == 0) {
		return u_extension_list_create();
	}
	try {
		ExtensionListBuilder builder(size);
		for (uint32_t i = 0; i < size; ++i) {
			if (arr[i] != nullptr) {
				builder.append(arr[i]);
			}
		}
		auto ret = std::make_unique<u_extension_list>(std::move(builder).build());
		return ret.release();
	} catch (std::exception const &) {
		return nullptr;
	}
}

uint32_t
u_extension_list_get_size(const struct u_extension_list *uel)
{
	if (uel == nullptr) {
		return 0;
	}
	return uel->list.size();
}

const char *const *
u_extension_list_get_data(const struct u_extension_list *uel)
{
	if (uel == nullptr) {
		return nullptr;
	}
	return uel->list.data();
}

bool
u_extension_list_contains(const struct u_extension_list *uel, const char *str)
{
	if (uel == nullptr || str == nullptr) {
		return false;
	}
	return uel->list.contains(str);
}

bool
u_extension_list_contains_len(const struct u_extension_list *uel, const char *str, size_t len)
{
	if (uel == nullptr || str == nullptr) {
		return false;
	}
	return uel->list.contains(std::string_view(str, len));
}

void
u_extension_list_destroy(struct u_extension_list **list_ptr)
{
	if (list_ptr == nullptr) {
		return;
	}
	u_extension_list *list = *list_ptr;
	if (list == nullptr) {
		return;
	}
	delete list;
	*list_ptr = nullptr;
}


/*
 *
 * Extension List Builder Methods
 *
 */

void
ExtensionListBuilder::sortForExtensions()
{
	if (strings.empty()) {
		return;
	}

	// Sort using our custom comparison function that works with std::string
	auto cmp = [](const std::string_view a, const std::string_view b) {
		return get_extension_sort_key(a) < get_extension_sort_key(b);
	};
	std::sort(strings.begin(), strings.end(), cmp);
}


/*
 *
 * 'Exported' functions - Extension List Builder
 *
 */

struct u_extension_list_builder *
u_extension_list_builder_create()
{
	try {
		auto ret = std::make_unique<u_extension_list_builder>();
		return ret.release();
	} catch (std::exception const &) {
		return nullptr;
	}
}

struct u_extension_list_builder *
u_extension_list_builder_create_with_capacity(uint32_t capacity)
{
	try {
		auto ret = std::make_unique<u_extension_list_builder>(capacity);
		return ret.release();
	} catch (std::exception const &) {
		return nullptr;
	}
}

struct u_extension_list_builder *
u_extension_list_builder_create_from_array(const char *const *arr, uint32_t size)
{
	if (arr == nullptr || size == 0) {
		return u_extension_list_builder_create();
	}
	try {
		auto ret = std::make_unique<u_extension_list_builder>();
		for (uint32_t i = 0; i < size; ++i) {
			if (arr[i] != nullptr) {
				ret->builder.append(arr[i]);
			}
		}
		return ret.release();
	} catch (std::exception const &) {
		return nullptr;
	}
}

int
u_extension_list_builder_append(struct u_extension_list_builder *uelb, const char *str)
{
	if (uelb == nullptr || str == nullptr) {
		return -1;
	}
	try {
		uelb->builder.append(str);
		return 1;
	} catch (std::exception const &) {
		return -1;
	}
}

int
u_extension_list_builder_append_len(struct u_extension_list_builder *uelb, const char *str, size_t len)
{
	if (uelb == nullptr || str == nullptr) {
		return -1;
	}
	try {
		uelb->builder.append(std::string_view(str, len));
		return 1;
	} catch (std::exception const &) {
		return -1;
	}
}

int
u_extension_list_builder_append_array(struct u_extension_list_builder *uelb, const char *const *arr, uint32_t size)
{
	if (uelb == nullptr || arr == nullptr) {
		return -1;
	}
	try {
		for (uint32_t i = 0; i < size; ++i) {
			if (arr[i] != nullptr) {
				uelb->builder.append(arr[i]);
			}
		}
		return 1;
	} catch (std::exception const &) {
		return -1;
	}
}

int
u_extension_list_builder_append_unique(struct u_extension_list_builder *uelb, const char *str)
{
	if (uelb == nullptr || str == nullptr) {
		return -1;
	}
	try {
		auto added = uelb->builder.appendUnique(str);
		return added ? 1 : 0;
	} catch (std::exception const &) {
		return -1;
	}
}

int
u_extension_list_builder_append_unique_len(struct u_extension_list_builder *uelb, const char *str, size_t len)
{
	if (uelb == nullptr || str == nullptr) {
		return -1;
	}
	try {
		auto added = uelb->builder.appendUnique(std::string_view(str, len));
		return added ? 1 : 0;
	} catch (std::exception const &) {
		return -1;
	}
}

struct u_extension_list *
u_extension_list_builder_build(struct u_extension_list_builder **builder_ptr)
{
	u_extension_list_builder *builder = *builder_ptr;
	if (builder == nullptr) {
		return nullptr;
	}

	try {
		auto ret = std::make_unique<u_extension_list>(std::move(builder->builder).build());
		delete builder;
		*builder_ptr = nullptr;
		return ret.release();
	} catch (std::exception const &) {
		return nullptr;
	}
}

struct u_extension_list *
u_extension_list_builder_build_sorted_for_extensions(struct u_extension_list_builder **builder_ptr)
{
	u_extension_list_builder *builder = *builder_ptr;
	if (builder == nullptr) {
		return nullptr;
	}

	try {
		// Sort the builder in-place before building
		builder->builder.sortForExtensions();

		// Now build the sorted list
		auto ret = std::make_unique<u_extension_list>(std::move(builder->builder).build());
		delete builder;
		*builder_ptr = nullptr;
		return ret.release();
	} catch (std::exception const &) {
		return nullptr;
	}
}

void
u_extension_list_builder_destroy(struct u_extension_list_builder **builder_ptr)
{
	u_extension_list_builder *builder = *builder_ptr;
	if (builder == nullptr) {
		return;
	}
	delete builder;
	*builder_ptr = nullptr;
}
