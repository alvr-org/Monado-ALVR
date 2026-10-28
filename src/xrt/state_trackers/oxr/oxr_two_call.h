// Copyright 2018-2019, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Two call helper functions.
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup oxr_main
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define OXR_TWO_CALL_CHECK_ONLY(log, cnt_input, cnt_output, count, sval)                                               \
	do {                                                                                                           \
		if ((cnt_output) == NULL) {                                                                            \
			return oxr_error(log, XR_ERROR_VALIDATION_FAILURE, #cnt_output);                               \
		}                                                                                                      \
		*(cnt_output) = (uint32_t)(count);                                                                     \
                                                                                                                       \
		if ((cnt_input) == 0) {                                                                                \
			return sval;                                                                                   \
		}                                                                                                      \
		if ((cnt_input) < (uint32_t)(count)) {                                                                 \
			return oxr_error(log, XR_ERROR_SIZE_INSUFFICIENT, #cnt_input);                                 \
		}                                                                                                      \
	} while (false)

#define OXR_TWO_CALL_CHECK_GOTO(log, cnt_input, cnt_output, count, sval, goto_label)                                   \
	do {                                                                                                           \
		if ((cnt_output) == NULL) {                                                                            \
			sval = oxr_error(log, XR_ERROR_VALIDATION_FAILURE, #cnt_output);                               \
			goto goto_label;                                                                               \
		}                                                                                                      \
		*(cnt_output) = (uint32_t)(count);                                                                     \
                                                                                                                       \
		if ((cnt_input) == 0) {                                                                                \
			goto goto_label;                                                                               \
		}                                                                                                      \
		if ((cnt_input) < (uint32_t)(count)) {                                                                 \
			sval = oxr_error(log, XR_ERROR_SIZE_INSUFFICIENT, #cnt_input);                                 \
			goto goto_label;                                                                               \
		}                                                                                                      \
	} while (false)

#define OXR_TWO_CALL_HELPER(log, cnt_input, cnt_output, output, count, data, sval)                                     \
	do {                                                                                                           \
		OXR_TWO_CALL_CHECK_ONLY(log, cnt_input, cnt_output, count, sval);                                      \
                                                                                                                       \
		for (uint32_t i = 0; i < (count); i++) {                                                               \
			(output)[i] = (data)[i];                                                                       \
		}                                                                                                      \
		return (sval);                                                                                         \
	} while (false)

//! Calls fill_fn(&output_struct[i], &source_struct[i]) to fill output_structs
#define OXR_TWO_CALL_FILL_IN_HELPER(log, cnt_input, cnt_output, output_structs, count, fill_fn, source_structs, sval)  \
	do {                                                                                                           \
		OXR_TWO_CALL_CHECK_ONLY(log, cnt_input, cnt_output, count, sval);                                      \
                                                                                                                       \
		for (uint32_t i = 0; i < count; i++) {                                                                 \
			fill_fn(&output_structs[i], &source_structs[i]);                                               \
		}                                                                                                      \
		return sval;                                                                                           \
	} while (false)

//! Calls fill_fn(&output_struct[i], &source_struct[i]) to fill output_structs
#define OXR_TWO_CALL_FILL_IN_GOTO(log, cnt_input, cnt_output, output_structs, count, fill_macro, source_structs, sval, \
                                  goto_label)                                                                          \
	do {                                                                                                           \
		OXR_TWO_CALL_CHECK_GOTO(log, cnt_input, cnt_output, count, sval, goto_label);                          \
                                                                                                                       \
		for (uint32_t i = 0; i < count; i++) {                                                                 \
			fill_macro(output_structs[i], source_structs[i]);                                              \
		}                                                                                                      \
	} while (false)

#ifdef __cplusplus
}
#endif
