#!/usr/bin/env python3
# Copyright 2019-2024, Collabora, Ltd.
# Copyright 2025-2026, NVIDIA CORPORATION.
# SPDX-License-Identifier: BSL-1.0
"""Generate OpenXR extension support header file."""

import argparse
import os
from string import Template
from typing import List


def _add_defined(s):
    if "defined" in s:
        return s
    return "defined({})".format(s)


def or_(*args):
    """
    Create an "OR" in the definition condition list.

    Takes any number of strings directly or through e.g. "not_".
    """
    return "({})".format(" || ".join(_add_defined(s) for s in args))


def not_(s):
    """
    Create a "NOT" in the condition list.

    Takes a single string, directly or through e.g. "or_".
    """
    return "(!{})".format(_add_defined(s))


# Each extension that we implement gets an entry in this tuple.
# Each entry should be a list of defines that are checked for an extension:
# the first one must be the name of the extension itself.
# The second and later items might be modified using or_() and not_().
# Keep sorted, KHR, EXT, Vendor, experimental (same order).
EXTENSIONS = (
    # Khronos extensions, sorted alphabetically.
    ['XR_KHR_android_create_instance', 'XR_USE_PLATFORM_ANDROID'],
    ['XR_KHR_android_thread_settings', 'XR_USE_PLATFORM_ANDROID'],
    ['XR_KHR_binding_modification'],
    ['XR_KHR_composition_layer_color_scale_bias', 'XRT_FEATURE_OPENXR_LAYER_COLOR_SCALE_BIAS'],
    ['XR_KHR_composition_layer_cube', 'XRT_FEATURE_OPENXR_LAYER_CUBE'],
    ['XR_KHR_composition_layer_cylinder', 'XRT_FEATURE_OPENXR_LAYER_CYLINDER'],
    ['XR_KHR_composition_layer_depth', 'XRT_FEATURE_OPENXR_LAYER_DEPTH'],
    ['XR_KHR_composition_layer_equirect', 'XRT_FEATURE_OPENXR_LAYER_EQUIRECT1'],
    ['XR_KHR_composition_layer_equirect2', 'XRT_FEATURE_OPENXR_LAYER_EQUIRECT2'],
    ['XR_KHR_convert_timespec_time', 'XR_USE_TIMESPEC', not_('XR_USE_PLATFORM_WIN32')],
    ['XR_KHR_D3D11_enable', 'XR_USE_GRAPHICS_API_D3D11'],
    ['XR_KHR_D3D12_enable', 'XR_USE_GRAPHICS_API_D3D12'],
    ['XR_KHR_extended_struct_name_lengths'],
    ['XR_KHR_generic_controller', 'XRT_FEATURE_OPENXR_INTERACTION_KHR_GENERIC'],
    ['XR_KHR_loader_init', 'XR_USE_PLATFORM_ANDROID'],
    ['XR_KHR_loader_init_android', 'OXR_HAVE_KHR_loader_init', 'XR_USE_PLATFORM_ANDROID'],
    ['XR_KHR_locate_spaces'],
    ['XR_KHR_maintenance1'],
    ['XR_KHR_opengl_enable', 'XR_USE_GRAPHICS_API_OPENGL'],
    ['XR_KHR_opengl_es_enable', 'XR_USE_GRAPHICS_API_OPENGL_ES'],
    ['XR_KHR_swapchain_usage_input_attachment_bit'],
    ['XR_KHR_visibility_mask', 'XRT_FEATURE_OPENXR_VISIBILITY_MASK'],
    ['XR_KHR_vulkan_enable', 'XR_USE_GRAPHICS_API_VULKAN'],
    ['XR_KHR_vulkan_enable2', 'XR_USE_GRAPHICS_API_VULKAN'],
    ['XR_KHR_vulkan_swapchain_format_list', 'XR_USE_GRAPHICS_API_VULKAN', 'XRT_FEATURE_OPENXR_VULKAN_SWAPCHAIN_FORMAT_LIST'],
    ['XR_KHR_win32_convert_performance_counter_time', 'XR_USE_PLATFORM_WIN32'],
    # EXT extensions, sorted alphabetically.
    ['XR_EXT_active_action_set_priority', 'XRT_FEATURE_OPENXR_ACTIVE_ACTION_SET_PRIORITY'],
    ['XR_EXT_composition_layer_inverted_alpha', 'XRT_FEATURE_OPENXR_INVERTED_ALPHA'],
    ['XR_EXT_debug_utils', 'XRT_FEATURE_OPENXR_DEBUG_UTILS'],
    ['XR_EXT_dpad_binding'],
    ['XR_EXT_enumerate_swapchain_formats2', 'XRT_FEATURE_OPENXR_ENUMERATE_SWAPCHAIN_FORMATS2'],
    ['XR_EXT_eye_gaze_interaction', 'XRT_FEATURE_OPENXR_INTERACTION_EXT_EYE_GAZE'],
    ['XR_EXT_future', 'XRT_FEATURE_OPENXR_FUTURE_EXT'],
    ['XR_EXT_hand_interaction', 'XRT_FEATURE_OPENXR_INTERACTION_EXT_HAND'],
    ['XR_EXT_hand_tracking', 'XRT_FEATURE_OPENXR_HAND_TRACKING_EXT'],
    ['XR_EXT_hand_tracking_data_source', 'XRT_FEATURE_OPENXR_HAND_TRACKING_DATA_SOURCE_EXT'],
    ['XR_EXT_hp_mixed_reality_controller', 'XRT_FEATURE_OPENXR_INTERACTION_WINMR'],
    ['XR_EXT_local_floor', 'XRT_FEATURE_OPENXR_SPACE_LOCAL_FLOOR'],
    ['XR_EXT_palm_pose', 'XRT_FEATURE_OPENXR_INTERACTION_EXT_PALM_POSE'],
    ['XR_EXT_performance_settings', 'XRT_FEATURE_OPENXR_PERFORMANCE_SETTINGS'],
    ['XR_EXT_plane_detection', 'XRT_FEATURE_OPENXR_PLANE_DETECTION'],
    ['XR_EXT_samsung_odyssey_controller', 'XRT_FEATURE_OPENXR_INTERACTION_WINMR'],
    ['XR_EXT_user_presence', 'XRT_FEATURE_OPENXR_USER_PRESENCE'],
    ['XR_EXT_view_configuration_views_change', 'XRT_FEATURE_OPENXR_VIEW_CONFIGURATION_VIEWS_CHANGE'],
    # Vendor extensions, sorted alphabetically.
    ['XR_ANDROID_face_tracking', 'XRT_FEATURE_OPENXR_FACE_TRACKING_ANDROID'],
    ['XR_BD_body_tracking', 'XRT_FEATURE_OPENXR_BODY_TRACKING_BD'],
    ['XR_BD_controller_interaction', 'XRT_FEATURE_OPENXR_INTERACTION_BYTEDANCE'],
    ['XR_FB_body_tracking', 'XRT_FEATURE_OPENXR_BODY_TRACKING_FB'],
    ['XR_FB_composition_layer_alpha_blend', 'XRT_FEATURE_OPENXR_LAYER_FB_ALPHA_BLEND'],
    ['XR_FB_composition_layer_depth_test', 'XRT_FEATURE_OPENXR_LAYER_FB_DEPTH_TEST'],
    ['XR_FB_composition_layer_image_layout', 'XRT_FEATURE_OPENXR_LAYER_FB_IMAGE_LAYOUT'],
    ['XR_FB_composition_layer_settings', 'XRT_FEATURE_OPENXR_LAYER_FB_SETTINGS'],
    ['XR_FB_display_refresh_rate', 'XRT_FEATURE_OPENXR_DISPLAY_REFRESH_RATE'],
    ['XR_FB_face_tracking2', 'XRT_FEATURE_OPENXR_FACE_TRACKING2_FB'],
    ['XR_FB_haptic_pcm', 'XRT_FEATURE_OPENXR_HAPTIC_PCM'],
    ['XR_FB_passthrough', 'XRT_FEATURE_OPENXR_LAYER_FB_PASSTHROUGH'],
    ['XR_FB_touch_controller_pro', 'XRT_FEATURE_OPENXR_INTERACTION_TOUCH_PRO'],
    ['XR_FB_touch_controller_proximity', 'XRT_FEATURE_OPENXR_INTERACTION_FB_PROXIMITY'],
    ['XR_HTC_facial_tracking', 'XRT_FEATURE_OPENXR_FACIAL_TRACKING_HTC'],
    ['XR_HTC_vive_cosmos_controller_interaction', 'XRT_FEATURE_OPENXR_INTERACTION_VIVE_COSMOS'],
    ['XR_HTC_vive_focus3_controller_interaction', 'XRT_FEATURE_OPENXR_INTERACTION_VIVE_FOCUS3'],
    ['XR_LOGITECH_mx_ink_stylus_interaction', 'XRT_FEATURE_OPENXR_INTERACTION_LOGITECH_MX_INK'],
    ['XR_META_body_tracking_calibration', 'XRT_FEATURE_OPENXR_BODY_TRACKING_CALIBRATION_META'],
    ['XR_META_body_tracking_fidelity', 'XRT_FEATURE_OPENXR_BODY_TRACKING_FIDELITY_META'],
    ['XR_META_body_tracking_full_body', 'XRT_FEATURE_OPENXR_BODY_TRACKING_FULL_BODY_META'],
    ['XR_META_touch_controller_plus', 'XRT_FEATURE_OPENXR_INTERACTION_TOUCH_PLUS'],
    ['XR_ML_ml2_controller_interaction', 'XRT_FEATURE_OPENXR_INTERACTION_ML2'],
    ['XR_MND_headless', 'XRT_FEATURE_OPENXR_HEADLESS'],
    ['XR_MND_query_egl_device', 'XR_USE_PLATFORM_EGL'],
    ['XR_MND_swapchain_usage_input_attachment_bit'],
    ['XR_MND_contactglove2_interaction', 'XRT_FEATURE_OPENXR_INTERACTION_CONTACTGLOVE2'],
    ['XR_MSFT_hand_interaction', 'XRT_FEATURE_OPENXR_INTERACTION_MSFT_HAND'],
    ['XR_MSFT_unbounded_reference_space', 'XRT_FEATURE_OPENXR_SPACE_UNBOUNDED'],
    ['XR_OPPO_controller_interaction', 'XRT_FEATURE_OPENXR_INTERACTION_OPPO'],
    ['XR_VARJO_quad_views', 'XRT_FEATURE_OPENXR_VARJO_QUAD_VIEWS'],
    # Experimental extensions, sorted alphabetically.
    ['XR_EXTX_overlay', 'XRT_FEATURE_OPENXR_OVERLAY'],
    ['XR_HTCX_vive_tracker_interaction', 'ALWAYS_DISABLED'],
    ['XR_MNDX_ball_on_a_stick_controller', 'XRT_FEATURE_OPENXR_INTERACTION_MNDX'],
    ['XR_MNDX_blubur_s1', 'XRT_FEATURE_OPENXR_INTERACTION_MNDX'],
    ['XR_MNDX_egl_enable', 'XR_USE_PLATFORM_EGL'],
    ['XR_MNDX_flipvr', 'XRT_FEATURE_OPENXR_INTERACTION_MNDX'],
    ['XR_MNDX_force_feedback_curl', 'XRT_FEATURE_OPENXR_FORCE_FEEDBACK_CURL'],
    ['XR_MNDX_hydra', 'XRT_FEATURE_OPENXR_INTERACTION_MNDX'],
    ['XR_MNDX_oculus_remote', 'XRT_FEATURE_OPENXR_INTERACTION_MNDX'],
    ['XR_MNDX_psvr2_interaction', 'XRT_FEATURE_OPENXR_INTERACTION_MNDX'],
    ['XR_MNDX_system_buttons', 'XRT_FEATURE_OPENXR_INTERACTION_MNDX'],
    ['XR_MNDX_xdev_space', 'XRT_FEATURE_OPENXR_XDEV_SPACE'],
)

