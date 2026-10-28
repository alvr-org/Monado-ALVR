#!/usr/bin/env python3
# Copyright 2020-2025, Collabora, Ltd.
# Copyright 2024-2026, NVIDIA CORPORATION.
# SPDX-License-Identifier: BSL-1.0
"""Generate OpenXR binding code using the auxiliary bindings scripts."""

from bindings import *

import argparse
import json
import os
from operator import attrgetter
from string import Template

def get_verify_switch_body(dict_of_lists, profile, profile_name, tab_char):
    """Generate function to check if a string is in a set of strings.
    Input is a dict where keys are length and
    the values are lists of strings of that length. And a suffix if any.
    Returns a list of lines."""
    ret = [ f"{tab_char}\tswitch (length) {{" ]
    for length in sorted(dict_of_lists.keys()):
        ret += [
            f'{tab_char}\tcase {str(length)}:',
        ]

        for i, path in enumerate(sorted(dict_of_lists[length])):
            if_or_else_if = 'if' if i == 0 else '} else if'
            ret += [
                f'{tab_char}\t\t{if_or_else_if} (strcmp(str, "{path}") == 0) {{',
                f'{tab_char}\t\t\treturn true;',
             ]
        ret += [
            f'{tab_char}\t\t}}',
            f'{tab_char}\t\tbreak;',
        ]
    ret += [
        f'{tab_char}\tdefault: break;',
        f'{tab_char}\t}}',
    ]
    return ret

def get_verify_func_switch(dict_of_lists, profile, profile_name, availability):
    """Generate function to check if a string is in a set of strings.
    Input is a dict where keys are length and
    the values are lists of strings of that length. And a suffix if any.
    Returns a list of lines."""
    if len(dict_of_lists) == 0:
        return ''

    ret = [
        f"\t// generated from: {profile_name}"
    ]

    # Example: pico neo 3 can be enabled by either enabling XR_BD_controller_interaction ext or using OpenXR 1.1+.
    # Disabling OXR_HAVE_BD_controller_interaction should NOT remove pico neo from OpenXR 1.1+ (it makes "exts->BD_controller_interaction" invalid C code).
    # Therefore separate code blocks for ext and version checks generated to avoid ifdef hell.
    feature_sets = sorted(availability.feature_sets, key=FeatureSet.as_tuple)
    for feature_set in feature_sets:
        requires_version = is_valid_version(feature_set.required_version)
        requires_extensions = bool(feature_set.required_extensions)

        tab_char = ''
        closing = []

        if requires_version:
            tab_char += '\t'
            ret += [
                f'{tab_char}if (openxr_version >= XR_MAKE_VERSION({feature_set.required_version["major"]}, {feature_set.required_version["minor"]}, 0)) {{',
            ]
            closing.append(f'{tab_char}}}\n')

        if requires_extensions:
            tab_char += '\t'
            exts = sorted(feature_set.required_extensions)
            ext_defines = ' && '.join(f'defined(OXR_HAVE_{ext})' for ext in exts)
            ret += [
                f'#if {ext_defines}',
                f'{tab_char}if ('+' && '.join(f'exts->{ext}' for ext in exts)+') {',
            ]
            closing.append(f'{tab_char}}}\n#endif // {ext_defines}\n\n')

        ret += get_verify_switch_body(dict_of_lists, profile, profile_name, tab_char)

        ret += reversed(closing)

    return ret

def get_verify_func_body(profile, dict_name, availability):
    """returns a list of lines"""
    ret = []
    if profile is None or dict_name is None or len(dict_name) == 0:
        return ret
    ret += get_verify_func_switch(getattr(
        profile, dict_name), profile, profile.name, availability)
    if profile.parent_profiles is None:
        return ret
    for pp in sorted(profile.parent_profiles, key=attrgetter("name")):
        ret += get_verify_func_body(pp, dict_name, availability.intersection(pp.availability()))

    return ret


