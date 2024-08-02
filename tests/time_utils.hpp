// Copyright 2022, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Utilities for tests involving time points and durations
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 */

#pragma once

#include <chrono>

using unanoseconds = std::chrono::duration<int64_t, std::nano>;

class MockClock
{
public:
	int64_t
	now() const noexcept
	{
		return std::chrono::duration_cast<unanoseconds>(now_.time_since_epoch()).count();
	}

	std::chrono::steady_clock::time_point
	now_typed() const noexcept
	{
		return now_;
	}

	void
	advance(unanoseconds ns)
	{
		now_ += ns;
	}

	void
	advance_to(int64_t timestamp_ns)
	{
		CHECK(now() <= timestamp_ns);
		now_ = std::chrono::steady_clock::time_point(
		    std::chrono::steady_clock::duration(unanoseconds(timestamp_ns)));
	}

private:
	std::chrono::steady_clock::time_point now_{std::chrono::steady_clock::duration(std::chrono::seconds(1000000))};
};

struct FutureEvent
{
	std::chrono::steady_clock::time_point time_point;
	std::function<void()> action;
};
