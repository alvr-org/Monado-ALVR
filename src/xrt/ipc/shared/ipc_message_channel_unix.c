// Copyright 2020-2023, Collabora, Ltd.
// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  IPC message channel functions for UNIX platforms.
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @author Pete Black <pblack@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup ipc_shared
 */

#include "xrt/xrt_config_os.h"

#ifdef XRT_OS_WINDOWS
#error "This file shouldn't be compiled on Windows!"
#endif

#include "util/u_logging.h"
#include "util/u_pretty_print.h"

#include "shared/ipc_protocol.h"
#include "shared/ipc_message_channel.h"

#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>


/*
 *
 * Logging
 *
 */

#define IPC_TRACE(d, ...) U_LOG_IFL_T(d->log_level, __VA_ARGS__)
#define IPC_DEBUG(d, ...) U_LOG_IFL_D(d->log_level, __VA_ARGS__)
#define IPC_INFO(d, ...) U_LOG_IFL_I(d->log_level, __VA_ARGS__)
#define IPC_WARN(d, ...) U_LOG_IFL_W(d->log_level, __VA_ARGS__)
#define IPC_ERROR(d, ...) U_LOG_IFL_E(d->log_level, __VA_ARGS__)


/*
 *
 * Structs and defines.
 *
 */

union imcontrol_buf {
	uint8_t buf[512];
	struct cmsghdr align;
};


/*
 *
 * Helpers.
 *
 */

static void
log_fd_io_failure(struct ipc_message_channel *imc,
                  const char *call,
                  ssize_t transferred,
                  int err,
                  size_t size,
                  uint32_t handle_count,
                  const int *fds,
                  uint32_t fd_count)
{
	struct u_pp_sink_stack_only sink;
	u_pp_delegate_t dg = u_pp_sink_stack_only_init(&sink);

	// Return value and strerror stay on the first line. Every other field is indented.
	if (transferred < 0) {
		u_pp(dg, "%s(%i) failed: error: '%i' '%s'!", call, imc->ipc_handle, err, strerror(err));
	} else {
		u_pp(dg, "%s(%i) failed: wrong size '%zd', expected '%zu'!", call, imc->ipc_handle, transferred, size);
	}

	u_pp(dg, "\n\tcount: %u", handle_count);
	u_pp(dg, "\n\tsize: %zu", size);
	u_pp(dg, "\n\ttransferred: %zd", transferred);

	for (uint32_t i = 0; i < fd_count; i++) {
		u_pp(dg, "\n\t\tfd #%u: %i", i, fds[i]);
	}

	IPC_ERROR(imc, "%s", sink.buffer);
}

static void
close_received_scm_rights(const struct msghdr *msg)
{
	struct cmsghdr *cmsg = CMSG_FIRSTHDR(msg);
	if (cmsg == NULL || cmsg->cmsg_level != SOL_SOCKET || cmsg->cmsg_type != SCM_RIGHTS) {
		return;
	}

	if (cmsg->cmsg_len < CMSG_LEN(sizeof(int))) {
		return;
	}

	const int *fds = (const int *)CMSG_DATA(cmsg);
	size_t fd_count = (cmsg->cmsg_len - CMSG_LEN(0)) / sizeof(int);

	for (size_t i = 0; i < fd_count; i++) {
		close(fds[i]);
	}
}


/*
 *
 * 'Exported' functions.
 *
 */

void
ipc_message_channel_close(struct ipc_message_channel *imc)
{
	if (imc->ipc_handle < 0) {
		return;
	}
	close(imc->ipc_handle);
	imc->ipc_handle = -1;
}

xrt_result_t
ipc_send(struct ipc_message_channel *imc, const void *data, size_t size)
{
	const uint8_t *buf = data;
	size_t sent = 0;

	// SOCK_STREAM sendmsg may return a short count. Send the rest of the buffer.
	while (sent < size) {
		struct iovec iov = {
		    .iov_base = (void *)(buf + sent),
		    .iov_len = size - sent,
		};

		struct msghdr msg = {
		    .msg_name = NULL,
		    .msg_namelen = 0,
		    .msg_iov = &iov,
		    .msg_iovlen = 1,
		    .msg_flags = 0,
		};

		ssize_t ret = sendmsg(imc->ipc_handle, &msg, MSG_NOSIGNAL);
		if (ret < 0) {
			int code = errno;
			if (code == EINTR) {
				continue;
			}

			IPC_ERROR(imc, "sendmsg(%i) failed at %zu of %zu: '%i' '%s'!", imc->ipc_handle, sent, size,
			          code, strerror(code));
			return XRT_ERROR_IPC_FAILURE;
		}

		// Defensive: blocking AF_UNIX stream send does not return 0 for non-zero iov_len.
		if (ret == 0) {
			IPC_ERROR(imc, "sendmsg(%i) failed at %zu of %zu: sent 0!", imc->ipc_handle, sent, size);
			return XRT_ERROR_IPC_FAILURE;
		}

		sent += (size_t)ret;
	}

	return XRT_SUCCESS;
}

