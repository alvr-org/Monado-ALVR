// Copyright 2018-2024, Collabora, Ltd.
// Copyright 2024-2026, NVIDIA CORPORATION.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Holds instance related functions.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup oxr_main
 */

#include "xrt/xrt_config_os.h"
#include "xrt/xrt_config_build.h"
#include "xrt/xrt_instance.h"

#include "math/m_mathinclude.h"

#include "util/u_var.h"
#include "util/u_time.h"
#include "util/u_misc.h"
#include "util/u_debug.h"
#include "util/u_git_tag.h"

#ifdef XRT_FEATURE_CLIENT_DEBUG_GUI
#include "util/u_debug_gui.h"
#endif

#ifdef XRT_OS_ANDROID
#include "android/android_globals.h"
#endif

#include "oxr_objects.h"
#include "oxr_logger.h"
#include "oxr_handle_base.h"
#include "oxr_extension_support.h"
#include "oxr_chain.h"
#include "oxr_roles.h"
#include "oxr_generated_bindings.h"
#include "actions/oxr_binding.h"
#include "actions/oxr_binding_registry.h"
#include "actions/oxr_subaction.h"

#include <sys/types.h>
#ifdef XRT_OS_UNIX
#include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

DEBUG_GET_ONCE_BOOL_OPTION(debug_views, "OXR_DEBUG_VIEWS", false)
DEBUG_GET_ONCE_BOOL_OPTION(debug_spaces, "OXR_DEBUG_SPACES", false)
DEBUG_GET_ONCE_BOOL_OPTION(debug_bindings, "OXR_DEBUG_BINDINGS", false)
DEBUG_GET_ONCE_BOOL_OPTION(lifecycle_verbose, "OXR_LIFECYCLE_VERBOSE", false)
DEBUG_GET_ONCE_TRISTATE_OPTION(parallel_views, "OXR_PARALLEL_VIEWS")
DEBUG_GET_ONCE_TRISTATE_OPTION(no_texture_source_alpha, "OXR_NO_TEXTURE_SOURCE_ALPHA")
DEBUG_GET_ONCE_TRISTATE_OPTION(disable_quad_views, "OXR_DISABLE_QUAD_VIEWS")
DEBUG_GET_ONCE_BOOL_OPTION(map_stage_to_local_floor, "OXR_RECENTER_STAGE", false)


#ifdef XRT_OS_ANDROID
static bool
on_activity_lifecycle_state_changed(struct xrt_instance_android *xinst_android,
                                    enum xrt_android_lifecycle_event event,
                                    void *userdata)
{
	struct oxr_instance *inst = (struct oxr_instance *)userdata;
	inst->activity_state = event;

	// Return false to not be removed from the list of callbacks
	return false;
}
#endif // #ifdef XRT_OS_ANDROID

static XrResult
oxr_instance_destroy(struct oxr_logger *log, struct oxr_handle_base *hb)
{
	struct oxr_instance *inst = (struct oxr_instance *)hb;

	// Does a null-ptr check.
	xrt_syscomp_destroy(&inst->system.xsysc);

	u_var_remove_root((void *)inst);

	if (inst->action_context != NULL) {
		oxr_refcounted_unref(&inst->action_context->base);
		inst->action_context = NULL;
	}

	oxr_binding_registry_unref(inst->binding_registry);
	inst->binding_registry = NULL;

	// Maybe a no-op, needs to happen before path_store if not.
	oxr_instance_path_cache_fini(&inst->path_cache);

	oxr_path_store_fini(&inst->path_store);

	// Free the mask here, no system destroy yet.
	for (uint32_t i = 0; i < ARRAY_SIZE(inst->system.visibility_mask); i++) {
		free(inst->system.visibility_mask[i]);
		inst->system.visibility_mask[i] = NULL;
	}

	os_mutex_destroy(&inst->system_init_lock);
	os_mutex_destroy(&inst->sessions_mutex);

	if (inst->system.view_config_mutex_created) {
		os_mutex_destroy(&inst->system.view_config_mutex);
	}

	xrt_space_overseer_destroy(&inst->system.xso);
	xrt_system_devices_destroy(&inst->system.xsysd);
	xrt_app_system_destroy(&inst->system.xasys);
	xrt_system_destroy(&inst->system.xsys);

#ifdef XRT_FEATURE_CLIENT_DEBUG_GUI
	u_debug_gui_stop(&inst->debug_ui);
#endif

	xrt_app_instance_destroy(&inst->xainst);

	xrt_instance_destroy(&inst->xinst);

	// Does null checking and sets to null.
	time_state_destroy(&inst->timekeeping);

	// Mutex goes last.
	os_mutex_destroy(&inst->event.mutex);

	free(inst);

	return XR_SUCCESS;
}