def get_verify_func(profile, dict_name, suffix):
    """returns a list of lines"""
    name = f"oxr_verify_{profile.validation_func_name}{suffix}"

    ret = [
        'bool',
        '{name}(const struct oxr_extension_status *exts, XrVersion openxr_version, const char *str, size_t length)'.format(name=name),
        '{',

        *get_verify_func_body(profile, dict_name, profile.availability()),

        '\treturn false;',
        '}',
        ''
    ]

    return ret

def get_verify_functions(profile):
    """returns a list of lines"""
    ret = [
        *get_verify_func(profile, "subpaths_by_length", "_subpath"),
        *get_verify_func(profile, "dpad_paths_by_length", "_dpad_path"),
        *get_verify_func(profile, "dpad_emulators_by_length", "_dpad_emulator"),

        'void',
        f'oxr_verify_{profile.validation_func_name}_ext(const struct oxr_extension_status *extensions, XrVersion openxr_version, bool *out_supported, bool *out_enabled)',
        '{',
        '',
    ]

    is_promoted = is_valid_version(profile.openxr_version_promoted)
    if is_promoted:
        ret += [
            f'\tif (openxr_version >= XR_MAKE_VERSION({profile.openxr_version_promoted["major"]}, {profile.openxr_version_promoted["minor"]}, 0)) {{',
            '\t\t*out_supported = true;',
            '\t\t*out_enabled = true;',
            '\t\treturn;',
            '\t}',
            '',
        ]


    if profile.extension_name is not None:
        ret += [
            f'#ifdef OXR_HAVE_{profile.extension_name}',
            '\t*out_supported = true;',
            f'\t*out_enabled = extensions->{profile.extension_name};',
            '#else',
            '\t*out_supported = false;',
            '\t*out_enabled = false;',
            f'#endif // OXR_HAVE_{profile.extension_name}',
            '}',
            '',
        ]

    else:
        ret += [
            '\t*out_supported = true;',
            '\t*out_enabled = true;',
            '}',
            '',
        ]

    return ret


def get_component_binding_point_paths(component):
    """Return every concrete binding point path emitted for a component."""
    basepath = component.subaction_path + component.identifier_json_path
    if component.component_name == "position":
        return [
            basepath + "/x",
            basepath + "/y",
            basepath + "/xy",
        ]
    if component.component_name == "haptic":
        return [basepath]
    return [component.get_full_path()]


def quote_str_or_null(value):
    if value is None:
        return "NULL"
    return f'"{value}"'


def get_upgrade_target_paths(identifier):
    """Return concrete binding point paths for each action kind upgrade."""
    targets = {}
    components = {component.component_name: component for component in identifier.components}

    if "click" in components:
        targets["boolean"] = components["click"].get_full_path()
    elif "value" in components:
        targets["boolean"] = components["value"].get_full_path()

    if "value" in components:
        targets["float"] = components["value"].get_full_path()
    elif "click" in components:
        targets["float"] = components["click"].get_full_path()

    for component in identifier.components:
        if component.component_name == "position":
            basepath = component.subaction_path + component.identifier_json_path
            targets["vec2"] = basepath + "/xy"
        elif component.component_name == "pose":
            targets["pose"] = component.get_full_path()
        elif component.component_name == "haptic":
            targets["haptic"] = component.subaction_path + component.identifier_json_path

    return {
        action_kind: path
        for action_kind, path in targets.items()
        if path != identifier.identifier_path
    }


def should_generate_upgrade(identifier):
    return len(get_upgrade_target_paths(identifier)) > 0


