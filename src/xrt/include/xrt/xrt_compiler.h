// Copyright 2019, Collabora, Ltd.
// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header holding common defines.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup xrt_iface
 */

#pragma once


/*
 * C99 is not a high bar to reach.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

#ifdef _MSC_VER
#include <intrin.h>
// for atomic intrinsics
#include "xrt_windows.h"
// Older MSVC versions do not have stdalign.h, define it manually using __declspec(align).
#define XRT_ALIGNAS(n) __declspec(align(n))
#elif defined(XRT_DOXYGEN)
#define XRT_ALIGNAS(align)
#else // _MSC_VER
#include <stdalign.h>
#define XRT_ALIGNAS(align) alignas(align)
#endif

#if (defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN) ||                                                         \
    (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__) ||                                             \
    (defined(__STDC_ENDIAN_NATIVE__) && __STDC_ENDIAN_NATIVE__ == __STDC_ENDIAN_BIG__) || defined(__BIG_ENDIAN__) ||   \
    defined(__ARMEB__) || defined(__THUMBEB__) || defined(__AARCH64EB__) || defined(_MIBSEB) || defined(__MIBSEB) ||   \
    defined(__MIBSEB__)

#define XRT_BIG_ENDIAN

#elif (defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN) ||                                                    \
    (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) ||                                          \
    (defined(__STDC_ENDIAN_NATIVE__) && __STDC_ENDIAN_NATIVE__ == __STDC_ENDIAN_LITTLE__) ||                           \
    defined(__LITTLE_ENDIAN__) || defined(__ARMEL__) || defined(__THUMBEL__) || defined(__AARCH64EL__) ||              \
    defined(_MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__) || defined(__x86_64__) || defined(_M_X64) ||          \
    defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)

#undef XRT_BIG_ENDIAN

#else
#error "@todo: Unable to determine current architecture."
#endif

/*!
 * Array size helper.
 */
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__) || defined(_ARCH_PPC64) || defined(__s390x__) ||    \
    (defined(__SIZEOF_POINTER__) && __SIZEOF_POINTER__ == 8)
#define XRT_64_BIT
#else
#define XRT_32_BIT
#endif

/*
 * Printf helper attribute.
 */
#if defined(__GNUC__)
#define XRT_PRINTF_FORMAT(fmt, list) __attribute__((format(printf, fmt, list)))
#else
#define XRT_PRINTF_FORMAT(fmt, list)
#endif


/*
 * To silence unused warnings.
 */
#if defined(__GNUC__)
#define XRT_MAYBE_UNUSED __attribute__((unused))
#elif defined(_MSC_VER) && defined(__cplusplus)
#define XRT_MAYBE_UNUSED [[maybe_unused]]
#else
#define XRT_MAYBE_UNUSED
#endif


/*
 * To indicate that all pointer parameters must not be null.
 *
 * Note: MSVC does not have a function-level equivalent. To get similar checking
 * on MSVC, individual parameters would need SAL annotations like _In_, _Out_, etc.
 */
#if defined(__GNUC__)
#define XRT_NONNULL_ALL __attribute__((nonnull))
#else
#define XRT_NONNULL_ALL
#endif


/*
 * To indicate that the first parameter must not be null.
 *
 * Note: MSVC does not have a function-level equivalent. To get similar checking
 * on MSVC, individual parameters would need SAL annotations like _In_, _Out_, etc.
 */
#if defined(__GNUC__)
#define XRT_NONNULL_FIRST __attribute__((nonnull(1)))
#else
#define XRT_NONNULL_FIRST
#endif


/*
 * To make sure return values are checked.
 */
#if defined(__GNUC__) && (__GNUC__ >= 4)
#define XRT_CHECK_RESULT __attribute__((warn_unused_result))
#elif defined(_MSC_VER) && (_MSC_VER >= 1700)
#define XRT_CHECK_RESULT _Check_return_
#else
#define XRT_CHECK_RESULT
#endif


/*
 * To stop inlining.
 */
#if defined(__GNUC__)
#define XRT_NO_INLINE __attribute__((noinline))
#elif defined(_MSC_VER)
#define XRT_NO_INLINE __declspec(noinline)
#else
#define XRT_NO_INLINE
#endif


#ifdef XRT_DOXYGEN
/*!
 * To trigger a trap/break in the debugger.
 *
 * @ingroup xrt_iface
 */
#define XRT_DEBUGBREAK()
#elif defined(__clang__) || defined(__GNUC__)
#define XRT_DEBUGBREAK() __builtin_trap()
#elif defined(_MSC_VER)
#define XRT_DEBUGBREAK() __debugbreak()
#else
#error "compiler not supported"
#endif


/*
 * typeof extension, until C23 is widely available. This macro may not always be available,
 * and should be checked before usage.
 *
 * @todo Remove this macro when Monado is updated project-wide to C23.
 */
#if defined(__cpp_decltype) && __cpp_decltype >= 200707L
#define XRT_TYPEOF(x) decltype(x)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#define XRT_TYPEOF(x) typeof(x)
#elif defined(__GNUC__) || defined(__clang__)
#define XRT_TYPEOF(x) __typeof__(x)
#endif


