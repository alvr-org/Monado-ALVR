// Copyright 2025, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Helper to implement @ref xrt_future,
 *         A basic CPU based future implementation.
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup aux_util
 */
#include "u_future.h"

#include "util/u_debug.h"
#include "util/u_logging.h"

#include "os/os_threading.h"

#include <errno.h>
#include <atomic>
#include <memory>

DEBUG_GET_ONCE_LOG_OPTION(log_level_future, "U_FUTURE_LOG", U_LOGGING_WARN)

#define UFT_LOG_T(...) U_LOG_IFL_T(debug_get_log_option_log_level_future(), __VA_ARGS__)
#define UFT_LOG_D(...) U_LOG_IFL_D(debug_get_log_option_log_level_future(), __VA_ARGS__)
#define UFT_LOG_I(...) U_LOG_IFL_I(debug_get_log_option_log_level_future(), __VA_ARGS__)
#define UFT_LOG_W(...) U_LOG_IFL_W(debug_get_log_option_log_level_future(), __VA_ARGS__)
#define UFT_LOG_E(...) U_LOG_IFL_E(debug_get_log_option_log_level_future(), __VA_ARGS__)

#define U_FUTURE_CLEANUP_TIMEOUT_NS (3000000000LL) // 3 seconds

/*!
 * A helper to implement a @ref xrt_future,
 * a basic CPU based future implementation
 *
 * @ingroup aux_util
 * @implements xrt_future
 */
struct u_future
{
	struct xrt_future base;
	struct os_mutex mtx;
	struct os_cond cv;
	std::atomic<xrt_future_state_t> state{XRT_FUTURE_STATE_PENDING};
	std::atomic<xrt_result_t> result{XRT_SUCCESS};
	struct xrt_future_value value = XRT_NULL_FUTURE_VALUE;
};

static inline xrt_result_t
u_future_get_xrt_result(const struct u_future *uft)
{
	assert(uft != NULL);
	return uft->result.load(std::memory_order::acquire);
}

static inline xrt_future_state_t
u_future_get_state_priv(const struct u_future *uft)
{
	assert(uft != NULL);
	return uft->state.load(std::memory_order::acquire);
}

static inline void
u_future_set_xrt_result(struct u_future *uft, const xrt_result_t result)
{
	assert(uft != NULL);
	uft->result.store(result, std::memory_order::release);
}

static inline void
u_future_set_state(struct u_future *uft, const xrt_future_state_t new_state)
{
	assert(uft != NULL);
	uft->state.store(new_state, std::memory_order::release);
}

//! internal helper only, does not atomically set both.
static inline void
u_future_set_state_and_xrt_result(struct u_future *uft, const xrt_future_state_t new_state, const xrt_result_t result)
{
	u_future_set_xrt_result(uft, result);
	u_future_set_state(uft, new_state);
}

static inline xrt_result_t
u_future_get_state(const struct xrt_future *xft, enum xrt_future_state *out_state)
{
	const struct u_future *uft = (const struct u_future *)xft;
	if (uft == NULL || out_state == NULL) {
		return XRT_ERROR_INVALID_ARGUMENT;
	}
	*out_state = u_future_get_state_priv(uft);
	return XRT_SUCCESS;
}

static xrt_result_t
u_future_get_result(const struct xrt_future *xft, struct xrt_future_result *out_result)
{
	const struct u_future *uft = (const struct u_future *)xft;
	if (uft == NULL || out_result == NULL) {
		return XRT_ERROR_INVALID_ARGUMENT;
	}

	struct os_mutex *mtx = (struct os_mutex *)&uft->mtx;
	os_mutex_lock(mtx);

	const xrt_future_state_t curr_state = u_future_get_state_priv(uft);
	if (curr_state == XRT_FUTURE_STATE_PENDING) {
		os_mutex_unlock(mtx);
		return XRT_ERROR_FUTURE_RESULT_NOT_READY;
	}

	out_result->result = u_future_get_xrt_result(uft);
	if (out_result->result == XRT_SUCCESS && //
	    curr_state == XRT_FUTURE_STATE_READY) {
		out_result->value = uft->value;
	}

	os_mutex_unlock(mtx);
	return XRT_SUCCESS;
}

static xrt_result_t
u_future_cancel(struct xrt_future *xft)
{
	struct u_future *uft = (struct u_future *)xft;
	if (uft == NULL) {
		return XRT_ERROR_INVALID_ARGUMENT;
	}

	os_mutex_lock(&uft->mtx);
	if (u_future_get_state_priv(uft) == XRT_FUTURE_STATE_PENDING) {
		u_future_set_state_and_xrt_result(uft, XRT_FUTURE_STATE_CANCELLED, XRT_OPERATION_CANCELLED);
		os_cond_broadcast(&uft->cv);
	}
	os_mutex_unlock(&uft->mtx);
	return XRT_SUCCESS;
}