def get_dpad_binding_point_enum(path):
    if path.endswith("/dpad_up"):
        return "OXR_DPAD_BINDING_POINT_UP"
    if path.endswith("/dpad_down"):
        return "OXR_DPAD_BINDING_POINT_DOWN"
    if path.endswith("/dpad_left"):
        return "OXR_DPAD_BINDING_POINT_LEFT"
    if path.endswith("/dpad_right"):
        return "OXR_DPAD_BINDING_POINT_RIGHT"
    if path.endswith("/dpad_center"):
        return "OXR_DPAD_BINDING_POINT_CENTER"
    return "OXR_DPAD_BINDING_POINT_NONE"


def get_component_priority_group(component):
    return component.subaction_path + component.identifier_json_path


def get_binding_value_transform(binding_path):
    if binding_path.endswith("/x"):
        return "OXR_BINDING_VALUE_TRANSFORM_VEC2_GET_X"
    if binding_path.endswith("/y"):
        return "OXR_BINDING_VALUE_TRANSFORM_VEC2_GET_Y"
    return "OXR_BINDING_VALUE_TRANSFORM_NONE"


def get_profile_binding_point_paths(profile):
    """Return all paths accepted by generated binding point templates."""
    paths = set()

    for identifier in profile.identifiers:
        if should_generate_upgrade(identifier):
            paths.add(identifier.identifier_path)

    for component in profile.components:
        paths.update(get_component_binding_point_paths(component))

    for identifier in profile.identifiers:
        if identifier.dpad:
            paths.update(identifier.dpad.paths)

    return paths


def validate_profile_templates(profile):
    """Validate generated profile template metadata before writing C code."""
    generated_paths = get_profile_binding_point_paths(profile)
    errors = []

    for identifier in profile.identifiers:
        for target_path in get_upgrade_target_paths(identifier).values():
            if target_path not in generated_paths:
                errors.append(
                    f"{profile.name}: upgrade path {identifier.identifier_path} targets missing binding point "
                    f"{target_path}")

    for component in profile.components:
        priority_group = get_component_priority_group(component)
        if not priority_group:
            errors.append(f"{profile.name}: component {component.get_full_path()} has empty priority group")

    for identifier in profile.identifiers:
        if identifier.dpad is None:
            continue
        if identifier.dpad.position_component is None:
            errors.append(f"{profile.name}: dpad {identifier.identifier_path} has no position component")
        for dpad_path in identifier.dpad.paths:
            if get_dpad_binding_point_enum(dpad_path) == "OXR_DPAD_BINDING_POINT_NONE":
                errors.append(f"{profile.name}: dpad path {dpad_path} has no generated region")

    if errors:
        raise ValueError("\n".join(errors))


