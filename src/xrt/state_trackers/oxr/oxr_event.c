// Copyright 2018-2020, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Holds event related functions.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup oxr_main
 */

#include "os/os_threading.h"

#include "util/u_misc.h"

#include "oxr_objects.h"
#include "oxr_logger.h"
#include "oxr_conversions.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/*
 *
 * Struct and defines.
 *
 */

struct oxr_event
{
	struct oxr_event *next;
	size_t length;
	XrResult result;
};


/*
 *
 * Internal helpers.
 *
 */

void
lock(struct oxr_instance *inst)
{
	os_mutex_lock(&inst->event.mutex);
}

void
unlock(struct oxr_instance *inst)
{
	os_mutex_unlock(&inst->event.mutex);
}

void *
oxr_event_extra(struct oxr_event *event)
{
	return &event[1];
}

struct oxr_event *
pop(struct oxr_instance *inst)
{
	struct oxr_event *ret = inst->event.next;
	if (ret == NULL) {
		return NULL;
	}

	inst->event.next = ret->next;
	ret->next = NULL;

	if (ret == inst->event.last) {
		inst->event.last = NULL;
	}

	return ret;
}

void
push(struct oxr_instance *inst, struct oxr_event *event)
{
	struct oxr_event *last = inst->event.last;
	if (last != NULL) {
		last->next = event;
	}
	inst->event.last = event;

	if (inst->event.next == NULL) {
		inst->event.next = event;
	}
}

#define ALLOC(log, inst, event, extra)                                                                                 \
	do {                                                                                                           \
		XrResult ret = oxr_event_alloc(log, inst, sizeof(**extra), event);                                     \
		if (ret != XR_SUCCESS) {                                                                               \
			return ret;                                                                                    \
		}                                                                                                      \
		*((void **)extra) = oxr_event_extra(*event);                                                           \
	} while (false)

static XrResult
oxr_event_alloc(struct oxr_logger *log, struct oxr_instance *inst, size_t size, struct oxr_event **out_event)
{
	struct oxr_event *event = U_CALLOC_WITH_CAST(struct oxr_event, sizeof(struct oxr_event) + size);

	if (event == NULL) {
		return oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Out of memory");
	}

	event->next = NULL;
	event->length = size;
	event->result = XR_SUCCESS;

	*out_event = event;

	return XR_SUCCESS;
}

static bool
is_session_link_to_event(struct oxr_event *event, XrSession session)
{
	XrStructureType *type = oxr_event_extra(event);

	switch (*type) {
	case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
		XrEventDataSessionStateChanged *changed = (XrEventDataSessionStateChanged *)type;
		return changed->session == session;
	}
	case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED: {
		XrEventDataInteractionProfileChanged *changed = (XrEventDataInteractionProfileChanged *)type;
		return changed->session == session;
	}
	case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING: {
		XrEventDataReferenceSpaceChangePending *pending = (XrEventDataReferenceSpaceChangePending *)type;
		return pending->session == session;
	}
#ifdef OXR_HAVE_KHR_visibility_mask
	case XR_TYPE_EVENT_DATA_VISIBILITY_MASK_CHANGED_KHR: {
		XrEventDataVisibilityMaskChangedKHR *changed = (XrEventDataVisibilityMaskChangedKHR *)type;
		return changed->session == session;
	}
#endif // OXR_HAVE_KHR_visibility_mask
#ifdef OXR_HAVE_EXT_user_presence
	case XR_TYPE_EVENT_DATA_USER_PRESENCE_CHANGED_EXT: {
		XrEventDataUserPresenceChangedEXT *changed = (XrEventDataUserPresenceChangedEXT *)type;
		return changed->session == session;
	}
#endif // OXR_HAVE_EXT_user_presence
#ifdef OXR_HAVE_ML_localization_map
	case XR_TYPE_EVENT_DATA_LOCALIZATION_CHANGED_ML: {
		XrEventDataLocalizationChangedML *changed = (XrEventDataLocalizationChangedML *)type;
		return changed->session == session;
	}
#endif // OXR_HAVE_ML_localization_map
	default: return false;
	}
}


/*
 *
 * 'Exported' functions.
 *
 */

XrResult
oxr_event_push_XrEventDataSessionStateChanged(struct oxr_logger *log,
                                              struct oxr_session *sess,
                                              XrSessionState state,
                                              XrTime time)
{
	struct oxr_instance *inst = sess->sys->inst;
	XrEventDataSessionStateChanged *changed;
	struct oxr_event *event = NULL;

	ALLOC(log, inst, &event, &changed);

	changed->type = XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED;
	changed->session = oxr_session_to_openxr(sess);
	changed->state = state;
	changed->time = time;

	event->result = XR_SUCCESS;

	lock(inst);
	push(inst, event);
	unlock(inst);

	return XR_SUCCESS;
}

