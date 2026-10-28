// Copyright 2018-2022, Collabora, Ltd.
// Copyright 2026, NVIDIA CORPORATION.
// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Logging functions.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup openvr_common
 */

#pragma once

#include "util/u_pretty_print.h"
#include "util/u_logging.h"

#include <type_traits>
#include <typeinfo>


namespace xrt::state_trackers::openvr {

template <class T>
T &
classMacroImpl(const T *t);
#define CURRENT_CLASSNAME typeid(std::remove_reference<decltype(classMacroImpl(this))>::type).name()

struct openvr_logger
{
	const char *class_name;
	const char *api_func_name;
	u_logging_level log_level;
};

void
openvrLogInit(openvr_logger &logger, const char *class_name, const char *api_func_name);

#define OPENVR_LOGGER_INIT(logger) openvrLogInit(logger, CURRENT_CLASSNAME, __FUNCTION__)

void
openvrLog(openvr_logger &logger, u_logging_level log_level, const char *fmt, ...) XRT_PRINTF_FORMAT(3, 4);

#define OPENVR_LOG_TRACE(logger, fmt, ...) openvrLog(logger, U_LOGGING_TRACE, fmt __VA_OPT__(, ) __VA_ARGS__)
#define OPENVR_LOG_DEBUG(logger, fmt, ...) openvrLog(logger, U_LOGGING_DEBUG, fmt __VA_OPT__(, ) __VA_ARGS__)
#define OPENVR_LOG_INFO(logger, fmt, ...) openvrLog(logger, U_LOGGING_INFO, fmt __VA_OPT__(, ) __VA_ARGS__)
#define OPENVR_LOG_WARN(logger, fmt, ...) openvrLog(logger, U_LOGGING_WARN, fmt __VA_OPT__(, ) __VA_ARGS__)
#define OPENVR_LOG_ERROR(logger, fmt, ...) openvrLog(logger, U_LOGGING_ERROR, fmt __VA_OPT__(, ) __VA_ARGS__)

//! Specialized macro for logging an unimplemented function.
#define OPENVR_LOG_UNIMPLEMENTED(logger, fmt, ...)                                                                     \
	do {                                                                                                           \
		OPENVR_LOG_WARN(logger, "[UNIMPLEMENTED] " fmt __VA_OPT__(, ) __VA_ARGS__);                            \
	} while (false)

//! Specialized macro for logging an unimplemented function and returning a value.
#define OPENVR_LOG_UNIMPLEMENTED_RET(logger, fmt, ret, ...)                                                            \
	do {                                                                                                           \
		OPENVR_LOG_UNIMPLEMENTED(logger, fmt __VA_OPT__(, ) __VA_ARGS__);                                      \
		return ret;                                                                                            \
	} while (false)

//! Logs an xrt_result_t error with a formatted message.
#define OPENVR_LOG_ERROR_XRET(logger, fmt, xret, ...)                                                                  \
	do {                                                                                                           \
		u_pp_sink_stack_only sink;                                                                             \
		u_pp_delegate_t dg = u_pp_sink_stack_only_init(&sink);                                                 \
		u_pp_xrt_result(dg, xret);                                                                             \
		OPENVR_LOG_ERROR(logger, fmt " (xrt_result: %s)" __VA_OPT__(, ) __VA_ARGS__, sink.buffer);             \
	} while (false)

}; // namespace xrt::state_trackers::openvr
