// Copyright 2020-2023, Collabora, Ltd.
// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Per client thread listening on the socket.
 * @author Pete Black <pblack@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup ipc_server
 */

#include "util/u_misc.h"
#include "util/u_trace_marker.h"

#include "xrt/xrt_body_tracker.h"
#include "xrt/xrt_hand_tracker.h"
#include "xrt/xrt_io_cache.h"

#include "shared/ipc_protocol.h"
#include "shared/ipc_shmem.h"
#include "shared/ipc_utils.h"
#include "server/ipc_server.h"
#include "server/ipc_server_thread_shutdown.h"
#include "ipc_server_generated.h"

#ifndef XRT_OS_WINDOWS

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#endif // XRT_OS_WINDOWS


/*
 *
 * Helper functions.
 *
 */

static void *
delayed_exit_thread(void *_server)
{
	struct ipc_server *s = (struct ipc_server *)_server;

	// Sleep for the delay duration
	os_nanosleep(s->exit_when_idle_delay_ns);

	// Check if we still have no clients after the delay
	os_mutex_lock(&s->global_state.lock);
	bool should_exit =
	    (s->exit_when_idle && s->global_state.connected_client_count == 0 && s->last_client_disconnect_ns > 0);
	os_mutex_unlock(&s->global_state.lock);

	if (should_exit) {
		IPC_INFO(s, "All clients disconnected for %u milliseconds, shutting down.",
		         (unsigned int)(s->exit_when_idle_delay_ns / U_TIME_1MS_IN_NS));
		s->running = false;
	}

	return NULL;
}

static void
handle_exit_on_disconnect(struct ipc_server *s)
{
	if (!s->running) {
		return;
	}

	// Should we stop the server when a client disconnects?
	if (s->exit_on_disconnect) {
		s->running = false;
	}

	os_mutex_lock(&s->global_state.lock);
	bool do_delayed_exit = s->exit_when_idle && s->global_state.connected_client_count == 0;
	os_mutex_unlock(&s->global_state.lock);

	// Should we stop when all clients disconnect?
	if (do_delayed_exit) {
		s->last_client_disconnect_ns = os_monotonic_get_ns();

		struct os_thread thread;
		os_thread_start(&thread, delayed_exit_thread, (void *)s);
		// We intentionally don't join this thread - it's fire and forget
	}
}

