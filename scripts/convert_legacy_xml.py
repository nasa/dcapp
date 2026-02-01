#!/usr/bin/env python3
"""
DCAPP Legacy to New XML Syntax Converter

Converts legacy dcapp XML files to the new syntax based on the rules
documented in documentation/notes.txt.

Usage:
    python convert_legacy_xml.py input.xml [output.xml]

If output.xml is not specified, outputs to stdout.
"""

import re
import sys
import argparse
from typing import Optional, Tuple
from lxml import etree


# ============================================================================
# SECTION 1: ELEMENT NAME MAPPINGS
# ============================================================================

ELEMENT_RENAMES = {
    'String': 'Text',
    'Defaults': 'Default',
    'DisplayLogic': 'Logic',
    # Button child elements - context-dependent, handled separately
    'OnPress': 'MousePressed',
    'Stencil': 'StencilAdd',      # Inside <Mask>
    'Projection': 'StencilDraw',  # Inside <Mask>
    # TrickIO renames
    'TrickIo': 'TrickIO',
    'FromTrick': 'TrickFrom',
    'ToTrick': 'TrickTo',
}

# Button-specific child element renames
BUTTON_CHILD_RENAMES = {
    'On': 'ButtonIndicatorOn',
    'Off': 'ButtonIndicatorOff',
    'Active': 'ButtonEnabled',
    'Inactive': 'ButtonDisabled',
    'Transition': 'ButtonTransition',
    'OnRelease': 'MouseReleased',
}

# Button-specific attribute renames
BUTTON_ATTRIBUTE_RENAMES = {
    'ActiveVariable': 'EnabledVariable',
    'ActiveOn': 'EnabledOn',
    'SwitchVariable': 'TargetVariable',
    'SwitchOn': 'TargetOn',
    'SwitchOff': 'TargetOff',
}


# ============================================================================
# SECTION 2: ATTRIBUTE NAME MAPPINGS
# ============================================================================

# Simple attribute renames (element-agnostic)
ATTRIBUTE_RENAMES = {
    'Operator': 'Operation',  # In <If> elements
    'Value': 'Value1',        # In <If> elements (single-value conditional)
    'Color': 'FillColor',     # For Text elements (not Line!)
}

# Element-specific attribute renames
ELEMENT_ATTRIBUTE_RENAMES = {
    'Line': {'Color': 'LineColor'},
    'Text': {'Color': 'FillColor'},
    'String': {'Color': 'FillColor'},  # Before element rename
}

# Attributes to remove entirely
REMOVE_ATTRIBUTES = {
    'ForceUpdate',      # Window - frame rate limiter (use vsync instead)
    # Note: MinimumValue/MaximumValue are handled specially by convert_set_operator()
    # Note: DisplayIndex/ActiveDisplay are handled specially by convert_display_index_pattern()
}

# Attributes to prefix with '_' (not yet implemented, preserved for future)
COMMENT_OUT_ATTRIBUTES = {
    'UpdateRate',       # Text - throttle string refresh rate
    'Pattern',          # Shapes/Lines - line pattern (dashed, dotted, etc.)
    'LinePattern',      # Lines - line pattern (dashed, dotted, etc.)
    'Key',              # Button - keyboard shortcut
    'BezelKey',         # Button - bezel key binding
    'ShadowOffset',     # Text - shadow/drop shadow effect
    'ForceMono',        # Text - force monospace rendering
}

# Elements to comment out with TODO markers
# Maps element name -> (marker, message)
COMMENT_OUT_ELEMENTS = {
    'EdgeIo': ('TODO(migration)', 'EdgeIo not yet supported'),
    'ADI': ('TODO(deprecated)', 'Use TexturedSphere instead of ADI'),
    'Animation': ('TODO(migration)', 'Animation not yet supported'),
    'Map': ('TODO(deprecated)', 'Use Terrain instead of Map'),
}


# ============================================================================
# SECTION 3: VARIABLE TYPE CONSTANT MAPPINGS
# ============================================================================