static bool
starts_with(const char *with, const char *string)
{
	assert(with != NULL);

	if (string == NULL) {
		return false;
	}

	for (uint32_t i = 0; with[i] != 0; i++) {
		if (string[i] != with[i]) {
			return false;
		}
	}

	return true;
}

static void
debug_print_devices(struct oxr_logger *log, struct oxr_system *sys)
{
#define P(XDEV) (XDEV != NULL ? XDEV->str : "<none>")

	// Get all roles using oxr_roles struct.
	struct oxr_roles roles = XRT_STRUCT_INIT;
	XrResult result = oxr_roles_init_on_stack(log, &roles, sys);
	if (result != XR_SUCCESS) {
		// Just log and continue, this is debug output
		oxr_warn(log, "Failed to get device roles for debug printing");
		return;
	}

	struct xrt_device *h = GET_STATIC_XDEV_BY_ROLE(sys, head);
	struct xrt_device *e = GET_STATIC_XDEV_BY_ROLE(sys, eyes);
	struct xrt_device *l = GET_XDEV_BY_ROLE(&roles, left);
	struct xrt_device *r = GET_XDEV_BY_ROLE(&roles, right);
	struct xrt_device *gp = GET_XDEV_BY_ROLE(&roles, gamepad);
	struct xrt_device *uhl = GET_STATIC_XDEV_BY_ROLE(sys, hand_tracking_unobstructed_left);
	struct xrt_device *uhr = GET_STATIC_XDEV_BY_ROLE(sys, hand_tracking_unobstructed_right);
	struct xrt_device *chl = GET_STATIC_XDEV_BY_ROLE(sys, hand_tracking_conforming_left);
	struct xrt_device *chr = GET_STATIC_XDEV_BY_ROLE(sys, hand_tracking_conforming_right);

	oxr_log(log,
	        "Selected devices"
	        "\n\tHead: '%s'"
	        "\n\tEyes: '%s'"
	        "\n\tLeft: '%s'"
	        "\n\tRight: '%s'"
	        "\n\tGamepad: '%s'"
	        "\n\tHand-Tracking Left (unobstructed): '%s'"
	        "\n\tHand-Tracking Right (unobstructed): '%s'"
	        "\n\tHand-Tracking Left (conforming): '%s'"
	        "\n\tHand-Tracking Right (conforming): '%s'",
	        P(h), P(e), P(l), P(r), P(gp), P(uhl), P(uhr), P(chl), P(chr));

#undef P
}

static void
detect_engine(struct oxr_logger *log, struct oxr_instance *inst, const XrInstanceCreateInfo *createInfo)
{
	if (starts_with("UnrealEngine4", createInfo->applicationInfo.engineName)) {
		inst->appinfo.detected.engine.name = "UnrealEngine";
		inst->appinfo.detected.engine.major = 4;
		inst->appinfo.detected.engine.minor = (createInfo->applicationInfo.engineVersion >> 16) & 0xffff;
		inst->appinfo.detected.engine.patch = createInfo->applicationInfo.engineVersion & 0xffff;
	}

	if (starts_with("UnrealEngine5", createInfo->applicationInfo.engineName)) {
		inst->appinfo.detected.engine.name = "UnrealEngine";
		inst->appinfo.detected.engine.major = 5;
		inst->appinfo.detected.engine.minor = (createInfo->applicationInfo.engineVersion >> 16) & 0xffff;
		inst->appinfo.detected.engine.patch = createInfo->applicationInfo.engineVersion & 0xffff;
	}
}

