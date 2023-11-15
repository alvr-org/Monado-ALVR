// Copyright 2021-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief Generic callback collection tests.
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 */

#include "catch/catch.hpp"

#include <util/u_generic_callbacks.hpp>

using xrt::auxiliary::util::GenericCallbacks;

enum class MyEvent
{
	ACQUIRED = 1u << 0u,
	LOST = 1u << 1u,
};

using mask_t = std::underlying_type_t<MyEvent>;

static bool
increment_userdata_int(MyEvent event, void *userdata)
{
	*static_cast<int *>(userdata) += 1;
	return true;
}


using callback_t = bool (*)(MyEvent event, void *userdata);

TEST_CASE("u_generic_callbacks")
{
	GenericCallbacks<callback_t, MyEvent> callbacks;
	// Simplest possible invoker.
	auto invoker = [](MyEvent event, callback_t callback, void *userdata) { return callback(event, userdata); };

	SECTION("call when empty")
	{
		CHECK(0 == callbacks.invokeCallbacks(MyEvent::ACQUIRED, invoker));
		CHECK(0 == callbacks.invokeCallbacks(MyEvent::LOST, invoker));
		CHECK(0 == callbacks.removeCallback(&increment_userdata_int, (mask_t)MyEvent::LOST, nullptr));
	}
	SECTION("same function, different mask and userdata")
	{
		int numAcquired = 0;
		int numLost = 0;
		REQUIRE_NOTHROW(callbacks.addCallback(increment_userdata_int, (mask_t)MyEvent::ACQUIRED, &numAcquired));
		REQUIRE_NOTHROW(callbacks.addCallback(increment_userdata_int, (mask_t)MyEvent::LOST, &numLost));
		SECTION("contains")
		{
			CHECK(callbacks.contains(increment_userdata_int, (mask_t)MyEvent::LOST, &numLost));
			CHECK_FALSE(callbacks.contains(increment_userdata_int, (mask_t)MyEvent::LOST, &numAcquired));
		}
		SECTION("removal matching")
		{
			CHECK(0 ==
			      callbacks.removeCallback(increment_userdata_int, (mask_t)MyEvent::LOST, &numAcquired));
			CHECK(0 ==
			      callbacks.removeCallback(increment_userdata_int, (mask_t)MyEvent::ACQUIRED, &numLost));
		}
		SECTION("duplicates, contains, and removal")
		{
			REQUIRE(callbacks.contains(increment_userdata_int, (mask_t)MyEvent::ACQUIRED, &numAcquired));
			REQUIRE_NOTHROW(
			    callbacks.addCallback(increment_userdata_int, (mask_t)MyEvent::ACQUIRED, &numAcquired));
			CHECK(callbacks.contains(increment_userdata_int, (mask_t)MyEvent::ACQUIRED, &numAcquired));
			// Now we have two ACQUIRED and one LOST callback.
			SECTION("max_remove")
			{
				CHECK(0 == callbacks.removeCallback(increment_userdata_int, (mask_t)MyEvent::ACQUIRED,
				                                    &numAcquired, 0, 0));
				CHECK(callbacks.contains(increment_userdata_int, (mask_t)MyEvent::ACQUIRED,
				                         &numAcquired));

				CHECK(1 == callbacks.removeCallback(increment_userdata_int, (mask_t)MyEvent::ACQUIRED,
				                                    &numAcquired, 0, 1));
				CHECK(callbacks.contains(increment_userdata_int, (mask_t)MyEvent::ACQUIRED,
				                         &numAcquired));

				// LOST callback should still be there to remove
				CHECK(1 == callbacks.removeCallback(increment_userdata_int, (mask_t)MyEvent::LOST,
				                                    &numLost));
			}
			SECTION("large max_remove")
			{
				CHECK(2 == callbacks.removeCallback(increment_userdata_int, (mask_t)MyEvent::ACQUIRED,
				                                    &numAcquired, 0, 3));
				CHECK_FALSE(callbacks.contains(increment_userdata_int, (mask_t)MyEvent::ACQUIRED,
				                               &numAcquired));

				// LOST callback should still be there to remove
				CHECK(1 == callbacks.removeCallback(increment_userdata_int, (mask_t)MyEvent::LOST,
				                                    &numLost));
			}
			SECTION("num_skip")
			{
				CHECK(0 == callbacks.removeCallback(increment_userdata_int, (mask_t)MyEvent::ACQUIRED,
				                                    &numAcquired, 3));
				CHECK(callbacks.contains(increment_userdata_int, (mask_t)MyEvent::ACQUIRED,
				                         &numAcquired));

				CHECK(1 == callbacks.removeCallback(increment_userdata_int, (mask_t)MyEvent::ACQUIRED,
				                                    &numAcquired, 1));
				CHECK(callbacks.contains(increment_userdata_int, (mask_t)MyEvent::ACQUIRED,
				                         &numAcquired));

				// LOST callback should still be there to remove
				CHECK(1 == callbacks.removeCallback(increment_userdata_int, (mask_t)MyEvent::LOST,
				                                    &numLost));
			}
			SECTION("invoke acquired")
			{
				CHECK(2 == callbacks.invokeCallbacks(MyEvent::ACQUIRED, invoker));
				CHECK(2 == numAcquired);
				CHECK(0 == numLost);
				{
					INFO("should have removed themselves");
					CHECK_FALSE(callbacks.contains(increment_userdata_int,
					                               (mask_t)MyEvent::ACQUIRED, &numAcquired));
				}

				{
					INFO("LOST callbacks should still be there");
					CHECK(callbacks.contains(increment_userdata_int, (mask_t)MyEvent::LOST,
					                         &numLost));
					CHECK(1 == callbacks.removeCallback(increment_userdata_int,
					                                    (mask_t)MyEvent::LOST, &numLost));
				}
			}
			SECTION("invoke lost")
			{
				CHECK(1 == callbacks.invokeCallbacks(MyEvent::LOST, invoker));
				CHECK(0 == numAcquired);
				CHECK(1 == numLost);
				{
					INFO("should have removed themselves");
					CHECK_FALSE(callbacks.contains(increment_userdata_int, (mask_t)MyEvent::LOST,
					                               &numLost));
				}

				{
					INFO("ACQUIRED callbacks should still be there");
					CHECK(callbacks.contains(increment_userdata_int, (mask_t)MyEvent::ACQUIRED,
					                         &numAcquired));
					CHECK(2 == callbacks.removeCallback(increment_userdata_int,
					                                    (mask_t)MyEvent::ACQUIRED, &numAcquired));
				}
			}
		}
	}
}


enum MyCEvent
{
	MY_C_EVENT_ACQUIRE = 1u << 0u,
	MY_C_EVENT_LOST = 1u << 1u,
};

using c_callback_type = bool (*)(MyCEvent event, void *userdata);

TEST_CASE("u_generic_callbacks-C")
{
	GenericCallbacks<c_callback_type, MyCEvent> callbacks;
}