static void
common_shutdown(volatile struct ipc_client_state *ics)
{
	/*
	 * Remove the thread from the server.
	 */

	// Multiple threads might be looking at these fields.
	os_mutex_lock(&ics->server->global_state.lock);

	ipc_message_channel_close((struct ipc_message_channel *)&ics->imc);

	ipc_shmem_destroy((xrt_shmem_handle_t *)&ics->ism_handle, (void **)&ics->server->isms[ics->server_thread_index],
	                  sizeof(struct ipc_shared_memory));

	ics->server->threads[ics->server_thread_index].state = IPC_THREAD_STOPPING;
	ics->server_thread_index = -1;
	memset((void *)&ics->client_state, 0, sizeof(struct ipc_app_state));

	// Decrement the connected client counter
	ics->server->global_state.connected_client_count--;

	os_mutex_unlock(&ics->server->global_state.lock);


	/*
	 * Clean up various resources.
	 */

	// If the session hasn't been stopped, destroy the compositor.
	ipc_server_client_destroy_session_and_compositor(ics);

	// Make sure undestroyed spaces are unreferenced
	for (uint32_t i = 0; i < IPC_MAX_CLIENT_SPACES; i++) {
		// Cast away volatile.
		xrt_space_reference((struct xrt_space **)&ics->xspcs[i], NULL);
	}

	if (ics->local_space_overseer_index < IPC_MAX_CLIENT_SPACES) {
		xrt_space_reference((struct xrt_space **)&ics->server->xso->localspace[ics->local_space_overseer_index],
		                    NULL);
	}

	if (ics->local_floor_space_overseer_index < IPC_MAX_CLIENT_SPACES && //
	    ics->local_floor_space_overseer_index > 0) {
		xrt_space_reference(
		    (struct xrt_space **)&ics->server->xso->localfloorspace[ics->local_floor_space_overseer_index],
		    NULL);
	}

	// Mark an still in use reference spaces as no longer used.
	for (uint32_t i = 0; i < ARRAY_SIZE(ics->ref_space_used); i++) {
		bool used = ics->ref_space_used[i];
		if (!used) {
			continue;
		}

		xrt_space_overseer_ref_space_dec(ics->server->xso, (enum xrt_reference_space_type)i);
		ics->ref_space_used[i] = false;
	}

	// Make a still in use device features as no longer used.
	for (uint32_t i = 0; i < ARRAY_SIZE(ics->device_feature_used); i++) {
		bool used = ics->device_feature_used[i];
		if (!used) {
			continue;
		}

		xrt_system_devices_feature_dec(ics->server->xsysd, (enum xrt_device_feature_type)i);
		ics->device_feature_used[i] = false;
	}

	// Destroy body trackers owned by this client.
	for (uint32_t i = 0; i < IPC_MAX_CLIENT_BODY_TRACKERS; i++) {
		xrt_body_tracker_destroy((struct xrt_body_tracker **)&ics->objects.xbts[i]);
	}

	// Destroy hand trackers owned by this client.
	for (uint32_t i = 0; i < IPC_MAX_CLIENT_HAND_TRACKERS; i++) {
		xrt_hand_tracker_destroy((struct xrt_hand_tracker **)&ics->objects.xhts[i]);
	}

	// Destroy app systems owned by this client.
	for (uint32_t i = 0; i < IPC_MAX_CLIENT_APP_SYSTEMS; i++) {
		xrt_app_system_destroy((struct xrt_app_system **)&ics->objects.xasys[i]);
	}

	// Destroy app instances owned by this client.
	for (uint32_t i = 0; i < IPC_MAX_CLIENT_APP_INSTANCES; i++) {
		xrt_app_instance_destroy((struct xrt_app_instance **)&ics->objects.xainsts[i]);
	}

	// Destroy input/output caches owned by this client.
	for (uint32_t i = 0; i < IPC_MAX_CLIENT_IO_CACHES; i++) {
		xrt_io_cache_destroy((struct xrt_io_cache **)&ics->objects.xics[i]);
	}

	// Clear the tracking origins array.
	for (uint32_t i = 0; i < XRT_SYSTEM_MAX_DEVICES; i++) {
		/*
		 * We don't control the lifetime of the tracking origins,
		 * so we just set the pointer to NULL.
		 */
		ics->objects.xtracks[i] = NULL;
	}

	// Make sure undestroyed plane detections are cleaned up
	for (uint32_t i = 0; i < ics->plane_detection_count; i++) {
		xrt_device_destroy_plane_detection_ext(ics->plane_detection_xdev[i], ics->plane_detection_ids[i]);
	}
	free(ics->plane_detection_ids);
	free(ics->plane_detection_xdev);
	ics->plane_detection_ids = NULL;
	ics->plane_detection_xdev = NULL;
	ics->plane_detection_size = 0;
	ics->plane_detection_count = 0;

	handle_exit_on_disconnect(ics->server);

	ipc_server_deactivate_session(ics);
}


/*
 *
 * Client loop and per platform helpers.
 *
 */

#ifndef XRT_OS_WINDOWS // Linux & Android

static int
setup_epoll(volatile struct ipc_client_state *ics)
{
	int listen_socket = ics->imc.ipc_handle;
	assert(listen_socket >= 0);

	int ret = epoll_create1(EPOLL_CLOEXEC);
	if (ret < 0) {
		return ret;
	}

	int epoll_fd = ret;

	struct epoll_event ev = XRT_STRUCT_INIT;

	ev.events = EPOLLIN;
	ev.data.fd = listen_socket;
	ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_socket, &ev);
	if (ret < 0) {
		IPC_ERROR(ics->server, "Error epoll_ctl(listen_socket) failed '%i'.", ret);
		return ret;
	}

	return epoll_fd;
}

