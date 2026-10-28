// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header for glue classes to wrap xrt body tracker interfaces.
 * @ingroup aux_util
 */

#pragma once

#include "xrt/xrt_body_tracker.h"

#include "g_catch_guard.hpp"
#include "g_traits.hpp"

#include <type_traits>


namespace xrt::util {

/*!
 * CRTP glue wrapper for @ref xrt_body_tracker. Relies on standard layout to
 * recover the derived object from the C struct, and has some requirements and
 * limitations because of that. See @ref cpp-glue-wrappers for the guide and
 * conventions for these wrappers.
 */
template <class T> class BodyTrackerBase
{
public: // Methods
	BodyTrackerBase() noexcept
	{
		static_assert(std::is_standard_layout_v<BodyTrackerBase>,
		              "glue base must be standard layout for pointer recovery");
		static_assert(is_non_virtual_base_v<BodyTrackerBase, T>,
		              "glue base must be a non-virtual base of T for pointer recovery");

		auto &xbt = *getXBT();

		xbt = {
		    .supported = {/* Filled in by derived class. */},
		    .locate = locateWrap,
		    .get_skeleton = getSkeletonWrap,
		    .reset_calibration_meta = resetCalibrationMetaWrap,
		    .set_calibration_override_meta = setCalibrationOverrideMetaWrap,
		    .set_fidelity_meta = setFidelityMetaWrap,
		    .destroy = destroyBodyTrackerWrap,
		};
	}

	~BodyTrackerBase() noexcept = default;

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
	fromXBT(const xrt_body_tracker *xbt) noexcept
	{
		return &(reinterpret_cast<const BodyTrackerBase *>(xbt)->derived());
	}

	static T *
	fromXBT(xrt_body_tracker *xbt) noexcept
	{
		return &(reinterpret_cast<BodyTrackerBase *>(xbt)->derived());
	}

	const xrt_body_tracker *
	getXBT() const noexcept
	{
		return &mBodyTracker;
	}

	xrt_body_tracker *
	getXBT() noexcept
	{
		return &mBodyTracker;
	}


private: // Members
	/*!
	 * Wrapped @ref xrt_body_tracker. Must be the first data member: a pointer to
	 * it is then interconvertible with a pointer to this standard-layout base,
	 * which lets the glue cast a C pointer back to the derived C++ class. See
	 * @ref cpp-glue-wrappers.
	 */
	xrt_body_tracker mBodyTracker = {};


private: // Functions
#define GET(xbt) (fromXBT(xbt)->derived())

	static xrt_result_t
	locateWrap(struct xrt_body_tracker *xbt,
	           struct xrt_space_overseer *xso,
	           struct xrt_space *base_space,
	           const struct xrt_pose *base_offset,
	           int64_t at_timestamp_ns,
	           struct xrt_body_tracker_location *out_location) noexcept
	try {
		return GET(xbt).locate(xso, base_space, base_offset, at_timestamp_ns, out_location);
	}
	G_CATCH_GUARDS

	static xrt_result_t
	getSkeletonWrap(struct xrt_body_tracker *xbt, struct xrt_body_skeleton *out_skeleton) noexcept
	try {
		return GET(xbt).getSkeleton(out_skeleton);
	}
	G_CATCH_GUARDS

	static xrt_result_t
	resetCalibrationMetaWrap(struct xrt_body_tracker *xbt) noexcept
	try {
		return GET(xbt).resetCalibrationMeta();
	}
	G_CATCH_GUARDS

	static xrt_result_t
	setCalibrationOverrideMetaWrap(struct xrt_body_tracker *xbt, float body_height) noexcept
	try {
		return GET(xbt).setCalibrationOverrideMeta(body_height);
	}
	G_CATCH_GUARDS

	static xrt_result_t
	setFidelityMetaWrap(struct xrt_body_tracker *xbt, enum xrt_body_tracking_fidelity_meta fidelity) noexcept
	try {
		return GET(xbt).setFidelityMeta(fidelity);
	}
	G_CATCH_GUARDS

	static void
	destroyBodyTrackerWrap(struct xrt_body_tracker *xbt) noexcept
	try {
		T::destroyBodyTracker(xbt);
	}
	G_CATCH_GUARDS_VOID

#undef GET
};

} // namespace xrt::util
