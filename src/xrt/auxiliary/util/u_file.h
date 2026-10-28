// Copyright 2019-2025, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Very simple file opening functions.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup aux_util
 */

#pragma once

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

int
u_file_get_config_dir(char *out_path, size_t out_path_size);

int
u_file_get_path_in_config_dir(const char *suffix, char *out_path, size_t out_path_size);

FILE *
u_file_open_file_in_config_dir(const char *filename, const char *mode);

FILE *
u_file_open_file_in_config_dir_subpath(const char *subpath, const char *filename, const char *mode);

int
u_file_get_hand_tracking_models_dir(char *out_path, size_t out_path_size);

int
u_file_get_runtime_dir(char *out_path, size_t out_path_size);

char *
u_file_read_content(FILE *file, size_t *out_file_size);

char *
u_file_read_content_from_path(const char *path, size_t *out_file_size);

int
u_file_get_path_in_runtime_dir(const char *suffix, char *out_path, size_t out_path_size);

#ifdef __cplusplus
}
#endif
