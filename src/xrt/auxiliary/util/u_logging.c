// Copyright 2019-2025, Collabora, Ltd.
// Copyright 2024-2025, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Logging functions.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_util
 */

#include "util/u_logging.h"
#include "xrt/xrt_config_os.h"
#include "xrt/xrt_config_build.h"

#include "util/u_debug.h"
#include "util/u_pretty_print.h"
#include "u_json.h"
#include "util/u_truncate_printf.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>


/*
 *
 * Defines.
 *
 */

/*
 * Try to make the buffer size as small as possible, but not too small,
 * this stays within the "fast" stack variables limits on x64 bit builds on
 * Windows, but not on 32 bit builds.
 *
 * See https://learn.microsoft.com/en-us/windows/win32/devnotes/-win32-chkstk
 */
#define LOG_BUFFER_SIZE (7 * 1024)

/*
 * 16MB max binary data
 */
#define LOG_MAX_HEX_DUMP (0x00ffffff)

#define LOG_MAX_HEX_DUMP_HUMAN_READABLE "16MB"

/*
 * Hex dumps put 16 bytes per line
 */
#define LOG_HEX_BYTES_PER_LINE (16)

/*
 * This is enough space for the line's bytes to be in both hex and ascii
 */
#define LOG_HEX_LINE_BUF_SIZE (128)


#ifndef LOG_ANDROID_TAG_PREFIX
#define LOG_ANDROID_TAG_PREFIX "monado"
#endif

/*
 *
 * Global log level functions.
 *
 */

DEBUG_GET_ONCE_LOG_OPTION(global_log, "XRT_LOG", U_LOGGING_WARN)
DEBUG_GET_ONCE_BOOL_OPTION(json_log, "XRT_JSON_LOG", false)

enum u_logging_level
u_log_get_global_level(void)
{
	return debug_get_log_option_global_log();
}


/*
 *
 * Logging file code.
 *
 */

static FILE *g_log_file = NULL;

void
u_log_set_output_file(const char *filename)
{
	// While not thread safe, this function is externally synchronized.
	if (g_log_file != NULL) {
		FILE *tmp = g_log_file;
		g_log_file = NULL;

		fflush(tmp);
		fclose(tmp);
		tmp = NULL;
	}

	if (filename == NULL) {
		return; // Turning off file logging.
	}

	g_log_file = fopen(filename, "w");
	if (g_log_file == NULL) {
		U_LOG_E("Failed to open '%s'", filename);
	}
}


/*
 *
 * Logging sink.
 *
 */

// Logging sink global data.
static u_log_sink_func_t g_log_sink_func;
static void *g_log_sink_data;

void
u_log_set_sink(u_log_sink_func_t func, void *data)
{
	g_log_sink_func = func;
	g_log_sink_data = data;
}

#define DISPATCH_SINK(FILE, LINE, FUNC, LEVEL, FORMAT, ARGS)                                                           \
	if (g_log_sink_func != NULL) {                                                                                 \
		va_list copy;                                                                                          \
		va_copy(copy, ARGS);                                                                                   \
		g_log_sink_func(FILE, LINE, FUNC, LEVEL, FORMAT, copy, g_log_sink_data);                               \
		va_end(copy);                                                                                          \
	}


/*
 *
 * Hexdump functions.
 *
 */

static void
u_log_hexdump_line(char *buf, size_t offset, const uint8_t *data, size_t data_size)
{
	char *pos = buf;

	if (data_size > LOG_HEX_BYTES_PER_LINE) {
		data_size = LOG_HEX_BYTES_PER_LINE;
	}

	pos += sprintf(pos, "%08x: ", (uint32_t)offset);

	char *ascii = pos + ((ptrdiff_t)LOG_HEX_BYTES_PER_LINE * 3) + 1;
	size_t i;

	for (i = 0; i < data_size; i++) {
		pos += sprintf(pos, "%02x ", data[i]);

		if (data[i] >= ' ' && data[i] <= '~') {
			*ascii++ = data[i];
		} else {
			*ascii++ = '.';
		}
	}

	/* Pad short lines with spaces, and null terminate */
	while (i++ < LOG_HEX_BYTES_PER_LINE) {
		pos += sprintf(pos, "   ");
	}
	/* Replace the first NULL terminator with a space */
	*pos++ = ' ';
	/* and set it after the ASCII representation */
	*ascii++ = '\0';
}

void
u_log_hex(const char *file,
          int line,
          const char *func,
          enum u_logging_level level,
          const uint8_t *data,
          const size_t data_size)
{
	size_t offset = 0;

	while (offset < data_size) {
		char tmp[LOG_HEX_LINE_BUF_SIZE];
		u_log_hexdump_line(tmp, offset, data + offset, data_size - offset);
		u_log(file, line, func, level, "%s", tmp);

		offset += LOG_HEX_BYTES_PER_LINE;
		/*
		 * Limit the dump length to 16MB, this used to be 4GB which
		 * would on 32bit system always evaltuate to false. So we have
		 * the limit on something more sensible.
		 */
		if (offset > LOG_MAX_HEX_DUMP) {
			u_log(file, line, func, level, "Truncating output over " LOG_MAX_HEX_DUMP_HUMAN_READABLE);
			break;
		}
	}
}