static void
apply_quirks(struct oxr_logger *log, struct oxr_instance *inst, const XrInstanceCreateInfo *create_info)
{
	// Reset.
	inst->quirks.skip_end_session = false;
	inst->quirks.disable_vulkan_format_depth = false;
	inst->quirks.disable_vulkan_format_depth_stencil = false;
	inst->quirks.disable_quad_views = false;
	inst->quirks.no_validation_error_in_create_ref_space = false;
	inst->quirks.parallel_views = false;
	inst->quirks.no_texture_source_alpha = false;
	inst->quirks.no_usage_bit_validation_in_create_swapchain = false;
	inst->quirks.no_layer_flags_bit_validation = false;

	if (starts_with("UnrealEngine", inst->appinfo.detected.engine.name) && //
	    inst->appinfo.detected.engine.major == 4 &&                        //
	    inst->appinfo.detected.engine.minor <= 27) {
		inst->quirks.skip_end_session = true;
	}

	// This only works on Beat Saber <1.40.9, on newer versions
	// the application name is "Unity Application" which is too generic.
	if (strcmp("Beat Saber", create_info->applicationInfo.applicationName) == 0) {
		inst->quirks.parallel_views = true;
	}

	// Metro Awakening sets XR_SWAPCHAIN_USAGE_INPUT_ATTACHMENT_BIT_KHR without requesting the extension.
	if (strcmp("Impact", create_info->applicationInfo.applicationName) == 0) {
		inst->quirks.no_usage_bit_validation_in_create_swapchain = true;
	}

	// Microsoft Flight Simulator 2020 writes garbage in layerFlags for xrEndFrame
	if (strcmp("FS2020", create_info->applicationInfo.applicationName) == 0) {
		inst->quirks.no_layer_flags_bit_validation = true;
	}

	// Currently always true.
	inst->quirks.no_validation_error_in_create_ref_space = true;

	enum debug_tristate_option parallel_view = debug_get_tristate_option_parallel_views();
	enum debug_tristate_option no_texture_source_alpha = debug_get_tristate_option_no_texture_source_alpha();
	enum debug_tristate_option disable_quad_views = debug_get_tristate_option_disable_quad_views();

	// Only override hardcoded quirks when explicitly enabling or disabling, not on auto.
	if (parallel_view == DEBUG_TRISTATE_OFF) {
		inst->quirks.parallel_views = false;
	} else if (parallel_view == DEBUG_TRISTATE_ON) {
		inst->quirks.parallel_views = true;
	}

	if (no_texture_source_alpha == DEBUG_TRISTATE_OFF) {
		inst->quirks.no_texture_source_alpha = false;
	} else if (no_texture_source_alpha == DEBUG_TRISTATE_ON) {
		inst->quirks.no_texture_source_alpha = true;
	}

	if (disable_quad_views == DEBUG_TRISTATE_OFF) {
		inst->quirks.disable_quad_views = false;
	} else if (disable_quad_views == DEBUG_TRISTATE_ON) {
		inst->quirks.disable_quad_views = true;
	}


	inst->quirks.map_stage_to_local_floor = debug_get_bool_option_map_stage_to_local_floor();
}

XrResult
oxr_instance_create(struct oxr_logger *log,
                    const XrInstanceCreateInfo *createInfo,
                    XrVersion major_minor,
                    const struct oxr_extension_status *extensions,
                    struct oxr_instance **out_instance)
{
	struct oxr_instance *inst = NULL;
	int m_ret;
	xrt_result_t xret;
	XrResult ret;

	OXR_ALLOCATE_HANDLE_PARENT_OR_RETURN(log, inst, OXR_XR_DEBUG_INSTANCE, oxr_instance_destroy, NULL);

	inst->extensions = *extensions; // Sets the enabled extensions.
	inst->openxr_version.major_minor = major_minor;
	inst->lifecycle_verbose = debug_get_bool_option_lifecycle_verbose();
	inst->debug_spaces = debug_get_bool_option_debug_spaces();
	inst->debug_views = debug_get_bool_option_debug_views();
	inst->debug_bindings = debug_get_bool_option_debug_bindings();

	m_ret = os_mutex_init(&inst->event.mutex);
	if (m_ret < 0) {
		ret = oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to init mutex");
		return ret;
	}

