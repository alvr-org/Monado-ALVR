// Copyright 2026, Kitlith
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Auto detect CPU Architecture.
 * @author Kitlith <kitlith@kitl.pw>
 * @ingroup xrt_iface
 */

#pragma once

// referencing <https://wolfcon.github.io/Life/PreDefinedCC++CompilerMarcros.html> and
// <https://stackoverflow.com/a/66249936>

#if defined(__arm) || defined(__arm__) || defined(__thumb__) || defined(_ARM) || defined(_M_ARM) || defined(_M_ARMT)
#define XRT_ARCH_ARM
#define XRT_ARCH_WAS_AUTODETECTED
#endif

#if defined(__aarch64__) || defined(_M_ARM64)
#define XRT_ARCH_ARM64
#define XRT_ARCH_WAS_AUTODETECTED
#endif

#if defined(__i386) || defined(__i386__) || defined(_M_IX86) || defined(__X86__) || defined(_X86_)
#define XRT_ARCH_X86
#define XRT_ARCH_WAS_AUTODETECTED
#endif

#if defined(__x86_64__) || defined(_M_X64)
#define XRT_ARCH_X86_64
#define XRT_ARCH_WAS_AUTODETECTED
#endif

#ifndef XRT_ARCH_WAS_AUTODETECTED
#error "Arch could not be detected!"
#endif

#undef XRT_ARCH_WAS_AUTODETECTED
