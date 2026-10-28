# Copyright 2026, Beyley Cardellio
# SPDX-License-Identifier: BSL-1.0

###
# Wrapper around onnxruntime to make session management and error handling easier.
#
add_library(aux_onnx STATIC onnx_wrapper.cpp onnx_wrapper.hpp)

target_link_libraries(aux_onnx PUBLIC aux-includes aux_util ONNXRuntime::ONNXRuntime)