	m_ret = os_mutex_init(&inst->system_init_lock);
	if (m_ret < 0) {
		ret = oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to init system init mutex");
		return ret;
	}

	m_ret = os_mutex_init(&inst->sessions_mutex);
	if (m_ret < 0) {
		ret = oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to init sessions mutex");
		return ret;
	}


#ifdef XRT_FEATURE_CLIENT_DEBUG_GUI
	struct u_debug_gui_create_info udgci = {
	    .window_title = "Monado! ✨⚡🔥",
	    .open = U_DEBUG_GUI_OPEN_AUTO,
	};

	u_debug_gui_create(&udgci, &inst->debug_ui);
#endif

	ret = oxr_path_store_init(&inst->path_store);
	if (ret != XR_SUCCESS) {
		return oxr_error(log, ret, "Failed to init path store");
	}

	// Cache certain often looked up paths.
	ret = oxr_instance_path_cache_init(&inst->path_cache, &inst->path_store);
	if (ret != XR_SUCCESS) {
		return oxr_error(log, ret, "Failed to init action path cache");
	}

	// Might use the path_store and path_cache, do after them.
	ret = oxr_binding_registry_create(log, &inst->path_store, &inst->binding_registry);
	if (ret != XR_SUCCESS) {
		return oxr_error(log, ret, "Failed to create binding registry");
	}

	ret = oxr_instance_action_context_create(log, inst->binding_registry, &inst->action_context);
	if (ret != XR_SUCCESS) {
		return oxr_error(log, ret, "Failed to create instance action context");
	}


	// fill in our application info - @todo - replicate all createInfo
	// fields?

	struct xrt_instance_info i_info = {0};
	i_info.app_info = (struct xrt_application_info){
#ifdef OXR_HAVE_EXT_hand_tracking
	    .ext_hand_tracking_enabled = extensions->EXT_hand_tracking,
#endif
#ifdef OXR_HAVE_EXT_hand_tracking_data_source
	    .ext_hand_tracking_data_source_enabled = extensions->EXT_hand_tracking_data_source,
#endif
#ifdef OXR_HAVE_EXT_eye_gaze_interaction
	    .ext_eye_gaze_interaction_enabled = extensions->EXT_eye_gaze_interaction,
#endif
#ifdef OXR_HAVE_EXT_future
	    .ext_future_enabled = extensions->EXT_future,
#endif
#ifdef OXR_HAVE_EXT_hand_interaction
	    .ext_hand_interaction_enabled = extensions->EXT_hand_interaction,
#endif
#ifdef OXR_HAVE_EXT_view_configuration_views_change
	    .view_configuration_views_change_supported = extensions->EXT_view_configuration_views_change,
#endif
#ifdef OXR_HAVE_HTC_facial_tracking
	    .htc_facial_tracking_enabled = extensions->HTC_facial_tracking,
#endif
#ifdef OXR_HAVE_FB_body_tracking
	    .fb_body_tracking_enabled = extensions->FB_body_tracking,
#endif
#ifdef OXR_HAVE_FB_face_tracking2
	    .fb_face_tracking2_enabled = extensions->FB_face_tracking2,
#endif
#ifdef OXR_HAVE_META_body_tracking_full_body
	    .meta_body_tracking_full_body_enabled = extensions->META_body_tracking_full_body,
#endif
#ifdef OXR_HAVE_META_body_tracking_calibration
	    .meta_body_tracking_calibration_enabled = extensions->META_body_tracking_calibration,
#endif
#ifdef OXR_HAVE_META_body_tracking_fidelity
	    .meta_body_tracking_fidelity_enabled = extensions->META_body_tracking_fidelity,
#endif
#ifdef OXR_HAVE_ANDROID_face_tracking
	    .android_face_tracking_enabled = extensions->ANDROID_face_tracking,
#endif
	};
	snprintf(i_info.app_info.application_name, sizeof(i_info.app_info.application_name), "%s",
	         createInfo->applicationInfo.applicationName);

#ifdef XRT_OS_ANDROID
	/// @todo should not depend on this, use loader init data instead
	XrInstanceCreateInfoAndroidKHR const *create_info_android = OXR_GET_INPUT_FROM_CHAIN(
	    createInfo, XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR, XrInstanceCreateInfoAndroidKHR);
	/// @todo should be removed once we find a proper way to access JavaVM/context through
	///       xrt_instance_android interface
	android_globals_store_vm_and_activity((struct _JavaVM *)create_info_android->applicationVM,
	                                      create_info_android->applicationActivity);
	i_info.platform_info.vm = (struct _JavaVM *)create_info_android->applicationVM;
	i_info.platform_info.context = create_info_android->applicationActivity;
#endif


