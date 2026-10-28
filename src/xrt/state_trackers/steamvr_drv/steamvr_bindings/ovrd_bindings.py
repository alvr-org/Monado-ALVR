#!/usr/bin/env python3
# Copyright 2020-2025, Collabora, Ltd.
# Copyright 2024-2025, NVIDIA CORPORATION.
# SPDX-License-Identifier: BSL-1.0
"""Generate SteamVR binding code using the auxiliary bindings scripts."""

from bindings import *

import argparse
from string import Template

def ovrd_generate_bindings_c(file, b):
    """Generate the file with templates for interaction profiles."""
    f = open(file, "w")

    wl(f,
        header.format(brief='Generated bindings data', group='oxr_main'),
        '#include "b_ovrd_generated_bindings.h"',
        '#include <string.h>',
        '',
        '// clang-format off',
        '',
        f'\n\nstruct profile_template profile_templates[{len(b.profiles)}] = {{ // array of profile_template',
    )

    
    for profile in b.profiles:
        hw_name = str(profile.name.split("/")[-1])
        vendor_name = str(profile.name.split("/")[-2])
        fname = vendor_name + "_" + hw_name + "_profile.json"
        controller_type = "monado_" + vendor_name + "_" + hw_name

        binding_count = len(profile.components)
        wl(f,
            f'\t{{ // profile_template',
            f'\t\t.name = {profile.monado_device_enum},',
            f'\t\t.path = "{profile.name}",',
            f'\t\t.localized_name = "{profile.localized_name}",',
            f'\t\t.steamvr_input_profile_path = "{fname}",',
            f'\t\t.steamvr_controller_type = "{controller_type}",',
            f'\t\t.binding_count = {binding_count},',
            f'\t\t.bindings = (struct binding_template[]){{ // array of binding_template',
        )

        component: Component
        for idx, component in enumerate(profile.components):

            # @todo Doesn't handle pose yet.
            steamvr_path = component.steamvr_path
            if component.component_name in ["click", "touch", "force", "value", "proximity"]:
                steamvr_path += "/" + component.component_name

            wl(f,
                f'\t\t\t{{ // binding_template {idx}',
                f'\t\t\t\t.subaction_path = "{component.subaction_path}",',
                f'\t\t\t\t.steamvr_path = "{steamvr_path}",',
                f'\t\t\t\t.localized_name = "{component.subpath_localized_name}",',
                '',
                '\t\t\t\t.paths = { // array of paths',

                '\n'.join([f'\t\t\t\t\t"{path}",' for path in component.get_full_openxr_paths() ]),

                '\t\t\t\t\tNULL,',
                '\t\t\t\t}, // /array of paths',
            )

            # print("component", component.__dict__)

            component_str = component.component_name

            # controllers can have input that we don't have bindings for
            if component.monado_binding:
                monado_binding = component.monado_binding

                if component.is_input() and monado_binding is not None:
                    f.write(f'\t\t\t\t.input = {monado_binding},\n')
                else:
                    f.write(f'\t\t\t\t.input = 0,\n')

                if component.has_dpad_emulation() and "activate" in component.dpad_emulation:
                    activate_component = find_component_in_list_by_name(
                        component.dpad_emulation["activate"], profile.components,
                        subaction_path=component.subaction_path,
                        identifier_json_path=component.identifier_json_path)
                    f.write(
                        f'\t\t\t\t.dpad_activate = {activate_component.monado_binding},\n')
                else:
                    f.write(f'\t\t\t\t.dpad_activate = 0,\n')

                if component.is_output() and monado_binding is not None:
                    f.write(f'\t\t\t\t.output = {monado_binding},\n')
                else:
                    f.write(f'\t\t\t\t.output = 0,\n')
            f.write(f'\t\t\t}}, // /binding_template {idx}\n')

        f.write('\t\t}, // /array of binding_template\n')

        dpads = []
        for idx, identifier in enumerate(profile.identifiers):
            if identifier.dpad:
                dpads.append(identifier)

