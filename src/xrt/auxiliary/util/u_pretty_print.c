// Copyright 2022-2026, Collabora, Ltd.
// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Pretty printing various Monado things.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup aux_pretty
 */

#include "xrt/xrt_macro_lists.h"
#include "util/u_misc.h"
#include "util/u_extension_list.h"
#include "util/u_pretty_print.h"

#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <inttypes.h>


/*
 *
 * Internal helpers.
 *
 */

// clang-format off
#define X_MACRO_ENUM_CASE_RETURN_STRING(NAME) case NAME: return #NAME;
// clang-format on

#define DG(str) (dg.func(dg.ptr, str, strlen(str)))

const char *
get_xrt_input_type_short_str(enum xrt_input_type type)
{
	switch (type) {
	case XRT_INPUT_TYPE_VEC1_ZERO_TO_ONE: return "VEC1_ZERO_TO_ONE";
	case XRT_INPUT_TYPE_VEC1_MINUS_ONE_TO_ONE: return "VEC1_MINUS_ONE_TO_ONE";
	case XRT_INPUT_TYPE_VEC2_MINUS_ONE_TO_ONE: return "VEC2_MINUS_ONE_TO_ONE";
	case XRT_INPUT_TYPE_VEC3_MINUS_ONE_TO_ONE: return "VEC3_MINUS_ONE_TO_ONE";
	case XRT_INPUT_TYPE_BOOLEAN: return "BOOLEAN";
	case XRT_INPUT_TYPE_POSE: return "POSE";
	case XRT_INPUT_TYPE_HAND_TRACKING: return "HAND_TRACKING";
	case XRT_INPUT_TYPE_FACE_TRACKING: return "FACE_TRACKING";
	case XRT_INPUT_TYPE_BODY_TRACKING: return "BODY_TRACKING";
	}

	return "<UNKNOWN>";
}

const char *
get_xrt_output_type_short_str(enum xrt_output_type type)
{
	switch (type) {
	case XRT_OUTPUT_TYPE_VIBRATION: return "XRT_OUTPUT_TYPE_VIBRATION";
	case XRT_OUTPUT_TYPE_FORCE_FEEDBACK: return "XRT_OUTPUT_TYPE_FORCE_FEEDBACK";
	}

	return "<UNKNOWN>";
}

void
stack_only_sink(void *ptr, const char *str, size_t length)
{
	struct u_pp_sink_stack_only *sink = (struct u_pp_sink_stack_only *)ptr;

	size_t used = sink->used;
	size_t left = ARRAY_SIZE(sink->buffer) - used;
	if (left == 0) {
		return;
	}

	if (length >= left) {
		length = left - 1;
	}

	memcpy(sink->buffer + used, str, length);

	used += length;

	// Null terminate and update used.
	sink->buffer[used] = '\0';
	sink->used = used;
}

int
update_longest_extension_name_length(struct u_extension_list *list, int current_longest_extension)
{
	if (list == NULL) {
		return current_longest_extension;
	}

	size_t count = u_extension_list_get_size(list);
	const char *const *data = u_extension_list_get_data(list);

	for (size_t i = 0; i < count; i++) {
		int len = (int)strlen(data[i]);
		if (len > current_longest_extension) {
			current_longest_extension = len;
		}
	}
	return current_longest_extension;
}


/*
 *
 * 'Exported' str functions.
 *
 */

const char *
u_str_xrt_input_name_or_null(enum xrt_input_name name)
{
	// No default case so we get warnings of missing entries.
	switch (name) {
		XRT_INPUT_NAME_LIST(X_MACRO_ENUM_CASE_RETURN_STRING)
	}

	return NULL;
}

const char *
u_str_xrt_output_name_or_null(enum xrt_output_name name)
{
	// No default case so we get warnings of missing entries.
	switch (name) {
		XRT_OUTPUT_NAME_LIST(X_MACRO_ENUM_CASE_RETURN_STRING)
	}

	return NULL;
}