	/*
	 * Monado initialisation.
	 */

	xret = xrt_instance_create(&i_info, &inst->xinst);
	if (xret != XRT_SUCCESS) {
		ret = oxr_error(log, XR_ERROR_RUNTIME_UNAVAILABLE, "Failed to create instance '%i'", xret);
		oxr_instance_destroy(log, &inst->handle.base);
		return ret;
	}

	xret = xrt_instance_create_app_instance(inst->xinst, &inst->xainst);
	if (xret != XRT_SUCCESS) {
		ret = oxr_error(log, XR_ERROR_RUNTIME_UNAVAILABLE, "Failed to create instance '%i'", xret);
		oxr_instance_destroy(log, &inst->handle.base);
		return ret;
	}

#ifdef XRT_OS_ANDROID
	xret = xrt_instance_android_register_activity_lifecycle_callback(
	    inst->xinst->android_instance, on_activity_lifecycle_state_changed,
	    XRT_ANDROID_LIVECYCLE_EVENT_ON_RESUME | XRT_ANDROID_LIVECYCLE_EVENT_ON_PAUSE, inst);
	// overlay application might be a service instead of an activity, so do not return error if
	// failed to register activity lifecycle callback.
	if (xret != XRT_SUCCESS) {
		oxr_warn(log, "Failed to register activity lifecycle callback '%i'", xret);
	}
#endif // XRT_OS_ANDROID

	inst->timekeeping = time_state_create(inst->xinst->startup_timestamp);

	//! @todo check if this (and other creates) failed?

	// Detect game engine.
	detect_engine(log, inst, createInfo);

	// Apply any quirks
	apply_quirks(log, inst, createInfo);

	u_var_add_root((void *)inst, "XrInstance", true);

	oxr_log(log,
	        "Instance created\n"
	        "\tcreateInfo->applicationInfo.applicationName: %s\n"
	        "\tcreateInfo->applicationInfo.applicationVersion: %i\n"
	        "\tcreateInfo->applicationInfo.engineName: %s\n"
	        "\tcreateInfo->applicationInfo.engineVersion: %i\n"
	        "\tcreateInfo->applicationInfo.apiVersion: %d.%d.%d\n"
	        "\tappinfo.detected.engine.name: %s\n"
	        "\tappinfo.detected.engine.version: %i.%i.%i\n"
	        "\tquirks.disable_vulkan_format_depth: %s\n"
	        "\tquirks.disable_vulkan_format_depth_stencil: %s\n"
	        "\tquirks.disable_quad_views: %s\n"
	        "\tquirks.no_validation_error_in_create_ref_space: %s\n"
	        "\tquirks.skip_end_session: %s\n"
	        "\tquirks.parallel_views: %s\n"
	        "\tquirks.no_texture_source_alpha: %s\n"
	        "\tquirks.ignore_invalid_swapchain_usage_bits: %s\n"
	        "\tquirks.ignore_invalid_layer_flags_bits: %s\n",
	        createInfo->applicationInfo.applicationName,                                 //
	        createInfo->applicationInfo.applicationVersion,                              //
	        createInfo->applicationInfo.engineName,                                      //
	        createInfo->applicationInfo.engineVersion,                                   //
	        XR_VERSION_MAJOR(createInfo->applicationInfo.apiVersion),                    //
	        XR_VERSION_MINOR(createInfo->applicationInfo.apiVersion),                    //
	        XR_VERSION_PATCH(createInfo->applicationInfo.apiVersion),                    //
	        inst->appinfo.detected.engine.name,                                          //
	        inst->appinfo.detected.engine.major,                                         //
	        inst->appinfo.detected.engine.minor,                                         //
	        inst->appinfo.detected.engine.patch,                                         //
	        inst->quirks.disable_vulkan_format_depth ? "true" : "false",                 //
	        inst->quirks.disable_vulkan_format_depth_stencil ? "true" : "false",         //
	        inst->quirks.disable_quad_views ? "true" : "false",                          //
	        inst->quirks.no_validation_error_in_create_ref_space ? "true" : "false",     //
	        inst->quirks.skip_end_session ? "true" : "false",                            //
	        inst->quirks.parallel_views ? "true" : "false",                              //
	        inst->quirks.no_texture_source_alpha ? "true" : "false",                     //
	        inst->quirks.no_usage_bit_validation_in_create_swapchain ? "true" : "false", //
	        inst->quirks.no_layer_flags_bit_validation ? "true" : "false"                //
	);                                                                                   //

#ifdef XRT_FEATURE_RENDERDOC

#ifdef __GNUC__
// Keep the warnings about normal usage of dlsym away.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif // __GNUC_

