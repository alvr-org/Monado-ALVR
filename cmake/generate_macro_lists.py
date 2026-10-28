#!/usr/bin/env python3
# Copyright 2025-2026, NVIDIA CORPORATION.
# SPDX-License-Identifier: BSL-1.0
"""
Script to parse header files and generate X_macro lists for all enum values
found.

This script is used to generate the X_macro lists for the enums found in the
header files specified by the --inputs argument.

The script will generate the X_macro lists for the enums found in the header
files specified by the --inputs argument, and write them to the header file
specified by the --output argument.

The script will also verify that the found enums match the expected enums
specified by the --enums argument.

If the --ignore-other-enums argument is provided, the script will ignore enums
found in the input files that are not in the --enums list.

The script will also print verbose informational output if the --verbose
argument is provided.
"""

import sys
import re
import argparse
from pathlib import Path

# Global flag for info printing
_print_info_enabled = False


def info(message):
    """
    Print informational messages if info printing is enabled.

    Args:
        message: The message to print
    """
    if _print_info_enabled:
        print(message)


def error(message):
    """
    Print an error message and exit with code 1.

    Args:
        message: The error message to print
    """
    print(f"Error: {message}", file=sys.stderr)
    sys.exit(1)


def verify_enum_line(line):
    """
    Verify that the enum line is valid, this function errors on the side of
    rejecting unfamiliar lines rather then being more flexible.

    Args:
        line: The line to verify

    Returns:
        True if the line is valid, False otherwise
    """

    # Empty line
    if (line == '\n'):
        return True
    # C++ style comment
    if (line.startswith('\t//')):
        return True
    # C style comment opener.
    if (line == '\t/*\n'):
        return True
    # C style doc comment opener.
    if (line == '\t/*!\n'):
        return True
    # C empty comment line.
    if (line == '\t *\n'):
        return True
    # C comment line.
    if (line.startswith('\t * ')):
        return True
    # C comment closer.
    if (line == '\t */\n'):
        return True
    # Sure we could use a regex to verify enum decls,
    # this is simpler for now.
    if (line.startswith('\tXRT_')):
        return True
    return False


def parse_enum_body(lines, start_index, enum_name, is_typedef,
                    source_file, should_verify):
    """
    Parse the body of an enum declaration.

    Args:
        lines: List of all lines in the file
        start_index: Line index where enum body starts
        enum_name: Name of the enum
        is_typedef: Whether this is a typedef enum
        source_file: Path to source file (for error messages)
        should_verify: Whether to verify enum lines

    Returns:
        Tuple of (enum_values, new_index) where enum_values is a list
        of enum value names and new_index is the line index after
        the enum body
    """
    # Pattern for matching enum value names (ignore everything after the name)
    enum_pattern = r'^\s*([A-Z_][A-Z0-9_]*)'

    enum_values = []
    found_opening_brace = False
    i = start_index

    # Error if we don't find the opening brace
    if not lines[i] == '{\n':
        error(f"Expected opening brace for enum {enum_name} in "
              f"{source_file} at line {i}, got '{lines[i].strip()}'")
    else:
        i += 1

    # We have verified the opening brace, so we can start parsing the enum
    # body. We loop through the lines until we find the end of the enum.
    while i < len(lines):
        line = lines[i]
        i += 1

        # Check for end of enum (typedef or regular)
        if is_typedef:
            # For typedef enum, look for } <name>_t;
            end_pattern = (r'^\s*\}\s*' + re.escape(enum_name) +
                           r'_t\s*;')
            if re.match(end_pattern, line):
                break
        else:
            # For regular enum, look for };
            if re.match(r'^\s*\}\s*;', line):
                break

        # Verify the line (only if we should verify this enum)
        if should_verify and not verify_enum_line(line):
            error(f"Line {i} in {source_file} failed verification: "
                  f"'{line.strip()}'")

        # Extract enum value names
        # Look for pattern: ENUM_VALUE = <number>, or just ENUM_VALUE,
        match = re.match(enum_pattern, line)
        if match:
            enum_value = match.group(1)
            enum_values.append(enum_value)

    return enum_values, i


def find_all_enums(source_file, expected_enums=None, ignore_other=False):
    """
    Find all enum declarations in the source file.

    Args:
        source_file: Path to the source header file
        expected_enums: List of expected enum names (optional)
        ignore_other: If True, don't verify lines for enums not in
                      expected_enums

    Returns:
        List of tuples: (enum_name, list of enum values, is_typedef)
    """
    with open(source_file, 'r') as f:
        lines = f.readlines()

    enums = []
    i = 0

    # Look for pattern: typedef enum <ident>
    typedef_decl_pattern = r'^\s*typedef\s+enum\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*$'

    # Look for pattern: enum <ident>
    enum_decl_pattern = r'^\s*enum\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*$'

    while i < len(lines):
        line = lines[i]

        # Check for typedef enum declarations
        typedef_match = re.match(typedef_decl_pattern, line)
        # Check for regular enum declarations
        enum_match = re.match(enum_decl_pattern, line)

        if typedef_match:
            enum_name = typedef_match.group(1)
            is_typedef = True
            i += 1
        elif enum_match:
            enum_name = enum_match.group(1)
            is_typedef = False
            i += 1
        else:
            i += 1
            continue

        # Determine if we should verify lines for this enum
        should_verify = True
        if ignore_other and expected_enums is not None:
            should_verify = enum_name in expected_enums

        # Call the helper function to parse the enum body,
        # also advances the line index to the end of the enum.
        enum_values, i = parse_enum_body(
            lines, i, enum_name, is_typedef,
            source_file, should_verify
        )

        # Collect what we have found so far.
        if enum_values:
            enums.append((enum_name, enum_values, is_typedef))
            info(f"Found enum {enum_name} with {len(enum_values)} "
                 f"values in {source_file}")

    return enums


