// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Holds suggested profile array related functions.
 * @author Jakob Bornecrantz <tbornecrantz@nvidia.com>
 * @ingroup oxr_main
 */

#pragma once

#include "xrt/xrt_compiler.h"
#include "xrt/xrt_device.h"

#include "oxr_extension_support.h"
#include "oxr_forward_declarations.h"
#include "oxr_suggested_profile.h"


#ifdef __cplusplus
extern "C" {
#endif


/*!
 * Manages an array of suggested profiles, does not have a init function
 * but must be zero initialized where it is declared.
 */
struct oxr_suggested_profile_array
{
	struct oxr_suggested_profile **profiles;
	size_t count;
};

/*!
 * Frees all profiles on the suggested profile array.
 *
 * Frees all suggested profiles in the array and then frees the internal array.
 * Does not free the struct passed in. After this call, the array structure is
 * reset to an empty state.
 *
 * @param      array               Suggested profile array to destroy, must not be NULL
 * @public @memberof oxr_suggested_profile_array
 */
XRT_NONNULL_ALL void
oxr_suggested_profile_array_clear(struct oxr_suggested_profile_array *array);

/*!
 * Add a suggested profile to the array.
 *
 * The profile is added to the end of the array. The array takes ownership of
 * the profile pointer.
 *
 * @param      array               Suggested profile array to add to, must not be NULL
 * @param      profile             Suggested profile to add, must not be NULL
 * @public @memberof oxr_suggested_profile_array
 */
XRT_NONNULL_ALL void
oxr_suggested_profile_array_add(struct oxr_suggested_profile_array *array, struct oxr_suggested_profile *profile);

/*!
 * Clone a suggested profile array.
 *
 * Creates a deep copy of the source array, including cloning all suggested
 * profiles it contains. The destination array is first cleared (destroying any
 * existing profiles) before cloning. On failure the destination is left
 * cleared (empty).
 *
 * @param      src                 Source suggested profile array to clone, must not be NULL
 * @param      dst                 Destination suggested profile array, must not be NULL
 * @return                         XR_SUCCESS on success, XR_ERROR_RUNTIME_FAILURE on allocation failure
 * @public @memberof oxr_suggested_profile_array
 */
XRT_NONNULL_ALL XRT_CHECK_RESULT XrResult
oxr_suggested_profile_array_clone(const struct oxr_suggested_profile_array *src,
                                  struct oxr_suggested_profile_array *dst);

/*!
 * Find a suggested profile in the array by its path.
 *
 * Searches the array for a suggested profile with the matching XrPath.
 *
 * @param      array               Suggested profile array to search, must not be NULL
 * @param      path                XrPath to search for
 * @param[out] out_p               Found suggested profile; NULL if not found. Must not be NULL.
 * @return                         true if a matching profile was found, false otherwise
 * @public @memberof oxr_suggested_profile_array
 */
XRT_NONNULL_ALL bool
oxr_suggested_profile_array_find_by_path(const struct oxr_suggested_profile_array *array,
                                         XrPath path,
                                         struct oxr_suggested_profile **out_p);

/*!
 * Find a suggested profile by the XRT device name stored on its profile template.
 *
 * This directly compares @p name against each suggested profile's generated
 * template.
 *
 * @param      array               Suggested profile array to search, must not be NULL
 * @param      name                XRT device name stored on the profile template
 * @param[out] out_p               Found suggested profile; NULL if not found. Must not be NULL.
 * @return                         true if a matching profile was found, false otherwise
 */
XRT_NONNULL_ALL bool
oxr_suggested_profile_array_find_by_device_name(const struct oxr_suggested_profile_array *array,
                                                enum xrt_device_name name,
                                                struct oxr_suggested_profile **out_p);

/*!
 * Find the first rebinding profile that targets @p target_device_name and has a
 * suggested source profile in @p array.
 *
 * The helper walks the registry's generated rebinding profiles in registry
 * order. This lets template generation decide priority: higher-priority
 * rebindings, such as generic controller rebindings, should be emitted earlier.
 * For each matching rebinding target device, the helper looks for a suggested
 * profile with the rebinding source profile path.
 *
 * @param      array               Suggested profile array to search, must not be NULL
 * @param      registry            Binding registry whose rebinding profiles are walked, must not be NULL
 * @param      target_device_name  Device name the rebinding profile must target
 * @param[out] out_p               Suggested source profile, or NULL if not found
 * @param[out] out_rebinding       Matching rebinding profile, or NULL if not found
 * @return                         true if a matching rebinding and suggested profile were found
 */
XRT_NONNULL_ALL bool
oxr_suggested_profile_array_find_rebinding_for_device_name(const struct oxr_suggested_profile_array *array,
                                                           const struct oxr_binding_registry *registry,
                                                           enum xrt_device_name target_device_name,
                                                           struct oxr_suggested_profile **out_p,
                                                           const struct oxr_rebinding_profile **out_rebinding);

#ifdef __cplusplus
}
#endif
