#!/usr/bin/env python3
# Copyright 2025, NVIDIA CORPORATION.
# SPDX-License-Identifier: BSL-1.0
"""
Helper script to merge multiple JSON files into one.

This script merges the root JSON objects from multiple input files
while preserving field ordering. Files are processed in sorted order
to ensure consistent results. Fields are merged in the order they
appear across the input files.
"""

import argparse
import json
import sys
from pathlib import Path


def merge_json_files(json_files, output_file, allow_overwrite=False,
                     ignore_schema=False):
    """
    Merge multiple JSON files into a single output file.

    Args:
        json_files: List of paths to JSON files to merge
        output_file: Path to the output file
        allow_overwrite: If True, allow later files to overwrite keys
                         from earlier files
        ignore_schema: If True, ignore the "$schema" field in input files
    """
    merged = {}

    # Sort files to ensure consistent ordering
    sorted_files = sorted(json_files)

    for json_file in sorted_files:
        try:
            with open(json_file, 'r', encoding='utf-8') as f:
                data = json.load(f)

            if not isinstance(data, dict):
                print(f"Error: {json_file} does not contain a JSON "
                      f"object at root level",
                      file=sys.stderr)
                sys.exit(1)

            # Merge while preserving order - check for duplicates
            for key, value in data.items():
                if key == "$schema" and ignore_schema:
                    continue
                if key in merged:
                    if allow_overwrite:
                        print(f"Warning: Key '{key}' from {json_file} "
                              f"overwrites previous definition",
                              file=sys.stderr)
                    else:
                        print(f"Error: Duplicate key '{key}' found in "
                              f"{json_file}. Use --allow-overwrite to "
                              f"permit overwriting.",
                              file=sys.stderr)
                        sys.exit(1)
                merged[key] = value

        except FileNotFoundError:
            print(f"Error: File not found: {json_file}", file=sys.stderr)
            sys.exit(1)
        except json.JSONDecodeError as e:
            print(f"Error: Failed to parse JSON from {json_file}: {e}",
                  file=sys.stderr)
            sys.exit(1)
        except Exception as e:
            print(f"Error: Failed to read {json_file}: {e}",
                  file=sys.stderr)
            sys.exit(1)


    # Write merged JSON to output file
    try:
        with open(output_file, 'w', encoding='utf-8') as f:
            json.dump(merged, f, indent='\t', ensure_ascii=False)
            f.write('\n')  # Add trailing newline

        # Printing, left in if we want it in the future.
        if False:
            print(f"Successfully merged {len(json_files)} file(s) into "
                  f"{output_file}")
    except Exception as e:
        print(f"Error: Failed to write to {output_file}: {e}",
              file=sys.stderr)
        sys.exit(1)


def main():
    parser = argparse.ArgumentParser(
        description='Merge multiple JSON files into one, preserving '
                    'field ordering.',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s -o merged.json file1.json file2.json file3.json
  %(prog)s -o output.json base.json overrides.json
  %(prog)s -o output.json --allow-overwrite base.json overrides.json
  %(prog)s -o output.json --ignore-schema schema.json data.json
        """
    )

    parser.add_argument('-o', '--output',
                        required=True,
                        metavar='OUTPUT',
                        help='Output file path for the merged JSON')

    parser.add_argument('--allow-overwrite',
                        action='store_true',
                        help='Allow later files to overwrite duplicate '
                             'keys from earlier files')

    parser.add_argument('--ignore-schema',
                        action='store_true',
                        help='Ignore the "$schema" field in input files')

    parser.add_argument('json_files',
                        nargs='+',
                        metavar='JSON_FILE',
                        help='JSON files (will be sorted alphabetically)')

    args = parser.parse_args()

    # Validate that we have at least one input file
    if not args.json_files:
        print("Error: At least one JSON file must be provided",
              file=sys.stderr)
        sys.exit(1)

    merge_json_files(args.json_files, args.output, args.allow_overwrite,
                     args.ignore_schema)


if __name__ == '__main__':
    main()