VARIABLE_TYPE_MAP = {
    'Decimal': '#_variable_double_',
    'Float': '#_variable_double_',
    'Double': '#_variable_double_',
    'Integer': '#_variable_integer_',
    'Int': '#_variable_integer_',
    'String': '#_variable_string_',
    'Boolean': '#_variable_boolean_',
    'Bool': '#_variable_boolean_',
}


# ============================================================================
# SECTION 4: BUTTON TYPE CONSTANT MAPPINGS
# ============================================================================

BUTTON_TYPE_MAP = {
    'Standard': '#_button_standard_',
    'Momentary': '#_button_momentary_',
    'Toggle': '#_button_toggle_',
}


# ============================================================================
# SECTION 5: ALIGNMENT CONSTANT MAPPINGS
# ============================================================================

HORIZONTAL_ALIGN_MAP = {
    'Left': '#_align_left_',
    'Center': '#_align_center_',
    'Right': '#_align_right_',
}

VERTICAL_ALIGN_MAP = {
    'Bottom': '#_align_bottom_',
    'Middle': '#_align_middle_',
    'Top': '#_align_top_',
}


# ============================================================================
# SECTION 6: CONDITIONAL OPERATOR MAPPINGS
# ============================================================================

CONDITIONAL_OP_MAP = {
    'gt': '#_conditional_gt_',
    'lt': '#_conditional_lt_',
    'eq': '#_conditional_eq_',
    'ne': '#_conditional_ne_',
    'gte': '#_conditional_gte_',
    'lte': '#_conditional_lte_',
    'ge': '#_conditional_gte_',
    'le': '#_conditional_lte_',
    '>': '#_conditional_gt_',
    '<': '#_conditional_lt_',
    '==': '#_conditional_eq_',
    '!=': '#_conditional_ne_',
    '>=': '#_conditional_gte_',
    '<=': '#_conditional_lte_',
}


# ============================================================================
# SECTION 7: SET OPERATOR MAPPINGS
# ============================================================================

SET_OP_MAP = {
    '=': '#_set_equal_',
    '+=': '#_set_add_',
    '-=': '#_set_subtract_',
    '*=': '#_set_multiply_',
    '/=': '#_set_divide_',
}


# ============================================================================
# CONVERSION FUNCTIONS
# ============================================================================

def strip_at_prefix(value: str) -> str:
    """Remove @ prefix from variable references in Variable attributes."""
    if value.startswith('@'):
        return value[1:]
    return value


def convert_alignment(elem: etree._Element, has_x: bool, has_y: bool) -> None:
    """
    Convert HorizontalAlign/VerticalAlign to LocalAlign/ParentAlign (or AlignX/AlignY shorthand).

    Rules:
    - If axis has explicit position (X/Y), use only LocalAlign
    - If axis has no position, use AlignX/AlignY shorthand (expands to both Local and Parent)
    - Invalid values fall back to legacy defaults (Left for horizontal, Bottom for vertical)
    """
    h_align = elem.get('HorizontalAlign')
    v_align = elem.get('VerticalAlign')

    if h_align:
        # Legacy default for invalid horizontal values was Left
        new_value = HORIZONTAL_ALIGN_MAP.get(h_align, '#_align_left_')
        if has_x:
            # Only local align when position is explicit
            elem.set('LocalAlignX', new_value)
        else:
            # Use shorthand when both would be the same
            elem.set('AlignX', new_value)
        del elem.attrib['HorizontalAlign']

    if v_align:
        # Legacy default for invalid vertical values was Bottom
        new_value = VERTICAL_ALIGN_MAP.get(v_align, '#_align_bottom_')
        if has_y:
            # Only local align when position is explicit
            elem.set('LocalAlignY', new_value)
        else:
            # Use shorthand when both would be the same
            elem.set('AlignY', new_value)
        del elem.attrib['VerticalAlign']


def convert_variable_type(elem: etree._Element) -> None:
    """Convert Variable element Type attribute to new constant."""
    var_type = elem.get('Type')
    if var_type and var_type in VARIABLE_TYPE_MAP:
        elem.set('Type', VARIABLE_TYPE_MAP[var_type])


