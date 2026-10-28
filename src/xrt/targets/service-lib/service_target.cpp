// Copyright 2020, Collabora, Ltd.
// Copyright 2024-2025, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Library exposing IPC server.
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @ingroup ipc_android
 */

#include "jnipp.h"
#include "jni.h"

#include "wrap/android.view.h"

#include "server/ipc_server.h"
#include "server/ipc_server_interface.h"
#include "server/ipc_server_mainloop_android.h"
#include "util/u_logging.h"

#include <android/native_window.h>
#include <android/native_window_jni.h>

#include "android/android_globals.h"

#include <chrono>
#include <memory>
#include <thread>

using wrap::android::view::Surface;
using namespace std::chrono_literals;

namespace {
struct IpcServerHelper
{
public:
	static IpcServerHelper &
	instance()
	{
		static IpcServerHelper instance;
		return instance;
	}

	void
	signalStartupComplete(ipc_server *s)
	{
		std::unique_lock<std::mutex> lock{server_mutex};
		startup_complete = true;
		server = s;
		startup_cond.notify_all();
	}

	void
	startServer()
	{
		std::unique_lock lock(server_mutex);
		if (!server && !server_thread) {
			server_thread =
			    std::make_unique<std::thread>([&]() { ipc_server_main_common(&ismi, &callbacks, this); });
		}
	}

	static void
	signalInitFailed(xrt_result_t xret, void *data)
	{
		static_cast<IpcServerHelper *>(data)->signalStartupComplete(nullptr);
	}

	static void
	signalStartupCompleteTrampoline(ipc_server *s, xrt_instance *xsint, void *data)
	{
		static_cast<IpcServerHelper *>(data)->signalStartupComplete(s);
	}

	static void
	signalShuttingDownTrampoline(ipc_server *s, xrt_instance *xsint, void *data)
	{
		// No-op
	}

	static void
	signalClientConnectedTrampoline(struct ipc_server *s, uint32_t client_id, void *data)
	{
		// No-op
	}

	static void
	signalClientDisconnectedTrampoline(struct ipc_server *s, uint32_t client_id, void *data)
	{
		// No-op
	}

	int32_t
	addClient(int fd)
	{
		if (!waitForStartupComplete()) {
			return -1;
		}
		return ipc_server_mainloop_add_fd(server, &server->ml, fd);
	}

	int32_t
	shutdownServer()
	{
		if (!server || !server_thread) {
			// Should not happen.
			U_LOG_E("service: shutdownServer called before server started up!");
			return -1;
		}

		{
			// Wait until IPC server stop
			std::unique_lock lock(server_mutex);
			ipc_server_handle_shutdown_signal(server);
			server_thread->join();
			server_thread.reset(nullptr);
			server = NULL;
			startup_complete = false;
		}

		return 0;
	}

private:
	IpcServerHelper() {}

	bool
	waitForStartupComplete()
	{
		std::unique_lock<std::mutex> lock{server_mutex};
		bool completed = startup_cond.wait_for(lock, START_TIMEOUT_SECONDS, [&]() { return startup_complete; });

		if (!server) {
			U_LOG_E("Failed to create ipc server");
		}

		if (!completed) {
			U_LOG_E("Server startup timeout!");
		}
		return server && completed;
	}

	const struct ipc_server_main_info ismi = {
	    .udgci =
	        {
	            .window_title = "Monado Android Service",
	            .open = U_DEBUG_GUI_OPEN_NEVER,
	        },
	    .exit_on_disconnect = false,
	    .no_stdin = false,
	};

	const struct ipc_server_callbacks callbacks = {
	    .init_failed = signalInitFailed,
	    .mainloop_entering = signalStartupCompleteTrampoline,
	    .mainloop_leaving = signalShuttingDownTrampoline,
	    .client_connected = signalClientConnectedTrampoline,
	    .client_disconnected = signalClientDisconnectedTrampoline,
	};

	//! Reference to the ipc_server, managed by ipc_server_process
	struct ipc_server *server = NULL;

	//! Mutex for starting thread
	std::mutex server_mutex;

	//! Server thread
	std::unique_ptr<std::thread> server_thread{};

	//! Condition variable for starting thread
	std::condition_variable startup_cond;

	//! Server startup state
	bool startup_complete = false;

	//! Timeout duration in seconds
	static constexpr std::chrono::seconds START_TIMEOUT_SECONDS = 40s;
};
} // namespace

extern "C" JNIEXPORT void JNICALL
Java_org_freedesktop_monado_ipc_MonadoImpl_nativeStartServer(JNIEnv *env, jobject thiz, jobject context)
{
	JavaVM *jvm = nullptr;
	jint result = env->GetJavaVM(&jvm);

	assert(result == JNI_OK);
	if (result != JNI_OK) {
		U_LOG_E("service: GetJavaVM failed, got %d. This should never happen.", result);
	}

	assert(jvm);
	if (!jvm) {
		U_LOG_E("service: JavaVM is null? This should never happen.");
	}

	jni::init(env);
	jni::Object monadoImpl(thiz);
	U_LOG_D("service: Called nativeStartServer");

	android_globals_store_vm_and_context(jvm, context);

	IpcServerHelper::instance().startServer();
}

extern "C" JNIEXPORT jint JNICALL
Java_org_freedesktop_monado_ipc_MonadoImpl_nativeAddClient(JNIEnv *env, jobject thiz, int fd)
{
	jni::init(env);
	jni::Object monadoImpl(thiz);
	U_LOG_D("service: Called nativeAddClient with fd %d", fd);

	int native_fd = dup(fd);
	U_LOG_D("service: transfer ownership to native and native_fd %d", native_fd);

	// We try pushing the fd number to the server. If and only if we get a 0 return, has the server taken ownership.
	return IpcServerHelper::instance().addClient(native_fd);
}

extern "C" JNIEXPORT void JNICALL
Java_org_freedesktop_monado_ipc_MonadoImpl_nativeAppSurface(JNIEnv *env, jobject thiz, jobject surface)
{
	jni::init(env);
	Surface surf(surface);
	jni::Object monadoImpl(thiz);

	ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);
	android_globals_store_window((struct _ANativeWindow *)nativeWindow);
	U_LOG_D("Stored ANativeWindow: %p", (void *)nativeWindow);
}

extern "C" JNIEXPORT jint JNICALL
Java_org_freedesktop_monado_ipc_MonadoImpl_nativeShutdownServer(JNIEnv *env, jobject thiz)
{
	jni::init(env);
	jni::Object monadoImpl(thiz);

	return IpcServerHelper::instance().shutdownServer();
}
