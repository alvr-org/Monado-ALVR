// Copyright 2020, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Implementation exposing Android-specific IPC client code to C.
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @ingroup ipc_android
 */

#include "ipc_client_android.h"

#include "org.freedesktop.monado.ipc.hpp"

#include "xrt/xrt_config_android.h"
#include "util/u_logging.h"

#include "android/android_load_class.hpp"
#include "android/android_looper.h"

#include "wrap/android.app.h"

using wrap::android::app::Activity;
using wrap::org::freedesktop::monado::ipc::Client;
using xrt::auxiliary::android::loadClassFromRuntimeApk;

struct ipc_client_android
{
	ipc_client_android(struct _JavaVM *vm_, jobject act) : vm(vm_), activity(act) {}
	~ipc_client_android();
	struct _JavaVM *vm;

	Activity activity{};
	Client client{nullptr};
};

ipc_client_android::~ipc_client_android()
{

	// Tell Java that native code is done with this.
	try {
		if (!client.isNull()) {
			client.markAsDiscardedByNative();
		}
	} catch (std::exception const &e) {
		// Must catch and ignore any exceptions in the destructor!
		U_LOG_E("Failure while marking IPC client as discarded: %s", e.what());
	}
}

struct ipc_client_android *
ipc_client_android_create(struct _JavaVM *vm, void *activity)
{

	jni::init(vm);
	try {
		auto clazz = loadClassFromRuntimeApk((jobject)activity, Client::getFullyQualifiedTypeName());
		if (clazz.isNull()) {
			U_LOG_E("Could not load class '%s' from package '%s'", Client::getFullyQualifiedTypeName(),
			        XRT_ANDROID_PACKAGE);
			return nullptr;
		}

		// Teach the wrapper our class before we start to use it.
		Client::staticInitClass((jclass)clazz.object().getHandle());
		std::unique_ptr<ipc_client_android> ret = std::make_unique<ipc_client_android>(vm, (jobject)activity);

		ret->client = Client::construct(ret.get());

		return ret.release();
	} catch (std::exception const &e) {

		U_LOG_E("Could not start IPC client class: %s", e.what());
		return nullptr;
	}
}

int
ipc_client_android_blocking_connect(struct ipc_client_android *ica)
{
	try {
		// Trick to avoid deadlock on main thread: only applicable to NativeActivity with app-glue.
		// blockingConnect will block until binder is ready, the app-glue code will deadlock without this.
		JavaVM *vm = ica->vm;
		android_looper_poll_until_activity_resumed(vm, ica->activity.object().getHandle());
		int fd = ica->client.blockingConnect(ica->activity, XRT_ANDROID_PACKAGE);
		return fd;
	} catch (std::exception const &e) {
		U_LOG_E("Failure while connecting to IPC server: %s", e.what());
		return -1;
	}
}


void
ipc_client_android_destroy(struct ipc_client_android **ptr_ica)
{

	if (ptr_ica == NULL) {
		return;
	}
	struct ipc_client_android *ica = *ptr_ica;
	if (ica == NULL) {
		return;
	}
	try {
		delete ica;
	} catch (std::exception const &e) {
		// Must catch and ignore any exceptions in the destructor!
		U_LOG_E("Failure while destroying IPC clean: %s", e.what());
	}
	*ptr_ica = NULL;
}