def validate_rebindings(bindings, rebindings):
    """Validate rebinding JSON against generated binding point paths."""
    profiles_by_name = {profile.name: profile for profile in bindings.profiles}
    binding_paths_by_profile = {
        profile.name: get_profile_binding_point_paths(profile)
        for profile in bindings.profiles
    }

    errors = []
    seen_keys = set()

    for name, rebinding in sorted(rebindings.items()):
        if is_rebinding_helper(rebinding) and not isinstance(rebinding.get("bindings", {}), dict):
            errors.append(f"{name}: helper bindings must be an object")
            continue

        if not isinstance(rebinding.get("helpers", []), list):
            errors.append(f"{name}: helpers must be an array")
            continue

        if "bindings" in rebinding and not isinstance(rebinding.get("bindings"), dict):
            errors.append(f"{name}: bindings must be an object mapping source paths to true, false, or target paths")
            continue

        if is_rebinding_helper(rebinding):
            get_expanded_rebinding_entries(name, rebinding, rebindings, errors)
            continue

        missing_required = False
        for required_key in ["source_profile", "target_profile"]:
            if required_key not in rebinding:
                errors.append(f"{name}: missing required key {required_key}")
                missing_required = True
        if missing_required:
            continue

        source_profile = rebinding.get("source_profile")
        target_profile = rebinding.get("target_profile")
        key = (source_profile, target_profile)
        if key in seen_keys:
            errors.append(f"{name}: duplicate rebinding key {key}")
        seen_keys.add(key)

        if source_profile not in profiles_by_name:
            errors.append(f"{name}: unknown source profile {source_profile}")
        if target_profile not in profiles_by_name:
            errors.append(f"{name}: unknown target profile {target_profile}")

        source_paths = binding_paths_by_profile.get(source_profile, set())
        target_paths = binding_paths_by_profile.get(target_profile, set())
        seen_sources = set()

        rebinding_entries = get_expanded_rebinding_entries(name, rebinding, rebindings, errors)
        if not isinstance(rebinding_entries, dict):
            errors.append(f"{name}: bindings must be an object mapping source paths to true, false, or target paths")
            continue

        missing_sources = sorted(source_paths - set(rebinding_entries.keys()))
        for source in missing_sources:
            errors.append(f"{name}: missing source rebinding path {source}")

        for source, target_value in rebinding_entries.items():
            target = None
            if target_value is True:
                target = source
            elif target_value is False:
                target = None
            elif isinstance(target_value, str):
                target = target_value
            else:
                errors.append(f"{name}: rebinding value for {source} must be true, false, or a target path string")
                continue
            if source in seen_sources:
                errors.append(f"{name}: duplicate source rebinding path {source}")
            seen_sources.add(source)
            if source not in source_paths:
                errors.append(f"{name}: source path {source} is not generated by {source_profile}")
            if target is not None and target not in target_paths:
                errors.append(f"{name}: target path {target} is not generated by {target_profile}")

    if errors:
        raise ValueError("\n".join(errors))


def is_rebinding_helper(rebinding):
    return rebinding.get("helper", False) is True


def get_expanded_rebinding_entries(name, rebinding, rebindings, errors=None, stack=None):
    if stack is None:
        stack = []

    if name in stack:
        if errors is not None:
            cycle = " -> ".join(stack + [name])
            errors.append(f"{name}: circular rebinding helper dependency: {cycle}")
        return {}

    entries = {}
    stack = stack + [name]
    for helper_name in rebinding.get("helpers", []):
        helper = rebindings.get(helper_name)
        if helper is None:
            if errors is not None:
                errors.append(f"{name}: unknown rebinding helper {helper_name}")
            continue
        if not is_rebinding_helper(helper):
            if errors is not None:
                errors.append(f"{name}: rebinding helper {helper_name} is not marked as a helper")
            continue
        helper_entries = get_expanded_rebinding_entries(helper_name, helper, rebindings, errors, stack)
        entries.update(helper_entries)

    entries.update(rebinding.get("bindings", {}))
    return entries


def iter_rebinding_bindings(rebinding):
    """Yield enabled source and target rebinding paths."""
    for source, target in rebinding.get("expanded_bindings", rebinding.get("bindings", {})).items():
        if target is False:
            continue
        if target is True:
            yield source, source
        else:
            yield source, target


