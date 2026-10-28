// Copyright 2020, Collabora, Ltd.
// Copyright 2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Provides a utility macro for dealing with subaction paths
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @ingroup oxr_input
 */

#pragma once

#include "oxr_forward_declarations.h"
#include "oxr_extension_support.h"


/*!
 * Expansion macro (x-macro) that calls the macro you pass with the shorthand
 * name of each valid subaction path.
 *
 * Use to generate code that checks each subaction path in sequence, etc.
 *
 * If you also want the bogus subaction path of just plain `/user`, then see
 * OXR_FOR_EACH_SUBACTION_PATH()
 *
 * @note Keep this synchronized with OXR_ACTION_GET_FILLER!
 */
#define OXR_FOR_EACH_VALID_SUBACTION_PATH(_)                                                                           \
	_(left)                                                                                                        \
	_(right)                                                                                                       \
	_(head)                                                                                                        \
	_(gamepad)                                                                                                     \
	_(eyes)


/*!
 * Expansion macro (x-macro) that calls the macro you pass with the shorthand
 * name of each subaction path, including just bare `user`.
 *
 * Use to generate code that checks each subaction path in sequence, etc.
 *
 * @note Keep this synchronized with OXR_ACTION_GET_FILLER!
 */
#define OXR_FOR_EACH_SUBACTION_PATH(_)                                                                                 \
	OXR_FOR_EACH_VALID_SUBACTION_PATH(_)                                                                           \
	_(user)

/*!
 * Expansion macro (x-macro) that calls the macro you pass for each valid
 * subaction path, with the shorthand name of each subaction path, the same
 * name capitalized, the corresponding path string, and the localized name.
 *
 * If you also want the bogus subaction path of just plain `/user`, then see
 * OXR_FOR_EACH_SUBACTION_PATH_DETAILED()
 *
 * Use to generate code that checks each subaction path in sequence, etc.
 *
 * Most of the time you can just use OXR_FOR_EACH_VALID_SUBACTION_PATH() or
 * OXR_FOR_EACH_SUBACTION_PATH()
 */
#define OXR_FOR_EACH_VALID_SUBACTION_PATH_DETAILED(_)                                                                  \
	_(left, LEFT, "/user/hand/left", "Left")                                                                       \
	_(right, RIGHT, "/user/hand/right", "Right")                                                                   \
	_(head, HEAD, "/user/head", "Head")                                                                            \
	_(gamepad, GAMEPAD, "/user/gamepad", "Gamepad")                                                                \
	_(eyes, EYES, "/user/eyes_ext", "Eyes")

/*!
 * Expansion macro (x-macro) that calls the macro you pass for each subaction
 * path (including the bare `/user`), with the shorthand name of each subaction
 * path, the same name capitalized, the corresponding path string, and the
 * localized name.
 *
 * Use to generate code that checks each subaction path in sequence, etc.
 *
 * Most of the time you can just use OXR_FOR_EACH_VALID_SUBACTION_PATH() or
 * OXR_FOR_EACH_SUBACTION_PATH()
 */
#define OXR_FOR_EACH_SUBACTION_PATH_DETAILED(_)                                                                        \
	OXR_FOR_EACH_VALID_SUBACTION_PATH_DETAILED(_)                                                                  \
	_(user, USER, "/user", "User")


#ifdef __cplusplus
extern "C" {
#endif


/*!
 * A parsed equivalent of a list of sub-action paths.
 *
 * If @p any is true, then no paths were provided, which typically means any
 * input is acceptable.
 *
 * @ingroup oxr_main
 * @ingroup oxr_input
 */
struct oxr_subaction_paths
{
	bool any;
#define OXR_SUBPATH_MEMBER(X) bool X;
	OXR_FOR_EACH_SUBACTION_PATH(OXR_SUBPATH_MEMBER)
#undef OXR_SUBPATH_MEMBER
};

/*!
 * Helper function to determine if the set of paths in @p a is a subset of the
 * paths in @p b.
 *
 * @public @memberof oxr_subaction_paths
 */
static inline bool
oxr_subaction_paths_is_subset_of(const struct oxr_subaction_paths *a, const struct oxr_subaction_paths *b)
{
#define OXR_CHECK_SUBACTION_PATHS(X)                                                                                   \
	if (a->X && !b->X) {                                                                                           \
		return false;                                                                                          \
	}
	OXR_FOR_EACH_SUBACTION_PATH(OXR_CHECK_SUBACTION_PATHS)
#undef OXR_CHECK_SUBACTION_PATHS
	return true;
}

/*!
 * Classify an array of subaction paths into a parsed @ref oxr_subaction_paths
 * bitfield. The caller must do any error printing or reporting.
 *
 * Zeroes @p out_subaction_paths, then for each path in @p subaction_paths:
 * - If @p subaction_path_count is 0, sets @p out_subaction_paths->any to true
 *   and returns true.
 * - If the path is XR_NULL_PATH, sets @p out_subaction_paths->any to true.
 * - Otherwise matches the path against the instance's known subaction paths
 *   (e.g. /user/hand/left, /user/hand/right, /user/head) via @p cache and sets
 *   the corresponding flag in @p out_subaction_paths.
 *
 * @param[in] cache Instance path cache containing resolved subaction path
 *                 values
 * @param[in] subaction_path_count Number of paths in @p subaction_paths
 * @param[in] subaction_paths Array of XrPath subaction path handles to classify
 * @param[out] out_subaction_paths Filled with the classified path flags; set
 *             @ref oxr_subaction_paths::any if no paths or XR_NULL_PATH seen
 * @return true if all paths were recognized and classified
 * @return false if any path was not a known subaction path; the caller must
 *         report an error or warning
 * @public @memberof oxr_subaction_paths
 */
bool
oxr_classify_subaction_paths(const struct oxr_instance_path_cache *cache,
                             uint32_t subaction_path_count,
                             const XrPath *subaction_paths,
                             struct oxr_subaction_paths *out_subaction_paths);

/*!
 * Convenience wrapper around oxr_classify_subaction_paths() for the
 * single-path case, see @ref oxr_classify_subaction_paths().
 *
 * @public @memberof oxr_subaction_paths
 */
static inline bool
oxr_classify_subaction_path(const struct oxr_instance_path_cache *cache,
                            XrPath subaction_path,
                            struct oxr_subaction_paths *out_subaction_paths)
{
	return oxr_classify_subaction_paths(cache, 1, &subaction_path, out_subaction_paths);
}


#ifdef __cplusplus
}
#endif