xrt_result_t
ipc_receive(struct ipc_message_channel *imc, void *out_data, size_t size)
{
	uint8_t *buf = out_data;
	size_t got = 0;

	// SOCK_STREAM recvmsg may return a short count. Read until size bytes arrive.
	while (got < size) {
		struct iovec iov = {
		    .iov_base = buf + got,
		    .iov_len = size - got,
		};

		struct msghdr msg = {
		    .msg_name = NULL,
		    .msg_namelen = 0,
		    .msg_iov = &iov,
		    .msg_iovlen = 1,
		    .msg_flags = 0,
		};

		ssize_t len = recvmsg(imc->ipc_handle, &msg, MSG_NOSIGNAL);
		if (len < 0) {
			int code = errno;
			if (code == EINTR) {
				continue;
			}

			IPC_ERROR(imc, "recvmsg(%i) failed at %zu of %zu: '%i' '%s'!", (int)imc->ipc_handle, got, size,
			          code, strerror(code));
			return XRT_ERROR_IPC_FAILURE;
		}

		if (len == 0) {
			IPC_ERROR(imc, "recvmsg(%i) failed: peer closed connection at %zu of %zu!",
			          (int)imc->ipc_handle, got, size);
			return XRT_ERROR_IPC_FAILURE;
		}

		// Defensive: AF_UNIX stream recvmsg does not return more than iov_len.
		assert((size_t)len <= size - got);

		got += (size_t)len;
	}

	return XRT_SUCCESS;
}

xrt_result_t
ipc_receive_fds(struct ipc_message_channel *imc, void *out_data, size_t size, int *out_handles, uint32_t handle_count)
{
	assert(imc != NULL);
	assert(out_data != NULL);
	assert(size != 0);
	assert(out_handles != NULL);
	assert(handle_count != 0);

	/*
	 * Unusual size sanity check. IPC_BUF_SIZE bounds the fixed command buffer
	 * only. On Linux, recvmsg stops after the first stream chunk that carries
	 * SCM_RIGHTS, so fd messages much larger than one socket chunk fail the
	 * exact-size read below.
	 */
	if (size > IPC_BUF_SIZE) {
		IPC_WARN(imc, "fd message size %u is larger than IPC_BUF_SIZE (%u)", (uint32_t)size,
		         (uint32_t)IPC_BUF_SIZE);
	}

	union imcontrol_buf u;
	const size_t fds_size = sizeof(int) * handle_count;
	const size_t cmsg_size = CMSG_SPACE(fds_size);
	memset(u.buf, 0, cmsg_size);

	struct iovec iov = {
	    .iov_base = out_data,
	    .iov_len = size,
	};

	struct msghdr msg = {
	    .msg_name = NULL,
	    .msg_namelen = 0,
	    .msg_iov = &iov,
	    .msg_iovlen = 1,
	    .msg_flags = 0,
	    .msg_control = u.buf,
	    .msg_controllen = cmsg_size,
	};

	// EINTR means this recvmsg moved nothing. A short count can already hold SCM_RIGHTS, so do not retry it.
	ssize_t len;
	do {
		len = recvmsg(imc->ipc_handle, &msg, MSG_NOSIGNAL);
	} while (len < 0 && errno == EINTR);

	if (len != (ssize_t)size) {
		int err = errno;

		// recvmsg may install SCM_RIGHTS before returning a short count.
		if (len > 0) {
			close_received_scm_rights(&msg);
		}

		log_fd_io_failure(imc, "recvmsg", len, err, size, handle_count, NULL, 0);
		return XRT_ERROR_IPC_FAILURE;
	}

	// Did the other side actually send file descriptors.
	struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
	if (cmsg == NULL) {
		return XRT_SUCCESS;
	}

	memcpy(out_handles, (int *)CMSG_DATA(cmsg), fds_size);

	return XRT_SUCCESS;
}

xrt_result_t
ipc_send_fds(struct ipc_message_channel *imc, const void *data, size_t size, const int *handles, uint32_t handle_count)
{
	assert(imc != NULL);
	assert(data != NULL);
	assert(size != 0);
	assert(handles != NULL);

	/*
	 * Unusual size sanity check. IPC_BUF_SIZE bounds the fixed command buffer
	 * only. On Linux, recvmsg stops after the first stream chunk that carries
	 * SCM_RIGHTS, so fd messages much larger than one socket chunk fail the
	 * exact-size read on the peer.
	 */
	if (size > IPC_BUF_SIZE) {
		IPC_WARN(imc, "fd message size %u is larger than IPC_BUF_SIZE (%u)", (uint32_t)size,
		         (uint32_t)IPC_BUF_SIZE);
	}

	const size_t fds_size = sizeof(int) * handle_count;

	union imcontrol_buf u = {0};
	size_t cmsg_size = CMSG_SPACE(fds_size);

	struct iovec iov = {
	    .iov_base = (void *)data,
	    .iov_len = size,
	};

	struct msghdr msg = {
	    .msg_name = NULL,
	    .msg_namelen = 0,
	    .msg_iov = &iov,
	    .msg_iovlen = 1,
	    .msg_control = u.buf,
	    .msg_controllen = cmsg_size,
	    .msg_flags = 0,
	};

	struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	cmsg->cmsg_len = CMSG_LEN(fds_size);

	memcpy(CMSG_DATA(cmsg), handles, fds_size);

	// EINTR means this sendmsg moved nothing. A short count can already hold SCM_RIGHTS, so do not retry it.
	ssize_t ret;
	do {
		ret = sendmsg(imc->ipc_handle, &msg, MSG_NOSIGNAL);
	} while (ret < 0 && errno == EINTR);

	if (ret == (ssize_t)size) {
		return XRT_SUCCESS;
	}

	int err = errno;
	log_fd_io_failure(imc, "sendmsg", ret, err, size, handle_count, handles, handle_count);

	return XRT_ERROR_IPC_FAILURE;
}