static xrt_result_t
u_future_wait(struct xrt_future *xft, int64_t timeout_ns)
{
	struct u_future *uft = (struct u_future *)xft;
	if (uft == NULL) {
		return XRT_ERROR_INVALID_ARGUMENT;
	}

	if (timeout_ns < 0) {
		timeout_ns = INT64_MAX;
	}

	// on windows pthread_cond_timedwait can not be used with monotonic time
	const int64_t start_wait_rt = os_realtime_get_ns();
	const int64_t end_wait_rt =
	    (start_wait_rt > (INT64_MAX - timeout_ns)) ? INT64_MAX : (start_wait_rt + timeout_ns);

	struct timespec ts = {};
	os_ns_to_timespec(end_wait_rt, &ts);

	xrt_future_state_t curr_state = XRT_FUTURE_STATE_PENDING;

	os_mutex_lock(&uft->mtx);

	while ((curr_state = u_future_get_state_priv(uft)) == XRT_FUTURE_STATE_PENDING) {
		const int wait_res = pthread_cond_timedwait(&uft->cv.cond, &uft->mtx.mutex, &ts);
		if (wait_res == ETIMEDOUT) {
			if (os_realtime_get_ns() >= end_wait_rt) {
				// final state check - might have completed during timeout handling
				curr_state = u_future_get_state_priv(uft);
				break;
			}
		} else if (wait_res != 0) {

			break;
		}
	}

	os_mutex_unlock(&uft->mtx);

	if (curr_state == XRT_FUTURE_STATE_PENDING) {
		return XRT_TIMEOUT;
	}
	return u_future_get_xrt_result(uft);
}

static inline xrt_result_t
u_future_is_cancel_requested(const struct xrt_future *xft, bool *out_request_cancel)
{
	const struct u_future *uft = (const struct u_future *)xft;
	if (uft == NULL || out_request_cancel == NULL) {
		return XRT_ERROR_INVALID_ARGUMENT;
	}
	const xrt_future_state_t curr_state = u_future_get_state_priv(uft);
	*out_request_cancel = curr_state == XRT_FUTURE_STATE_CANCELLED;
	return XRT_SUCCESS;
}

static xrt_result_t
u_future_complete(struct xrt_future *xft, const struct xrt_future_result *ft_result)
{
	struct u_future *uft = (struct u_future *)xft;
	if (uft == NULL || ft_result == NULL) {
		return XRT_ERROR_INVALID_ARGUMENT;
	}

	os_mutex_lock(&uft->mtx);
	const xrt_future_state_t curr_state = u_future_get_state_priv(uft);
	if (curr_state != XRT_FUTURE_STATE_PENDING) {
		os_mutex_unlock(&uft->mtx);
		switch (curr_state) {
		case XRT_FUTURE_STATE_READY: return XRT_ERROR_FUTURE_ALREADY_COMPLETE;
		case XRT_FUTURE_STATE_CANCELLED:
		default: return XRT_OPERATION_CANCELLED;
		}
	}

	if (ft_result->result == XRT_SUCCESS) {
		uft->value = ft_result->value;
	}
	u_future_set_state_and_xrt_result(uft, XRT_FUTURE_STATE_READY, ft_result->result);

	os_cond_broadcast(&uft->cv);
	os_mutex_unlock(&uft->mtx);
	return XRT_SUCCESS;
}

static void
u_future_destroy(struct xrt_future *xft)
{
	struct u_future *uft = (struct u_future *)xft;
	if (uft == NULL) {
		return;
	}

	UFT_LOG_T("destroying u_future:%p", (void *)uft);

	u_future_cancel(&uft->base);
	u_future_wait(&uft->base, U_FUTURE_CLEANUP_TIMEOUT_NS);
	os_cond_destroy(&uft->cv);
	os_mutex_destroy(&uft->mtx);

	UFT_LOG_T("u_future:%p destroyed", (void *)uft);

	delete uft;
}

struct xrt_future *
u_future_create(void)
{
	std::unique_ptr<struct u_future> uft{new struct u_future()};
	os_mutex_init(&uft->mtx);
	os_cond_init(&uft->cv);

	struct xrt_future *xft = &uft->base;
	xft->reference.count = 1;
	xft->get_state = u_future_get_state;
	xft->get_result = u_future_get_result;
	xft->cancel = u_future_cancel;
	xft->wait = u_future_wait;
	xft->is_cancel_requested = u_future_is_cancel_requested;
	xft->complete = u_future_complete;
	xft->destroy = u_future_destroy;

	UFT_LOG_T("created u_future:%p", (void *)xft);
	return &uft.release()->base;
}
