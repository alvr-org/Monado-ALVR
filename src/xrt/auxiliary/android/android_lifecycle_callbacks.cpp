// Copyright 2021-2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Implementation of a callback collection for Android lifecycle events.
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @ingroup aux_android
 */

#include "android_lifecycle_callbacks.h"

#include "xrt/xrt_config_android.h"
#include "xrt/xrt_android.h"
#include "util/u_logging.h"
#include "util/u_generic_callbacks.hpp"

#include <memory>

using xrt::auxiliary::util::GenericCallbacks;

struct android_lifecycle_callbacks
{
	explicit android_lifecycle_callbacks(xrt_instance_android *xinst_android) : instance_android(xinst_android) {}
	xrt_instance_android *instance_android;
	GenericCallbacks<xrt_android_lifecycle_event_handler_t, enum xrt_android_lifecycle_event> callback_collection;
};

#define CATCH_CLAUSES(ACTION, RET)                                                                                     \
	catch (std::exception const &e)                                                                                \
	{                                                                                                              \
		U_LOG_E("Exception while " ACTION "! %s", e.what());                                                   \
		return RET;                                                                                            \
	}                                                                                                              \
	catch (...)                                                                                                    \
	{                                                                                                              \
		U_LOG_E("Unknown exception while " ACTION "!");                                                        \
		return RET;                                                                                            \
	}

int
android_lifecycle_callbacks_register_callback(struct android_lifecycle_callbacks *alc,
                                              xrt_android_lifecycle_event_handler_t callback,
                                              enum xrt_android_lifecycle_event event_mask,
                                              void *userdata)
{
	try {
		alc->callback_collection.addCallback(callback, event_mask, userdata);
		return 0;
	}
	CATCH_CLAUSES("adding callback to collection", -1)
}

int
android_lifecycle_callbacks_remove_callback(struct android_lifecycle_callbacks *alc,
                                            xrt_android_lifecycle_event_handler_t callback,
                                            enum xrt_android_lifecycle_event event_mask,
                                            void *userdata)
{
	try {
		return alc->callback_collection.removeCallback(callback, event_mask, userdata);
	}
	CATCH_CLAUSES("removing callback", -1)
}

int
android_lifecycle_callbacks_invoke(struct android_lifecycle_callbacks *alc, enum xrt_android_lifecycle_event event)
{
	try {
		return alc->callback_collection.invokeCallbacks(
		    event, [=](enum xrt_android_lifecycle_event event, xrt_android_lifecycle_event_handler_t callback,
		               void *userdata) { return callback(alc->instance_android, event, userdata); });
	}
	CATCH_CLAUSES("invoking callbacks", -1)
}

struct android_lifecycle_callbacks *
android_lifecycle_callbacks_create(struct xrt_instance_android *xinst_android)
{
	try {
		auto ret = std::make_unique<android_lifecycle_callbacks>(xinst_android);

		return ret.release();
	}
	CATCH_CLAUSES("creating callbacks structure", nullptr)
}

void
android_lifecycle_callbacks_destroy(struct android_lifecycle_callbacks **ptr_callbacks)
{
	if (ptr_callbacks == nullptr) {
		return;
	}
	struct android_lifecycle_callbacks *alc = *ptr_callbacks;
	if (alc == nullptr) {
		return;
	}
	delete alc;
	*ptr_callbacks = nullptr;
}