XrResult
oxr_event_push_XrEventDataInteractionProfileChanged(struct oxr_logger *log, struct oxr_session *sess)
{
	struct oxr_instance *inst = sess->sys->inst;
	XrEventDataInteractionProfileChanged *changed;
	struct oxr_event *event = NULL;

	ALLOC(log, inst, &event, &changed);

	changed->type = XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED;
	changed->session = oxr_session_to_openxr(sess);

	lock(inst);
	push(inst, event);
	unlock(inst);

	return XR_SUCCESS;
}

XrResult
oxr_event_push_XrEventDataReferenceSpaceChangePending(struct oxr_logger *log,
                                                      struct oxr_session *sess,
                                                      XrReferenceSpaceType referenceSpaceType,
                                                      XrTime changeTime,
                                                      XrBool32 poseValid,
                                                      const XrPosef *poseInPreviousSpace)
{
	struct oxr_instance *inst = sess->sys->inst;
	XrEventDataReferenceSpaceChangePending *pending;
	struct oxr_event *event = NULL;

	ALLOC(log, inst, &event, &pending);

	pending->type = XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING;
	pending->session = oxr_session_to_openxr(sess);
	pending->referenceSpaceType = referenceSpaceType;
	pending->changeTime = changeTime;
	pending->poseValid = poseValid;
	pending->poseInPreviousSpace = *poseInPreviousSpace;
	event->result = XR_SUCCESS;

	lock(inst);
	push(inst, event);
	unlock(inst);

	return XR_SUCCESS;
}

#ifdef OXR_HAVE_FB_display_refresh_rate
XrResult
oxr_event_push_XrEventDataDisplayRefreshRateChangedFB(struct oxr_logger *log,
                                                      struct oxr_session *sess,
                                                      float fromDisplayRefreshRate,
                                                      float toDisplayRefreshRate)
{
	struct oxr_instance *inst = sess->sys->inst;
	XrEventDataDisplayRefreshRateChangedFB *changed;
	struct oxr_event *event = NULL;

	ALLOC(log, inst, &event, &changed);
	changed->type = XR_TYPE_EVENT_DATA_DISPLAY_REFRESH_RATE_CHANGED_FB;
	changed->fromDisplayRefreshRate = fromDisplayRefreshRate;
	changed->toDisplayRefreshRate = toDisplayRefreshRate;
	event->result = XR_SUCCESS;
	lock(inst);
	push(inst, event);
	unlock(inst);

	return XR_SUCCESS;
}
#endif // OXR_HAVE_FB_display_refresh_rate

#ifdef OXR_HAVE_EXTX_overlay
XrResult
oxr_event_push_XrEventDataMainSessionVisibilityChangedEXTX(struct oxr_logger *log,
                                                           struct oxr_session *sess,
                                                           bool visible)
{
	struct oxr_instance *inst = sess->sys->inst;
	XrEventDataMainSessionVisibilityChangedEXTX *changed;
	struct oxr_event *event = NULL;

	ALLOC(log, inst, &event, &changed);
	changed->type = XR_TYPE_EVENT_DATA_MAIN_SESSION_VISIBILITY_CHANGED_EXTX;
	changed->flags = 0;
	changed->visible = visible;
	event->result = XR_SUCCESS;
	lock(inst);
	push(inst, event);
	unlock(inst);

	return XR_SUCCESS;
}
#endif // OXR_HAVE_EXTX_overlay


#ifdef OXR_HAVE_EXT_performance_settings
XrResult
oxr_event_push_XrEventDataPerfSettingsEXT(struct oxr_logger *log,
                                          struct oxr_session *sess,
                                          enum xrt_perf_domain domain,
                                          enum xrt_perf_sub_domain subDomain,
                                          enum xrt_perf_notify_level fromLevel,
                                          enum xrt_perf_notify_level toLevel)
{
	struct oxr_instance *inst = sess->sys->inst;
	XrEventDataPerfSettingsEXT *changed;
	struct oxr_event *event = NULL;

	ALLOC(log, inst, &event, &changed);
	changed->type = XR_TYPE_EVENT_DATA_PERF_SETTINGS_EXT;
	changed->domain = xrt_perf_domain_to_xr(domain);
	changed->subDomain = xrt_perf_sub_domain_to_xr(subDomain);
	changed->fromLevel = xrt_perf_notify_level_to_xr(fromLevel);
	changed->toLevel = xrt_perf_notify_level_to_xr(toLevel);
	event->result = XR_SUCCESS;
	lock(inst);
	push(inst, event);
	unlock(inst);

	return XR_SUCCESS;
}
#endif // OXR_HAVE_EXT_performance_settings

#ifdef OXR_HAVE_EXT_view_configuration_views_change
XrResult
oxr_event_push_XrEventDataViewConfigurationViewsChangedEXT(struct oxr_logger *log,
                                                           struct oxr_system *system,
                                                           enum xrt_view_type viewConfigurationType)
{
	struct oxr_instance *inst = system->inst;
	XrEventDataViewConfigurationViewsChangedEXT *changed;
	struct oxr_event *event = NULL;

	ALLOC(log, inst, &event, &changed);
	changed->type = XR_TYPE_EVENT_DATA_VIEW_CONFIGURATION_VIEWS_CHANGED_EXT;
	changed->systemId = system->systemId;
	changed->viewConfigurationType = xrt_view_type_to_xr(viewConfigurationType);
	event->result = XR_SUCCESS;
	lock(inst);
	push(inst, event);
	unlock(inst);

