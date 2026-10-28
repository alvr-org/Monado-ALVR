// Copyright 2018-2024, Collabora, Ltd.
// Copyright 2025-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Header defining all API functions.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup oxr_api
 */

#pragma once

#include "oxr_extension_support.h"


#ifdef __cplusplus
extern "C" {
#endif


//! OpenXR API function @ep{xrCreateActionSet}
XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrCreateActionSet(XrInstance instance, const XrActionSetCreateInfo *createInfo, XrActionSet *actionSet);

//! OpenXR API function @ep{xrDestroyActionSet}
XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrDestroyActionSet(XrActionSet actionSet);

//! OpenXR API function @ep{xrCreateAction}
XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrCreateAction(XrActionSet actionSet, const XrActionCreateInfo *createInfo, XrAction *action);

//! OpenXR API function @ep{xrDestroyAction}
XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrDestroyAction(XrAction action);

//! OpenXR API function @ep{xrSuggestInteractionProfileBindings}
XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrSuggestInteractionProfileBindings(XrInstance instance,
                                        const XrInteractionProfileSuggestedBinding *suggestedBindings);

//! OpenXR API function @ep{xrAttachSessionActionSets}
XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrAttachSessionActionSets(XrSession session, const XrSessionActionSetsAttachInfo *bindInfo);

//! OpenXR API function @ep{xrGetCurrentInteractionProfile}
XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrGetCurrentInteractionProfile(XrSession session,
                                   XrPath topLevelUserPath,
                                   XrInteractionProfileState *interactionProfile);

//! OpenXR API function @ep{xrGetActionStateBoolean}
XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrGetActionStateBoolean(XrSession session, const XrActionStateGetInfo *getInfo, XrActionStateBoolean *data);

//! OpenXR API function @ep{xrGetActionStateFloat}
XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrGetActionStateFloat(XrSession session, const XrActionStateGetInfo *getInfo, XrActionStateFloat *data);

//! OpenXR API function @ep{xrGetActionStateVector2f}
XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrGetActionStateVector2f(XrSession session, const XrActionStateGetInfo *getInfo, XrActionStateVector2f *data);

//! OpenXR API function @ep{xrGetActionStatePose}
XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrGetActionStatePose(XrSession session, const XrActionStateGetInfo *getInfo, XrActionStatePose *data);

//! OpenXR API function @ep{xrSyncActions}
XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrSyncActions(XrSession session, const XrActionsSyncInfo *syncInfo);

//! OpenXR API function @ep{xrEnumerateBoundSourcesForAction}
XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrEnumerateBoundSourcesForAction(XrSession session,
                                     const XrBoundSourcesForActionEnumerateInfo *enumerateInfo,
                                     uint32_t sourceCapacityInput,
                                     uint32_t *sourceCountOutput,
                                     XrPath *sources);

//! OpenXR API function @ep{xrGetInputSourceLocalizedName}
XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrGetInputSourceLocalizedName(XrSession session,
                                  const XrInputSourceLocalizedNameGetInfo *getInfo,
                                  uint32_t bufferCapacityInput,
                                  uint32_t *bufferCountOutput,
                                  char *buffer);

//! OpenXR API function @ep{xrApplyHapticFeedback}
XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrApplyHapticFeedback(XrSession session,
                          const XrHapticActionInfo *hapticActionInfo,
                          const XrHapticBaseHeader *hapticEvent);

//! OpenXR API function @ep{xrStopHapticFeedback}
XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrStopHapticFeedback(XrSession session, const XrHapticActionInfo *hapticActionInfo);


#ifdef OXR_HAVE_FB_haptic_pcm

XRAPI_ATTR XrResult XRAPI_CALL
oxr_xrGetDeviceSampleRateFB(XrSession session,
                            const XrHapticActionInfo *hapticActionInfo,
                            XrDevicePcmSampleRateGetInfoFB *deviceSampleRate);

#endif // OXR_HAVE_FB_haptic_pcm


#ifdef __cplusplus
}
#endif