#        for identifier in dpads:
#            print(identifier.path, identifier.dpad_position_component)

        dpad_count = len(dpads)
        f.write(f'\t\t.dpad_count = {dpad_count},\n')
        if len(dpads) == 0:
            f.write(f'\t\t.dpads = NULL,\n')
        else:
            f.write(
                f'\t\t.dpads = (struct dpad_emulation[]){{ // array of dpad_emulation\n')
            for idx, identifier in enumerate(dpads):
                f.write('\t\t\t{\n')
                f.write(f'\t\t\t\t.subaction_path = "{identifier.subaction_path}",\n')
                f.write('\t\t\t\t.paths = {\n')
                for path in identifier.dpad.paths:
                    f.write(f'\t\t\t\t\t"{path}",\n')
                f.write('\t\t\t\t},\n')
                f.write(f'\t\t\t\t.position = {identifier.dpad.position_component.monado_binding},\n')
                if identifier.dpad.activate_component:
                    f.write(f'\t\t\t\t.activate = {identifier.dpad.activate_component.monado_binding},\n')
                else:
                    f.write(f'\t\t\t\t.activate = 0')

                f.write('\t\t\t},\n')
            f.write('\t\t}, // /array of dpad_emulation\n')

        f.write('\t}, // /profile_template\n')

    f.write('}; // /array of profile_template\n\n')

    f.write("\n// clang-format on\n")

    f.close()

# Reduced version of OpenXR templates, without the OpenXR bits the SteamVR plugin does not need
H_TEMPLATE = Template("""$header

#pragma once

#include <stddef.h>
#include "xrt/xrt_defines.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OXR_BINDINGS_PROFILE_TEMPLATE_COUNT $template_count

// clang-format off

#define PATHS_PER_BINDING_TEMPLATE 16

enum oxr_dpad_binding_point
{
\tOXR_DPAD_BINDING_POINT_NONE,
\tOXR_DPAD_BINDING_POINT_UP,
\tOXR_DPAD_BINDING_POINT_DOWN,
\tOXR_DPAD_BINDING_POINT_LEFT,
\tOXR_DPAD_BINDING_POINT_RIGHT,
};

struct dpad_emulation
{
\tconst char *subaction_path;
\tconst char *paths[PATHS_PER_BINDING_TEMPLATE];
\tenum xrt_input_name position;
\tenum xrt_input_name activate; // Can be zero
};

struct binding_template
{
\tconst char *subaction_path;
\tconst char *steamvr_path;
\tconst char *localized_name;
\tconst char *paths[PATHS_PER_BINDING_TEMPLATE];
\tenum xrt_input_name input;
\tenum xrt_input_name dpad_activate;
\tenum xrt_output_name output;
};

struct profile_template
{
\tenum xrt_device_name name;
\tconst char *path;
\tconst char *localized_name;
\tconst char *steamvr_input_profile_path;
\tconst char *steamvr_controller_type;
\tstruct binding_template *bindings;
\tsize_t binding_count;
\tstruct dpad_emulation *dpads;
\tsize_t dpad_count;

\tconst char *extension_name;
};

extern struct profile_template profile_templates[OXR_BINDINGS_PROFILE_TEMPLATE_COUNT];
// clang-format on")
#ifdef __cplusplus
}
#endif
""")


def ovrd_generate_bindings_h(file, b):
    """Generate the header with templates for interaction profiles."""

    with open(file, "w") as f:
        filled = H_TEMPLATE.substitute(
            header = header.format(brief='Generated bindings data', group='oxr_main'),
            template_count=len(b.profiles),
        )
        f.write(filled)


def main():
    """Handle command line and generate a file."""
    parser = argparse.ArgumentParser(description='SteamVR Bindings generator.')
    parser.add_argument(
        'bindings', help='Bindings file to use')
    parser.add_argument(
        'output', type=str, nargs='+',
        help='Output file, uses the name to choose output type')
    args = parser.parse_args()

    bindings = Bindings.load_and_parse(args.bindings)

    for output in args.output:
        if output.endswith("ovrd_generated_bindings.c"):
            ovrd_generate_bindings_c(output, bindings)
        if output.endswith("ovrd_generated_bindings.h"):
            ovrd_generate_bindings_h(output, bindings)


if __name__ == "__main__":
    main()