static void
client_loop(volatile struct ipc_client_state *ics)
{
	U_TRACE_SET_THREAD_NAME("IPC Client");

	// Call the client connected callback.
	ics->server->callbacks->client_connected( //
	    ics->server,                          //
	    ics->client_state.id,                 //
	    ics->server->callback_data);          //

	// Claim the client fd.
	int epoll_fd = setup_epoll(ics);
	if (epoll_fd < 0) {
		return;
	}

	struct ipc_thread *it = &ics->server->threads[ics->server_thread_index];
	ipc_server_client_thread_set_running(ics->server, it);
	while (ipc_server_client_thread_is_running(ics->server, it)) {
		const int half_a_second_ms = 500;
		struct epoll_event event = XRT_STRUCT_INIT;
		int ret = 0;

		// On temporary failures retry.
		do {
			// We use epoll here to be able to timeout.
			ret = epoll_wait(epoll_fd, &event, 1, half_a_second_ms);
		} while (ret == -1 && errno == EINTR);

		if (ret < 0) {
			IPC_ERROR(ics->server, "Failed epoll_wait '%i', disconnecting client.", ret);
			break;
		}

		// Timed out, loop again.
		if (ret == 0) {
			continue;
		}

		// Detect clients disconnecting gracefully.
		if (ret > 0 && (event.events & EPOLLHUP) != 0) {
			break;
		}

		// Peek the first 4 bytes to get the command type
		enum ipc_command cmd;
		ssize_t len = recv(ics->imc.ipc_handle, &cmd, sizeof(cmd), MSG_PEEK);
		if (len != sizeof(cmd)) {
			IPC_ERROR(ics->server, "Invalid command received.");
			break;
		}

		size_t cmd_size = ipc_command_size(cmd);
		if (cmd_size == 0) {
			IPC_ERROR(ics->server, "Invalid command size.");
			break;
		}

		// This needs to hold true.
		if (cmd_size > IPC_BUF_SIZE) {
			IPC_ERROR(ics->server, "Command too large! (%u > %u)", (uint32_t)cmd_size, IPC_BUF_SIZE);
			break;
		}

		// Read the whole command now that we know its size
		uint8_t buf[IPC_BUF_SIZE] = {0};

		len = recv(ics->imc.ipc_handle, &buf, cmd_size, 0);
		if (len != (ssize_t)cmd_size) {
			IPC_ERROR(ics->server, "Invalid packet received, disconnecting client.");
			break;
		}

		// Check the first 4 bytes of the message and dispatch.
		ipc_command_t *ipc_command = (ipc_command_t *)buf;

		IPC_TRACE_BEGIN(ipc_dispatch);
		xrt_result_t result = ipc_dispatch(ics, ipc_command);
		IPC_TRACE_END(ipc_dispatch);

		if (result != XRT_SUCCESS) {
			IPC_ERROR(ics->server, "During packet handling, disconnecting client.");
			break;
		}
	}

	close(epoll_fd);

	// Call the client disconnected callback.
	ics->server->callbacks->client_disconnected( //
	    ics->server,                             //
	    ics->client_state.id,                    //
	    ics->server->callback_data);             //

	// Following code is same for all platforms.
	common_shutdown(ics);
}

#else // XRT_OS_WINDOWS

static void
pipe_print_get_last_error(volatile struct ipc_client_state *ics, const char *func)
{
	// This is the error path.
	DWORD err = GetLastError();
	if (err == ERROR_BROKEN_PIPE) {
		IPC_INFO(ics->server, "%s: %lu %s", func, err, ipc_winerror(err));
	} else {
		IPC_ERROR(ics->server, "%s failed: %lu %s", func, err, ipc_winerror(err));
	}
}

