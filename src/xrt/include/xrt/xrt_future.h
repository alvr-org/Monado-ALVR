// Copyright 2025, Collabora, Ltd.
// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Interface for creating futures.
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup xrt_iface
 */
#pragma once

#include "xrt/xrt_defines.h"
#include "xrt/xrt_future_value.h"

#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * The (future) status of an asynchronous operation
 *
 * @see xrt_future::get_state
 */
typedef enum xrt_future_state
{
	XRT_FUTURE_STATE_PENDING,
	XRT_FUTURE_STATE_READY,
	XRT_FUTURE_STATE_CANCELLED,
} xrt_future_state_t;

/*!
 * The (future) result of an asynchronous operation
 *
 * @see xrt_future::get_result, xrt_future::complete
 */
struct xrt_future_result
{
	/*!
	 * The result value of a successfully completed asynchronous operation
	 * @see xrt_future_value_make
	 */
	struct xrt_future_value value;

	//! The error/ok status of a completed asynchronous operation
	XRT_ALIGNAS(8) xrt_result_t result;
};

#define XRT_FUTURE_RESULT(TYPED_VALUE, ERR_CODE)                                                                       \
	XRT_C11_COMPOUND(struct xrt_future_result)                                                                     \
	{                                                                                                              \
		.value = xrt_future_value_make(TYPED_VALUE), .result = ERR_CODE,                                       \
	}

/*!
 * @interface xrt_future
 * @ingroup xrt_iface
 *
 * @brief A future is a concurrency primitive that provides a mechanism to access results of asynchronous operations.
 *
 * The xrt_future interface shares similarities with OpenXR futures but is not identical. In comparison to C++
 * std::future, xrt_future combines concepts from both std::shared_future and std::promise with built-in
 * cancellation support in a single interface. The interface provides separate method sets for producers
 * (result generators) and consumers (result pollers/waiters).
 *
 * Thread Safety and Reference Counting:
 * Each thread that references an xrt_future must properly manage the reference count using
 * @ref xrt_future_reference when entering and exiting the thread's scope.
 *
 * @see "Server-side / driver — implementing async callbacks" in @ref async for producer example
 * code
 */
struct xrt_future
{
	/*!
	 * Reference helper.
	 */
	struct xrt_reference reference;

	/*!
	 * Destroys the future.
	 */
	void (*destroy)(struct xrt_future *xft);

	/*!
	 * @brief Gets the current state of the future
	 *
	 * @param[in] xft          The future.
	 * @param[out] out_state   The current state of @p xft
	 *
	 * @note Consumer Interface
	 *
	 * @note Blocking behavior - non-blocking
	 *
	 * @see xrt_future_state
	 */
	xrt_result_t (*get_state)(const struct xrt_future *xft, enum xrt_future_state *out_state);

	/*!
	 * @brief Gets the future results (after async operation has finished)
	 *
	 * @param[in] xft           The future.
	 * @param[out] out_result   The future result of @p xft
	 *
	 * @note Consumer Interface
	 *
	 * @note Blocking behavior - Non-blocking w.r.t. result: returns immediately,
	 *       may briefly block acquiring an internal mutex to check/consume state
	 *
	 * @note differs from std::future::get in that std::future will block & wait the calling thread
	 *       until the result is ready where as xrt_future::get_result is non-blocking (w.r.t. result)
	 *       to achieve the equivalent without using polling interface would be:
	 *
	 *       // std::future::get ==
	 *       xrt_future_wait(xft, INT64_MAX);
	 *       xrt_get_result(xft, &my_result);
	 *
	 * @note Similar to or used by OpenXR future/async complete functions
	 *
	 * @see xrt_future::complete, xrt_future_result, xrt_future_value
	 */
	xrt_result_t (*get_result)(const struct xrt_future *xft, struct xrt_future_result *out_result);

	/*!
	 * @brief Signals an asynchronous operation associated with the future to cancel.
	 *
	 * @param[in] xft           The future.
	 *
	 * @note Consumer Interface
	 *
	 * @note Blocking behavior - Non-blocking, may briefly block acquiring an internal mutex to check/consume state
	 */
	xrt_result_t (*cancel)(struct xrt_future *xft);

	/*!
	 * @brief Waits on a pending/cancelled future
	 *
	 * @param[in] xft           The future.
	 * @param[in] timeout_ns    Timeout in nanoseconds or INT64_MAX for infinite duration
	 *
	 * @note Consumer Interface
	 *
	 * @note Blocking behavior - Blocking
	 *
	 * @see xrt_future::cancel, xrt_future::complete
	 */
	xrt_result_t (*wait)(struct xrt_future *xft, int64_t timeout_ns);

