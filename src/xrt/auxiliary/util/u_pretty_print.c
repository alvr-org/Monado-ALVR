// Copyright 2022-2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Pretty printing various Monado things.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup aux_pretty
 */

#include "util/u_misc.h"
#include "util/u_pretty_print.h"

#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>


/*
 *
 * Internal helpers.
 *
 */

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
	default: return "<UNKNOWN>";
	}
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

	dg.func(dg.ptr, dst, size);

	if (tmp != dst) {
		free(dst);
	}
}

void
u_pp_xrt_input_name(struct u_pp_delegate dg, enum xrt_input_name name)
{
#define XRT_INPUT_LIST_TO_CASE(NAME, _)                                                                                \
	case NAME: DG(#NAME); return;

	switch (name) {
		XRT_INPUT_LIST(XRT_INPUT_LIST_TO_CASE)
	}

#undef XRT_INPUT_LIST_TO_CASE

	/*
	 * No default case so we get warnings of missing entries.
	 * Invalid values handled below.
	 */

	uint32_t id = XRT_GET_INPUT_ID(name);
	enum xrt_input_type type = XRT_GET_INPUT_TYPE(name);
	const char *str = get_xrt_input_type_short_str(type);

	u_pp(dg, "XRT_INPUT_0x%04x_%s", id, str);
}

void
u_pp_xrt_result(struct u_pp_delegate dg, xrt_result_t xret)
{
	// clang-format off
	switch (xret) {
	case XRT_SUCCESS:                                    DG("XRT_SUCCESS"); return;
	case XRT_TIMEOUT:                                    DG("XRT_TIMEOUT"); return;
	case XRT_SPACE_BOUNDS_UNAVAILABLE:                   DG("XRT_SPACE_BOUNDS_UNAVAILABLE"); return;
	case XRT_ERROR_IPC_FAILURE:                          DG("XRT_ERROR_IPC_FAILURE"); return;
	case XRT_ERROR_NO_IMAGE_AVAILABLE:                   DG("XRT_ERROR_NO_IMAGE_AVAILABLE"); return;
	case XRT_ERROR_VULKAN:                               DG("XRT_ERROR_VULKAN"); return;
	case XRT_ERROR_OPENGL:                               DG("XRT_ERROR_OPENGL"); return;
	case XRT_ERROR_FAILED_TO_SUBMIT_VULKAN_COMMANDS:     DG("XRT_ERROR_FAILED_TO_SUBMIT_VULKAN_COMMANDS"); return;
	case XRT_ERROR_SWAPCHAIN_FLAG_VALID_BUT_UNSUPPORTED: DG("XRT_ERROR_SWAPCHAIN_FLAG_VALID_BUT_UNSUPPORTED"); return;
	case XRT_ERROR_ALLOCATION:                           DG("XRT_ERROR_ALLOCATION"); return;
	case XRT_ERROR_POSE_NOT_ACTIVE:                      DG("XRT_ERROR_POSE_NOT_ACTIVE"); return;
	case XRT_ERROR_FENCE_CREATE_FAILED:                  DG("XRT_ERROR_FENCE_CREATE_FAILED"); return;
	case XRT_ERROR_NATIVE_HANDLE_FENCE_ERROR:            DG("XRT_ERROR_NATIVE_HANDLE_FENCE_ERROR"); return;
	case XRT_ERROR_MULTI_SESSION_NOT_IMPLEMENTED:        DG("XRT_ERROR_MULTI_SESSION_NOT_IMPLEMENTED"); return;
	case XRT_ERROR_SWAPCHAIN_FORMAT_UNSUPPORTED:         DG("XRT_ERROR_SWAPCHAIN_FORMAT_UNSUPPORTED"); return;
	case XRT_ERROR_EGL_CONFIG_MISSING:                   DG("XRT_ERROR_EGL_CONFIG_MISSING"); return;
	case XRT_ERROR_THREADING_INIT_FAILURE:               DG("XRT_ERROR_THREADING_INIT_FAILURE"); return;
	case XRT_ERROR_IPC_SESSION_NOT_CREATED:              DG("XRT_ERROR_IPC_SESSION_NOT_CREATED"); return;
	case XRT_ERROR_IPC_SESSION_ALREADY_CREATED:          DG("XRT_ERROR_IPC_SESSION_ALREADY_CREATED"); return;
	case XRT_ERROR_PROBER_NOT_SUPPORTED:                 DG("XRT_ERROR_PROBER_NOT_SUPPORTED"); return;
	case XRT_ERROR_PROBER_CREATION_FAILED:               DG("XRT_ERROR_PROBER_CREATION_FAILED"); return;
	case XRT_ERROR_PROBER_LIST_LOCKED:                   DG("XRT_ERROR_PROBER_LIST_LOCKED"); return;
	case XRT_ERROR_PROBER_LIST_NOT_LOCKED:               DG("XRT_ERROR_PROBER_LIST_NOT_LOCKED"); return;
	case XRT_ERROR_PROBING_FAILED:                       DG("XRT_ERROR_PROBING_FAILED"); return;
	case XRT_ERROR_DEVICE_CREATION_FAILED:               DG("XRT_ERROR_DEVICE_CREATION_FAILED"); return;
	case XRT_ERROR_D3D:                                  DG("XRT_ERROR_D3D"); return;
	case XRT_ERROR_D3D11:                                DG("XRT_ERROR_D3D11"); return;
	case XRT_ERROR_D3D12:                                DG("XRT_ERROR_D3D12"); return;
	case XRT_ERROR_RECENTERING_NOT_SUPPORTED:            DG("XRT_ERROR_RECENTERING_NOT_SUPPORTED"); return;
	case XRT_ERROR_COMPOSITOR_NOT_SUPPORTED:             DG("XRT_ERROR_COMPOSITOR_NOT_SUPPORTED"); return;
	case XRT_ERROR_IPC_COMPOSITOR_NOT_CREATED:           DG("XRT_ERROR_IPC_COMPOSITOR_NOT_CREATED"); return;
	case XRT_ERROR_NOT_IMPLEMENTED:      DG("XRT_ERROR_NOT_IMPLEMENTED"); return;
	}
	// clang-format on

	/*
	 * No default case so we get warnings of missing entries.
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
 * Sink functions.
 *
 */

u_pp_delegate_t
u_pp_sink_stack_only_init(struct u_pp_sink_stack_only *sink)
{
	sink->used = 0;
	return (u_pp_delegate_t){sink, stack_only_sink};
}