def verify_enums(found_enums, expected_enums, ignore_other):
    """
    Verify that the found enums match the expected enums.

    Args:
        found_enums: List of tuples (enum_name, enum_values, is_typedef)
        expected_enums: List of expected enum names
        ignore_other: If True, ignore unexpected enums instead of
                      erroring
    """
    if expected_enums is None:
        # No expectations defined, so any enums are acceptable
        return

    found_names = set(enum_name for enum_name, _, _ in found_enums)
    expected_names = set(expected_enums)

    missing_enums = expected_names - found_names
    unexpected_enums = found_names - expected_names

    # Always error on missing enums
    if missing_enums:
        msg = "Enum list mismatch!\n"
        msg += f"  Expected enums: {sorted(expected_names)}\n"
        msg += f"  Found enums:    {sorted(found_names)}\n"
        msg += f"  Missing enums:  {sorted(missing_enums)}"
        error(msg)

    # Error on unexpected enums only if not ignoring them
    if unexpected_enums and not ignore_other:
        msg = "Enum list mismatch!\n"
        msg += f"  Expected enums: {sorted(expected_names)}\n"
        msg += f"  Found enums:    {sorted(found_names)}\n"
        msg += f"  Unexpected enums: {sorted(unexpected_enums)}"
        error(msg)


def generate_xmacro_header(all_enums, output_file):
    """
    Generate the X_macro include file (without headers, just the
    macro definitions).

    Args:
        all_enums: List of tuples (enum_name, enum_values, is_typedef)
        output_file: Path to the output include file
    """
    # Create the output directory if it doesn't exist
    output_path = Path(output_file)
    output_path.parent.mkdir(parents=True, exist_ok=True)

    with open(output_file, 'w') as f:
        f.write("// DO NOT EDIT - Auto-generated by "
                "generate_macro_lists.py\n")
        f.write("\n")

        for enum_name, enum_values, is_typedef in all_enums:
            # Convert enum name to uppercase for the macro name
            macro_name = enum_name.upper() + "_LIST"

            f.write(f"#define {macro_name}(_) \\\n")

            for i, value in enumerate(enum_values):
                if i < len(enum_values) - 1:
                    f.write(f"\t_({value}) \\\n")
                else:
                    # Last entry doesn't need a backslash
                    f.write(f"\t_({value})\n")

            # Add a blank line between enum macros (except for the
            # last one)
            if (enum_name, enum_values, is_typedef) != all_enums[-1]:
                f.write("\n")


def main():
    global _print_info_enabled

    parser = argparse.ArgumentParser(
        description='Generate X_macro lists for all enums found in '
                    'header files.'
    )
    parser.add_argument(
        '--inputs',
        nargs='+',
        required=True,
        metavar='FILE',
        help='Input header file(s) to scan for enum declarations'
    )
    parser.add_argument(
        '--enums',
        nargs='+',
        required=True,
        metavar='NAME',
        help='List of expected enum names'
    )
    parser.add_argument(
        '-o', '--output',
        required=True,
        metavar='FILE',
        help='Output file for generated X_macro lists'
    )
    parser.add_argument(
        '--verbose', '-v',
        action='store_true',
        help='Enable verbose informational output'
    )
    parser.add_argument(
        '--ignore-other-enums',
        action='store_true',
        help='Ignore enums found in input files that are not in '
             'the --enums list'
    )

    args = parser.parse_args()

    # Set global info printing flag
    _print_info_enabled = args.verbose

    try:
        all_enums = []

        # Scan all input files for enums
        for source_file in args.inputs:
            if not Path(source_file).exists():
                error(f"File not found: {source_file}")

            enums = find_all_enums(source_file, args.enums,
                                   args.ignore_other_enums)
            all_enums.extend(enums)

        if not all_enums:
            error("No enums found in the provided header files")

        # Verify that found enums match expected enums
        verify_enums(all_enums, args.enums, args.ignore_other_enums)

        # Filter to only include expected enums if ignore flag is set
        if args.ignore_other_enums:
            expected_set = set(args.enums)
            filtered_enums = [
                enum_tuple for enum_tuple in all_enums
                if enum_tuple[0] in expected_set
            ]
            info(f"Found {len(all_enums)} enum(s) total, "
                 f"using {len(filtered_enums)}")
            all_enums = filtered_enums
        else:
            info(f"Found {len(all_enums)} enum(s) total")

        generate_xmacro_header(all_enums, args.output)
        info(f"Generated {args.output}")

    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == '__main__':
    main()