void
u_log_xdev_hex(const char *file,
               int line,
               const char *func,
               enum u_logging_level level,
               struct xrt_device *xdev,
               const uint8_t *data,
               const size_t data_size)
{
	size_t offset = 0;

	while (offset < data_size) {
		char tmp[LOG_HEX_LINE_BUF_SIZE];
		u_log_hexdump_line(tmp, offset, data + offset, data_size - offset);
		u_log_xdev(file, line, func, level, xdev, "%s", tmp);

		offset += LOG_HEX_BYTES_PER_LINE;
		/*
		 * Limit the dump length to 16MB, this used to be 4GB which
		 * would on 32bit system always evaltuate to false. So we have
		 * the limit on something more sensible.
		 */
		if (offset > LOG_MAX_HEX_DUMP) {
			u_log_xdev(file, line, func, level, xdev,
			           "Truncating output over " LOG_MAX_HEX_DUMP_HUMAN_READABLE);
			break;
		}
	}
}


/*
 *
 * Platform specific functions.
 *
 */

#if defined(XRT_OS_ANDROID)

#include <android/log.h>

static android_LogPriority
u_log_convert_priority(enum u_logging_level level)
{
	switch (level) {
	case U_LOGGING_TRACE: return ANDROID_LOG_VERBOSE;
	case U_LOGGING_DEBUG: return ANDROID_LOG_DEBUG;
	case U_LOGGING_INFO: return ANDROID_LOG_INFO;
	case U_LOGGING_WARN: return ANDROID_LOG_WARN;
	case U_LOGGING_ERROR: return ANDROID_LOG_ERROR;
	case U_LOGGING_RAW: return ANDROID_LOG_INFO;
	default: break;
	}
	return ANDROID_LOG_INFO;
}

#elif defined(XRT_OS_WINDOWS)

#include <debugapi.h>

#else

#include <unistd.h>

#endif


/*
 *
 * Helper functions.
 *
 */

#define CHECK_RET_AND_UPDATE_STATE()                                                                                   \
	do {                                                                                                           \
		if (ret < 0) {                                                                                         \
			return ret;                                                                                    \
		}                                                                                                      \
                                                                                                                       \
		printed += ret; /* Is not negative, checked above. */                                                  \
		remaining -= ret;                                                                                      \
		buf += ret; /* Continue in the buffer from where we left off. */                                       \
	} while (false);

#ifdef XRT_FEATURE_COLOR_LOG
#define COLOR_TRACE "\033[2m"
#define COLOR_DEBUG "\033[36m"
#define COLOR_INFO "\033[32m"
#define COLOR_WARN "\033[33m"
#define COLOR_ERROR "\033[31m"
#define COLOR_RESET "\033[0m"

static int
print_prefix_color(const char *func, enum u_logging_level level, char *buf, int remaining)
{
	switch (level) {
	case U_LOGGING_TRACE: return u_truncate_snprintf(buf, remaining, COLOR_TRACE "TRACE " COLOR_RESET);
	case U_LOGGING_DEBUG: return u_truncate_snprintf(buf, remaining, COLOR_DEBUG "DEBUG " COLOR_RESET);
	case U_LOGGING_INFO: return u_truncate_snprintf(buf, remaining, COLOR_INFO " INFO " COLOR_RESET);
	case U_LOGGING_WARN: return u_truncate_snprintf(buf, remaining, COLOR_WARN " WARN " COLOR_RESET);
	case U_LOGGING_ERROR: return u_truncate_snprintf(buf, remaining, COLOR_ERROR "ERROR " COLOR_RESET);
	case U_LOGGING_RAW: return 0;
	default: return 0;
	}
}
#endif

static int
print_prefix_mono(const char *func, enum u_logging_level level, char *buf, int remaining)
{
	switch (level) {
	case U_LOGGING_TRACE: return u_truncate_snprintf(buf, remaining, "TRACE ");
	case U_LOGGING_DEBUG: return u_truncate_snprintf(buf, remaining, "DEBUG ");
	case U_LOGGING_INFO: return u_truncate_snprintf(buf, remaining, " INFO ");
	case U_LOGGING_WARN: return u_truncate_snprintf(buf, remaining, " WARN ");
	case U_LOGGING_ERROR: return u_truncate_snprintf(buf, remaining, "ERROR ");
	case U_LOGGING_RAW: return 0;
	default: return 0;
	}
}

static int
print_prefix(const char *func, enum u_logging_level level, char *buf, int remaining)
{
	int printed = 0;
	int ret = 0;

#ifdef XRT_FEATURE_COLOR_LOG
	if (g_log_file == NULL && isatty(STDERR_FILENO)) {
		ret = print_prefix_color(func, level, buf, remaining);
	} else {
		ret = print_prefix_mono(func, level, buf, remaining);
	}
#else
	ret = print_prefix_mono(func, level, buf, remaining);
#endif

	// Does what it says.
	CHECK_RET_AND_UPDATE_STATE();

	// Print the function name.
	if (level != U_LOGGING_RAW && func != NULL) {
		ret = u_truncate_snprintf(buf, remaining, "[%s] ", func);
	}

	// Does what it says.
	CHECK_RET_AND_UPDATE_STATE();

	// Total printed characters.
	return printed;
}