typedef volatile int32_t xrt_atomic_s32_t;

static inline int32_t
xrt_atomic_s32_inc_return(xrt_atomic_s32_t *p)
{
#if defined(__GNUC__)
	return __sync_add_and_fetch(p, 1);
#elif defined(_MSC_VER)
	return InterlockedIncrement((volatile LONG *)p);
#else
#error "compiler not supported"
#endif
}
static inline int32_t
xrt_atomic_s32_dec_return(xrt_atomic_s32_t *p)
{
#if defined(__GNUC__)
	return __sync_sub_and_fetch(p, 1);
#elif defined(_MSC_VER)
	return InterlockedDecrement((volatile LONG *)p);
#else
#error "compiler not supported"
#endif
}
static inline int32_t
xrt_atomic_s32_cmpxchg(xrt_atomic_s32_t *p, int32_t old_, int32_t new_)
{
#if defined(__GNUC__)
	return __sync_val_compare_and_swap(p, old_, new_);
#elif defined(_MSC_VER)
	return InterlockedCompareExchange((volatile LONG *)p, old_, new_);
#else
#error "compiler not supported"
#endif
}
static inline void
xrt_atomic_s32_store(xrt_atomic_s32_t *p, int32_t v)
{
#if defined(__GNUC__)
	__atomic_store_n(p, v, __ATOMIC_SEQ_CST);
#elif defined(_MSC_VER)
	InterlockedExchange((volatile LONG *)p, v);
#else
#error "compiler not supported"
#endif
}
static inline int32_t
xrt_atomic_s32_load(xrt_atomic_s32_t *p)
{
#if defined(__GNUC__)
	return __atomic_load_n(p, __ATOMIC_SEQ_CST);
#elif defined(_MSC_VER)
	return InterlockedCompareExchange((volatile LONG *)p, 0, 0);
#else
#error "compiler not supported"
#endif
}

typedef volatile int64_t xrt_atomic_s64_t;

static inline int64_t
xrt_atomic_s64_inc_return(xrt_atomic_s64_t *p)
{
#if defined(__GNUC__)
	return __sync_add_and_fetch(p, 1);
#elif defined(_MSC_VER)
	return InterlockedIncrement64((volatile LONG64 *)p);
#else
#error "compiler not supported"
#endif
}
static inline int64_t
xrt_atomic_s64_dec_return(xrt_atomic_s64_t *p)
{
#if defined(__GNUC__)
	return __sync_sub_and_fetch(p, 1);
#elif defined(_MSC_VER)
	return InterlockedDecrement64((volatile LONG64 *)p);
#else
#error "compiler not supported"
#endif
}
static inline int64_t
xrt_atomic_s64_cmpxchg(xrt_atomic_s64_t *p, int64_t old_, int64_t new_)
{
#if defined(__GNUC__)
	return __sync_val_compare_and_swap(p, old_, new_);
#elif defined(_MSC_VER)
	return InterlockedCompareExchange64((volatile LONG64 *)p, old_, new_);
#else
#error "compiler not supported"
#endif
}
static inline void
xrt_atomic_s64_store(xrt_atomic_s64_t *p, int64_t v)
{
#if defined(__GNUC__)
	__atomic_store_n(p, v, __ATOMIC_SEQ_CST);
#elif defined(_MSC_VER)
	InterlockedExchange64((volatile LONG64 *)p, v);
#else
#error "compiler not supported"
#endif
}
static inline int64_t
xrt_atomic_s64_load(xrt_atomic_s64_t *p)
{
#if defined(__GNUC__)
	return __atomic_load_n(p, __ATOMIC_SEQ_CST);
#elif defined(_MSC_VER)
	return InterlockedCompareExchange64((volatile LONG64 *)p, 0, 0);
#else
#error "compiler not supported"
#endif
}

#ifdef _MSC_VER
typedef intptr_t ssize_t;
#define _SSIZE_T_
#define _SSIZE_T_DEFINED
#endif

/*!
 * Get the holder from a pointer to a field.
 *
 * @ingroup xrt_iface
 */
#define container_of(ptr, type, field) (type *)((char *)ptr - offsetof(type, field))


#ifdef XRT_DOXYGEN

/*!
 * Very small default init for structs that works in both C and C++. Helps with
 * code that needs to be compiled with both C and C++.
 *
 * @ingroup xrt_iface
 */

// clang-format off
#define XRT_STRUCT_INIT {}
// clang-format on

#elif defined(__cplusplus)

// clang-format off
#define XRT_STRUCT_INIT {}
// clang-format on

#else

// clang-format off
#define XRT_STRUCT_INIT {0}
// clang-format on

#endif

/*
 * C11 style designated initializers (as compound literals) causes warning messages
 * in C++ (even in C++20), usage:
 *
 * @code{.c}
 *    static inline struct xrt_foo_bar
 *    xrt_foo_bar_make()
 *    {
 *        return XRT_C11_COMPOUND(struct xrt_foo_bar){
 *            .x = y,
 *            // ...
 *        };
 *    }
 * @endcode
 */
#ifdef __cplusplus
#define XRT_C11_COMPOUND(X)
#else
#define XRT_C11_COMPOUND(X) (X)
#endif
