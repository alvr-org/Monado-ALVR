// Copyright 2022, Collabora, Ltd.
// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  onnxruntime wrapper objects and functions.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @author Moshi Turner <moshiturner@protonmail.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_onnxruntime
 */

#pragma once

#include "xrt/xrt_config_build.h"

#include "util/u_logging.h"

#include <concepts>
#include <onnxruntime_c_api.h>
#include <source_location>
#include <string>
#include <utility>
#include <stdexcept>


#define ONNX_TRACE(onnx, ...) U_LOG_IFL_T(onnx->log_level, __VA_ARGS__)
#define ONNX_DEBUG(onnx, ...) U_LOG_IFL_D(onnx->log_level, __VA_ARGS__)
#define ONNX_INFO(onnx, ...) U_LOG_IFL_I(onnx->log_level, __VA_ARGS__)
#define ONNX_WARN(onnx, ...) U_LOG_IFL_W(onnx->log_level, __VA_ARGS__)
#define ONNX_ERROR(onnx, ...) U_LOG_IFL_E(onnx->log_level, __VA_ARGS__)

namespace xrt::auxiliary::onnx {

class Error : public std::runtime_error
{
public:
	explicit Error(const std::string &message) : std::runtime_error(message) {}
};

class OnnxWrapper
{
public:
	enum u_logging_level log_level = U_LOGGING_INFO;

	const OrtApi *api = nullptr;
	OrtEnv *env = nullptr;

	OrtMemoryInfo *meminfo = nullptr;
	OrtSession *session = nullptr;

	OnnxWrapper() {}

	OnnxWrapper(enum u_logging_level log_level, std::string path, std::string env_name = "monado_onnx");

	template <typename Fn>
	inline void
	ort_safe(Fn &&fn, std::source_location loc = std::source_location::current())
	{
		assert(this->api != nullptr);

		OrtStatus *status = std::forward<Fn>(fn)(this->api);
		if (status != nullptr) {
			std::string msg = this->api->GetErrorMessage(status);
			ONNX_ERROR(this, "[%s:%u]: %s", loc.file_name(), loc.line(), msg.c_str());
			this->api->ReleaseStatus(status);
			throw onnx::Error(msg); // throw it as an exception.
		}
	}

	~OnnxWrapper();
};

#define ORT_SAFE(wrap, fn) (wrap).ort_safe([&](const OrtApi *api) { return api->fn; })

}; // namespace xrt::auxiliary::onnx