const char *
u_str_xrt_device_name_or_null(enum xrt_device_name name)
{
	// No default case so we get warnings of missing entries.
	switch (name) {
		XRT_DEVICE_NAME_LIST(X_MACRO_ENUM_CASE_RETURN_STRING)
	}

	return NULL;
}

const char *
u_str_xrt_result_or_null(xrt_result_t xret)
{
	// No default case so we get warnings of missing entries.
	switch (xret) {
		XRT_RESULT_LIST(X_MACRO_ENUM_CASE_RETURN_STRING)
	}

	return NULL;
}


/*
 *
 * 'Exported' functions.
 *
 */

void
u_pp(struct u_pp_delegate dg, const char *fmt, ...)
{
	// Should be plenty enough for most prints.
	char tmp[1024];
	char *dst = tmp;
	va_list args;

	va_start(args, fmt);
	int ret = vsnprintf(NULL, 0, fmt, args);
	va_end(args);

	if (ret <= 0) {
		return;
	}

	size_t size = (size_t)ret;
	// Safe to do because MAX_INT should be less then MAX_SIZE_T
	size_t size_with_null = size + 1;

	if (size_with_null > ARRAY_SIZE(tmp)) {
		dst = U_TYPED_ARRAY_CALLOC(char, size_with_null);
	}

	va_start(args, fmt);
	ret = vsnprintf(dst, size_with_null, fmt, args);
	va_end(args);

	// Second call to `vsnprintf` with a buffer of the right size is supposed to succeed and write the exact amount.
	assert(ret == (int)size);

	if (ret > 0) {
		dg.func(dg.ptr, dst, size);
	}

	if (tmp != dst) {
		free(dst);
	}
}

void
u_pp_xrt_input_name(struct u_pp_delegate dg, enum xrt_input_name name)
{
	const char *might_be_null = u_str_xrt_input_name_or_null(name);
	if (might_be_null != NULL) {
		DG(might_be_null);
		return;
	}

	/*
	 * Invalid values handled below.
	 */

	uint32_t id = XRT_GET_INPUT_ID(name);
	enum xrt_input_type type = XRT_GET_INPUT_TYPE(name);
	const char *str = get_xrt_input_type_short_str(type);

	u_pp(dg, "XRT_INPUT_0x%04x_%s", id, str);
}

void
u_pp_xrt_output_name(struct u_pp_delegate dg, enum xrt_output_name name)
{
	const char *might_be_null = u_str_xrt_output_name_or_null(name);
	if (might_be_null != NULL) {
		DG(might_be_null);
		return;
	}

	/*
	 * Invalid values handled below.
	 */

	uint32_t id = XRT_GET_OUTPUT_ID(name);
	enum xrt_output_type type = XRT_GET_OUTPUT_TYPE(name);
	const char *str = get_xrt_output_type_short_str(type);

	u_pp(dg, "XRT_OUTPUT_0x%04x_%s", id, str);
}

void
u_pp_xrt_result(struct u_pp_delegate dg, xrt_result_t xret)
{
	const char *might_be_null = u_str_xrt_result_or_null(xret);
	if (might_be_null != NULL) {
		DG(might_be_null);
		return;
	}

	/*
	 * Invalid values handled below.
	 */

	if (xret < 0) {
		u_pp(dg, "XRT_ERROR_0x%08x", xret);
	} else {
		u_pp(dg, "XRT_SUCCESS_0x%08x", xret);
	}
}