def convert_button_type(elem: etree._Element) -> None:
    """Convert Button element Type attribute to new constant."""
    btn_type = elem.get('Type')
    if btn_type and btn_type in BUTTON_TYPE_MAP:
        elem.set('Type', BUTTON_TYPE_MAP[btn_type])


def convert_conditional_operator(elem: etree._Element) -> None:
    """Convert If element Operator to Operation with constant."""
    operator = elem.get('Operator')
    if operator:
        new_value = CONDITIONAL_OP_MAP.get(operator, operator)
        elem.set('Operation', new_value)
        del elem.attrib['Operator']


def has_variable_reference(value: Optional[str]) -> bool:
    """Check if a value contains a runtime variable reference (@prefix or #{} macro)."""
    if value is None:
        return False
    return '@' in value or '#{' in value


def convert_if_to_staticif(elem: etree._Element) -> bool:
    """
    Convert <If> to <StaticIf> if it has no runtime variable references.

    Returns True if conversion was made, False otherwise.
    """
    value1 = elem.get('Value1') or elem.get('Value')
    value2 = elem.get('Value2')

    # If neither value has a @ variable reference, this is a static conditional
    if not has_variable_reference(value1) and not has_variable_reference(value2):
        elem.tag = 'StaticIf'
        return True
    return False


def wrap_implicit_children_in_true(elem: etree._Element) -> None:
    """
    Wrap implicit children of StaticIf in explicit <True> blocks.

    Only needed for StaticIf - the preprocessor handles implicit children
    for runtime If elements automatically.

    Children that are not <True> or <False> are implicit true-branch children.
    These get collected and wrapped in a <True> element inserted at the beginning.
    """
    implicit_children = []
    children_to_remove = []

    for child in elem:
        # Skip comments and non-element nodes
        if not isinstance(child.tag, str):
            continue
        # Collect non-True/False children (these are implicit true-branch)
        if child.tag not in ('True', 'False'):
            implicit_children.append(child)
            children_to_remove.append(child)

    # If we have implicit children, wrap them in <True>
    if implicit_children:
        # Remove from parent first
        for child in children_to_remove:
            elem.remove(child)

        # Create True wrapper and add implicit children to it
        true_elem = etree.Element('True')
        for child in implicit_children:
            true_elem.append(child)

        # Insert True at the beginning (before any existing True/False)
        elem.insert(0, true_elem)


def convert_set_operator(elem: etree._Element) -> Tuple[Optional[str], Optional[str]]:
    """
    Convert Set element Operator and handle MinimumValue/MaximumValue.

    Returns tuple of (min_value, max_value) for generating additional Set elements.
    """
    operator = elem.get('Operator')
    min_val = elem.get('MinimumValue')
    max_val = elem.get('MaximumValue')

    # Convert operator
    if operator:
        new_value = SET_OP_MAP.get(operator, operator)
        elem.set('Operator', new_value)
    else:
        # Default to #_set_equal_ if no operator
        elem.set('Operator', '#_set_equal_')

    # Remove min/max attributes (will be handled as separate Set elements)
    if min_val:
        del elem.attrib['MinimumValue']
    if max_val:
        del elem.attrib['MaximumValue']

    return (min_val, max_val)


def convert_variable_reference(elem: etree._Element, attr_name: str) -> None:
    """Convert Variable="@name" to Variable="name" (strip @ prefix)."""
    var_ref = elem.get(attr_name)
    if var_ref:
        elem.set(attr_name, strip_at_prefix(var_ref))


def convert_blink_attributes(elem: etree._Element) -> None:
    """Convert Blink element attributes."""
    # FnStartBlink -> Variable
    fn_start = elem.get('FnStartBlink')
    if fn_start:
        elem.set('Variable', strip_at_prefix(fn_start))
        del elem.attrib['FnStartBlink']

    # Duration: -1 -> 0 (both mean indefinite)
    duration = elem.get('Duration')
    if duration and duration.strip() == '-1':
        elem.set('Duration', '0')


