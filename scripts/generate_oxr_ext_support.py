#!/usr/bin/env python3
# Copyright 2019-2022, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0
"""Simple script to update oxr_extension_support.h."""

from pathlib import Path

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
    ['XR_KHR_loader_init', 'XR_USE_PLATFORM_ANDROID'],
    ['XR_KHR_loader_init_android', 'OXR_HAVE_KHR_loader_init', 'XR_USE_PLATFORM_ANDROID'],
    ['XR_KHR_opengl_enable', 'XR_USE_GRAPHICS_API_OPENGL'],
    ['XR_KHR_opengl_es_enable', 'XR_USE_GRAPHICS_API_OPENGL_ES'],
    ['XR_KHR_swapchain_usage_input_attachment_bit'],
    ['XR_KHR_visibility_mask', 'XRT_FEATURE_OPENXR_VISIBILITY_MASK'],
    ['XR_KHR_vulkan_enable', 'XR_USE_GRAPHICS_API_VULKAN'],
    ['XR_KHR_vulkan_enable2', 'XR_USE_GRAPHICS_API_VULKAN'],
    ['XR_KHR_vulkan_swapchain_format_list', 'XR_USE_GRAPHICS_API_VULKAN', 'XRT_FEATURE_OPENXR_VULKAN_SWAPCHAIN_FORMAT_LIST'],
    ['XR_KHR_win32_convert_performance_counter_time', 'XR_USE_PLATFORM_WIN32'],
    ['XR_EXT_debug_utils', 'XRT_FEATURE_OPENXR_DEBUG_UTILS'],
    ['XR_EXT_dpad_binding'],
    ['XR_EXT_eye_gaze_interaction', 'XRT_FEATURE_OPENXR_INTERACTION_EXT_EYE_GAZE'],
    ['XR_EXT_hand_interaction', 'XRT_FEATURE_OPENXR_INTERACTION_EXT_HAND'],
    ['XR_EXT_hand_tracking'],
    ['XR_EXT_hp_mixed_reality_controller', 'XRT_FEATURE_OPENXR_INTERACTION_WINMR'],
    ['XR_EXT_local_floor', 'XRT_FEATURE_OPENXR_SPACE_LOCAL_FLOOR'],
    ['XR_EXT_palm_pose', 'XRT_FEATURE_OPENXR_INTERACTION_EXT_PALM_POSE'],
    ['XR_EXT_performance_settings', 'XRT_FEATURE_OPENXR_PERFORMANCE_SETTINGS'],
    ['XR_EXT_samsung_odyssey_controller', 'XRT_FEATURE_OPENXR_INTERACTION_WINMR'],
    ['XR_FB_composition_layer_alpha_blend', 'XRT_FEATURE_OPENXR_LAYER_FB_ALPHA_BLEND'],
    ['XR_FB_composition_layer_image_layout', 'XRT_FEATURE_OPENXR_LAYER_FB_IMAGE_LAYOUT'],
    ['XR_FB_composition_layer_settings', 'XRT_FEATURE_OPENXR_LAYER_FB_SETTINGS'],
    ['XR_FB_display_refresh_rate', 'XRT_FEATURE_OPENXR_DISPLAY_REFRESH_RATE'],
    ['XR_ML_ml2_controller_interaction', 'XRT_FEATURE_OPENXR_INTERACTION_ML2'],
    ['XR_MND_headless', 'XRT_FEATURE_OPENXR_HEADLESS'],
    ['XR_MND_swapchain_usage_input_attachment_bit'],
    ['XR_MSFT_hand_interaction', 'XRT_FEATURE_OPENXR_INTERACTION_MSFT_HAND'],
    ['XR_MSFT_unbounded_reference_space', 'XRT_FEATURE_OPENXR_SPACE_UNBOUNDED'],
    ['XR_OPPO_controller_interaction', 'XRT_FEATURE_OPENXR_INTERACTION_OPPO'],
    ['XR_EXTX_overlay', 'XRT_FEATURE_OPENXR_OVERLAY'],
    ['XR_HTCX_vive_tracker_interaction', 'ALWAYS_DISABLED'],
    ['XR_MNDX_ball_on_a_stick_controller', 'XRT_FEATURE_OPENXR_INTERACTION_MNDX'],
    ['XR_MNDX_egl_enable', 'XR_USE_PLATFORM_EGL', 'XR_USE_GRAPHICS_API_OPENGL'],
    ['XR_MNDX_force_feedback_curl', 'XRT_FEATURE_OPENXR_FORCE_FEEDBACK_CURL'],
    ['XR_MNDX_hydra', 'XRT_FEATURE_OPENXR_INTERACTION_MNDX'],
    ['XR_MNDX_system_buttons', 'XRT_FEATURE_OPENXR_INTERACTION_MNDX'],
)


ROOT = Path(__file__).resolve().parent.parent
FN = ROOT / 'src' / 'xrt'/'state_trackers' / 'oxr' / 'oxr_extension_support.h'

INVOCATION_PREFIX = 'OXR_EXTENSION_SUPPORT'
BEGIN_OF_PER_EXTENSION = '// beginning of GENERATED defines - do not modify - used by scripts'
END_OF_PER_EXTENSION = '// end of GENERATED per-extension defines - do not modify - used by scripts'
CLANG_FORMAT_OFF = '// clang-format off'
CLANG_FORMAT_ON = '// clang-format on'


def trim_ext_name(name):
    return name[3:]


def generate_first_chunk():
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
    return '\n'.join(parts)


def generate_second_chunk():
    trimmed_names = [trim_ext_name(data[0]) for data in EXTENSIONS]
    invocations = ('{}_{}(_)'.format(INVOCATION_PREFIX, name)
                   for name in trimmed_names)

    macro_lines = ['#define OXR_EXTENSION_SUPPORT_GENERATE(_)']
    macro_lines.extend(invocations)

    lines = [CLANG_FORMAT_OFF]
    lines.append(' \\\n    '.join(macro_lines))
    lines.append(CLANG_FORMAT_ON)
    return '\n'.join(lines)


if __name__ == "__main__":
    with open(str(FN), 'r', encoding='utf-8') as fp:
        orig = [line.rstrip() for line in fp.readlines()]
    beginning = orig[:orig.index(BEGIN_OF_PER_EXTENSION)+1]
    middle_start = orig.index(END_OF_PER_EXTENSION)
    middle_end = orig.index(CLANG_FORMAT_OFF)
    middle = orig[middle_start:middle_end]

    new_contents = beginning
    new_contents.append(generate_first_chunk())
    new_contents.extend(middle)
    new_contents.append(generate_second_chunk())

    with open(str(FN), 'w', encoding='utf-8') as fp:
        fp.write('\n'.join(new_contents))
        fp.write('\n')