void
u_pp_xrt_reference_space_type(struct u_pp_delegate dg, enum xrt_reference_space_type type)
{
	// clang-format off
	switch (type) {
	case XRT_SPACE_REFERENCE_TYPE_VIEW:                  DG("XRT_SPACE_REFERENCE_TYPE_VIEW"); return;
	case XRT_SPACE_REFERENCE_TYPE_LOCAL:                 DG("XRT_SPACE_REFERENCE_TYPE_LOCAL"); return;
	case XRT_SPACE_REFERENCE_TYPE_LOCAL_FLOOR:           DG("XRT_SPACE_REFERENCE_TYPE_LOCAL_FLOOR"); return;
	case XRT_SPACE_REFERENCE_TYPE_STAGE:                 DG("XRT_SPACE_REFERENCE_TYPE_STAGE"); return;
	case XRT_SPACE_REFERENCE_TYPE_UNBOUNDED:             DG("XRT_SPACE_REFERENCE_TYPE_UNBOUNDED"); return;
	}
	// clang-format on

	/*
	 * No default case so we get warnings of missing entries.
	 * Invalid values handled below.
	 */

	switch ((uint32_t)type) {
	case XRT_SPACE_REFERENCE_TYPE_COUNT: DG("XRT_SPACE_REFERENCE_TYPE_COUNT"); return;
	case XRT_SPACE_REFERENCE_TYPE_INVALID: DG("XRT_SPACE_REFERENCE_TYPE_INVALID"); return;
	default: u_pp(dg, "XRT_SPACE_REFERENCE_TYPE_0x%08x", type); return;
	}
}

void
u_pp_padded_pretty_ms(u_pp_delegate_t dg, uint64_t value_ns)
{
	uint64_t in_us = value_ns / 1000;
	uint64_t in_ms = in_us / 1000;
	uint64_t in_1_000_ms = in_ms / 1000;
	uint64_t in_1_000_000_ms = in_1_000_ms / 1000;

	// Prints " M'TTT'###.FFFms"

	// " M'"
	if (in_1_000_000_ms >= 1) {
		u_pp(dg, " %" PRIu64 "'", in_1_000_000_ms);
	} else {
		//       " M'"
		u_pp(dg, "   ");
	}

	// "TTT'"
	if (in_1_000_ms >= 1000) {
		// Need to pad with zeros
		u_pp(dg, "%03" PRIu64 "'", in_1_000_ms % 1000);
	} else if (in_1_000_ms >= 1) {
		// Pad with spaces, we need to write a number.
		u_pp(dg, "%3" PRIu64 "'", in_1_000_ms);
	} else {
		//       "TTT'"
		u_pp(dg, "    ");
	}

	// "###"
	if (in_ms >= 1000) {
		// Need to pad with zeros
		u_pp(dg, "%03" PRIu64, in_ms % 1000);
	} else {
		// Pad with spaces, always need a numbere here.
		u_pp(dg, "%3" PRIu64, in_ms % 1000);
	}

	// ".FFFms"
	u_pp(dg, ".%03" PRIu64 "ms", in_us % 1000);
}


/*
 *
 * Math structs printers.
 *
 */

void
u_pp_small_vec3(u_pp_delegate_t dg, const struct xrt_vec3 *vec)
{
	u_pp(dg, "[%f, %f, %f]", vec->x, vec->y, vec->z);
}

void
u_pp_small_pose(u_pp_delegate_t dg, const struct xrt_pose *pose)
{
	const struct xrt_vec3 *p = &pose->position;
	const struct xrt_quat *q = &pose->orientation;

	u_pp(dg, "[%f, %f, %f] [%f, %f, %f, %f]", p->x, p->y, p->z, q->x, q->y, q->z, q->w);
}

void
u_pp_small_matrix_3x3(u_pp_delegate_t dg, const struct xrt_matrix_3x3 *m)
{
	u_pp(dg,
	     "[\n"
	     "\t%f, %f, %f,\n"
	     "\t%f, %f, %f,\n"
	     "\t%f, %f, %f \n"
	     "]",
	     m->v[0], m->v[3], m->v[6],  //
	     m->v[1], m->v[4], m->v[7],  //
	     m->v[2], m->v[5], m->v[8]); //
}

