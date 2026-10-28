// Copyright 2022, Magic Leap, Inc.
// Copyright 2020-2023, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  IPC util helpers.
 * @author Julian Petrov <jpetrov@magicleap.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup ipc_shared
 */

#include "ipc_utils.h"

#ifdef XRT_OS_WINDOWS
#include "util/u_windows.h"
#endif


/*
 *
 * Misc utils 'exported' functions.
 *
 */

#ifdef XRT_OS_WINDOWS
const char *
ipc_winerror(DWORD err)
{
	static char s_buf[4096]; // N.B. Not thread-safe. If needed, use a thread var
	return u_winerror(s_buf, sizeof(s_buf), err, false);
}
#else
void
_a_function_to_not_create_an_empty_compile_unit(void)
{
	// To avoid warnings.
}
#endif