def append_binding_point_template(lines,
                                  idx,
                                  kind,
                                  subaction_path,
                                  steamvr_path,
                                  localized_name,
                                  priority_group,
                                  path,
                                  upgrade_targets=None,
                                  dpad_binding_point="OXR_DPAD_BINDING_POINT_NONE",
                                  dpad_emulation_path=None,
                                  value_transform="OXR_BINDING_VALUE_TRANSFORM_NONE",
                                  input_value="0",
                                  output_value="0"):
    if upgrade_targets is None:
        upgrade_targets = {}

    lines.extend([
        f'\t\t\t{{ // oxr_binding_point_template {idx}',
        f'\t\t\t\t.kind = {kind},',
        f'\t\t\t\t.subaction_path = "{subaction_path}",',
        f'\t\t\t\t.steamvr_path = "{steamvr_path}",',
        f'\t\t\t\t.localized_name = "{localized_name}",',
        f'\t\t\t\t.priority_group = "{priority_group}",',
        f'\t\t\t\t.path = "{path}",',
        f'\t\t\t\t.upgrade_boolean_path = {quote_str_or_null(upgrade_targets.get("boolean"))},',
        f'\t\t\t\t.upgrade_float_path = {quote_str_or_null(upgrade_targets.get("float"))},',
        f'\t\t\t\t.upgrade_vec2_path = {quote_str_or_null(upgrade_targets.get("vec2"))},',
        f'\t\t\t\t.upgrade_pose_path = {quote_str_or_null(upgrade_targets.get("pose"))},',
        f'\t\t\t\t.upgrade_haptic_path = {quote_str_or_null(upgrade_targets.get("haptic"))},',
        f'\t\t\t\t.dpad_binding_point = {dpad_binding_point},',
        f'\t\t\t\t.dpad_emulation_path = {quote_str_or_null(dpad_emulation_path)},',
        f'\t\t\t\t.value_transform = {value_transform},',
        f'\t\t\t\t.input = {input_value},',
        f'\t\t\t\t.output = {output_value},',
        f'\t\t\t}}, // /oxr_binding_point_template {idx}',
    ])


