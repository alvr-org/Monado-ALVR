// Copyright 2020-2024, Collabora, Ltd.
// Copyright 2024-2026, NVIDIA CORPORATION.
// Copyright 2026, Nick Kennedy
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  IPC client thread shutdown and blocking I/O cancellation.
 * @author Jonathan Steffan <jsteffan@fedoraproject.org>
 * @author Nick Kennedy <code@nick-kennedy.com>
 * @ingroup ipc_server
 */

#include "server/ipc_server.h"
#include "server/ipc_server_thread_shutdown.h"

#ifndef XRT_OS_WINDOWS
#include <sys/socket.h>
#endif

/*!
 * Cancel any blocking I/O a client thread is sitting in.
 *
 * An idle epoll based client loop wakes up on its own timeout and so notices
 * the thread entering STOPPING, but the Windows one blocks in ReadFile() with no
 * timeout and would only notice once the client happens to send something. On
 * every platform a handler can also block mid-message, on a client that never
 * sends the rest of a payload or never reads a reply. Cancelling the pending
 * I/O makes it fail so the thread can run its shutdown and be joined.
 *
 * The client thread closes this handle in common_shutdown() while holding the
 * global state lock, so take it here as well, otherwise we could cancel I/O on
 * a handle that has just been closed and possibly reused.
 */
static void
ipc_server_cancel_all_client_io(struct ipc_server *s)
{
	os_mutex_lock(&s->global_state.lock);

	for (uint32_t i = 0; i < IPC_MAX_CLIENTS; i++) {
		struct ipc_thread *it = &s->threads[i];
		if (it->state == IPC_THREAD_READY) {
			continue;
		}

		xrt_ipc_handle_t ipc_handle = it->ics.imc.ipc_handle;
		if (!xrt_ipc_handle_is_valid(ipc_handle)) {
			continue;
		}

#ifdef XRT_OS_WINDOWS
		CancelIoEx(ipc_handle, NULL);
		// Also prevent a read starting just after cancellation from blocking.
		// This is the server end of a named pipe, owned by the client thread.
		DisconnectNamedPipe(ipc_handle);
#else
		/*
		 * Unlike close(), this wakes I/O that is already blocked and fails I/O that starts later.
		 * The client thread still owns the fd and closes it in common_shutdown().
		 */
		shutdown(ipc_handle, SHUT_RDWR);
#endif
	}

	os_mutex_unlock(&s->global_state.lock);
}

void
ipc_server_client_thread_set_running(struct ipc_server *s, struct ipc_thread *it)
{
	os_mutex_lock(&s->global_state.lock);
	if (it->state == IPC_THREAD_STARTING) {
		it->state = IPC_THREAD_RUNNING;
	}
	os_mutex_unlock(&s->global_state.lock);
}

bool
ipc_server_client_thread_is_running(struct ipc_server *s, struct ipc_thread *it)
{
	os_mutex_lock(&s->global_state.lock);
	bool running = it->state == IPC_THREAD_RUNNING;
	os_mutex_unlock(&s->global_state.lock);
	return running;
}

/*!
 * Stop and join any remaining per-client threads.
 *
 * Client threads may still be inside common_shutdown(), which destroys each
 * client's compositor and dereferences `s->xsysd` / `s->xso`. Joining them before
 * teardown_all() destroys those resources avoids a race where a client thread
 * locks a mutex that teardown has already freed (observed as an
 * `om->initialized` assertion in multi_compositor_destroy).
 *
 * Stop their loops before joining, then wake every thread blocked in client
 * I/O with @ref ipc_server_cancel_all_client_io. That covers the Windows loop's
 * ReadFile() and handlers stuck mid-message on any platform.
 *
 * Called from shutdown_clients() before the main loop returns, and again from
 * teardown_all() as a fallback.
 */
void
ipc_server_stop_and_join_client_threads(struct ipc_server *s)
{
	bool have_threads = false;
	bool join_thread[IPC_MAX_CLIENTS] = {false};
	os_mutex_lock(&s->global_state.lock);
	for (uint32_t i = 0; i < IPC_MAX_CLIENTS; i++) {
		struct ipc_thread *it = &s->threads[i];
		if (it->state != IPC_THREAD_READY) {
			it->state = IPC_THREAD_STOPPING;
			join_thread[i] = true;
			have_threads = true;
		}
	}
	os_mutex_unlock(&s->global_state.lock);

	if (!have_threads) {
		return;
	}
	// Wake every thread blocked in client I/O before waiting for any one client to finish.
	ipc_server_cancel_all_client_io(s);
	for (uint32_t i = 0; i < IPC_MAX_CLIENTS; i++) {
		struct ipc_thread *it = &s->threads[i];
		if (!join_thread[i]) {
			continue;
		}
		os_thread_join(&it->thread);
		os_thread_destroy(&it->thread);
		os_mutex_lock(&s->global_state.lock);
		it->state = IPC_THREAD_READY;
		os_mutex_unlock(&s->global_state.lock);
	}
}