def convert_image_content_to_attribute(elem: etree._Element) -> None:
    """Convert <Image>path.tga</Image> to <Image File="path.png"/>."""
    if elem.text and elem.text.strip():
        file_path = elem.text.strip()
        # Also convert .tga to .png (recommended)
        if file_path.lower().endswith('.tga'):
            file_path = file_path[:-4] + '.png'
        elem.set('File', file_path)
        elem.text = None


def convert_displaylogic_to_logic(elem: etree._Element) -> None:
    """Convert DisplayLogic element to Logic with File attribute."""
    # Handle content form: <DisplayLogic>file.so</DisplayLogic>
    if elem.text and elem.text.strip():
        elem.set('File', elem.text.strip())
        elem.text = None


def convert_panel_background_color(elem: etree._Element) -> None:
    """
    Convert Panel BackgroundColor attribute to a Rectangle child.

    Legacy: <Panel BackgroundColor="0 0 0">...</Panel>
    New:    <Panel><Rectangle FillColor="0 0 0"/>...</Panel>

    The Rectangle inherits dimensions from the Panel automatically.
    """
    bg_color = elem.get('BackgroundColor')
    if bg_color:
        # Create rectangle with the background color
        rect = etree.Element('Rectangle')
        rect.set('FillColor', bg_color)
        # Insert as first child
        elem.insert(0, rect)
        # Remove the attribute
        del elem.attrib['BackgroundColor']


def remove_unsupported_attributes(elem: etree._Element) -> None:
    """Remove attributes that are not supported in new dcapp."""
    for attr in list(elem.attrib.keys()):
        if attr in REMOVE_ATTRIBUTES:
            del elem.attrib[attr]


def comment_out_attributes(elem: etree._Element) -> None:
    """Prefix attributes with '_' that are not yet implemented but should be preserved."""
    for attr in list(elem.attrib.keys()):
        if attr in COMMENT_OUT_ATTRIBUTES:
            value = elem.get(attr)
            del elem.attrib[attr]
            elem.set('_' + attr, value)


def extract_variable_name(value: str) -> Optional[str]:
    """Extract the variable name from a variable reference like @varname or #{varname}."""
    if not value:
        return None
    # Match @varname pattern
    match = re.match(r'^@(\w+)$', value)
    if match:
        return match.group(1)
    # Match #{varname} pattern (just the variable, no expression)
    match = re.match(r'^#\{(\w+)\}$', value)
    if match:
        return match.group(1)
    return None