def generate_profile_template_entry(profile):
    """Generate a single oxr_profile_template entry. Returns a string."""
    validate_profile_templates(profile)

    lines = []
    hw_name = str(profile.name.split("/")[-1])
    vendor_name = str(profile.name.split("/")[-2])
    fname = vendor_name + "_" + hw_name + "_profile.json"
    controller_type = "monado_" + vendor_name + "_" + hw_name

    upgrade_count = sum(1 for identifier in profile.identifiers if should_generate_upgrade(identifier))
    xy_count = sum(1 for component in profile.components if component.component_name == "position")
    dpad_binding_point_count = sum(len(identifier.dpad.paths) for identifier in profile.identifiers if identifier.dpad)
    binding_count = len(profile.components) + upgrade_count + (2 * xy_count) + dpad_binding_point_count
    lines.extend([
        '\t{ // oxr_profile_template',
        f'\t\t.name = {profile.monado_device_enum},',
        f'\t\t.path = "{profile.name}",',
        f'\t\t.localized_name = "{profile.localized_name}",',
        f'\t\t.steamvr_input_profile_path = "{fname}",',
        f'\t\t.steamvr_controller_type = "{controller_type}",',
        f'\t\t.binding_count = {binding_count},',
        '\t\t.bindings = (struct oxr_binding_point_template[]){ // array of oxr_binding_point_template',
    ])

    binding_idx = 0

    for identifier in profile.identifiers:
        upgrade_targets = get_upgrade_target_paths(identifier)
        if len(upgrade_targets) == 0:
            continue

        append_binding_point_template(
            lines,
            binding_idx,
            "OXR_BINDING_POINT_KIND_UPGRADE",
            identifier.subaction_path,
            identifier.json_path,
            "Upgrade",
            identifier.identifier_path,
            identifier.identifier_path,
            upgrade_targets=upgrade_targets)
        binding_idx += 1

    component: Component
    for idx, component in enumerate(profile.components):
        # @todo Doesn't handle pose yet.
        steamvr_path = component.steamvr_path
        if component.component_name in ["click", "touch", "force", "value", "proximity"]:
            steamvr_path += "/" + component.component_name

        # controllers can have input that we don't have bindings for
        input_value = '0'
        output_value = '0'
        if component.monado_binding:
            monado_binding = component.monado_binding

            # Input and output default to 0.
            # If a binding specifies an actual binding value for them in json, those get overridden.
            if component.is_input() and monado_binding is not None:
                input_value = monado_binding

            if component.is_output() and monado_binding is not None:
                output_value = monado_binding

        for binding_path in get_component_binding_point_paths(component):
            binding_kind = "OXR_BINDING_POINT_KIND_INPUT" if component.is_input() else "OXR_BINDING_POINT_KIND_OUTPUT"
            append_binding_point_template(
                lines,
                binding_idx,
                binding_kind,
                component.subaction_path,
                steamvr_path,
                component.subpath_localized_name,
                get_component_priority_group(component),
                binding_path,
                value_transform=get_binding_value_transform(binding_path),
                input_value=input_value,
                output_value=output_value)
            binding_idx += 1

    for identifier in profile.identifiers:
        if not identifier.dpad:
            continue

        for dpad_path in identifier.dpad.paths:
            append_binding_point_template(
                lines,
                binding_idx,
                "OXR_BINDING_POINT_KIND_DPAD",
                identifier.subaction_path,
                identifier.json_path,
                "DPad",
                identifier.identifier_path,
                dpad_path,
                dpad_binding_point=get_dpad_binding_point_enum(dpad_path),
                dpad_emulation_path=identifier.identifier_path,
                input_value=identifier.dpad.position_component.monado_binding)
            binding_idx += 1

    lines.append('\t\t}, // /array of oxr_binding_point_template')

    # Handle dpads
    dpads = []
    for identifier in profile.identifiers:
        if identifier.dpad:
            dpads.append(identifier)

    dpad_count = len(dpads)
    lines.append(f'\t\t.dpad_emulation_count = {dpad_count},')
    if len(dpads) == 0:
        lines.append('\t\t.dpad_emulations = NULL,')
    else:
        lines.append('\t\t.dpad_emulations = (struct oxr_dpad_emulation_template[]){ // array of oxr_dpad_emulation_template')
        for identifier in dpads:
            lines.append('\t\t\t{')
            lines.append(f'\t\t\t\t.path = "{identifier.identifier_path}",')
            lines.append(f'\t\t\t\t.subaction_path = "{identifier.subaction_path}",')
            lines.append(f'\t\t\t\t.position = {identifier.dpad.position_component.monado_binding},')
            if identifier.dpad.activate_component:
                lines.append(f'\t\t\t\t.activate = {identifier.dpad.activate_component.monado_binding},')
            else:
                lines.append('\t\t\t\t.activate = XRT_INPUT_NAME_INVALID')
            lines.append('\t\t\t},')
        lines.append('\t\t}, // /array of oxr_dpad_emulation_template')

    lines.append(f'\t\t.openxr_version.promoted.major = {profile.openxr_version_promoted["major"]},')
    lines.append(f'\t\t.openxr_version.promoted.minor = {profile.openxr_version_promoted["minor"]},')

    fn_prefixes = ["subpath", "dpad_path", "dpad_emulator"]
    for prefix in fn_prefixes:
        lines.append(f'\t\t.{prefix}_fn = oxr_verify_{profile.validation_func_name}_{prefix},')
    lines.append(f'\t\t.ext_verify_fn = oxr_verify_{profile.validation_func_name}_ext,')

    if profile.extension_name is None:
        lines.append('\t\t.extension_name = NULL,')
    else:
        lines.append(f'\t\t.extension_name = "{profile.extension_name}",')

    lines.append('\t}, // /oxr_profile_template')

    return '\n'.join(lines)


def generate_rebinding_profile_template_entry(name, rebinding):
    lines = [
        '\t{ // oxr_rebinding_profile_template',
        f'\t\t.source_profile_path = "{rebinding["source_profile"]}",',
        f'\t\t.target_profile_path = "{rebinding["target_profile"]}",',
        f'\t\t.binding_count = {sum(1 for _ in iter_rebinding_bindings(rebinding))},',
        '\t\t.bindings = (struct oxr_rebinding_binding_template[]){',
    ]

    for source, target in iter_rebinding_bindings(rebinding):
        lines.extend([
            '\t\t\t{',
            f'\t\t\t\t.source_path = "{source}",',
            f'\t\t\t\t.target_path = "{target}",',
            '\t\t\t},',
        ])

    lines.extend([
        '\t\t},',
        '\t}, // /oxr_rebinding_profile_template',
    ])
    return '\n'.join(lines)


