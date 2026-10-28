// Copyright 2018-2024, Collabora, Ltd.
// Copyright 2023-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Helper functions for device role getting.
 * @ingroup oxr_main
 */

#pragma once

#include "oxr_objects.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Helper struct that wraps xrt_system_roles for OpenXR state tracker usage.
 *
 * Is intended to be used on the stack and have a short lifetime, so no
 * references of it should be kept. Is mainly used in xrSyncActions and in
 * xrAttachSessionActionSets, where it is used to get the current roles of the
 * devices that the session is using.
 *
 * @see xrt_system_roles
 * @ingroup oxr_main
 */
struct oxr_roles
{
	//! To access the @ref xrt_system_devices struct.
	struct oxr_system *sys;

	//! The roles of the devices that the session is using.
	struct xrt_system_roles roles;
};

/*!
 * Initialize an oxr_roles struct on the stack.
 *
 * @ingroup oxr_main
 */
XRT_CHECK_RESULT XrResult
oxr_roles_init_on_stack(struct oxr_logger *log, struct oxr_roles *roles, struct oxr_system *sys);


/*
 *
 * Static device roles.
 *
 */

// clang-format off
static inline struct xrt_device *get_static_role_head(struct oxr_system *sys) { return sys->xsysd->static_roles.head; }
static inline struct xrt_device *get_static_role_eyes(struct oxr_system *sys) { return sys->xsysd->static_roles.eyes; }
static inline struct xrt_device *get_static_role_face(struct oxr_system* sys) { return sys->xsysd->static_roles.face; }
static inline struct xrt_device *get_static_role_body(struct oxr_system* sys) { return sys->xsysd->static_roles.body; }
static inline struct xrt_device *get_static_role_hand_tracking_unobstructed_left(struct oxr_system* sys) { return sys->xsysd->static_roles.hand_tracking.unobstructed.left; }
static inline struct xrt_device *get_static_role_hand_tracking_unobstructed_right(struct oxr_system* sys) { return sys->xsysd->static_roles.hand_tracking.unobstructed.right; }
static inline struct xrt_device *get_static_role_hand_tracking_conforming_left(struct oxr_system* sys) { return sys->xsysd->static_roles.hand_tracking.conforming.left; }
static inline struct xrt_device *get_static_role_hand_tracking_conforming_right(struct oxr_system* sys) { return sys->xsysd->static_roles.hand_tracking.conforming.right; }
// clang-format on

#define GET_STATIC_XDEV_BY_ROLE(SYS, ROLE) (get_static_role_##ROLE((SYS)))


/*
 *
 * Dynamic device roles.
 *
 */

// clang-format off
#define STATIC_WRAP(ROLE)                                                                                              \
	static inline struct xrt_device *get_role_##ROLE(const struct oxr_roles *roles)                                \
	{                                                                                                              \
		return get_static_role_##ROLE(roles->sys);                                                             \
	}
STATIC_WRAP(head)
STATIC_WRAP(eyes)
STATIC_WRAP(face)
STATIC_WRAP(body)
STATIC_WRAP(hand_tracking_unobstructed_left)
STATIC_WRAP(hand_tracking_unobstructed_right)
STATIC_WRAP(hand_tracking_conforming_left)
STATIC_WRAP(hand_tracking_conforming_right)
#undef STATIC_WRAP
// clang-format on

#define MAKE_GET_DYN_ROLES_FN(ROLE)                                                                                    \
	static inline struct xrt_device *get_role_##ROLE(const struct oxr_roles *roles)                                \
	{                                                                                                              \
		const int32_t xdev_idx = roles->roles.ROLE;                                                            \
		struct xrt_system_devices *xsysd = roles->sys->xsysd;                                                  \
		if (xdev_idx < 0 || xdev_idx >= (int32_t)xsysd->static_xdev_count) {                                   \
			return NULL;                                                                                   \
		}                                                                                                      \
		return xsysd->static_xdevs[xdev_idx];                                                                  \
	}
MAKE_GET_DYN_ROLES_FN(left)
MAKE_GET_DYN_ROLES_FN(right)
MAKE_GET_DYN_ROLES_FN(gamepad)
#undef MAKE_GET_DYN_ROLES_FN

#define GET_XDEV_BY_ROLE(ROLES, ROLE) (get_role_##ROLE((ROLES)))


/*
 *
 * Dynamic profile roles.
 *
 */

static inline enum xrt_device_name
get_role_profile_head(const struct oxr_roles *roles)
{
	return XRT_DEVICE_INVALID;
}
static inline enum xrt_device_name
get_role_profile_eyes(const struct oxr_roles *roles)
{
	return XRT_DEVICE_INVALID;
}
static inline enum xrt_device_name
get_role_profile_face(const struct oxr_roles *roles)
{
	return XRT_DEVICE_INVALID;
}
static inline enum xrt_device_name
get_role_profile_body(const struct oxr_roles *roles)
{
	return XRT_DEVICE_INVALID;
}
static inline enum xrt_device_name
get_role_profile_hand_tracking_unobstructed_left(const struct oxr_roles *roles)
{
	return XRT_DEVICE_INVALID;
}
static inline enum xrt_device_name
get_role_profile_hand_tracking_unobstructed_right(const struct oxr_roles *roles)
{
	return XRT_DEVICE_INVALID;
}

static inline enum xrt_device_name
get_role_profile_hand_tracking_conforming_left(const struct oxr_roles *roles)
{
	return XRT_DEVICE_INVALID;
}
static inline enum xrt_device_name
get_role_profile_hand_tracking_conforming_right(const struct oxr_roles *roles)
{
	return XRT_DEVICE_INVALID;
}

#define MAKE_GET_DYN_ROLE_PROFILE_FN(ROLE)                                                                             \
	static inline enum xrt_device_name get_role_profile_##ROLE(const struct oxr_roles *roles)                      \
	{                                                                                                              \
		return roles->roles.ROLE##_profile;                                                                    \
	}
MAKE_GET_DYN_ROLE_PROFILE_FN(left)
MAKE_GET_DYN_ROLE_PROFILE_FN(right)
MAKE_GET_DYN_ROLE_PROFILE_FN(gamepad)
#undef MAKE_GET_DYN_ROLES_FN

#define GET_PROFILE_NAME_BY_ROLE(ROLES, ROLE) (get_role_profile_##ROLE((ROLES)))


#ifdef __cplusplus
}
#endif
