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

#include "xrt/xrt_compiler.h"


#ifdef __cplusplus
extern "C" {
#endif


/*!
 * @brief An immutable collection of extension strings, these are used to
 * interface with APIs such as Vulkan and OpenXR who use strings for extensions.
 *
 * This structure stores copies of strings internally and provides read-only access.
 *
 * @see xrt::auxiliary::util::ExtensionList
 */
struct u_extension_list;

/*!
 * @brief A builder for constructing immutable extension lists.
 *
 * This structure allows write-only operations to build up a list of extension
 * strings.
 *
 * @see xrt::auxiliary::util::ExtensionListBuilder
 */
struct u_extension_list_builder;

/*
 *
 * Immutable Extension List API
 *
 */

/*!
 * @brief Create an empty extension list, some APIs might require an extension
 * list to be passed in so there is some utility for this function.
 *
 * @public @memberof u_extension_list
 */
struct u_extension_list *
u_extension_list_create(void);

/*!
 * @brief Create a new extension list from an array of strings.
 *
 * The strings are copied into the list.
 *
 * @param arr an array of zero or more non-null, null-terminated strings.
 * @param size the number of elements in the array.
 *
 * @public @memberof u_extension_list
 */
struct u_extension_list *
u_extension_list_create_from_array(const char *const *arr, uint32_t size);

/*!
 * @brief Retrieve the number of elements in the list
 *
 * @public @memberof u_extension_list
 */
uint32_t
u_extension_list_get_size(const struct u_extension_list *uel);

/*!
 * @brief Retrieve the data pointer of the list
 *
 * @public @memberof u_extension_list
 */
const char *const *
u_extension_list_get_data(const struct u_extension_list *uel);

/*!
 * @brief Check if the string is in the list.
 *
 * (Comparing string contents, not pointers)
 *
 * @param usl self pointer
 * @param str a non-null, null-terminated string.
 *
 * @return true if the string is in the list.
 *
 * @public @memberof u_extension_list
 */
bool
u_extension_list_contains(const struct u_extension_list *uel, const char *str);

/*!
 * @brief Check if the string is in the list.
 *
 * (Comparing string contents, not pointers)
 *
 * @param usl self pointer
 * @param str a string pointer.
 * @param len the length of the string.
 *
 * @return true if the string is in the list.
 *
 * @public @memberof u_extension_list
 */
bool
u_extension_list_contains_len(const struct u_extension_list *uel, const char *str, size_t len);

/*!
 * @brief Destroy an extension list.
 *
 * Performs null checks and sets your pointer to zero.
 *
 * @public @memberof u_extension_list
 */
void
u_extension_list_destroy(struct u_extension_list **list_ptr);


/*
 *
 * Extension List Builder API
 *
 */

/*!
 * @brief Create an extension list builder.
 *
 * @public @memberof u_extension_list_builder
 */
struct u_extension_list_builder *
u_extension_list_builder_create(void);

/*!
 * @brief Create an extension list builder with room for at least the given number of strings.
 *
 * @public @memberof u_extension_list_builder
 */
struct u_extension_list_builder *
u_extension_list_builder_create_with_capacity(uint32_t capacity);

/*!
 * @brief Create an extension list builder from an array of strings.
 *
 * The strings are copied into the builder.
 *
 * @param arr an array of zero or more non-null, null-terminated strings.
 * @param size the number of elements in the array.
 *
 * @public @memberof u_extension_list_builder
 */
struct u_extension_list_builder *
u_extension_list_builder_create_from_array(const char *const *arr, uint32_t size);

/*!
 * @brief Append a new string to the builder.
 *
 * The string is copied into the builder.
 *
 * @param uslb self pointer
 * @param str a non-null, null-terminated string.
 * @return 1 if successfully added, negative for errors.
 *
 * @public @memberof u_extension_list_builder
 */
int
u_extension_list_builder_append(struct u_extension_list_builder *uelb, const char *str);

/*!
 * @brief Append a new string to the builder.
 *
 * The string is copied into the builder.
 *
 * @param uslb self pointer
 * @param str a string pointer.
 * @param len the length of the string.
 * @return 1 if successfully added, negative for errors.
 *
 * @public @memberof u_extension_list_builder
 */
int
u_extension_list_builder_append_len(struct u_extension_list_builder *uelb, const char *str, size_t len);

/*!
 * @brief Append an array of new strings to the builder.
 *
 * The strings are copied into the builder.
 *
 * @param uslb self pointer
 * @param arr an array of zero or more non-null, null-terminated strings.
 * @param size the number of elements in the array.
 * @return 1 if successfully added, negative for errors.
 *
 * @public @memberof u_extension_list_builder
 */
int
u_extension_list_builder_append_array(struct u_extension_list_builder *uelb, const char *const *arr, uint32_t size);

/*!
 * @brief Append a new string to the builder, if it's not the same as a string already in the builder.
 *
 * (Comparing string contents, not pointers)
 *
 * The string is copied into the builder.
 *
 * @param uslb self pointer
 * @param str a non-null, null-terminated string.
 * @return 1 if successfully added, 0 if already existing so not added, negative for errors.
 *
 * @public @memberof u_extension_list_builder
 */
int
u_extension_list_builder_append_unique(struct u_extension_list_builder *uelb, const char *str);

/*!
 * @brief Append a new string to the builder, if it's not the same as a string already in the builder.
 *
 * (Comparing string contents, not pointers)
 *
 * The string is copied into the builder.
 *
 * @param uslb self pointer
 * @param str a string pointer.
 * @param len the length of the string.
 * @return 1 if successfully added, 0 if already existing so not added, negative for errors.
 *
 * @public @memberof u_extension_list_builder
 */
int
u_extension_list_builder_append_unique_len(struct u_extension_list_builder *uelb, const char *str, size_t len);

/*!
 * @brief Build an immutable extension list from the builder.
 *
 * After calling this, the builder is destroyed and the pointer is set to NULL.
 *
 * @param builder_ptr pointer to self pointer
 * @return a new immutable extension list, or NULL on error.
 *
 * @public @memberof u_extension_list_builder
 */
XRT_NONNULL_ALL struct u_extension_list *
u_extension_list_builder_build(struct u_extension_list_builder **builder_ptr);

/*!
 * @brief Build an immutable extension list from the builder and sort it for
 * extension lists.
 *
 * The list will be sorted first by API (VK, XR, etc.), then all KHR extensions,
 * then all EXT extensions, then all Vendor extensions, then all experimental
 * extensions. (Alphabetical within each group.)
 *
 * After calling this, the builder is destroyed and the pointer is set to NULL.
 *
 * @param builder_ptr pointer to self pointer
 * @return a new immutable extension list, sorted for extension lists, or NULL on error.
 *
 * @public @memberof u_extension_list_builder
 */
XRT_NONNULL_ALL struct u_extension_list *
u_extension_list_builder_build_sorted_for_extensions(struct u_extension_list_builder **builder_ptr);

/*!
 * @brief Destroy an extension list builder.
 *
 * Performs null checks and sets your pointer to zero.
 *
 * @public @memberof u_extension_list_builder
 */
XRT_NONNULL_ALL void
u_extension_list_builder_destroy(struct u_extension_list_builder **builder_ptr);

#ifdef __cplusplus
} // extern "C"
#endif
