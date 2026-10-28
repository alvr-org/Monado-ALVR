# SPDX-FileCopyrightText: 2021-2022, Collabora, Ltd.
# SPDX-FileCopyrightText: 2025-2026, NVIDIA CORPORATION.
# SPDX-License-Identifier: CC0-1.0

with section("parse"):

    # Specify structure for custom cmake functions
    additional_commands = {
        "generate_openxr_runtime_manifest_at_install": {
            "kwargs": {
                "COMPONENT": 1,
                "DESTINATION": 1,
                "LIBMONADO": 1,
                "MANIFEST_TEMPLATE": 1,
                "OUT_FILENAME": 1,
                "RELATIVE_RUNTIME_DIR": 1,
                "RUNTIME_DIR_RELATIVE_TO_MANIFEST": 1,
                "RUNTIME_TARGET": 1,
            },
            "pargs": {"flags": ["ABSOLUTE_RUNTIME_PATH"], "nargs": "*"},
        },
        "generate_openxr_runtime_manifest_buildtree": {
            "kwargs": {
                "LIBMONADO": 1,
                "MANIFEST_TEMPLATE": 1,
                "OUT_FILE": 1,
                "RUNTIME_TARGET": 1,
            },
            "pargs": {"flags": [], "nargs": "*"},
        },
        "generate_macro_lists": {
            "kwargs": {"OUTPUT": 1, "WORKING_DIRECTORY": 1, "INPUTS": "+", "ENUMS": "+"},
            "pargs": {"flags": ["IGNORE_OTHER_ENUMS", "VERBOSE"], "nargs": "*"},
        },
        "monado_set_options": {
            "kwargs": {"ENABLE": "+", "KEEP": "+", "KEEP_SET": "+"},
            "pargs": {"flags": [], "nargs": "*"},
        },
        "option_with_deps": {
            "kwargs": {"DEFAULT": 1, "DEPENDS": "+"},
            "pargs": {"flags": [], "nargs": "2+"},
        },
    }

with section("format"):
    line_width = 100
    tab_size = 8
    use_tabchars = True
    fractional_tab_policy = "use-space"

    max_prefix_chars = 4

    dangle_parens = True
    dangle_align = "prefix-indent"
    max_pargs_hwrap = 4
    max_rows_cmdline = 1

    keyword_case = "upper"


# Do not reflow comments

with section("markup"):
    enable_markup = False