def convert_origin_attributes(elem: etree._Element) -> Tuple[list, list, Optional[str]]:
    """
    Convert OriginX/OriginY attributes to ParentAlign with negated position.

    Legacy OriginX="Right" means X is measured from the right edge.
    New system uses ParentAlignX="#_align_right_" with negated X value.

    For variable values, uses push/negate/pop to temporarily negate the variable.

    Returns a tuple of:
    - before_elements: Set elements to insert before this element (push/negate)
    - after_elements: Set elements to insert after this element (pop)
    - comment: TODO comment if conversion couldn't be automated (complex expressions)
    """
    before_elements = []
    after_elements = []
    comment = None
    tag = elem.tag
    origin_x = elem.get('OriginX')
    origin_y = elem.get('OriginY')

    if origin_x:
        if origin_x == 'Right':
            x_val = elem.get('X')
            if x_val and has_variable_reference(x_val):
                var_name = extract_variable_name(x_val)
                if var_name:
                    # For Vertex, we can't add Set elements inside Line/Polygon
                    # Add a comment for manual intervention instead
                    if tag == 'Vertex':
                        comment = f'TODO(manual): Add push/negate for {var_name} before parent Line/Polygon, pop after'
                        elem.set('ParentAlignX', '#_align_right_')
                        del elem.attrib['OriginX']
                    else:
                        # Use push/negate/pop pattern for variable
                        push_elem = etree.Element('Set')
                        push_elem.set('Variable', var_name)
                        push_elem.set('Operator', '#_set_push_')
                        push_elem.text = '0'
                        negate_elem = etree.Element('Set')
                        negate_elem.set('Variable', var_name)
                        negate_elem.set('Operator', '#_set_negate_')
                        negate_elem.text = '0'
                        pop_elem = etree.Element('Set')
                        pop_elem.set('Variable', var_name)
                        pop_elem.set('Operator', '#_set_pop_')
                        pop_elem.text = '0'
                        before_elements.extend([push_elem, negate_elem])
                        after_elements.append(pop_elem)
                        elem.set('ParentAlignX', '#_align_right_')
                        del elem.attrib['OriginX']
            elif x_val:
                # Negate the literal X value
                try:
                    x_num = float(x_val)
                    negated = -x_num if x_num != 0 else 0
                    # Format as int if it's a whole number
                    elem.set('X', str(int(negated)) if negated == int(negated) else str(negated))
                except ValueError:
                    pass  # Not a number, leave as-is
                elem.set('ParentAlignX', '#_align_right_')
                del elem.attrib['OriginX']
            else:
                # No X value, just set the alignment
                elem.set('ParentAlignX', '#_align_right_')
                del elem.attrib['OriginX']
        elif origin_x == 'Left':
            # Left is the default, just remove it
            del elem.attrib['OriginX']

    if origin_y:
        if origin_y == 'Top':
            y_val = elem.get('Y')
            if y_val and has_variable_reference(y_val):
                var_name = extract_variable_name(y_val)
                if var_name:
                    # For Vertex, we can't add Set elements inside Line/Polygon
                    # Add a comment for manual intervention instead
                    if tag == 'Vertex':
                        if comment:
                            comment += f'; also Y var {var_name}'
                        else:
                            comment = f'TODO(manual): Add push/negate for {var_name} before parent Line/Polygon, pop after'
                        elem.set('ParentAlignY', '#_align_top_')
                        del elem.attrib['OriginY']
                    else:
                        # Use push/negate/pop pattern for variable
                        push_elem = etree.Element('Set')
                        push_elem.set('Variable', var_name)
                        push_elem.set('Operator', '#_set_push_')
                        push_elem.text = '0'
                        negate_elem = etree.Element('Set')
                        negate_elem.set('Variable', var_name)
                        negate_elem.set('Operator', '#_set_negate_')
                        negate_elem.text = '0'
                        pop_elem = etree.Element('Set')
                        pop_elem.set('Variable', var_name)
                        pop_elem.set('Operator', '#_set_pop_')
                        pop_elem.text = '0'
                        before_elements.extend([push_elem, negate_elem])
                        after_elements.append(pop_elem)
                        elem.set('ParentAlignY', '#_align_top_')
                        del elem.attrib['OriginY']
            elif y_val:
                # Negate the literal Y value
                try:
                    y_num = float(y_val)
                    negated = -y_num if y_num != 0 else 0
                    # Format as int if it's a whole number
                    elem.set('Y', str(int(negated)) if negated == int(negated) else str(negated))
                except ValueError:
                    pass  # Not a number, leave as-is
                elem.set('ParentAlignY', '#_align_top_')
                del elem.attrib['OriginY']
            else:
                # No Y value, just set the alignment
                elem.set('ParentAlignY', '#_align_top_')
                del elem.attrib['OriginY']
        elif origin_y == 'Bottom':
            # Bottom is the default, just remove it
            del elem.attrib['OriginY']

    return (before_elements, after_elements, comment)


def convert_display_index_pattern(elem: etree._Element) -> None:
    """
    Convert DisplayIndex/ActiveDisplay pattern to If conditionals.

    This handles the legacy page-switching pattern where Window has ActiveDisplay
    and Panel children have DisplayIndex. Converts to explicit If conditionals.

    Before:
        <Window ActiveDisplay="@currentPage">
            <Panel DisplayIndex="1">...</Panel>
            <Panel DisplayIndex="2">...</Panel>
        </Window>

    After:
        <Window>
            <If Value1="@currentPage" Value2="1" Operation="#_conditional_eq_">
                <Panel>...</Panel>
            </If>
            <If Value1="@currentPage" Value2="2" Operation="#_conditional_eq_">
                <Panel>...</Panel>
            </If>
        </Window>
    """
    # Only process Window elements with ActiveDisplay
    if elem.tag != 'Window':
        return

    active_display = elem.get('ActiveDisplay')
    if not active_display:
        return

    # Remove ActiveDisplay from Window
    del elem.attrib['ActiveDisplay']

    # Find Panel children with DisplayIndex and wrap them in If
    panels_to_wrap = []
    for child in elem:
        if not isinstance(child.tag, str):
            continue
        if child.tag == 'Panel' and child.get('DisplayIndex'):
            panels_to_wrap.append(child)

    for panel in panels_to_wrap:
        display_index = panel.get('DisplayIndex')
        del panel.attrib['DisplayIndex']

        # Get panel's position in parent
        panel_index = list(elem).index(panel)

        # Remove panel from parent
        elem.remove(panel)

        # Create If wrapper
        if_elem = etree.Element('If')
        if_elem.set('Value1', active_display)
        if_elem.set('Value2', display_index)
        if_elem.set('Operation', '#_conditional_eq_')

        # Add panel as child of If
        if_elem.append(panel)

        # Insert If at panel's original position
        elem.insert(panel_index, if_elem)