	pRENDERDOC_GetAPI RENDERDOC_GetAPI = NULL;

#if defined(XRT_OS_LINUX) && !defined(XRT_OS_ANDROID)
	void *mod = dlopen("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD);
	if (mod) {
		RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)dlsym(mod, "RENDERDOC_GetAPI");
	}
#endif
#ifdef XRT_OS_ANDROID
	void *mod = dlopen("libVkLayer_GLES_RenderDoc.so", RTLD_NOW | RTLD_NOLOAD);
	if (mod) {
		RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)dlsym(mod, "RENDERDOC_GetAPI");
	}
#endif
#ifdef XRT_OS_WINDOWS
	HMODULE mod = GetModuleHandleA("renderdoc.dll");
	if (mod) {
		RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
	}
#endif

	// If we got the symbol, try to get the API and assert it succeeds.
	if (RENDERDOC_GetAPI) {
		ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_5_0, (void **)&inst->rdoc_api);
		assert(ret == 1);
		if (ret != 1) {
			oxr_log(log, "RENDERDOC_GetAPI failed, got %d.", ret);
		}
	}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif // __GNUC_

#endif

	*out_instance = inst;

	return XR_SUCCESS;
}

XrResult
oxr_instance_init_system_locked(struct oxr_logger *log, struct oxr_instance *inst)
{
	struct oxr_system *sys = &inst->system;
	if (sys->xsys) {
		return XR_SUCCESS;
	}

	xrt_result_t xret;
	XrResult ret;

	bool available = false;
	xret = xrt_instance_is_system_available(inst->xinst, &available);
	if (xret != XRT_SUCCESS) {
		struct u_pp_sink_stack_only sink;
		u_pp_delegate_t dg = u_pp_sink_stack_only_init(&sink);
		u_pp(dg, "Call to xrt_instance_is_system_available failed: ");
		u_pp_xrt_result(dg, xret);
		ret = oxr_error(log, xret == XRT_ERROR_IPC_FAILURE ? XR_ERROR_INSTANCE_LOST : XR_ERROR_RUNTIME_FAILURE,
		                "%s", sink.buffer);
		return ret;
	}
	if (!available) {
		return XR_ERROR_FORM_FACTOR_UNAVAILABLE;
	}

	// Create the compositor if we are not headless, currently always create it.
	bool should_create_compositor = true /* !inst->extensions.MND_headless */;

	// Create the system.
	if (should_create_compositor) {
		xret = xrt_instance_create_system(inst->xinst, &sys->xsys, &sys->xsysd, &sys->xso, &sys->xsysc);
	} else {
		xret = xrt_instance_create_system(inst->xinst, &sys->xsys, &sys->xsysd, &sys->xso, NULL);
	}

	if (xret != XRT_SUCCESS) {
		struct u_pp_sink_stack_only sink;
		u_pp_delegate_t dg = u_pp_sink_stack_only_init(&sink);
		u_pp(dg, "Call to xrt_instance_create_system failed: ");
		u_pp_xrt_result(dg, xret);
		ret = oxr_error(log, XR_ERROR_INITIALIZATION_FAILED, "%s", sink.buffer);
		return ret;
	}

	int iret = os_mutex_init(&sys->view_config_mutex);
	if (iret < 0) {
		ret = oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Call to os_mutex_init failed: %d", iret);
		return ret;
	}
	sys->view_config_mutex_created = true;

	xret = xrt_app_instance_create_app_system(inst->xainst, sys->xsys, &sys->xasys);
	if (xret != XRT_SUCCESS) {
		struct u_pp_sink_stack_only sink;
		u_pp_delegate_t dg = u_pp_sink_stack_only_init(&sink);
		u_pp(dg, "Call to xrt_app_instance_create_app_system failed: ");
		u_pp_xrt_result(dg, xret);
		ret = oxr_error(log, XR_ERROR_INITIALIZATION_FAILED, "%s", sink.buffer);
		return ret;
	}

#ifdef XRT_FEATURE_CLIENT_DEBUG_GUI
	// Do this after creating the system.
	u_debug_gui_start(inst->debug_ui, inst->xinst, sys->xsysd);
#endif

	ret = XR_SUCCESS;
	if (sys->xsysd == NULL) {
		ret = oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Huh?! Field sys->xsysd was NULL?");
	} else if (should_create_compositor && sys->xsysc == NULL) {
		ret = oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Huh?! Field sys->xsysc was NULL?");
	} else if (!should_create_compositor && sys->xsysc != NULL) {
		ret = oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Huh?! Field sys->xsysc was not NULL?");
	}

	if (ret != XR_SUCCESS) {
		return ret;
	}

	// Did we find any HMD
	// @todo Headless with only controllers?
	struct xrt_device *dev = GET_STATIC_XDEV_BY_ROLE(sys, head);
	if (dev == NULL) {
		ret = oxr_error(log, XR_ERROR_RUNTIME_FAILURE, "Failed to find any HMD device");
		return ret;
	}
	uint32_t view_count = (uint32_t)dev->hmd->view_count;
	ret = oxr_system_fill_in(log, inst, XRT_SYSTEM_ID, view_count, &inst->system);
	if (ret != XR_SUCCESS) {
		return ret;
	}

	debug_print_devices(log, sys);

	return XR_SUCCESS;
}