INVOCATION_PREFIX = 'OXR_EXTENSION_SUPPORT'


def trim_ext_name(name):
    return name[3:]


def generate_ext_defines() -> str:
    """Generate per-extension define blocks."""
    parts = []
    for data in EXTENSIONS:
        ext_name = data[0]
        trimmed_name = trim_ext_name(ext_name)
        upper_name = trimmed_name.upper()
        condition = " && ".join(_add_defined(x) for x in data)

        parts.append(f"""
/*
 * {ext_name}
 */
#if {condition}
#define OXR_HAVE_{trimmed_name}
#define {INVOCATION_PREFIX}_{trimmed_name}(_) \\
    _({trimmed_name}, {upper_name})
#else
#define {INVOCATION_PREFIX}_{trimmed_name}(_)
#endif
""")
    return "\n".join(parts)


def generate_generate_macro() -> str:
    """Generate the OXR_EXTENSION_SUPPORT_GENERATE macro."""
    trimmed_names = [trim_ext_name(data[0]) for data in EXTENSIONS]
    invocations = ('{}_{}(_)'.format(INVOCATION_PREFIX, name)
                   for name in trimmed_names)

    macro_lines = ['#define OXR_EXTENSION_SUPPORT_GENERATE(_)']
    macro_lines.extend(invocations)

    lines = ['// clang-format off']
    lines.append(' \\\n    '.join(macro_lines))
    lines.append('// clang-format on')
    return "\n".join(lines)


def generate_extension_support_h(output: str):
    """Generate the oxr_extension_support.h file from template."""
    script_dir = os.path.dirname(os.path.abspath(__file__))
    template_file = os.path.join(script_dir, 'oxr_extension_support.h.template')

    with open(template_file, 'r') as f:
        src = Template(f.read())

    ext_defines = generate_ext_defines()
    generate_macro = generate_generate_macro()

    with open(output, "w", encoding="utf-8") as fp:
        fp.write(src.substitute(
            ext_defines=ext_defines,
            generate_macro=generate_macro
        ))


def main():
    """Handle command line and generate file(s)."""
    parser = argparse.ArgumentParser(description='OpenXR extension support generator.')
    parser.add_argument(
        'output', type=str, nargs='+',
        help='Output file, uses the name to choose output type')
    args = parser.parse_args()

    for output in args.output:
        if output.endswith("oxr_extension_support.h"):
            generate_extension_support_h(output)
        else:
            raise ValueError(f"Unknown output file: {output}")


if __name__ == "__main__":
    main()