def get_element_context(elem: etree._Element, parent_tag: Optional[str]) -> str:
    """Determine the context of an element for context-dependent conversions."""
    return parent_tag or ''


def process_element(elem: etree._Element, parent_tag: Optional[str] = None) -> Tuple[list, list, Optional[str]]:
    """
    Process a single element and its attributes.

    Returns a tuple of:
    - List of elements to insert before this one (e.g., for push/negate)
    - List of elements to insert after this one (e.g., for min/max clamping, pop)
    - Optional comment string to insert before this element (e.g., for manual conversion TODOs)
    """
    before_elements = []
    after_elements = []
    comment = None
    tag = elem.tag

    # Skip non-element nodes (comments, processing instructions, etc.)
    if not isinstance(tag, str):
        return (before_elements, after_elements, comment)

    # ---- Element-specific attribute conversions ----

    # Variable element
    if tag == 'Variable':
        convert_variable_type(elem)

    # Button element
    elif tag == 'Button':
        convert_button_type(elem)
        convert_variable_reference(elem, 'Variable')
        # Rename Button-specific attributes
        for old_attr, new_attr in BUTTON_ATTRIBUTE_RENAMES.items():
            if old_attr in elem.attrib:
                elem.set(new_attr, elem.get(old_attr))
                del elem.attrib[old_attr]

    # If element
    elif tag == 'If':
        convert_conditional_operator(elem)
        # Handle Value -> Value1 rename
        if 'Value' in elem.attrib and 'Value1' not in elem.attrib:
            elem.set('Value1', elem.get('Value'))
            del elem.attrib['Value']
        # Convert to StaticIf if no runtime variables
        # Only StaticIf needs explicit <True> wrapper (preprocessor handles If)
        if convert_if_to_staticif(elem):
            wrap_implicit_children_in_true(elem)

    # Panel element
    elif tag == 'Panel':
        convert_panel_background_color(elem)

    # Set element
    elif tag == 'Set':
        convert_variable_reference(elem, 'Variable')
        min_val, max_val = convert_set_operator(elem)

        # Generate additional Set elements for min/max clamping
        var_name = elem.get('Variable')
        if var_name:
            if min_val:
                # min_val means "minimum allowed value" -> use #_set_max_
                clamp_elem = etree.Element('Set')
                clamp_elem.set('Variable', var_name)
                clamp_elem.set('Operator', '#_set_max_')
                clamp_elem.text = min_val
                after_elements.append(clamp_elem)
            if max_val:
                # max_val means "maximum allowed value" -> use #_set_min_
                clamp_elem = etree.Element('Set')
                clamp_elem.set('Variable', var_name)
                clamp_elem.set('Operator', '#_set_min_')
                clamp_elem.text = max_val
                after_elements.append(clamp_elem)

    # Blink element
    elif tag == 'Blink':
        convert_blink_attributes(elem)
        convert_variable_reference(elem, 'Variable')

    # MouseMotion element
    elif tag == 'MouseMotion':
        convert_variable_reference(elem, 'XVariable')
        convert_variable_reference(elem, 'YVariable')

    # Image element
    elif tag == 'Image':
        convert_image_content_to_attribute(elem)

    # DisplayLogic element (before rename)
    elif tag == 'DisplayLogic':
        convert_displaylogic_to_logic(elem)

    # ---- Common conversions ----

    # Alignment conversion for positionable elements
    if tag in ('Text', 'String', 'Button', 'Rectangle', 'Circle', 'Image',
               'Container', 'Line', 'Polygon', 'Ellipse', 'Arc', 'Vertex'):
        has_x = 'X' in elem.attrib or 'PositionX' in elem.attrib
        has_y = 'Y' in elem.attrib or 'PositionY' in elem.attrib
        convert_alignment(elem, has_x, has_y)

    # Origin conversion for positionable elements
    if tag in ('Text', 'String', 'Button', 'Rectangle', 'Circle', 'Image',
               'Container', 'Line', 'Polygon', 'Ellipse', 'Arc', 'Vertex'):
        origin_before, origin_after, origin_comment = convert_origin_attributes(elem)
        before_elements.extend(origin_before)
        after_elements.extend(origin_after)
        if origin_comment:
            comment = origin_comment

    # Element-specific attribute renames
    if tag in ELEMENT_ATTRIBUTE_RENAMES:
        for old_attr, new_attr in ELEMENT_ATTRIBUTE_RENAMES[tag].items():
            if old_attr in elem.attrib:
                elem.set(new_attr, elem.get(old_attr))
                del elem.attrib[old_attr]

    # Remove unsupported attributes
    remove_unsupported_attributes(elem)

    # Comment out not-yet-implemented attributes (prefix with '_')
    comment_out_attributes(elem)

    # ---- Element name conversion ----

    # Context-dependent element renames (Button children)
    if parent_tag == 'Button' and tag in BUTTON_CHILD_RENAMES:
        elem.tag = BUTTON_CHILD_RENAMES[tag]
    # Regular element renames
    elif tag in ELEMENT_RENAMES:
        elem.tag = ELEMENT_RENAMES[tag]

    return (before_elements, after_elements, comment)