static int
log_as_json(const char *file, const char *func, enum u_logging_level level, const char *format, va_list args)
{
	cJSON *root = cJSON_CreateObject();

	char *level_s;
	switch (level) {
	case U_LOGGING_TRACE: level_s = "trace"; break;
	case U_LOGGING_DEBUG: level_s = "debug"; break;
	case U_LOGGING_INFO: level_s = "info"; break;
	case U_LOGGING_WARN: level_s = "warn"; break;
	case U_LOGGING_ERROR: level_s = "error"; break;
	default: level_s = "raw"; break;
	}

	// Add metadata.
	cJSON_AddItemToObject(root, "level", cJSON_CreateString(level_s));
	cJSON_AddItemToObject(root, "file", cJSON_CreateString(file));
	cJSON_AddItemToObject(root, "func", cJSON_CreateString(func));

	// Add message.
	char msg_buf[LOG_BUFFER_SIZE];
	u_truncate_vsnprintf(msg_buf, LOG_BUFFER_SIZE, format, args);
	cJSON_AddItemToObject(root, "message", cJSON_CreateString(msg_buf));

	// Get string and print to stderr.
	char *out = cJSON_PrintUnformatted(root);
	int printed = fprintf(stderr, "%s\n", out);

	// Clean up after us.
	cJSON_Delete(root);
	cJSON_free(out);

	return printed;
}

static int
do_print(const char *file, int line, const char *func, enum u_logging_level level, const char *format, va_list args)
{
	if (debug_get_bool_option_json_log()) {
		return log_as_json(file, func, level, format, args);
	}

	char storage[LOG_BUFFER_SIZE];

	int remaining = sizeof(storage) - 2; // 2 for \n\0
	int printed = 0;
	char *buf = storage; // We update the pointer.
	int ret = 0;

	// The prefix of the log.
	ret = print_prefix(func, level, buf, remaining);

	// Does what it says.
	CHECK_RET_AND_UPDATE_STATE();

	ret = u_truncate_vsnprintf(buf, remaining, format, args);

	// Does what it says.
	CHECK_RET_AND_UPDATE_STATE();

	/*
	 * The variable storage now holds the entire null-terminated message,
	 * but without a new-line character, proceed to output it.
	 */
	assert(storage[printed] == '\0');


#ifdef XRT_OS_ANDROID
	android_LogPriority prio = u_log_convert_priority(level);

	char storage_tag[256];
	int tag_ret = u_truncate_snprintf(storage_tag, sizeof(storage_tag), LOG_ANDROID_TAG_PREFIX ".%s", func);
	if (tag_ret < 0) {
		__android_log_write(prio, func, storage);
	} else {
		__android_log_write(prio, storage_tag, storage);
	}
#elif defined(XRT_OS_WINDOWS) || defined(XRT_OS_LINUX) || defined(XRT_OS_OSX)

	// We want a newline, so add it, then null-terminate again.
	storage[printed++] = '\n';
	storage[printed] = '\0'; // Don't count zero termination as printed.

#if defined(XRT_OS_WINDOWS)
	// Visual Studio output needs the newline char
	OutputDebugStringA(storage);
#endif

	FILE *output = g_log_file != NULL ? g_log_file : stderr;
	fwrite(storage, printed, 1, output);

#if defined(XRT_OS_WINDOWS)
	/*
	 * Windows like to buffer messages, call flush here to make sure all
	 * logs gets written out in case of sudden exits and crashes.
	 */
	fflush(output);
#endif

#else
#error "Port needed for logging function"
#endif

	return printed;
}


/*
 *
 * 'Exported' functions.
 *
 */

void
u_log_print_result(enum u_logging_level cond_level,
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

static u_log_filter_func_t g_filter = NULL;

void
u_log_set_filter(u_log_filter_func_t filter)
{
	g_filter = filter;
}

void
u_log(const char *file, int line, const char *func, enum u_logging_level level, const char *format, ...)
{
	va_list args;
	// Check filter first
	if (g_filter != NULL && !g_filter(file, line, func, level)) {
		return; // Skip this message
	}

	va_start(args, format);
	DISPATCH_SINK(file, line, func, level, format, args);
	do_print(file, line, func, level, format, args);
	va_end(args);
}

void
u_log_xdev(const char *file,
           int line,
           const char *func,
           enum u_logging_level level,
           struct xrt_device *xdev,
           const char *format,
           ...)
{
	va_list args;
	// Check filter first
	if (g_filter != NULL && !g_filter(file, line, func, level)) {
		return; // Skip this message
	}
	va_start(args, format);
	DISPATCH_SINK(file, line, func, level, format, args);
	do_print(file, line, func, level, format, args);
	va_end(args);
}