void
u_pp_small_matrix_4x4(u_pp_delegate_t dg, const struct xrt_matrix_4x4 *m)
{
	u_pp(dg,
	     "[\n"
	     "\t%f, %f, %f, %f,\n"
	     "\t%f, %f, %f, %f,\n"
	     "\t%f, %f, %f, %f,\n"
	     "\t%f, %f, %f, %f\n"
	     "]",
	     m->v[0], m->v[4], m->v[8], m->v[12],   //
	     m->v[1], m->v[5], m->v[9], m->v[13],   //
	     m->v[2], m->v[6], m->v[10], m->v[14],  //
	     m->v[3], m->v[7], m->v[11], m->v[15]); //
}

void
u_pp_small_matrix_4x4_f64(u_pp_delegate_t dg, const struct xrt_matrix_4x4_f64 *m)
{
	u_pp(dg,
	     "[\n"
	     "\t%f, %f, %f, %f,\n"
	     "\t%f, %f, %f, %f,\n"
	     "\t%f, %f, %f, %f,\n"
	     "\t%f, %f, %f, %f\n"
	     "]",
	     m->v[0], m->v[4], m->v[8], m->v[12],   //
	     m->v[1], m->v[5], m->v[9], m->v[13],   //
	     m->v[2], m->v[6], m->v[10], m->v[14],  //
	     m->v[3], m->v[7], m->v[11], m->v[15]); //
}

void
u_pp_small_array_f64(struct u_pp_delegate dg, const double *arr, size_t n)
{
	assert(n != 0);
	DG("[");
	for (size_t i = 0; i < n - 1; i++) {
		u_pp(dg, "%lf, ", arr[i]);
	}
	u_pp(dg, "%lf]", arr[n - 1]);
}

void
u_pp_small_array2d_f64(struct u_pp_delegate dg, const double *arr, size_t n, size_t m)
{
	DG("[\n");
	for (size_t i = 0; i < n; i++) {
		u_pp_small_array_f64(dg, &arr[i], m);
	}
	DG("\n]");
}

void
u_pp_vec3(u_pp_delegate_t dg, const struct xrt_vec3 *vec, const char *name, const char *indent)
{
	u_pp(dg, "\n%s%s = ", indent, name);
	u_pp_small_vec3(dg, vec);
}

void
u_pp_pose(u_pp_delegate_t dg, const struct xrt_pose *pose, const char *name, const char *indent)
{
	u_pp(dg, "\n%s%s = ", indent, name);
	u_pp_small_pose(dg, pose);
}

void
u_pp_matrix_3x3(u_pp_delegate_t dg, const struct xrt_matrix_3x3 *m, const char *name, const char *indent)
{
	u_pp(dg,
	     "\n%s%s = ["
	     "\n%s\t%f, %f, %f,"
	     "\n%s\t%f, %f, %f,"
	     "\n%s\t%f, %f, %f"
	     "\n%s]",
	     indent, name,                      //
	     indent, m->v[0], m->v[3], m->v[6], //
	     indent, m->v[1], m->v[4], m->v[7], //
	     indent, m->v[2], m->v[5], m->v[8], //
	     indent);                           //
}

void
u_pp_matrix_4x4(u_pp_delegate_t dg, const struct xrt_matrix_4x4 *m, const char *name, const char *indent)
{
	u_pp(dg,
	     "\n%s%s = ["
	     "\n%s\t%f, %f, %f, %f,"
	     "\n%s\t%f, %f, %f, %f,"
	     "\n%s\t%f, %f, %f, %f,"
	     "\n%s\t%f, %f, %f, %f"
	     "\n%s]",
	     indent, name,                                 //
	     indent, m->v[0], m->v[4], m->v[8], m->v[12],  //
	     indent, m->v[1], m->v[5], m->v[9], m->v[13],  //
	     indent, m->v[2], m->v[6], m->v[10], m->v[14], //
	     indent, m->v[3], m->v[7], m->v[11], m->v[15], //
	     indent);                                      //
}

