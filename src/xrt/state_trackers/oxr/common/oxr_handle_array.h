// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  A dynamic array of handles.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup oxr_main
 */

#pragma once

#include "os/os_threading.h"


struct oxr_handle_base;

/*!
 * Manages an array of handles, does not have a init function but must be zero initialized where it is declared.
 */
struct oxr_handle_array
{
	struct os_mutex mutex;

	struct oxr_handle_base **handles;
	uint32_t count;
	uint32_t capacity;
#ifndef NDEBUG
	bool init;
#endif
};

/*!
 * Initializes the array, must be called before adding handles to it.
 */
XrResult
oxr_handle_array_init(struct oxr_logger *log, struct oxr_handle_array *array);

/*!
 * Destroys all handles in the array, and the array itself.
 */
XrResult
oxr_handle_array_destroy(struct oxr_logger *log, struct oxr_handle_array *array, int level);

/*!
 * Adds a handle to the array.
 */
bool
oxr_handle_array_add(struct oxr_handle_array *array, struct oxr_handle_base *handle);

/*!
 * Removes the handle at the given index from the array, shifting all later handles down by one.
 */
bool
oxr_handle_array_remove(struct oxr_handle_array *array, uint32_t index);