	return XR_SUCCESS;
}
#endif // OXR_HAVE_EXT_view_configuration_views_change

#ifdef OXR_HAVE_FB_passthrough
XrResult
oxr_event_push_XrEventDataPassthroughStateChangedFB(struct oxr_logger *log,
                                                    struct oxr_session *sess,
                                                    XrPassthroughStateChangedFlagsFB flags)
{
	struct oxr_instance *inst = sess->sys->inst;
	XrEventDataPassthroughStateChangedFB *changed;
	struct oxr_event *event = NULL;

	ALLOC(log, inst, &event, &changed);
	changed->type = XR_TYPE_EVENT_DATA_PASSTHROUGH_STATE_CHANGED_FB;
	changed->flags = flags;
	event->result = XR_SUCCESS;
	lock(inst);
	push(inst, event);
	unlock(inst);

	return XR_SUCCESS;
}
#endif // OXR_HAVE_FB_passthrough

#ifdef OXR_HAVE_KHR_visibility_mask
XrResult
oxr_event_push_XrEventDataVisibilityMaskChangedKHR(struct oxr_logger *log,
                                                   struct oxr_session *sess,
                                                   XrViewConfigurationType viewConfigurationType,
                                                   uint32_t viewIndex)
{
	struct oxr_instance *inst = sess->sys->inst;
	XrEventDataVisibilityMaskChangedKHR *changed;
	struct oxr_event *event = NULL;

	ALLOC(log, inst, &event, &changed);
	changed->type = XR_TYPE_EVENT_DATA_VISIBILITY_MASK_CHANGED_KHR;
	changed->session = oxr_session_to_openxr(sess);
	changed->viewConfigurationType = viewConfigurationType;
	changed->viewIndex = viewIndex;
	event->result = XR_SUCCESS;
	lock(inst);
	push(inst, event);
	unlock(inst);

	return XR_SUCCESS;
}
#endif // OXR_HAVE_KHR_visibility_mask

#ifdef OXR_HAVE_EXT_user_presence
XrResult
oxr_event_push_XrEventDataUserPresenceChangedEXT(struct oxr_logger *log, struct oxr_session *sess, bool isUserPresent)
{
	struct oxr_instance *inst = sess->sys->inst;
	XrEventDataUserPresenceChangedEXT *changed;
	struct oxr_event *event = NULL;

	ALLOC(log, inst, &event, &changed);
	changed->type = XR_TYPE_EVENT_DATA_USER_PRESENCE_CHANGED_EXT;
	changed->session = oxr_session_to_openxr(sess);
	changed->isUserPresent = isUserPresent;
	event->result = XR_SUCCESS;
	lock(inst);
	push(inst, event);
	unlock(inst);

	return XR_SUCCESS;
}
#endif // OXR_HAVE_EXT_user_presence

XrResult
oxr_event_remove_session_events(struct oxr_logger *log, struct oxr_session *sess)
{
	struct oxr_instance *inst = sess->sys->inst;
	XrSession session = oxr_session_to_openxr(sess);

	lock(inst);

	struct oxr_event *prev = NULL;
	struct oxr_event *e = inst->event.next;
	while (e != NULL) {
		struct oxr_event *cur = e;
		e = e->next;

		if (!is_session_link_to_event(cur, session)) {
			prev = cur;
			continue;
		}

		// Unlink cur, keeping the predecessor's next pointer valid.
		if (prev != NULL) {
			prev->next = cur->next;
		}

		if (cur == inst->event.next) {
			inst->event.next = cur->next;
		}

		// The new tail is the last retained node (prev), or NULL if
		// nothing was kept before cur.
		if (cur == inst->event.last) {
			inst->event.last = prev;
		}

		free(cur);
	}

	unlock(inst);

	return XR_SUCCESS;
}

XrResult
oxr_poll_event(struct oxr_logger *log, struct oxr_instance *inst, XrEventDataBuffer *eventData)
{
	XrResult ret;

	// Sessions can be created and destroyed on other threads while we poll.
	os_mutex_lock(&inst->sessions_mutex);

	// The system is lazily created by xrGetSystem, which may be running on another thread.
	os_mutex_lock(&inst->system_init_lock);
	ret = oxr_system_poll(log, &inst->system);
	os_mutex_unlock(&inst->system_init_lock);

	struct oxr_session *sess = inst->sessions_locked;
	while (ret == XR_SUCCESS && sess) {
		ret = oxr_session_poll(log, sess);
		sess = sess->next;
	}

	os_mutex_unlock(&inst->sessions_mutex);

	if (ret != XR_SUCCESS) {
		return ret;
	}

	lock(inst);
	struct oxr_event *event = pop(inst);
	unlock(inst);

	if (event == NULL) {
		return XR_EVENT_UNAVAILABLE;
	}

	ret = event->result;
	memcpy(eventData, oxr_event_extra(event), event->length);
	free(event);

	return ret;
}