void
u_pp_matrix_4x4_f64(u_pp_delegate_t dg, const struct xrt_matrix_4x4_f64 *m, const char *name, const char *indent)
{
	u_pp(dg,
	     "\n%s%s = ["
	     "\n%s\t%f, %f, %f, %f,"
	     "\n%s\t%f, %f, %f, %f,"
	     "\n%s\t%f, %f, %f, %f,"
	     "\n%s\t%f, %f, %f, %f"
	     "\n%s]",
	     indent, name,                                 //
	     indent, m->v[0], m->v[4], m->v[8], m->v[12],  //
	     indent, m->v[1], m->v[5], m->v[9], m->v[13],  //
	     indent, m->v[2], m->v[6], m->v[10], m->v[14], //
	     indent, m->v[3], m->v[7], m->v[11], m->v[15], //
	     indent);                                      //
}

void
u_pp_array_f64(u_pp_delegate_t dg, const double *arr, size_t n, const char *name, const char *indent)
{
	u_pp(dg, "\n%s%s = ", indent, name);
	u_pp_small_array_f64(dg, arr, n);
}

void
u_pp_array2d_f64(u_pp_delegate_t dg, const double *arr, size_t n, size_t m, const char *name, const char *indent)
{
	u_pp(dg, "\n%s%s = ", indent, name);
	u_pp_small_array2d_f64(dg, arr, n, m);
}


/*
 *
 * Extension list printers.
 *
 */

void
u_pp_string_list(struct u_pp_delegate dg, struct u_extension_list *usl, const char *prefix)
{
	uint32_t count = u_extension_list_get_size(usl);
	const char *const *data = u_extension_list_get_data(usl);

	for (uint32_t i = 0; i < count; i++) {
		u_pp(dg, "%s%s", prefix, data[i]);
	}
}

void
u_pp_string_list_extensions(struct u_pp_delegate dg,
                            struct u_extension_list *enabled_list,
                            struct u_extension_list *optional_list,
                            struct u_extension_list *skipped_list)
{
	assert(optional_list != NULL);
	assert(skipped_list != NULL);

	uint32_t count = u_extension_list_get_size(enabled_list);
	const char *const *data = u_extension_list_get_data(enabled_list);

	// Find the longest extension name for alignment
	int longest_extension = update_longest_extension_name_length(enabled_list, 0);
	longest_extension = update_longest_extension_name_length(skipped_list, longest_extension);
	longest_extension = update_longest_extension_name_length(optional_list, longest_extension);

	u_pp(dg, "\n\tEnabled extensions:");

	// Print enabled extensions, marking which are optional
	uint32_t optional_enabled_count = 0;
	for (uint32_t i = 0; i < count; i++) {
		const char *ext = data[i];
		bool is_optional = u_extension_list_contains(optional_list, ext);
		if (is_optional) {
			optional_enabled_count++;
		}

		u_pp(dg, "\n\t\t%-*s    %s", longest_extension, ext, is_optional ? "optional" : "required");
	}

	// All optional extensions have been enabled.
	uint32_t optional_count = u_extension_list_get_size(optional_list);
	if (optional_enabled_count == optional_count) {
		return;
	}

	// Print optional extensions that were not enabled
	u_pp(dg, "\n\tNot enabled optional extensions:");
	const char *const *optional_data = u_extension_list_get_data(optional_list);
	for (uint32_t i = 0; i < optional_count; i++) {
		const char *ext = optional_data[i];
		if (u_extension_list_contains(enabled_list, ext)) {
			continue;
		}

		// Check if this extension was skipped or unsupported
		bool was_skipped = u_extension_list_contains(skipped_list, ext);
		const char *reason = was_skipped ? "skipped" : "unsupported";

		u_pp(dg, "\n\t\t%-*s    %s", longest_extension, ext, reason);
	}
}


/*
 *
 * Sink functions.
 *
 */

u_pp_delegate_t
u_pp_sink_stack_only_init(struct u_pp_sink_stack_only *sink)
{
	sink->used = 0;
	return (u_pp_delegate_t){sink, stack_only_sink};
}
