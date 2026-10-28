// Copyright 2025, rcelyte
// SPDX-License-Identifier: BSL-1.0

#include "solarxr_ipc_socket.h"
#include "solarxr_ipc_message.h"

#include "os/os_time.h"
#include "shared/ipc_message_channel.h"
#include "util/u_file.h"

#include <endian.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define SLIMEVR_IDENTIFIER "dev.slimevr.SlimeVR"

static bool
solarxr_ipc_socket_ensure_capacity(struct solarxr_ipc_socket *const state, const uint32_t capacity)
{
	if (state->buffer_cap >= capacity) {
		return true;
	}

	if (capacity > 0x100000u) {
		U_LOG_IFL_E(state->log_level, "packet too large");
		return false;
	}

	uint8_t *const new_buffer = realloc(state->buffer, capacity);
	if (new_buffer == NULL) {
		U_LOG_IFL_E(state->log_level, "realloc failed");
		return false;
	}

	state->buffer = new_buffer;
	state->buffer_cap = capacity;
	return true;
}

void
solarxr_ipc_socket_init(struct solarxr_ipc_socket *const state, const enum u_logging_level log_level)
{
	*state = (struct solarxr_ipc_socket){
	    .ipc_handle = XRT_IPC_HANDLE_INVALID,
	    .log_level = log_level,
	    .timestamp = os_monotonic_get_ns(),
	};
}

static bool
solarxr_ipc_socket_close(struct solarxr_ipc_socket *const state)
{
	struct ipc_message_channel channel = {
	    .ipc_handle = atomic_exchange(&state->ipc_handle, XRT_IPC_HANDLE_INVALID),
	    .log_level = state->log_level,
	};
	if (channel.ipc_handle == XRT_IPC_HANDLE_INVALID) {
		return false;
	}

	shutdown(channel.ipc_handle, SHUT_RDWR); // unblock `solarxr_ipc_socket_wait_timeout()`
	static_assert(sizeof(state->reference.count) == sizeof(volatile _Atomic(int)), "");

	while (atomic_load((volatile _Atomic(int) *)&state->reference.count) != 0) {
		sched_yield();
	}

	ipc_message_channel_close(&channel);
	return true;
}

static void
solarxr_ipc_socket_free_buffer(struct solarxr_ipc_socket *const state)
{
	free(state->buffer);
	state->buffer = NULL;
	state->buffer_cap = 0;
}

void
solarxr_ipc_socket_destroy(struct solarxr_ipc_socket *const state)
{
	// guards against double-free
	if (solarxr_ipc_socket_close(state)) {
		solarxr_ipc_socket_free_buffer(state);
	}
}

static bool
check_socket_path(const enum u_logging_level log_level, const char path[const])
{
	struct stat result = {0};
	const bool found = stat(path, &result) == 0 && S_ISSOCK(result.st_mode);
	if (!found) {
		U_LOG_IFL_W(log_level, "path not found: %s", path);
	}
	return found;
}

struct sockaddr_un
solarxr_ipc_socket_find(const enum u_logging_level log_level, const char filename[const])
{
	struct sockaddr_un addr = {
	    .sun_family = AF_UNIX,
	};
	const size_t filename_len = strlen(filename);
	if (filename_len >= sizeof(addr.sun_path)) {
		U_LOG_IFL_E(log_level, "filename too long");
		return (struct sockaddr_un){0};
	}

	const ssize_t runtime_dir_len =
	    u_file_get_path_in_runtime_dir("", addr.sun_path, sizeof(addr.sun_path) - 1 - filename_len);
	if (runtime_dir_len <= 0 || (size_t)runtime_dir_len >= sizeof(addr.sun_path) - 1 - filename_len) {
		U_LOG_IFL_E(log_level, "u_file_get_path_in_runtime_dir() failed");
		return (struct sockaddr_un){0};
	}

	// try path in runtime dir
	memcpy(&addr.sun_path[runtime_dir_len], filename, filename_len + 1); // include null terminator
	if (check_socket_path(log_level, addr.sun_path)) {
		return addr;
	}

	// try path used by SlimeVR flatpak
	static const char flatpak_runtime_dir[] = ".flatpak/" SLIMEVR_IDENTIFIER "/xdg-run/";
	if (sizeof(addr.sun_path) - 1 - filename_len - runtime_dir_len >= sizeof(flatpak_runtime_dir) - 1) {
		memcpy(&addr.sun_path[runtime_dir_len], flatpak_runtime_dir, sizeof(flatpak_runtime_dir) - 1);
		memcpy(&addr.sun_path[runtime_dir_len + (sizeof(flatpak_runtime_dir) - 1)], filename, filename_len + 1);
		if (check_socket_path(log_level, addr.sun_path)) {
			return addr;
		}
	}

	// try fallback path in SlimeVR data dir
	const char *env;
	int path_len = 0;
	if ((env = getenv("XDG_DATA_HOME")) != NULL) {
		path_len =
		    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s/" SLIMEVR_IDENTIFIER "/%s", env, filename);
	} else if ((env = getenv("HOME")) != NULL) {
		path_len = snprintf(addr.sun_path, sizeof(addr.sun_path), "%s/.local/share/" SLIMEVR_IDENTIFIER "/%s",
		                    env, filename);
	}

	if (path_len <= 0 || (unsigned)path_len >= sizeof(addr.sun_path) ||
	    !check_socket_path(log_level, addr.sun_path)) {
		U_LOG_IFL_E(log_level, "failed to resolve socket path");
		return (struct sockaddr_un){0};
	}

	return addr;
}