def generate_bindings_c(output, b, rebindings):
    """Generate the file to verify subpaths on a interaction profile."""
    script_dir = os.path.dirname(os.path.abspath(__file__))
    template_file = os.path.join(script_dir, 'oxr_generated_bindings.c.template')

    with open(template_file, 'r') as f:
        src = Template(f.read())

    # Generate verify functions
    lines = []
    for profile in b.profiles:
        lines.extend(get_verify_functions(profile))
    verify_functions = '\n'.join(lines)

    # Generate profile templates
    profile_templates = '\n'.join(generate_profile_template_entry(profile) for profile in b.profiles)
    rebinding_names = [name for name in sorted(rebindings.keys()) if not is_rebinding_helper(rebindings[name])]
    rebinding_profile_templates = '\n'.join(
        generate_rebinding_profile_template_entry(name, rebindings[name])
        for name in rebinding_names)

    with open(output, "w", encoding="utf-8") as fp:
        fp.write(src.substitute(
            template_count=len(b.profiles),
            rebinding_template_count=len(rebinding_names),
            verify_functions=verify_functions,
            profile_templates=profile_templates,
            rebinding_profile_templates=rebinding_profile_templates,
        ))


def generate_bindings_h(output, b, rebindings):
    """Generate header for the verify subpaths functions."""
    script_dir = os.path.dirname(os.path.abspath(__file__))
    template_file = os.path.join(script_dir, 'oxr_generated_bindings.h.template')

    with open(template_file, 'r') as f:
        src = Template(f.read())

    # Generate function prototypes
    verify_protos = []
    fn_prefixes = ["_subpath", "_dpad_path", "_dpad_emulator"]
    for profile in b.profiles:
        for fn_suffix in fn_prefixes:
            verify_protos.extend([
                'bool',
                f'oxr_verify_{profile.validation_func_name}{fn_suffix}(const struct oxr_extension_status *extensions, XrVersion openxr_major_minor, const char *str, size_t length);',
                '',
            ])
        verify_protos.extend([
            'void',
            f'oxr_verify_{profile.validation_func_name}_ext(const struct oxr_extension_status *extensions, XrVersion openxr_version, bool *out_supported, bool *out_enabled);',
            '',
        ])

    with open(output, "w", encoding="utf-8") as fp:
        fp.write(src.substitute(
            template_count=len(b.profiles),
            rebinding_template_count=sum(1 for rebinding in rebindings.values() if not is_rebinding_helper(rebinding)),
            verify_protos='\n'.join(verify_protos)
        ))


def main():
    """Handle command line and generate file(s)."""
    parser = argparse.ArgumentParser(description='OpenXR Bindings generator.')
    parser.add_argument(
        'bindings', help='Bindings file to use')
    parser.add_argument(
        '--rebindings', help='Rebindings file to use', default=None)
    parser.add_argument(
        'output', type=str, nargs='+',
        help='Output file, uses the name to choose output type')
    args = parser.parse_args()

    bindings = Bindings.load_and_parse(args.bindings)
    rebindings = {}
    if args.rebindings is not None:
        with open(args.rebindings, "r", encoding="utf-8") as rebinding_file:
            rebindings = json.load(rebinding_file)
    validate_rebindings(bindings, rebindings)
    for name, rebinding in rebindings.items():
        if not is_rebinding_helper(rebinding):
            rebinding["expanded_bindings"] = get_expanded_rebinding_entries(name, rebinding, rebindings)

    for output in args.output:
        if output.endswith("oxr_generated_bindings.c"):
            generate_bindings_c(output, bindings, rebindings)
        elif output.endswith("oxr_generated_bindings.h"):
            generate_bindings_h(output, bindings, rebindings)
        else:
            raise ValueError(f"Unknown output file: {output}")


if __name__ == "__main__":
    main()