def process_mask_element(elem: etree._Element) -> None:
    """
    Convert legacy <Mask><Stencil>...<Projection>... structure to new format.

    Legacy:
        <Mask>
            <Stencil>...</Stencil>
            <Projection>...</Projection>
        </Mask>

    New:
        <Stencil>
            <StencilAdd>...</StencilAdd>
            <StencilDraw>...</StencilDraw>
        </Stencil>
    """
    # Find Stencil and Projection children
    stencil_child = elem.find('Stencil')
    projection_child = elem.find('Projection')

    # Rename Mask to Stencil
    elem.tag = 'Stencil'

    # Rename children
    if stencil_child is not None:
        stencil_child.tag = 'StencilAdd'
    if projection_child is not None:
        projection_child.tag = 'StencilDraw'


def process_tree(elem: etree._Element, parent: Optional[etree._Element] = None, parent_tag: Optional[str] = None) -> list:
    """
    Recursively process an element tree.

    Returns a list of additional elements to insert after elem.
    """
    all_after = []

    # Skip comments and other non-element nodes
    if not isinstance(elem.tag, str):
        return all_after

    # Special handling for Mask elements
    if elem.tag == 'Mask':
        process_mask_element(elem)

    # Convert DisplayIndex/ActiveDisplay pattern to If conditionals
    if elem.tag == 'Window':
        convert_display_index_pattern(elem)

    # Comment out deprecated/unsupported elements (e.g., EdgeIo, ADI)
    if elem.tag in COMMENT_OUT_ELEMENTS and parent is not None:
        marker, message = COMMENT_OUT_ELEMENTS[elem.tag]
        # Serialize the element and its children to XML string
        elem_xml = etree.tostring(elem, encoding='unicode', pretty_print=True).strip()
        # Create comment with marker
        comment_text = f' {marker}: {message}\n{elem_xml}\n'
        comment = etree.Comment(comment_text)
        # Replace element with comment in parent
        idx = list(parent).index(elem)
        parent.remove(elem)
        parent.insert(idx, comment)
        return all_after  # Don't process children

    # Process this element
    before_elements, after_elements, todo_comment = process_element(elem, parent_tag)
    all_after.extend(after_elements)

    # Insert before elements (e.g., push/negate for origin conversion)
    if before_elements and parent is not None:
        idx = list(parent).index(elem)
        for i, before_elem in enumerate(before_elements):
            parent.insert(idx + i, before_elem)

    # Insert TODO comment before element if origin conversion needs manual review
    if todo_comment and parent is not None:
        comment = etree.Comment(todo_comment)
        idx = list(parent).index(elem)
        parent.insert(idx, comment)

    # Process children (skip comments)
    children_to_process = list(elem)
    for i, child in enumerate(children_to_process):
        # Skip comments (their tag is a callable, not a string)
        if not isinstance(child.tag, str):
            continue
        child_after = process_tree(child, elem, elem.tag)

        # Insert additional elements after the child
        if child_after:
            child_index = list(elem).index(child)
            for j, add_elem in enumerate(child_after):
                elem.insert(child_index + 1 + j, add_elem)

    return all_after