	/*!
	 * @brief Waits on a cancelled future
	 *
	 * @param[in] xft                  The future.
	 * @param[out] out_request_cancel  Has the consumer requested to cancel the async operation?
	 *
	 * @note Producer interface
	 *
	 * @note Blocking behavior - non-blocking
	 *
	 * @see xrt_future::cancel
	 */
	xrt_result_t (*is_cancel_requested)(const struct xrt_future *xft, bool *out_request_cancel);

	/*!
	 * @brief Signals that the asynchronous operation has completed and sets the future's result.
	 *
	 * @param[in] xft                  The future.
	 * @param[in] ft_result            the result of an async operation associated with @p xft.
	 *
	 * @note Producer interface
	 *
	 * @note Blocking behavior - Non-blocking, may briefly block acquiring an internal mutex to check/consume state
	 *
	 * @note Differs from OpenXR future/async complete functions as those are used to only get the results of
	 *       a future once the async-operation has finished where as this callback is used by async operation
	 *       to mark/signal completion and return a result
	 *
	 * @note Similar to std::promise::set_value
	 *
	 * @see xrt_future::get_result, xrt_future_result, xrt_future_value
	 */
	xrt_result_t (*complete)(struct xrt_future *xft, const struct xrt_future_result *ft_result);
};

/*!
 * Update the reference counts on xrt_future(s).
 *
 * @param[in,out] dst Pointer to a object reference: if the object reference is
 *                non-null will decrement its counter. The reference that
 *                @p dst points to will be set to @p src.
 * @param[in] src New object for @p dst to refer to (may be null).
 *                If non-null, will have its refcount increased.
 * @ingroup xrt_iface
 * @relates xrt_future
 */
static inline void
xrt_future_reference(struct xrt_future **dst, struct xrt_future *src)
{
	struct xrt_future *old_dst = *dst;

	if (old_dst == src) {
		return;
	}

	if (src) {
		xrt_reference_inc(&src->reference);
	}

	*dst = src;

	if (old_dst) {
		if (xrt_reference_dec_and_is_zero(&old_dst->reference)) {
			assert(old_dst->destroy);
			old_dst->destroy(old_dst);
		}
	}
}

/*!
 * Helper function for @ref xrt_future::get_result.
 *
 * @copydoc xrt_future::get_result
 *
 * @public @memberof xrt_future
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_future_get_result(const struct xrt_future *xft, struct xrt_future_result *out_result)
{
	assert(xft->get_result);
	return xft->get_result(xft, out_result);
}

/*!
 * Helper function for @ref xrt_future::get_state.
 *
 * @copydoc xrt_future::get_state
 *
 * @public @memberof xrt_future
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_future_get_state(const struct xrt_future *xft, enum xrt_future_state *out_state)
{
	assert(xft->get_state);
	return xft->get_state(xft, out_state);
}

/*!
 * Helper function for @ref xrt_future::cancel.
 *
 * @copydoc xrt_future::cancel
 *
 * @public @memberof xrt_future
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_future_cancel(struct xrt_future *xft)
{
	assert(xft->cancel);
	return xft->cancel(xft);
}

/*!
 * Helper function for @ref xrt_future::wait.
 *
 * @copydoc xrt_future::wait
 *
 * @public @memberof xrt_future
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_future_wait(struct xrt_future *xft, int64_t timeout_ns)
{
	assert(xft->wait);
	return xft->wait(xft, timeout_ns);
}

/*!
 * Helper function for @ref xrt_future::is_cancel_requested.
 *
 * @copydoc xrt_future::is_cancel_requested
 *
 * @public @memberof xrt_future
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_future_is_cancel_requested(const struct xrt_future *xft, bool *out_request_cancel)
{
	assert(xft->is_cancel_requested);
	return xft->is_cancel_requested(xft, out_request_cancel);
}

/*!
 * Helper function for @ref xrt_future::complete.
 *
 * @copydoc xrt_future::complete
 *
 * @public @memberof xrt_future
 */
XRT_NONNULL_ALL static inline xrt_result_t
xrt_future_complete(struct xrt_future *xft, const struct xrt_future_result *ft_result)
{
	assert(xft->complete);
	return xft->complete(xft, ft_result);
}

#ifdef __cplusplus
}
#endif
