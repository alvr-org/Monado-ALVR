// Copyright 2025, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Endian-specific byte order defines.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup aux_os
 */

#pragma once

#include "xrt_compiler.h"
#include "xrt_defines.h"

#include <stdint.h>

#ifdef __linux__

// On Linux, all these conversion functions are defined for both endians
#include <asm/byteorder.h>

#elif defined(XRT_BIG_ENDIAN)

#error "@todo: Add byte order constants and functions for this OS or big endian machines."

#else

#define __be64 uint64_t
#define __be32 uint32_t
#define __be16 uint16_t

#define __be16_to_cpu(x) ((((uint16_t)x & (uint16_t)0x00FFU) << 8) | (((uint16_t)x & (uint16_t)0xFF00U) >> 8))
#define __cpu_to_be16(x) __be16_to_cpu(x)

#define __be32_to_cpu(x)                                                                                               \
	((((uint32_t)x & (uint32_t)0x000000FFUL) << 24) | (((uint32_t)x & (uint32_t)0x0000FF00UL) << 8) |              \
	 (((uint32_t)x & (uint32_t)0x00FF0000UL) >> 8) | (((uint32_t)x & (uint32_t)0xFF000000UL) >> 24))
#define __cpu_to_be32(x) __be32_to_cpu(x)

#define __be64_to_cpu(x)                                                                                               \
	((((uint64_t)x & (uint64_t)0x00000000000000FFULL) << 56) |                                                     \
	 (((uint64_t)x & (uint64_t)0x000000000000FF00ULL) << 40) |                                                     \
	 (((uint64_t)x & (uint64_t)0x0000000000FF0000ULL) << 24) |                                                     \
	 (((uint64_t)x & (uint64_t)0x00000000FF000000ULL) << 8) |                                                      \
	 (((uint64_t)x & (uint64_t)0x000000FF00000000ULL) >> 8) |                                                      \
	 (((uint64_t)x & (uint64_t)0x0000FF0000000000ULL) >> 24) |                                                     \
	 (((uint64_t)x & (uint64_t)0x00FF000000000000ULL) >> 40) |                                                     \
	 (((uint64_t)x & (uint64_t)0xFF00000000000000ULL) >> 56))
#define __cpu_to_be64(x) __be64_to_cpu(x)

#define __le64 uint64_t
#define __le32 uint32_t
#define __le16 uint16_t
#define __u8 uint8_t
#define __s8 int8_t
#define __cpu_to_le16(x) (x)
#define __le16_to_cpu(x) (x)
#define __cpu_to_le32(x) (x)
#define __le32_to_cpu(x) (x)
#define __cpu_to_le64(x) (x)
#define __le64_to_cpu(x) (x)

#endif

/*
 * 32-bit float
 */

/*!
 * Little endian 32-bit float wrapper struct.
 */
typedef struct
{
	__le32 val;
} __lef32;

/*!
 * Big endian 32-bit float wrapper struct.
 */
typedef struct
{
	__be32 val;
} __bef32;

static inline float
__lef32_to_cpu(__lef32 f)
{
	union {
		uint32_t raw;
		float f32;
	} safe_copy;

	safe_copy.raw = __le32_to_cpu(f.val);
	return safe_copy.f32;
}

static inline __lef32
__cpu_to_lef32(float f)
{
	union {
		uint32_t wire;
		float f32;
	} safe_copy;

	safe_copy.f32 = f;

	return XRT_C11_COMPOUND(__lef32){.val = __cpu_to_le32(safe_copy.wire)};
}

static inline float
__bef32_to_cpu(__bef32 f)
{
	union {
		uint32_t raw;
		float f32;
	} safe_copy;

	safe_copy.raw = __be32_to_cpu(f.val);
	return safe_copy.f32;
}

static inline __bef32
__cpu_to_bef32(float f)
{
	union {
		uint32_t wire;
		float f32;
	} safe_copy;

	safe_copy.f32 = f;
	return XRT_C11_COMPOUND(__bef32){.val = __cpu_to_be32(safe_copy.wire)};
}

/*
 * 64-bit float
 */

/*!
 * Little endian 64-bit float wrapper struct.
 */
typedef struct
{
	__le64 val;
} __lef64;

/*!
 * Big endian 64-bit float wrapper struct.
 */
typedef struct
{
	__be64 val;
} __bef64;

static inline double
__lef64_to_cpu(__lef64 f)
{
	union {
		uint64_t raw;
		double f64;
	} safe_copy;

	safe_copy.raw = __le64_to_cpu(f.val);
	return safe_copy.f64;
}

static inline __lef64
__cpu_to_lef64(double f)
{
	union {
		uint64_t wire;
		double f64;
	} safe_copy;

	safe_copy.f64 = f;

	return XRT_C11_COMPOUND(__lef64){.val = __cpu_to_le64(safe_copy.wire)};
}

static inline double
__bef64_to_cpu(__bef64 f)
{
	union {
		uint64_t raw;
		double f64;
	} safe_copy;

	safe_copy.raw = __be64_to_cpu(f.val);
	return safe_copy.f64;
}

static inline __bef64
__cpu_to_bef64(double f)
{
	union {
		uint64_t wire;
		double f64;
	} safe_copy;

	safe_copy.f64 = f;
	return XRT_C11_COMPOUND(__bef64){.val = __cpu_to_be64(safe_copy.wire)};
}

/*
 *
 * Vec2
 *
 */

struct __levec2
{
	__lef32 x;
	__lef32 y;
};

static inline struct xrt_vec2
__levec2_to_cpu(struct __levec2 v)
{
	return XRT_C11_COMPOUND(struct xrt_vec2){
	    .x = __lef32_to_cpu(v.x),
	    .y = __lef32_to_cpu(v.y),
	};
}

static inline struct __levec2
__cpu_to_levec2(struct xrt_vec2 v)
{
	return XRT_C11_COMPOUND(struct __levec2){
	    .x = __cpu_to_lef32(v.x),
	    .y = __cpu_to_lef32(v.y),
	};
}

/*
 *
 * Vec3
 *
 */

struct __levec3
{
	__lef32 x;
	__lef32 y;
	__lef32 z;
};

static inline struct xrt_vec3
__levec3_to_cpu(struct __levec3 v)
{
	return XRT_C11_COMPOUND(struct xrt_vec3){
	    .x = __lef32_to_cpu(v.x),
	    .y = __lef32_to_cpu(v.y),
	    .z = __lef32_to_cpu(v.z),
	};
}

static inline struct __levec3
__cpu_to_levec3(struct xrt_vec3 v)
{
	return XRT_C11_COMPOUND(struct __levec3){
	    .x = __cpu_to_lef32(v.x),
	    .y = __cpu_to_lef32(v.y),
	    .z = __cpu_to_lef32(v.z),
	};
}
