#!/usr/bin/env python3
"""
Fix incorrectly split <If> elements from convert_legacy_xml.py

The old converter incorrectly transformed:
    <If Value="@bool">
        <True>content_a</True>
        <False>content_b</False>
    </If>

Into:
    <If Value1="@bool" Operation="#_if_true_">content_a</If>
    <If Value1="@bool" Operation="#_if_false_">content_b</If>

This script recombines them back into the correct format:
    <If Value1="@bool" Operation="#_if_true_">
        <True>content_a</True>
        <False>content_b</False>
    </If>

Usage:
    python fix_split_if.py input.xml [output.xml]
    python fix_split_if.py input.xml -i  # in-place

If output.xml is not specified, outputs to stdout.
"""

import sys
import argparse
from lxml import etree


def find_matching_false_if(parent, true_if, index):
    """
    Find a matching False If element that follows the True If.

    Returns (matching_element, index) or (None, -1) if not found.
    """
    value1 = true_if.get('Value1')
    if not value1:
        return None, -1

    # Look at subsequent siblings
    siblings = list(parent)
    for i in range(index + 1, len(siblings)):
        sibling = siblings[i]

        # Skip non-elements (comments, etc.)
        if not isinstance(sibling.tag, str):
            continue

        # Must be an If element
        if sibling.tag != 'If':
            continue

        # Must have matching Value1
        if sibling.get('Value1') != value1:
            continue

        # Must have #_if_false_ operation
        op = sibling.get('Operation')
        if op in ('#_if_false_', '2'):  # 2 is the numeric value for false
            return sibling, i

    return None, -1


def is_true_conditional(elem):
    """Check if element is an If with #_if_true_ operation."""
    if elem.tag != 'If':
        return False
    op = elem.get('Operation')
    return op in ('#_if_true_', '1')  # 1 is the numeric value for true


def is_false_conditional(elem):
    """Check if element is an If with #_if_false_ operation."""
    if elem.tag != 'If':
        return False
    op = elem.get('Operation')
    return op in ('#_if_false_', '2')  # 2 is the numeric value for false


def fix_split_ifs(elem, parent=None):
    """
    Recursively fix split If elements.

    Returns the number of fixes made.
    """
    fixes = 0

    # Skip non-elements
    if not isinstance(elem.tag, str):
        return fixes

    # First, recursively process children (we'll modify the list, so iterate a copy)
    for child in list(elem):
        fixes += fix_split_ifs(child, elem)

    # Now look for split If patterns in this element's children
    children = list(elem)
    i = 0
    while i < len(children):
        child = children[i]

        # Skip non-elements
        if not isinstance(child.tag, str):
            i += 1
            continue

        # Look for If with #_if_true_
        if is_true_conditional(child):
            # Look for matching #_if_false_ sibling
            false_if, false_idx = find_matching_false_if(elem, child, i)

            if false_if is not None:
                # Found a pair! Recombine them
                value1 = child.get('Value1')

                # Create True element with true_if's children
                true_elem = etree.Element('True')
                for true_child in list(child):
                    true_elem.append(true_child)

                # Create False element with false_if's children
                false_elem = etree.Element('False')
                for false_child in list(false_if):
                    false_elem.append(false_child)

                # Clear the true If and add True/False children
                for c in list(child):
                    child.remove(c)
                child.append(true_elem)
                child.append(false_elem)

                # Remove the false If from parent
                elem.remove(false_if)

                # Update children list
                children = list(elem)

                fixes += 1
                print(f"  Fixed split If with Value1=\"{value1}\"", file=sys.stderr)

        i += 1

    return fixes


def fix_xml(input_text: str) -> tuple[str, int]:
    """
    Fix split If elements in XML.

    Args:
        input_text: XML as a string

    Returns:
        Tuple of (fixed XML string, number of fixes made)
    """
    # Parse XML with lxml (preserves comments)
    parser = etree.XMLParser(remove_comments=False, remove_blank_text=False)
    root = etree.fromstring(input_text.encode('utf-8'), parser)

    # Fix split Ifs
    fixes = fix_split_ifs(root)

    # Convert back to string
    output = etree.tostring(root, encoding='unicode', pretty_print=False)

    # Add XML declaration
    output = '<?xml version="1.0"?>\n' + output

    return output, fixes


def format_xml(xml_string: str) -> str:
    """Format XML with proper indentation."""
    parser = etree.XMLParser(remove_comments=False, remove_blank_text=True)
    root = etree.fromstring(xml_string.encode('utf-8'), parser)
    formatted = etree.tostring(root, encoding='unicode', pretty_print=True)
    return '<?xml version="1.0"?>\n' + formatted


def process_file(input_path: str, output_path: str = None, format_output: bool = False, in_place: bool = False) -> tuple[bool, int]:
    """
    Process a single XML file.

    Returns tuple of (success, fixes_count).
    """
    try:
        with open(input_path, 'r') as f:
            input_text = f.read()

        output_text, fixes = fix_xml(input_text)

        if fixes > 0:
            print(f"Fixed {fixes} split If element(s) in {input_path}", file=sys.stderr)

        if format_output:
            output_text = format_xml(output_text)

        if in_place:
            with open(input_path, 'w') as f:
                f.write(output_text)
        elif output_path:
            with open(output_path, 'w') as f:
                f.write(output_text)
        else:
            print(output_text)

        return True, fixes
    except Exception as e:
        print(f"Error processing {input_path}: {e}", file=sys.stderr)
        return False, 0


def process_directory(input_dir: str, format_output: bool = False) -> tuple[int, int, int]:
    """
    Recursively process all XML files in a directory (in-place).

    Returns tuple of (success_count, error_count, total_fixes).
    """
    import os

    success_count = 0
    error_count = 0
    total_fixes = 0

    for root, dirs, files in os.walk(input_dir):
        for filename in files:
            if filename.lower().endswith('.xml'):
                filepath = os.path.join(root, filename)
                success, fixes = process_file(filepath, in_place=True, format_output=format_output)
                if success:
                    success_count += 1
                    total_fixes += fixes
                else:
                    error_count += 1

    return success_count, error_count, total_fixes


def main():
    parser = argparse.ArgumentParser(
        description='Fix incorrectly split If elements from convert_legacy_xml.py',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument('input', nargs='?', help='Input XML file')
    parser.add_argument('output', nargs='?', help='Output XML file (default: stdout)')
    parser.add_argument('-d', '--directory', help='Process all XML files in directory recursively (in-place)')
    parser.add_argument('-f', '--format', action='store_true',
                        help='Format output with indentation')
    parser.add_argument('-i', '--in-place', action='store_true',
                        help='Modify input file in place')

    args = parser.parse_args()

    # Directory mode
    if args.directory:
        success, errors, total_fixes = process_directory(args.directory, format_output=args.format)
        print(f"\nProcessed {success + errors} file(s): {success} succeeded, {errors} failed", file=sys.stderr)
        print(f"Total fixes: {total_fixes}", file=sys.stderr)
        sys.exit(0 if errors == 0 else 1)

    # Single file mode
    if not args.input:
        parser.error("Either 'input' or '--directory' is required")

    success, fixes = process_file(args.input, args.output, format_output=args.format, in_place=args.in_place)
    sys.exit(0 if success else 1)


if __name__ == '__main__':
    main()