XrResult
oxr_instance_get_properties(struct oxr_logger *log, struct oxr_instance *inst, XrInstanceProperties *instanceProperties)
{
	instanceProperties->runtimeVersion = XR_MAKE_VERSION(u_version_major, u_version_minor, u_version_patch);
	snprintf(instanceProperties->runtimeName, XR_MAX_RUNTIME_NAME_SIZE - 1, "%s '%s'", u_runtime_description,
	         u_git_tag);

	return XR_SUCCESS;
}

#ifdef XR_USE_TIMESPEC

XrResult
oxr_instance_convert_time_to_timespec(struct oxr_logger *log,
                                      struct oxr_instance *inst,
                                      XrTime time,
                                      struct timespec *timespecTime)
{
	time_state_to_timespec(inst->timekeeping, time, timespecTime);
	return XR_SUCCESS;
}

XrResult
oxr_instance_convert_timespec_to_time(struct oxr_logger *log,
                                      struct oxr_instance *inst,
                                      const struct timespec *timespecTime,
                                      XrTime *time)
{
	*time = time_state_from_timespec(inst->timekeeping, timespecTime);
	return XR_SUCCESS;
}
#endif // XR_USE_TIMESPEC

#ifdef XR_USE_PLATFORM_WIN32

XrResult
oxr_instance_convert_time_to_win32perfcounter(struct oxr_logger *log,
                                              struct oxr_instance *inst,
                                              XrTime time,
                                              LARGE_INTEGER *win32perfcounterTime)
{
	time_state_to_win32perfcounter(inst->timekeeping, time, win32perfcounterTime);
	return XR_SUCCESS;
}

XrResult
oxr_instance_convert_win32perfcounter_to_time(struct oxr_logger *log,
                                              struct oxr_instance *inst,
                                              const LARGE_INTEGER *win32perfcounterTime,
                                              XrTime *time)
{
	*time = time_state_from_win32perfcounter(inst->timekeeping, win32perfcounterTime);
	return XR_SUCCESS;
}

#endif // XR_USE_PLATFORM_WIN32