bool
solarxr_ipc_socket_connect(struct solarxr_ipc_socket *const state, const struct sockaddr_un addr)
{
	solarxr_ipc_socket_close(state);

	xrt_ipc_handle_t ipc_handle = XRT_IPC_HANDLE_INVALID;
	if (addr.sun_family != AF_UNIX) {
		goto fail;
	}

#ifdef SOCK_CLOEXEC
	const int flags = SOCK_CLOEXEC;
#else
	const int flags = 0;
#endif
	ipc_handle = socket(PF_UNIX, SOCK_STREAM | flags, 0);
	if (ipc_handle == XRT_IPC_HANDLE_INVALID) {
		U_LOG_IFL_E(state->log_level, "socket() failed");
		goto fail;
	}

	U_LOG_IFL_I(state->log_level, "Connecting to domain socket: %s", addr.sun_path);
	if (connect(ipc_handle, (const struct sockaddr *)&addr, sizeof(addr)) != 0) {
		U_LOG_IFL_E(state->log_level, "connect() failed: %s", strerror(errno));
		goto fail;
	}

	// optimistic allocation to avoid needless reallocs during receive
	solarxr_ipc_socket_ensure_capacity(state, 0x1000);

	atomic_store(&state->ipc_handle, ipc_handle);
	return true;

fail:
	xrt_ipc_handle_close(ipc_handle);

	// The IPC handle is used as a lock guard in `solarxr_ipc_socket_destroy()`, so this is the last place to safely
	// free the buffer without one
	solarxr_ipc_socket_free_buffer(state);

	return false;
}

// release reference with `xrt_reference_dec(&state->reference);`
static bool
solarxr_ipc_socket_reference_channel(struct solarxr_ipc_socket *const state,
                                     struct ipc_message_channel *const channel_out)
{
	xrt_reference_inc(&state->reference);

	const xrt_ipc_handle_t ipc_handle = atomic_load(&state->ipc_handle);
	if (ipc_handle == XRT_IPC_HANDLE_INVALID) {
		xrt_reference_dec(&state->reference);
		return false;
	}

	*channel_out = (struct ipc_message_channel){
	    .ipc_handle = ipc_handle,
	    .log_level = state->log_level,
	};
	return true;
}

bool
solarxr_ipc_socket_wait_timeout(struct solarxr_ipc_socket *const state, const time_duration_ns timeout)
{
	struct ipc_message_channel channel;
	if (!solarxr_ipc_socket_reference_channel(state, &channel)) {
		return false;
	}

	struct pollfd fd = {
	    channel.ipc_handle,
	    POLLIN,
	    0,
	};
	const struct timespec timeout_ts = {
	    .tv_sec = timeout / U_TIME_1S_IN_NS,
	    .tv_nsec = timeout % U_TIME_1S_IN_NS,
	};
	const bool result = (ppoll(&fd, 1, (timeout >= 0) ? &timeout_ts : NULL, NULL) != -1)
	                        ? (fd.revents & POLLERR) == 0
	                        : errno == EINTR;

	xrt_reference_dec(&state->reference);
	return result;
}

bool
solarxr_ipc_socket_send_raw(struct solarxr_ipc_socket *const state,
                            const uint8_t packet[const],
                            const uint32_t packet_len)
{
	struct ipc_message_channel channel;
	if (!solarxr_ipc_socket_reference_channel(state, &channel)) {
		return false;
	}

	const xrt_result_t result = ipc_send(&channel, packet, packet_len);

	xrt_reference_dec(&state->reference);
	return result == XRT_SUCCESS;
}

static bool
recv_nonblock(const struct ipc_message_channel channel,
              uint8_t buffer[const],
              uint32_t *const head,
              const uint32_t buffer_cap)
{
	// TODO: use a platform agnostic function like `ipc_receive()`, but with support for partial reads
	const ssize_t length = recv(channel.ipc_handle, &buffer[*head], buffer_cap - *head, MSG_DONTWAIT);
	if (length < 0) {
		if (errno == EAGAIN) {
			return true;
		}
		U_LOG_IFL_E(channel.log_level, "recv() failed: %s", strerror(errno));
		return false;
	}

	if ((size_t)length > buffer_cap - *head) {
		U_LOG_IFL_E(channel.log_level, "recv() returned invalid length");
		return false;
	}

	*head += (size_t)length;
	return true;
}

uint32_t
solarxr_ipc_socket_receive(struct solarxr_ipc_socket *const state)
{
	struct ipc_message_channel channel;
	if (!solarxr_ipc_socket_reference_channel(state, &channel)) {
		return 0;
	}

	if (state->head == state->buffer_len) {
		state->buffer_len = 0;
		state->head = 0;
	}

	if (state->buffer_len == 0) {
		struct solarxr_ipc_message header;
		if (!solarxr_ipc_socket_ensure_capacity(state, sizeof(header)) ||
		    !recv_nonblock(channel, state->buffer, &state->head, sizeof(header))) {
			goto fail;
		}

		if (state->head < sizeof(header)) {
			goto unref;
		}

		memcpy(&header, state->buffer, sizeof(header));

		const uint32_t packet_length = le32toh(header.length) - sizeof(header);
		if (!solarxr_ipc_socket_ensure_capacity(state, packet_length)) {
			goto fail;
		}

		state->buffer_len = packet_length;
		state->head = 0;
		state->timestamp = os_monotonic_get_ns();
	}

	if (!recv_nonblock(channel, state->buffer, &state->head, state->buffer_len)) {
		goto fail;
	}

unref:
	xrt_reference_dec(&state->reference);
	return (state->head < state->buffer_len) ? 0 : state->buffer_len;

fail:
	xrt_reference_dec(&state->reference);
	solarxr_ipc_socket_destroy(state);
	return 0;
}