xrt_result_t
ipc_receive_handles_shmem(struct ipc_message_channel *imc,
                          void *out_data,
                          size_t size,
                          xrt_shmem_handle_t *out_handles,
                          uint32_t handle_count)
{
	return ipc_receive_fds(imc, out_data, size, out_handles, handle_count);
}

xrt_result_t
ipc_send_handles_shmem(struct ipc_message_channel *imc,
                       const void *data,
                       size_t size,
                       const xrt_shmem_handle_t *handles,
                       uint32_t handle_count)
{
	return ipc_send_fds(imc, data, size, handles, handle_count);
}


/*
 *
 * AHardwareBuffer graphics buffer functions.
 *
 */

#if defined(XRT_GRAPHICS_BUFFER_HANDLE_IS_AHARDWAREBUFFER)

#include <android/hardware_buffer.h>


xrt_result_t
ipc_receive_handles_graphics_buffer(struct ipc_message_channel *imc,
                                    void *out_data,
                                    size_t size,
                                    xrt_graphics_buffer_handle_t *out_handles,
                                    uint32_t handle_count)
{
	xrt_result_t result = ipc_receive(imc, out_data, size);
	if (result != XRT_SUCCESS) {
		return result;
	}
	bool failed = false;
	for (uint32_t i = 0; i < handle_count; ++i) {
		int err = AHardwareBuffer_recvHandleFromUnixSocket(imc->ipc_handle, &(out_handles[i]));
		if (err != 0) {
			failed = true;
		}
	}
	return failed ? XRT_ERROR_IPC_FAILURE : XRT_SUCCESS;
}


xrt_result_t
ipc_send_handles_graphics_buffer(struct ipc_message_channel *imc,
                                 const void *data,
                                 size_t size,
                                 const xrt_graphics_buffer_handle_t *handles,
                                 uint32_t handle_count)
{
	xrt_result_t result = ipc_send(imc, data, size);
	if (result != XRT_SUCCESS) {
		return result;
	}
	bool failed = false;
	for (uint32_t i = 0; i < handle_count; ++i) {
		int err = AHardwareBuffer_sendHandleToUnixSocket(handles[i], imc->ipc_handle);
		if (err != 0) {
			failed = true;
		}
	}
	return failed ? XRT_ERROR_IPC_FAILURE : XRT_SUCCESS;
}


/*
 *
 * FD graphics buffer functions.
 *
 */

#elif defined(XRT_GRAPHICS_BUFFER_HANDLE_IS_FD)

xrt_result_t
ipc_receive_handles_graphics_buffer(struct ipc_message_channel *imc,
                                    void *out_data,
                                    size_t size,
                                    xrt_graphics_buffer_handle_t *out_handles,
                                    uint32_t handle_count)
{
	return ipc_receive_fds(imc, out_data, size, out_handles, handle_count);
}

xrt_result_t
ipc_send_handles_graphics_buffer(struct ipc_message_channel *imc,
                                 const void *data,
                                 size_t size,
                                 const xrt_graphics_buffer_handle_t *handles,
                                 uint32_t handle_count)
{
	return ipc_send_fds(imc, data, size, handles, handle_count);
}

#else
#error "Need port to transport these graphics buffers"
#endif


/*
 *
 * FD graphics sync functions.
 *
 */

#if defined(XRT_GRAPHICS_SYNC_HANDLE_IS_FD)

xrt_result_t
ipc_receive_handles_graphics_sync(struct ipc_message_channel *imc,
                                  void *out_data,
                                  size_t size,
                                  xrt_graphics_sync_handle_t *out_handles,
                                  uint32_t handle_count)
{
	//! @todo Temporary hack to send no handles.
	if (handle_count == 0) {
		return ipc_receive(imc, out_data, size);
	}
	return ipc_receive_fds(imc, out_data, size, out_handles, handle_count);
}

xrt_result_t
ipc_send_handles_graphics_sync(struct ipc_message_channel *imc,
                               const void *data,
                               size_t size,
                               const xrt_graphics_sync_handle_t *handles,
                               uint32_t handle_count)
{
	//! @todo Temporary hack to send no handles.
	if (handle_count == 0) {
		return ipc_send(imc, data, size);
	}
	return ipc_send_fds(imc, data, size, handles, handle_count);
}

#else
#error "Need port to transport these graphics buffers"
#endif
