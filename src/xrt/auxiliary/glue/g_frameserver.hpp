// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header for glue classes to wrap xrt frameserver interfaces.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup aux_util
 */

#pragma once

#include "xrt/xrt_frameserver.h"

#include "util/u_frameserver.h"

#include "g_catch_guard.hpp"
#include "g_traits.hpp"

#include <type_traits>


namespace xrt::util {

/*!
 * Selects which optional functions that the @ref FrameServerBase wrapper
 * should set.
 */
struct FrameServerFunctions
{
	//! @ref xrt_fs::slam_stream_start
	bool slam{false};
};

/*!
 * CRTP glue wrapper for @ref xrt_fs. Relies on standard layout to recover the
 * derived object from the C struct, and has some requirements and limitations
 * because of that. See @ref cpp-glue-wrappers for the guide and conventions
 * for these wrappers.
 *
 * @ref xrt_fs has no destroy function, a frameserver is instead destroyed
 * through a @ref xrt_frame_node added to a @ref xrt_frame_context, see
 * @ref FrameNodeBase.
 *
 * The identifying fields (name, product, manufacturer, serial and source_id)
 * are zeroed and are to be filled in by the derived class via @ref getXFS.
 */
template <class T, FrameServerFunctions functions> class FrameServerBase
{
public: // Methods
	FrameServerBase() noexcept
	{
		static_assert(std::is_standard_layout_v<FrameServerBase>,
		              "glue base must be standard layout for pointer recovery");
		static_assert(is_non_virtual_base_v<FrameServerBase, T>,
		              "glue base must be a non-virtual base of T for pointer recovery");

		auto &xfs = *getXFS();

		u_frameserver_populate_function_pointers(&xfs);

		xfs.enumerate_modes = enumerateModesWrap;
		xfs.configure_capture = configureCaptureWrap;
		xfs.stream_start = streamStartWrap;
		xfs.stream_stop = streamStopWrap;
		xfs.is_running = isRunningWrap;

		if constexpr (functions.slam) {
			xfs.slam_stream_start = slamStreamStartWrap;
		}
	}

	~FrameServerBase() noexcept = default;

	const T &
	derived() const noexcept
	{
		return static_cast<const T &>(*this);
	}

	T &
	derived() noexcept
	{
		return static_cast<T &>(*this);
	}

	static const T *
	fromXFS(const xrt_fs *xfs) noexcept
	{
		return &(reinterpret_cast<const FrameServerBase *>(xfs)->derived());
	}

	static T *
	fromXFS(xrt_fs *xfs) noexcept
	{
		return &(reinterpret_cast<FrameServerBase *>(xfs)->derived());
	}

	const xrt_fs *
	getXFS() const noexcept
	{
		return &mFrameServer;
	}

	xrt_fs *
	getXFS() noexcept
	{
		return &mFrameServer;
	}


private: // Members
	/*!
	 * Wrapped @ref xrt_fs. Must be the first data member: a pointer to it is
	 * then interconvertible with a pointer to this standard-layout base, which
	 * lets the glue cast a C pointer back to the derived C++ class. See
	 * @ref cpp-glue-wrappers.
	 */
	xrt_fs mFrameServer = {};


private: // Functions
// Not `->derived()`, that is ambiguous when T has more than one glue base.
#define GET(xfs) (*fromXFS(xfs))

	static xrt_result_t
	enumerateModesWrap(struct xrt_fs *xfs, struct xrt_fs_mode **out_modes, uint32_t *out_count) noexcept
	try {
		return GET(xfs).enumerateModes(out_modes, out_count);
	}
	G_CATCH_GUARDS_WITH_RETURN(XRT_ERROR_UNCAUGHT_EXCEPTION)

	static xrt_result_t
	configureCaptureWrap(struct xrt_fs *xfs, struct xrt_fs_capture_parameters *cp) noexcept
	try {
		return GET(xfs).configureCapture(cp);
	}
	G_CATCH_GUARDS_WITH_RETURN(XRT_ERROR_UNCAUGHT_EXCEPTION)

	static xrt_result_t
	streamStartWrap(struct xrt_fs *xfs,
	                struct xrt_frame_sink *xs,
	                enum xrt_fs_capture_type capture_type,
	                uint32_t descriptor_index) noexcept
	try {
		return GET(xfs).streamStart(xs, capture_type, descriptor_index);
	}
	G_CATCH_GUARDS_WITH_RETURN(XRT_ERROR_UNCAUGHT_EXCEPTION)

	static xrt_result_t
	slamStreamStartWrap(struct xrt_fs *xfs, struct xrt_slam_sinks *sinks) noexcept
	try {
		return GET(xfs).slamStreamStart(sinks);
	}
	G_CATCH_GUARDS_WITH_RETURN(XRT_ERROR_UNCAUGHT_EXCEPTION)

	static xrt_result_t
	streamStopWrap(struct xrt_fs *xfs) noexcept
	try {
		return GET(xfs).streamStop();
	}
	G_CATCH_GUARDS_WITH_RETURN(XRT_ERROR_UNCAUGHT_EXCEPTION)

	static xrt_result_t
	isRunningWrap(struct xrt_fs *xfs, bool *out_is_running) noexcept
	try {
		return GET(xfs).isRunning(*out_is_running);
	}
	G_CATCH_GUARDS_WITH_RETURN(XRT_ERROR_UNCAUGHT_EXCEPTION)

#undef GET
};

} // namespace xrt::util