def convert_xml(input_text: str) -> str:
    """
    Convert legacy dcapp XML to new syntax.

    Args:
        input_text: Legacy XML as a string

    Returns:
        Converted XML as a string
    """
    # Parse XML with lxml (preserves comments)
    parser = etree.XMLParser(remove_comments=False, remove_blank_text=False)
    root = etree.fromstring(input_text.encode('utf-8'), parser)

    # Process tree
    additional = process_tree(root)

    # Insert any top-level additional elements
    # (shouldn't normally happen, but handle it)
    for add_elem in additional:
        root.append(add_elem)

    # Convert back to string (lxml preserves comments)
    output = etree.tostring(root, encoding='unicode', pretty_print=False)

    # Add XML declaration
    output = '<?xml version="1.0"?>\n' + output

    return output


def format_xml(xml_string: str, indent: str = '    ') -> str:
    """
    Format XML with proper indentation.

    Args:
        xml_string: XML string to format
        indent: Indentation string (default 4 spaces)

    Returns:
        Formatted XML string
    """
    # Parse with lxml (preserves comments)
    parser = etree.XMLParser(remove_comments=False, remove_blank_text=True)
    root = etree.fromstring(xml_string.encode('utf-8'), parser)

    # Format with lxml's pretty_print (preserves comments)
    formatted = etree.tostring(root, encoding='unicode', pretty_print=True)

    # Re-add our declaration
    return '<?xml version="1.0"?>\n' + formatted


def process_file(input_path: str, output_path: str = None, format_output: bool = False, in_place: bool = False) -> bool:
    """
    Process a single XML file.

    Returns True if successful, False if an error occurred.
    """
    try:
        with open(input_path, 'r') as f:
            input_text = f.read()

        output_text = convert_xml(input_text)

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

        return True
    except Exception as e:
        print(f"Error processing {input_path}: {e}", file=sys.stderr)
        return False


def process_directory(input_dir: str, format_output: bool = False) -> tuple[int, int]:
    """
    Recursively process all XML files in a directory (in-place).

    Returns tuple of (success_count, error_count).
    """
    import os

    success_count = 0
    error_count = 0

    for root, dirs, files in os.walk(input_dir):
        for filename in files:
            if filename.lower().endswith('.xml'):
                filepath = os.path.join(root, filename)
                print(f"Processing: {filepath}", file=sys.stderr)
                if process_file(filepath, in_place=True, format_output=format_output):
                    success_count += 1
                else:
                    error_count += 1

    return success_count, error_count


def main():
    parser = argparse.ArgumentParser(
        description='Convert legacy dcapp XML to new syntax',
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
        success, errors = process_directory(args.directory, format_output=args.format)
        print(f"\nProcessed {success + errors} file(s): {success} succeeded, {errors} failed", file=sys.stderr)
        sys.exit(0 if errors == 0 else 1)

    # Single file mode
    if not args.input:
        parser.error("Either 'input' or '--directory' is required")

    success = process_file(args.input, args.output, format_output=args.format, in_place=args.in_place)
    sys.exit(0 if success else 1)


if __name__ == '__main__':
    main()
