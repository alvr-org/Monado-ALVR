// Copyright 2020-2021, Collabora, Ltd.
// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Implementations of handle functions
 *
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @ingroup aux_util
 *
 */

#include "u_handles.h"
#include "u_logging.h"


/*
 *
 * Helpers
 *
 */

#if defined(XRT_GRAPHICS_BUFFER_HANDLE_IS_FD) || defined(XRT_GRAPHICS_SYNC_HANDLE_IS_FD)
#include <errno.h>
#include <string.h>

static inline void
log_errno(int fd, const char *file, int line, const char *func, const char *thing)
{
	if (u_log_get_global_level() > U_LOGGING_ERROR) {
		return;
	}

	// Broken out for future improvements, like using strerrorname_np.
	const char *error_message = strerror(errno);

	u_log(                                 //
	    file, line, func, U_LOGGING_ERROR, //
	    "%s failed: %s (fd: %i) [%s:%d]",  //
	    thing, error_message,              //
	    fd, file, line);                   //
}

static inline int
fd_close(int fd, const char *file, int line, const char *func)
{
	int ret = close(fd);
	if (ret >= 0) {
		return ret;
	}

	log_errno(fd, file, line, func, "close");

	return -1;
}

static inline int
fd_dup(int fd, const char *file, int line, const char *func)
{
	int ret = dup(fd);
	if (ret >= 0) {
		return ret;
	}

	log_errno(fd, file, line, func, "dup");

	return -1;
}

#endif


/*
 *
 * Graphics Buffer Handles
 *
 */

#if defined(XRT_GRAPHICS_BUFFER_HANDLE_IS_AHARDWAREBUFFER)
#include <android/hardware_buffer.h>

static inline void
release_graphics_handle(xrt_graphics_buffer_handle_t handle)
{
	AHardwareBuffer_release(handle);
}

static inline xrt_graphics_buffer_handle_t
ref_graphics_handle(xrt_graphics_buffer_handle_t handle)
{
	AHardwareBuffer_acquire(handle);

	return handle;
}

#elif defined(XRT_GRAPHICS_BUFFER_HANDLE_IS_FD)
#include <unistd.h>

// To get the right function name and location.
#define release_graphics_handle(HANDLE) fd_close(HANDLE, __FILE__, __LINE__, __func__)
#define ref_graphics_handle(HANDLE) fd_dup(HANDLE, __FILE__, __LINE__, __func__)

#elif defined(XRT_GRAPHICS_BUFFER_HANDLE_IS_WIN32_HANDLE)

static inline void
release_graphics_handle(xrt_graphics_buffer_handle_t handle)
{
	CloseHandle(handle);
}

static inline xrt_graphics_buffer_handle_t
ref_graphics_handle(xrt_graphics_buffer_handle_t handle)
{
	HANDLE self = GetCurrentProcess();
	HANDLE result = NULL;
	if (DuplicateHandle(self, handle, self, &result, 0, FALSE, DUPLICATE_SAME_ACCESS) != 0) {
		return result;
	}
	return NULL;
}

#else
#error "need port"
#endif

xrt_graphics_buffer_handle_t
u_graphics_buffer_ref(xrt_graphics_buffer_handle_t handle)
{
	if (xrt_graphics_buffer_is_valid(handle)) {
		return ref_graphics_handle(handle);
	}

	return XRT_GRAPHICS_BUFFER_HANDLE_INVALID;
}

void
u_graphics_buffer_unref(xrt_graphics_buffer_handle_t *handle_ptr)
{
	if (handle_ptr == NULL) {
		return;
	}
	xrt_graphics_buffer_handle_t handle = *handle_ptr;
	if (!xrt_graphics_buffer_is_valid(handle)) {
		return;
	}
	release_graphics_handle(handle);
	*handle_ptr = XRT_GRAPHICS_BUFFER_HANDLE_INVALID;
}

/*
 *
 * Graphics Sync Handles
 *
 */

#if defined(XRT_GRAPHICS_SYNC_HANDLE_IS_FD)
#include <unistd.h>

// To get the right function name and location.
#define release_sync_handle(HANDLE) fd_close(HANDLE, __FILE__, __LINE__, __func__)
#define ref_sync_handle(HANDLE) fd_dup(HANDLE, __FILE__, __LINE__, __func__)

#elif defined(XRT_GRAPHICS_SYNC_HANDLE_IS_WIN32_HANDLE)

static inline void
release_sync_handle(xrt_graphics_sync_handle_t handle)
{
	CloseHandle(handle);
}

static inline xrt_graphics_sync_handle_t
ref_sync_handle(xrt_graphics_sync_handle_t handle)
{
	HANDLE self = GetCurrentProcess();
	HANDLE result = NULL;
	if (DuplicateHandle(self, handle, self, &result, 0, FALSE, DUPLICATE_SAME_ACCESS) != 0) {
		return result;
	}
	return NULL;
}

#else
#error "need port"
#endif

xrt_graphics_sync_handle_t
u_graphics_sync_ref(xrt_graphics_sync_handle_t handle)
{
	if (xrt_graphics_sync_handle_is_valid(handle)) {
		return ref_sync_handle(handle);
	}

	return XRT_GRAPHICS_SYNC_HANDLE_INVALID;
}

void
u_graphics_sync_unref(xrt_graphics_sync_handle_t *handle_ptr)
{
	if (handle_ptr == NULL) {
		return;
	}
	xrt_graphics_sync_handle_t handle = *handle_ptr;
	if (!xrt_graphics_sync_handle_is_valid(handle)) {
		return;
	}
	release_sync_handle(handle);
	*handle_ptr = XRT_GRAPHICS_SYNC_HANDLE_INVALID;
}
