// Copyright 2022, Collabora, Ltd.
// Copyright 2024-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Pretty printing various Monado things.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_pretty
 */
#pragma once

#include "xrt/xrt_defines.h"


#ifdef __cplusplus
extern "C" {
#endif


/*!
 * @defgroup aux_pretty Pretty printing functions and helpers
 * @ingroup aux_util
 *
 * This is common functionality used directly and shared by additional pretty
 * printing functions implemented in multiple modules, such as @ref oxr_api.
 *
 * Some functions have a `_indented` suffix added to them, this means that what
 * they print starts indented, but also they start with a newline. This is so
 * they can easily be chained together to form a debug message printing out
 * various information. Most of the final logging functions in Monado inserts a
 * newline at the end of the message and we don't want two to be inserted.
 *
 * There are also helpers that goes from an enum to a string that that doesn't
 * use the delegate to do the printing, these returns string that are compiled
 * into the binary. They will return 'UNKNOWN' if they don't know the value.
 * But can be made to return NULL on unknown.
 */

/*!
 * Returns a string of the input name, or NULL if invalid.
 *
 * @ingroup aux_pretty
 */
const char *
u_str_xrt_input_name_or_null(enum xrt_input_name name);

/*!
 * Returns a string of the output name, or NULL if invalid.
 *
 * @ingroup aux_pretty
 */
const char *
u_str_xrt_output_name_or_null(enum xrt_output_name name);

/*!
 * Returns a string of the device name, or NULL if invalid.
 *
 * @ingroup aux_pretty
 */
const char *
u_str_xrt_device_name_or_null(enum xrt_device_name name);

/*!
 * Returns a string of the result, or NULL if invalid.
 *
 * @ingroup aux_pretty
 */
const char *
u_str_xrt_result_or_null(xrt_result_t xret);

#define U_STR_NO_NULL(NAME, TYPE)                                                                                      \
	static inline const char *NAME(TYPE enumerate)                                                                 \
	{                                                                                                              \
		const char *str = NAME##_or_null(enumerate);                                                           \
		return str != NULL ? str : "UNKNOWN";                                                                  \
	}

U_STR_NO_NULL(u_str_xrt_input_name, enum xrt_input_name)
U_STR_NO_NULL(u_str_xrt_output_name, enum xrt_output_name)
U_STR_NO_NULL(u_str_xrt_device_name, enum xrt_device_name)
U_STR_NO_NULL(u_str_xrt_result, xrt_result_t)

#undef U_STR_NO_NULL


/*!
 * Function prototype for receiving pretty printed strings.
 *
 * @note Do not keep a reference to the pointer as it's often allocated on the
 * stack for speed.
 *
 * @ingroup aux_pretty
 */
typedef void (*u_pp_delegate_func_t)(void *ptr, const char *str, size_t length);

/*!
 * Helper struct to hold a function pointer and data pointer.
 *
 * @ingroup aux_pretty
 */
struct u_pp_delegate
{
	//! Userdata pointer, placed first to match D/Volt delegates.
	void *ptr;

	//! String receiving function.
	u_pp_delegate_func_t func;
};

/*!
 * Helper typedef for delegate struct, less typing.
 *
 * @ingroup aux_pretty
 */
typedef struct u_pp_delegate u_pp_delegate_t;

/*!
 * Formats a string and sends to the delegate.
 *
 * @ingroup aux_pretty
 */
void
u_pp(struct u_pp_delegate dg, const char *fmt, ...) XRT_PRINTF_FORMAT(2, 3);

/*!
 * Pretty prints the @ref xrt_input_name.
 *
 * @ingroup aux_pretty
 */
void
u_pp_xrt_input_name(struct u_pp_delegate dg, enum xrt_input_name name);

/*!
 * Pretty prints the @ref xrt_output_name.
 *
 * @ingroup aux_pretty
 */
void
u_pp_xrt_output_name(struct u_pp_delegate dg, enum xrt_output_name name);

/*!
 * Pretty prints the @ref xrt_result_t.
 *
 * @ingroup aux_pretty
 */
void
u_pp_xrt_result(struct u_pp_delegate dg, xrt_result_t xret);

/*!
 * Pretty prints the @ref xrt_reference_space_type.
 *
 * @ingroup aux_pretty
 */
void
u_pp_xrt_reference_space_type(struct u_pp_delegate dg, enum xrt_reference_space_type type);


/*!
 * Pretty prints a milliseconds padded to be at least 16 characters, the
 * formatting is meant to be human readable, does not use locale.
 *
 * Formatted as:  " M'TTT'###.FFFms"
 * Zero:          "         0.000ms"
 *
 * If the value is 10 seconds or larger (MM) then it will be longer then 16
 * characters.
 */
void
u_pp_padded_pretty_ms(u_pp_delegate_t dg, uint64_t value_ns);


/*
 *
 * Math struct printers.
 *
 */

/*!
 * Printers for math structs. None of these functions inserts trailing newlines
 * because it's hard to remove a trailing newline but easy to add one if one
 * should be needed. The small functions do not insert a starting newline while
 * the other functions does. This is so that you can easily chain print
 * functions to print a struct.
 *
 * @note xrt_matrix_* parameters assumed to be column major.
 *
 * @ingroup aux_pretty
 * @{
 */
void
u_pp_small_vec3(u_pp_delegate_t dg, const struct xrt_vec3 *vec);

void
u_pp_small_pose(u_pp_delegate_t dg, const struct xrt_pose *pose);

void
u_pp_small_matrix_3x3(u_pp_delegate_t dg, const struct xrt_matrix_3x3 *m);

void
u_pp_small_matrix_4x4(u_pp_delegate_t dg, const struct xrt_matrix_4x4 *m);

void
u_pp_small_matrix_4x4_f64(u_pp_delegate_t dg, const struct xrt_matrix_4x4_f64 *m);

void
u_pp_small_array_f64(struct u_pp_delegate dg, const double *arr, size_t n);

void
u_pp_small_array2d_f64(struct u_pp_delegate dg, const double *arr, size_t n, size_t m);

void
u_pp_vec3(u_pp_delegate_t dg, const struct xrt_vec3 *vec, const char *name, const char *indent);

void
u_pp_pose(u_pp_delegate_t dg, const struct xrt_pose *pose, const char *name, const char *indent);

void
u_pp_matrix_3x3(u_pp_delegate_t dg, const struct xrt_matrix_3x3 *m, const char *name, const char *indent);

void
u_pp_matrix_4x4(u_pp_delegate_t dg, const struct xrt_matrix_4x4 *m, const char *name, const char *indent);

void
u_pp_matrix_4x4_f64(u_pp_delegate_t dg, const struct xrt_matrix_4x4_f64 *m, const char *name, const char *indent);

//! Pretty prints `double arr[n]`
void
u_pp_array_f64(u_pp_delegate_t dg, const double *arr, size_t n, const char *name, const char *indent);

//! Pretty prints `double arr[n][m]`
void
u_pp_array2d_f64(u_pp_delegate_t dg, const double *arr, size_t n, size_t m, const char *name, const char *indent);

/*!
 * @}
 */


/*
 *
 * Extension list printers.
 *
 */

struct u_extension_list;

/*!
 * @brief Print all the strings in the list with the given prefix.
 *
 * @param dg delegate to use for printing
 * @param usl list of strings to print (must not be NULL)
 * @param prefix prefix to add to each string to be printed (must not be NULL)
 * @ingroup aux_pretty
 */
XRT_NONNULL_ALL void
u_pp_string_list(struct u_pp_delegate dg, struct u_extension_list *usl, const char *prefix);

/*!
 * @brief Pretty print the extension list with extension information.
 *
 * It will start on a new line with the enabled extensions, showing which are
 * required vs optional. Then if there are optional extensions not enabled it
 * will list those separately, distinguishing between unsupported and skipped.
 *
 * @param dg delegate to use for printing
 * @param enabled_list list of extensions that were enabled (must not be NULL)
 * @param optional_list optional list to compare against (must not be NULL)
 * @param skipped_list list of extensions that were skipped (must not be NULL)
 * @ingroup aux_pretty
 */
XRT_NONNULL_ALL void
u_pp_string_list_extensions(struct u_pp_delegate dg,
                            struct u_extension_list *enabled_list,
                            struct u_extension_list *optional_list,
                            struct u_extension_list *skipped_list);


/*
 *
 * Sinks.
 *
 */

/*!
 * Stack only pretty printer sink, no need to free, must be inited before use.
 *
 * @ingroup aux_pretty
 */
struct u_pp_sink_stack_only
{
	//! How much of the buffer is used.
	size_t used;

	//! Storage for the sink.
	char buffer[1024 * 8];
};

u_pp_delegate_t
u_pp_sink_stack_only_init(struct u_pp_sink_stack_only *sink);


#ifdef __cplusplus
}
#endif
