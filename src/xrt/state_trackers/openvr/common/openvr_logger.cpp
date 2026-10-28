// Copyright 2018-2023, Collabora, Ltd.
// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Logging functions.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_common
 */

#include "xrt/xrt_compiler.h"

#include "util/u_misc.h"
#include "util/u_debug.h"
#include "util/u_truncate_printf.h"

#include "openvr_logger.hpp"

#if __has_include(<cxxabi.h>)
#include <cxxabi.h>
#include <cstdlib>
#include <string>
#define HAS_CXA_DEMANGLE 1
#else
#define HAS_CXA_DEMANGLE 0
#endif


namespace xrt::state_trackers::openvr {

#define LOG_BUFFER_SIZE (4096)

#ifdef XRT_OS_WINDOWS
#define DEFAULT_NO_STDERR (true)
#define CHECK_SHOULD_NOT_PRINT (debug_get_bool_option_no_printing())
#else
#define DEFAULT_NO_STDERR (false)
#define CHECK_SHOULD_NOT_PRINT (debug_get_bool_option_no_printing() || debug_get_bool_option_no_printing_stderr())
#endif

DEBUG_GET_ONCE_BOOL_OPTION(no_printing, "OPENVR_NO_PRINTING", false)
DEBUG_GET_ONCE_BOOL_OPTION(no_printing_stderr, "OPENVR_NO_PRINTING_STDERR", DEFAULT_NO_STDERR)
DEBUG_GET_ONCE_BOOL_OPTION(entrypoints, "OPENVR_DEBUG_ENTRYPOINTS", false)
DEBUG_GET_ONCE_LOG_OPTION(log_level, "OPENVR_LOG_LEVEL", U_LOGGING_INFO)

static const char *
get_display_class_name(const char *class_name)
{
	if (class_name == nullptr) {
		return "";
	}

#if HAS_CXA_DEMANGLE
	int status = 0;
	size_t demangled_length = 0;
	char *demangled = abi::__cxa_demangle(class_name, nullptr, &demangled_length, &status);
	if (demangled == nullptr || status != 0) {
		return class_name;
	}

	thread_local std::string demangled_storage;
	demangled_storage = demangled;
	std::free(demangled);

	// Get just the trailing class name
	auto pos = demangled_storage.find_last_of("::");
	if (pos != std::string::npos) {
		demangled_storage = demangled_storage.substr(pos + 1);
	}

	return demangled_storage.c_str();
#else
	return class_name;
#endif
}

static void
do_output(const char *buf)
{
#if defined(XRT_OS_WINDOWS)
	OutputDebugStringA(buf);

	if (debug_get_bool_option_no_printing_stderr()) {
		return;
	}
#elif defined(XRT_OS_ANDROID)
	__android_log_write(ANDROID_LOG_DEBUG, "OXR", buf);
	if (debug_get_bool_option_no_printing_stderr()) {
		return;
	}
#endif

	fprintf(stderr, "%s", buf);
}

static void
do_print_func(struct openvr_logger &logger)
{
	const char *display_class_name = get_display_class_name(logger.class_name);
	char buf[LOG_BUFFER_SIZE];
	u_truncate_snprintf(buf, sizeof(buf), "%s::%s\n", display_class_name, logger.api_func_name);
	do_output(buf);
}

static void
do_print(struct openvr_logger &logger, const char *fmt, const char *prefix, va_list args)
{
	const char *display_class_name = get_display_class_name(logger.class_name);
	char buf[LOG_BUFFER_SIZE];

	int remaining = sizeof(buf) - 2; // 2 for \n\0
	int printed = 0;
	int ret;

	ret = u_truncate_snprintf(buf, remaining, "%s: %s::%s: ", prefix, display_class_name, logger.api_func_name);
	if (ret < 0) {
		U_LOG_E("Internal OpenVR logging error!");
		return;
	}
	printed += ret;

	ret = u_truncate_vsnprintf(buf + printed, remaining - printed, fmt, args);
	if (ret < 0) {
		U_LOG_E("Internal OpenVR logging error!");
		return;
	}
	printed += ret;

	// Always add newline.
	buf[printed++] = '\n';
	buf[printed++] = '\0';

	do_output(buf);
}

void
openvrLogInit(struct openvr_logger &logger, const char *class_name, const char *api_func_name)
{
	logger.class_name = class_name;
	logger.api_func_name = api_func_name;
	logger.log_level = debug_get_log_option_log_level();

	if (debug_get_bool_option_entrypoints()) {
		do_print_func(logger);
	}
}

void
openvrLog(struct openvr_logger &logger, u_logging_level log_level, const char *fmt, ...)
{
	if (CHECK_SHOULD_NOT_PRINT) {
		return;
	}

	if (log_level < logger.log_level) {
		return;
	}

	const char *prefix = NULL;
	switch (log_level) {
	case U_LOGGING_TRACE: prefix = "[TRACE]"; break;
	case U_LOGGING_DEBUG: prefix = "[DEBUG]"; break;
	case U_LOGGING_INFO: prefix = "[INFO]"; break;
	case U_LOGGING_WARN: prefix = "[WARN]"; break;
	case U_LOGGING_ERROR: prefix = "[ERROR]"; break;
	case U_LOGGING_RAW: prefix = ""; break;
	default:
		assert(prefix);
		prefix = "";
		break;
	}

	va_list args;
	va_start(args, fmt);
	do_print(logger, fmt, prefix, args);
	va_end(args);
}

}; // namespace xrt::state_trackers::openvr
