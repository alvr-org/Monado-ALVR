// Copyright 2025, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Variant/algebraic data-type for holding the values of xrt_futures
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 *
 * @see xrt_future, xrt_future_result
 *
 * @ingroup xrt_iface
 */
#pragma once

#include "xrt/xrt_compiler.h"
#include "xrt/xrt_defines.h"

#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

// X-Macro type definitions: typename/prefix, type, member, user-data
#define XRT_FUTURE_VALUE_TYPES_WITH(_, P)                                                                              \
	_(UINT64, uint64_t, uint64_, P)                                                                                \
	_(INT64, int64_t, int64_, P)

#define XRT_FUTURE_VALUE_WRAP_MACRO(N, T, M, X) X(N, T, M)

#define XRT_FUTURE_VALUE_TYPES(X) XRT_FUTURE_VALUE_TYPES_WITH(XRT_FUTURE_VALUE_WRAP_MACRO, X)

typedef enum xrt_future_value_type
{
	XRT_FUTURE_VALUE_TYPE_NONE,

#define X_ENUM_ENTRY(TYPE_NAME, T, M) XRT_FUTURE_VALUE_TYPE_##TYPE_NAME,
	XRT_FUTURE_VALUE_TYPES(X_ENUM_ENTRY)
#undef X_ENUM_ENTRY

	// clang-format off
    XRT_FUTURE_VALUE_TYPE_LIST_END,
	// clang-format on
	XRT_FUTURE_VALUE_TYPE_COUNT = XRT_FUTURE_VALUE_TYPE_LIST_END - 1,
} xrt_future_value_type_t;

struct xrt_future_value
{
	union {
#define X_MEMBER_ENTRY(N, TYPE, MEMBER) TYPE MEMBER;
		XRT_FUTURE_VALUE_TYPES(X_MEMBER_ENTRY)
#undef X_MEMBER_ENTRY
	};
	XRT_ALIGNAS(8) xrt_future_value_type_t type;
};

static inline bool
xrt_future_value_is_valid(const struct xrt_future_value *xfv)
{
	return xfv && xfv->type != XRT_FUTURE_VALUE_TYPE_NONE;
}

#define XRT_FUTURE_VALUE_MAKE(TYPE_NAME, MEMBER, VALUE)                                                                \
	XRT_C11_COMPOUND(struct xrt_future_value)                                                                      \
	{                                                                                                              \
		.MEMBER = VALUE, .type = XRT_FUTURE_VALUE_TYPE_##TYPE_NAME,                                            \
	}

#define XRT_NULL_FUTURE_VALUE XRT_FUTURE_VALUE_MAKE(NONE, uint64_, 0)

static inline struct xrt_future_value
xrt_future_value_make_none(const void *ignore)
{
	(void)ignore;
	return XRT_NULL_FUTURE_VALUE;
}

#define X_MAKE_CONS_FN(TYPE_NAME, TYPE, MEMBER)                                                                        \
	static inline struct xrt_future_value xrt_future_value_make_##MEMBER(TYPE value)                               \
	{                                                                                                              \
		return XRT_FUTURE_VALUE_MAKE(TYPE_NAME, MEMBER, value);                                                \
	}                                                                                                              \
                                                                                                                       \
	static inline struct xrt_future_value xrt_future_value_make_##MEMBER##_ptr(const TYPE *value)                  \
	{                                                                                                              \
		assert(value != NULL);                                                                                 \
		return XRT_FUTURE_VALUE_MAKE(TYPE_NAME, MEMBER, (*value));                                             \
	}
XRT_FUTURE_VALUE_TYPES(X_MAKE_CONS_FN)
#undef X_MAKE_CONS_FN
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C++" {
#define X_MAKE_CONS_FN(TYPE_NAME, TYPE, MEMBER)                                                                        \
	inline struct xrt_future_value xrt_future_value_make(const TYPE &value)                                        \
	{                                                                                                              \
		return XRT_FUTURE_VALUE_MAKE(TYPE_NAME, MEMBER, value);                                                \
	}
XRT_FUTURE_VALUE_TYPES(X_MAKE_CONS_FN)
#undef X_MAKE_CONS_FN
}
#else
// clang-format off
#define XRT_FUTURE_VALUE_TYPECASE(TYPE_NAME, TYPE, MEMBER, P)                                                 \
    TYPE: xrt_future_value_make_##MEMBER,                                                                     \
    const TYPE*: xrt_future_value_make_##MEMBER##_ptr,

#define xrt_future_value_make(VALUE)                                                                          \
    _Generic((VALUE),                                                                                         \
        XRT_FUTURE_VALUE_TYPES_WITH(XRT_FUTURE_VALUE_TYPECASE, _)                                             \
        default: xrt_future_value_make_none                                                                   \
    )(VALUE)
// clang-format on
#endif
