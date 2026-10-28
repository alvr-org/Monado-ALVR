// Copyright 2019-2020, Collabora, Ltd.
// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  A dynamic array of handles.
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup oxr_main
 */

#include "util/u_misc.h"

#include "math/m_api.h"

#include "oxr_handle_base.h"

#include "oxr_handle_array.h"


XrResult
oxr_handle_array_init(struct oxr_logger *log, struct oxr_handle_array *array)
{
#ifndef NDEBUG
	if (array->init) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE,
		                 "Attempted to initialize an already initialized handle array");
	}
#endif

	(*array) = (struct oxr_handle_array){
	    .handles = NULL,
	    .count = 0,
	    .capacity = 0,
	};

	if (os_mutex_init(&array->mutex) != 0) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to initialize mutex for handle array");
	}

#ifndef NDEBUG
	array->init = true;
#endif

	return XR_SUCCESS;
}

XrResult
oxr_handle_array_destroy(struct oxr_logger *log, struct oxr_handle_array *array, int level)
{
#ifndef NDEBUG
	if (!array->init) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Attempted to destroy an uninitialized handle array");
	}
#endif

	/*
	 * Destroying a child causes it to remove itself from this array (via
	 * oxr_handle_destroy_internal -> oxr_handle_array_remove), which shifts
	 * the remaining entries down and decrements count. So we must not walk
	 * the array with an incrementing index; instead keep destroying the
	 * current head until the array is empty.
	 */
	while (array->count > 0) {
		struct oxr_handle_base *child = array->handles[0];

		if (child == NULL) {
			// Should not happen, but guard against an infinite loop.
			oxr_handle_array_remove(array, 0);
			continue;
		}

		XrResult result = oxr_handle_destroy_internal(log, child, level + 1);
		if (result != XR_SUCCESS) {
			return result;
		}
	}

	os_mutex_destroy(&array->mutex);

	free(array->handles);
	array->handles = NULL;
	array->count = 0;
	array->capacity = 0;

#ifndef NDEBUG
	array->init = false;
#endif

	return XR_SUCCESS;
}

bool
oxr_handle_array_add(struct oxr_handle_array *array, struct oxr_handle_base *handle)
{
	assert(array->init);

	// Count should never exceed capacity
	assert(array->count <= array->capacity);

	os_mutex_lock(&array->mutex);

	if (array->count == array->capacity) {
		uint32_t new_capacity = MAX(1, array->capacity * 2);

		U_ARRAY_REALLOC_OR_FREE(array->handles, struct oxr_handle_base *, new_capacity);

		if (array->handles == NULL) {
			os_mutex_unlock(&array->mutex);

			return false;
		}

		array->capacity = new_capacity;
	}

	// When adding, we should always have capacity to do so
	assert(array->count < array->capacity);

	array->handles[array->count++] = handle;

	os_mutex_unlock(&array->mutex);

	return true;
}

bool
oxr_handle_array_remove(struct oxr_handle_array *array, uint32_t index)
{
	assert(array->init);

	os_mutex_lock(&array->mutex);
	if (index >= array->count) {
		os_mutex_unlock(&array->mutex);

		assert(!"Attempted to remove handle at index greater than or equal to count");
		return false;
	}

	// Shift all later handles down by one
	for (uint32_t i = index; i < array->count - 1; ++i) {
		array->handles[i] = array->handles[i + 1];
	}
	array->handles[array->count - 1] = NULL;
	array->count--;

	os_mutex_unlock(&array->mutex);

	return true;
}