static void
client_loop(volatile struct ipc_client_state *ics)
{
	U_TRACE_SET_THREAD_NAME("IPC Client");

	// Call the client connected callback.
	ics->server->callbacks->client_connected( //
	    ics->server,                          //
	    ics->client_state.id,                 //
	    ics->server->callback_data);          //

	struct ipc_thread *it = &ics->server->threads[ics->server_thread_index];
	ipc_server_client_thread_set_running(ics->server, it);
	while (ipc_server_client_thread_is_running(ics->server, it)) {
		uint8_t buf[IPC_BUF_SIZE] = {0};
		DWORD len = 0;
		BOOL bret = false;

		/*
		 * The pipe is created in message mode, the client IPC code will
		 * always send the *_msg structs as one message, and any extra
		 * variable length data as a different message. So even if the
		 * command is a variable length the first message will be sized
		 * to the command size, this is what we get here, variable
		 * length data is read in the dispatch function for the command.
		 */
		bret = ReadFile(ics->imc.ipc_handle, buf, sizeof(buf), &len, NULL);
		if (!bret) {
			pipe_print_get_last_error(ics, "ReadFile");
			IPC_ERROR(ics->server, "ReadFile failed, disconnecting client.");
			break;
		}

		// All commands are at least 4 bytes.
		if (len < 4) {
			IPC_ERROR(ics->server, "Not enough bytes received '%u', disconnecting client.", (uint32_t)len);
			break;
		}

		// Now safe to cast into a command pointer, used for dispatch.
		ipc_command_t *cmd_ptr = (ipc_command_t *)buf;

		// Read the command, we know we have at least 4 bytes.
		ipc_command_t cmd = *cmd_ptr;

		// Get the command length.
		size_t cmd_size = ipc_command_size(cmd);
		if (cmd_size == 0) {
			IPC_ERROR(ics->server, "Invalid command '%u', disconnecting client.", cmd);
			break;
		}

		// Check if the read message has the expected length.
		if (len != cmd_size) {
			IPC_ERROR(ics->server, "Invalid packet received, disconnecting client.");
			break;
		}

		IPC_TRACE_BEGIN(ipc_dispatch);
		xrt_result_t result = ipc_dispatch(ics, cmd_ptr);
		IPC_TRACE_END(ipc_dispatch);

		if (result != XRT_SUCCESS) {
			IPC_ERROR(ics->server, "During packet handling, disconnecting client.");
			break;
		}
	}

	// Call the client disconnected callback.
	ics->server->callbacks->client_disconnected( //
	    ics->server,                             //
	    ics->client_state.id,                    //
	    ics->server->callback_data);             //

	// Following code is same for all platforms.
	common_shutdown(ics);
}

#endif // XRT_OS_WINDOWS


/*
 *
 * 'Exported' functions.
 *
 */

void
ipc_server_client_destroy_session_and_compositor(volatile struct ipc_client_state *ics)
{
	// Multiple threads might be looking at these fields.
	os_mutex_lock(&ics->server->global_state.lock);

	ics->swapchain_count = 0;

	// Destroy all swapchains now.
	for (uint32_t j = 0; j < IPC_MAX_CLIENT_SWAPCHAINS; j++) {
		// Drop our reference, does NULL checking. Cast away volatile.
		xrt_swapchain_reference((struct xrt_swapchain **)&ics->xscs[j], NULL);
		ics->swapchain_data[j].active = false;
		IPC_TRACE(ics->server, "Destroyed swapchain %d.", j);
	}

	for (uint32_t j = 0; j < IPC_MAX_CLIENT_SEMAPHORES; j++) {
		// Drop our reference, does NULL checking. Cast away volatile.
		xrt_compositor_semaphore_reference((struct xrt_compositor_semaphore **)&ics->xcsems[j], NULL);
		IPC_TRACE(ics->server, "Destroyed compositor semaphore %d.", j);
	}

	for (uint32_t j = 0; j < IPC_MAX_CLIENT_FUTURES; j++) {
		// Drop our reference, does NULL checking. Cast away volatile.
		xrt_future_reference((struct xrt_future **)&ics->xfts[j], NULL);
		IPC_TRACE(ics->server, "Destroyed future %d.", j);
	}

	os_mutex_unlock(&ics->server->global_state.lock);

	// Cast away volatile.
	xrt_comp_destroy((struct xrt_compositor **)&ics->xc);

	// Cast away volatile.
	xrt_session_destroy((struct xrt_session **)&ics->xs);
}

void *
ipc_server_client_thread(void *_ics)
{
	volatile struct ipc_client_state *ics = (volatile struct ipc_client_state *)_ics;

	client_loop(ics);

	return NULL;
}
