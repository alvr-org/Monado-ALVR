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

#include "util/u_logging.h"
#include "util/u_pretty_print.h"

#ifdef XRT_OS_WINDOWS
#include "util/u_windows.h"
#endif


/*
 *
 * Misc utils 'exported' functions.
 *
 */

void
ipc_print_result(enum u_logging_level cond_level,
                 const char *file,
                 int line,
                 const char *calling_fn,
                 xrt_result_t xret,
                 const char *called_fn)
{
	bool success = xret == XRT_SUCCESS;
	enum u_logging_level level = success ? U_LOGGING_INFO : U_LOGGING_ERROR;

	// Should we be logging?
	if (level < cond_level) {
		return;
	}

	struct u_pp_sink_stack_only sink;
	u_pp_delegate_t dg = u_pp_sink_stack_only_init(&sink);

	if (success) {
		u_pp(dg, "%s: ", called_fn);
	} else {
		u_pp(dg, "%s failed: ", called_fn);
	}

	u_pp_xrt_result(dg, xret);
	u_pp(dg, " [%s:%i]", file, line);

	u_log(file, line, calling_fn, level, "%s", sink.buffer);
}

#ifdef XRT_OS_WINDOWS
const char *
ipc_winerror(DWORD err)
{
	static char s_buf[4096]; // N.B. Not thread-safe. If needed, use a thread var
	return u_winerror(s_buf, sizeof(s_buf), err, false);
}
#endif
