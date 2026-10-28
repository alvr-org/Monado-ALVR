// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Catch guards for glue classes.
 * @author Jakob Bornecrantz <tbornecrantz@nvidia.com>
 * @ingroup aux_util
 */

#pragma once

#include "xrt/xrt_results.h"
#include "util/u_logging.h"
#include <exception>


#define G_CATCH_GUARDS_WITH_RETURN(...)                                                                                \
	catch (const std::exception &e)                                                                                \
	{                                                                                                              \
		U_LOG_E("Uncaught C++ exception: %s", e.what());                                                       \
		return __VA_ARGS__;                                                                                    \
	}                                                                                                              \
	catch (...)                                                                                                    \
	{                                                                                                              \
		U_LOG_E("Uncaught unknown C++ exception");                                                             \
		return __VA_ARGS__;                                                                                    \
	}

#define G_CATCH_GUARDS_VOID G_CATCH_GUARDS_WITH_RETURN()
#define G_CATCH_GUARDS G_CATCH_GUARDS_WITH_RETURN(XRT_ERROR_UNCAUGHT_EXCEPTION)
