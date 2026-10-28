// Copyright 2025, rcelyte
// SPDX-License-Identifier: BSL-1.0

#pragma once
#include "util/u_logging.h"
#include "util/u_time.h"
#include "xrt/xrt_handles.h"

#include <stdatomic.h>
#ifdef XRT_OS_WINDOWS
#include <afunix.h>
#else
#include <sys/un.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct solarxr_ipc_socket
{
	_Atomic(xrt_ipc_handle_t) ipc_handle;
	struct xrt_reference reference;
	enum u_logging_level log_level;
	timepoint_ns timestamp;
	uint32_t head, buffer_len, buffer_cap;
	uint8_t *buffer;
};

void
solarxr_ipc_socket_init(struct solarxr_ipc_socket *state, enum u_logging_level log_level);

void
solarxr_ipc_socket_destroy(struct solarxr_ipc_socket *state); // thread safe

struct sockaddr_un
solarxr_ipc_socket_find(enum u_logging_level log_level, const char filename[]);

bool
solarxr_ipc_socket_connect(struct solarxr_ipc_socket *state, struct sockaddr_un addr);

bool
solarxr_ipc_socket_wait_timeout(struct solarxr_ipc_socket *state, time_duration_ns timeout); // thread safe

bool
solarxr_ipc_socket_send_raw(struct solarxr_ipc_socket *state,
                            const uint8_t packet[],
                            uint32_t packet_len); // thread safe

uint32_t
solarxr_ipc_socket_receive(struct solarxr_ipc_socket *state);

static inline bool
solarxr_ipc_socket_is_connected(struct solarxr_ipc_socket *const state)
{
	return atomic_load(&state->ipc_handle) != XRT_IPC_HANDLE_INVALID;
}

#ifdef __cplusplus
}
#endif
