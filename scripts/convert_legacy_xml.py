#!/usr/bin/env python3
"""
DCAPP Legacy to New XML Syntax Converter

Converts legacy dcapp XML files to the new syntax based on the rules
documented in documentation/notes.txt.

Usage:
    python convert_legacy_xml.py input.xml [output.xml]

If output.xml is not specified, outputs to stdout.
"""

import copy
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
    # Mask child elements - handled by process_mask_element, NOT here
    # TrickIO renames
    'TrickIo': 'TrickIO',
    'FromTrick': 'TrickFrom',
    'ToTrick': 'TrickTo',
    # EdgeIO renames
    'EdgeIo': 'EdgeIO',
    'FromEdge': 'EdgeFrom',
    'ToEdge': 'EdgeTo',
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
    # Note: DisplayIndex/ActiveDisplay are preserved for runtime handling
}

# Attributes to prefix with '_' (not yet implemented, preserved for future)
COMMENT_OUT_ATTRIBUTES = {
    'UpdateRate',       # Text - throttle string refresh rate
    'Pattern',          # Shapes/Lines - line pattern (dashed, dotted, etc.)
    'LinePattern',      # Lines - line pattern (dashed, dotted, etc.)
    'LineFactor',       # Lines - line pattern repeat factor
    'Key',              # Button - keyboard shortcut
    'BezelKey',         # Button - bezel key binding
    'ShadowOffset',     # Text - shadow/drop shadow effect
    'ForceMono',        # Text - force monospace rendering
    'DisconnectAction', # TrickIO - action on disconnect
    'FullScreen',       # Window - fullscreen mode
    'Font',             # Text - font selection
    'KeyASCII',         # Button - ASCII key binding
    'Face',             # Text - font face
    'Camera',           # PixelStream/Terrain - camera binding
    'ZeroTrim',         # Text - snap near-zero values to zero (use format specifier instead)
}

# Elements to comment out with TODO markers
# Maps element name -> (marker, message)
COMMENT_OUT_ELEMENTS = {
    'ADI': ('TODO(deprecated)', 'Use TexturedSphere instead of ADI'),
    'Animation': ('TODO(migration)', 'Animation not yet supported'),
    'CAN': ('TODO(deprecated)', 'CAN hardware interface not yet supported'),
    'Map': ('TODO(deprecated)', 'Use Terrain instead of Map'),
    'UEI': ('TODO(deprecated)', 'UEI hardware interface not yet supported'),
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
    'gt': '#_if_gt_',
    'lt': '#_if_lt_',
    'eq': '#_if_eq_',
    'ne': '#_if_ne_',
    'gte': '#_if_gte_',
    'lte': '#_if_lte_',
    'ge': '#_if_gte_',
    'le': '#_if_lte_',
    '>': '#_if_gt_',
    '<': '#_if_lt_',
    '==': '#_if_eq_',
    '!=': '#_if_ne_',
    '>=': '#_if_gte_',
    '<=': '#_if_lte_',
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
    - Preserve constants (#) and variables (@) as-is
    - Convert literal values (Left, Center, Right, etc.) to constants
    """
    h_align = elem.get('HorizontalAlign')
    v_align = elem.get('VerticalAlign')

    if h_align:
        # Preserve constants and variables as-is, only convert literal values
        if h_align.startswith('#') or h_align.startswith('@'):
            new_value = h_align
        else:
            new_value = HORIZONTAL_ALIGN_MAP.get(h_align, '#_align_left_')
        if has_x:
            # Only local align when position is explicit
            elem.set('LocalAlignX', new_value)
        else:
            # Use shorthand when both would be the same
            elem.set('AlignX', new_value)
        del elem.attrib['HorizontalAlign']

    if v_align:
        # Preserve constants and variables as-is, only convert literal values
        if v_align.startswith('#') or v_align.startswith('@'):
            new_value = v_align
        else:
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


def convert_if_operator(elem: etree._Element) -> None:
    """Convert If element Operator to Operation with constant."""
    operator = elem.get('Operator')
    if operator:
        new_value = CONDITIONAL_OP_MAP.get(operator, operator)
        elem.set('Operation', new_value)
        del elem.attrib['Operator']


def has_variable_reference(value: Optional[str]) -> bool:
    """Check if a value contains a runtime variable reference (@prefix)."""
    if value is None:
        return False
    return '@' in value


def convert_if_to_static(elem: etree._Element) -> bool:
    """
    Add Static="true" to <If> if it has no runtime variable references.

    Returns True if conversion was made, False otherwise.
    """
    value1 = elem.get('Value1') or elem.get('Value')
    value2 = elem.get('Value2')

    # If neither value has a @ variable reference, this is a static conditional
    if not has_variable_reference(value1) and not has_variable_reference(value2):
        elem.set('Static', 'true')
        return True
    return False


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
    """Convert <Image>path.tga</Image> to <Image File="path.tga"/>."""
    if elem.text and elem.text.strip():
        file_path = elem.text.strip()
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


def convert_pixelstream_attributes(elem: etree._Element) -> None:
    """
    Convert PixelStream attributes from legacy to new format.

    - Host/Port/Path -> URL (e.g., "localhost:8080/video")
    - Protocol="MJPEG" -> Type="#_pixelstream_mjpeg_"
    - Protocol="DFILE" -> Type="#_pixelstream_shmem_"
    - Type (if present) also converted

    Note: Unsupported Protocol/Type values are handled in process_tree() by commenting
    out the entire element before this function is called.
    """
    # Convert Host/Port/Path to URL
    host = elem.get('Host')
    port = elem.get('Port')
    path = elem.get('Path')

    if host or port or path:
        # Build URL from components
        url_parts = []
        if host:
            url_parts.append(host)
            del elem.attrib['Host']
        if port:
            if url_parts:
                url_parts.append(':')
            url_parts.append(port)
            del elem.attrib['Port']
        if path:
            # Ensure path starts with / if we have host/port
            if url_parts and not path.startswith('/'):
                url_parts.append('/')
            url_parts.append(path)
            del elem.attrib['Path']

        url = ''.join(url_parts)
        if url and not url.startswith(('http://', 'https://')):
            url = 'http://' + url
        if url:
            elem.set('URL', url)

    # Convert Protocol attribute (legacy) to Type
    protocol = elem.get('Protocol')
    if protocol:
        # Preserve constants and variables as-is
        if protocol.startswith('#') or protocol.startswith('@'):
            elem.set('Type', protocol)
        else:
            protocol_lower = protocol.lower()
            if protocol_lower == 'mjpeg':
                elem.set('Type', '#_pixelstream_mjpeg_')
            elif protocol_lower in ('dfile', 'dynamicfile', 'dynamic_file', 'shmem'):
                elem.set('Type', '#_pixelstream_shmem_')
            else:
                elem.set('Type', protocol)  # preserve unknown literal
        del elem.attrib['Protocol']

    # Also handle Type attribute if present (for already-converted or mixed files)
    pxs_type = elem.get('Type')
    if pxs_type and not pxs_type.startswith('#') and not pxs_type.startswith('@'):
        pxs_type_lower = pxs_type.lower()
        if pxs_type_lower == 'mjpeg':
            elem.set('Type', '#_pixelstream_mjpeg_')
        elif pxs_type_lower in ('dfile', 'dynamicfile', 'dynamic_file', 'shmem'):
            elem.set('Type', '#_pixelstream_shmem_')


def convert_origin_attributes(elem: etree._Element) -> None:
    """
    Convert OriginX/OriginY attributes to ParentAlign with NegateX/NegateY.

    Legacy OriginX="Right" means X is measured from the right edge.
    New system uses ParentAlignX="#_align_right_" with NegateX="true".

    For literal values, the X/Y value is negated directly.
    For variable values, NegateX/NegateY="true" is used to negate at runtime.
    Constants and variables are preserved as ParentAlignX/Y.
    """
    origin_x_raw = elem.get('OriginX')
    origin_y_raw = elem.get('OriginY')

    if origin_x_raw:
        # Preserve constants and variables as-is
        if origin_x_raw.startswith('#') or origin_x_raw.startswith('@'):
            elem.set('ParentAlignX', origin_x_raw)
        else:
            origin_x = origin_x_raw.strip().lower()
            if origin_x == 'right' or origin_x == '3':  # 3 = right enum value
                x_val = elem.get('X')
                if x_val and has_variable_reference(x_val):
                    # Use NegateX for variable references
                    elem.set('NegateX', 'true')
                    elem.set('ParentAlignX', '#_align_right_')
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
                else:
                    # No X value, just set the alignment
                    elem.set('ParentAlignX', '#_align_right_')
            elif origin_x == 'center' or origin_x == '2':  # 2 = center enum value
                # Center origin - set alignment, no negation needed
                elem.set('ParentAlignX', '#_align_center_')
            # Left/1 is default, no action needed
        del elem.attrib['OriginX']

    if origin_y_raw:
        # Preserve constants and variables as-is
        if origin_y_raw.startswith('#') or origin_y_raw.startswith('@'):
            elem.set('ParentAlignY', origin_y_raw)
        else:
            origin_y = origin_y_raw.strip().lower()
            if origin_y == 'top' or origin_y == '6':  # 6 = top enum value
                y_val = elem.get('Y')
                if y_val and has_variable_reference(y_val):
                    # Use NegateY for variable references
                    elem.set('NegateY', 'true')
                    elem.set('ParentAlignY', '#_align_top_')
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
                else:
                    # No Y value, just set the alignment
                    elem.set('ParentAlignY', '#_align_top_')
            elif origin_y == 'middle' or origin_y == '5':  # 5 = middle enum value
                # Middle origin - set alignment, no negation needed
                elem.set('ParentAlignY', '#_align_middle_')
            # Bottom/4 is default, no action needed
        del elem.attrib['OriginY']


def get_element_context(elem: etree._Element, parent_tag: Optional[str]) -> str:
    """Determine the context of an element for context-dependent conversions."""
    return parent_tag or ''


def process_element(elem: etree._Element, parent_tag: Optional[str] = None) -> list:
    """
    Process a single element and its attributes.

    Returns a list of elements to insert after this one (e.g., for min/max clamping).
    """
    after_elements = []
    tag = elem.tag

    # Skip non-element nodes (comments, processing instructions, etc.)
    if not isinstance(tag, str):
        return after_elements

    # ---- Early tag conversions (before attribute processing) ----

    # Circle inside Style -> duplicate as both Arc and Ellipse
    # (can't know which will be used, so provide both style defaults)
    if tag == 'Circle' and parent_tag == 'Style':
        # Create Ellipse copy first (preserves FillColor if present)
        ellipse_elem = copy.deepcopy(elem)
        ellipse_elem.tag = 'Ellipse'
        after_elements.append(ellipse_elem)
        # Convert this Circle to Arc
        elem.tag = 'Arc'
        tag = 'Arc'
        # Remove FillColor from Arc (line-only)
        if 'FillColor' in elem.attrib:
            del elem.attrib['FillColor']

    # Circle -> Arc or Ellipse conversion (must happen before attribute checks)
    # Arc is line-only (no fill). Ellipse supports fill and pie/wedge shapes.
    elif tag == 'Circle':
        if 'FillColor' in elem.attrib:
            # Has fill color - convert to Ellipse
            elem.tag = 'Ellipse'
            tag = 'Ellipse'
        else:
            # Line only - convert to Arc
            elem.tag = 'Arc'
            tag = 'Arc'

    # ---- Element-specific attribute conversions ----

    # Variable element
    if tag == 'Variable':
        convert_variable_type(elem)

    # Button element
    elif tag == 'Button':
        convert_button_type(elem)
        # Rename Button-specific attributes
        for old_attr, new_attr in BUTTON_ATTRIBUTE_RENAMES.items():
            if old_attr in elem.attrib:
                elem.set(new_attr, elem.get(old_attr))
                del elem.attrib[old_attr]
        # remoev leading @s
        convert_variable_reference(elem, 'Variable')
        convert_variable_reference(elem, 'IndicatorVariable')
        convert_variable_reference(elem, 'TargetVariable')
        convert_variable_reference(elem, 'EnabledVariable')

    # If element
    elif tag == 'If':
        convert_if_operator(elem)
        # Handle Value -> Value1 rename
        if 'Value' in elem.attrib and 'Value1' not in elem.attrib:
            elem.set('Value1', elem.get('Value'))
            del elem.attrib['Value']
        # Add Static="true" if no runtime variables
        convert_if_to_static(elem)

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

    # PixelStream element
    elif tag == 'PixelStream' or tag == 'Pixelstream':
        convert_pixelstream_attributes(elem)

    # TrickIo - updated variable reference
    elif tag == 'TrickIo':
        convert_variable_reference(elem, 'ConnectedVariable')

    # EdgeVariable - rename RcsCommand to Command
    elif tag == 'EdgeVariable':
        if 'RcsCommand' in elem.attrib:
            elem.set('Command', elem.get('RcsCommand'))
            del elem.attrib['RcsCommand']

    # EdgeIo - updated variable reference
    elif tag == 'EdgeIo':
        convert_variable_reference(elem, 'ConnectedVariable')

    # ---- Common conversions ----

    # Alignment conversion for positionable elements
    if tag in ('Text', 'String', 'Button', 'Rectangle', 'Circle', 'Image',
               'Container', 'Line', 'Polygon', 'Ellipse', 'Arc', 'Vertex',
               'Sphere', 'Terrain', 'PixelStream'):
        has_x = 'X' in elem.attrib or 'PositionX' in elem.attrib
        has_y = 'Y' in elem.attrib or 'PositionY' in elem.attrib
        convert_alignment(elem, has_x, has_y)

    # Origin conversion for positionable elements
    if tag in ('Text', 'String', 'Button', 'Rectangle', 'Circle', 'Image',
               'Container', 'Line', 'Polygon', 'Ellipse', 'Arc', 'Vertex',
               'Sphere', 'Terrain', 'PixelStream'):
        convert_origin_attributes(elem)

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

    return after_elements


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
    # Rename Mask to Stencil
    elem.tag = 'Stencil'

    # Rename all Stencil and Projection children
    for child in elem:
        if not isinstance(child.tag, str):
            continue
        if child.tag == 'Stencil':
            child.tag = 'StencilAdd'
        elif child.tag == 'Projection':
            child.tag = 'StencilDraw'


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

    # Note: DisplayIndex/ActiveDisplay are preserved - runtime handles this pattern

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

    # Comment out PixelStream with unsupported Type/Protocol (but not constants/variables)
    if (elem.tag == 'PixelStream' or elem.tag == 'Pixelstream') and parent is not None:
        # Check both Protocol (legacy) and Type attributes
        pxs_type_raw = elem.get('Protocol') or elem.get('Type') or ''
        # Don't comment out constants or variables - they might resolve to valid types at runtime
        if not pxs_type_raw.startswith('#') and not pxs_type_raw.startswith('@'):
            pxs_type = pxs_type_raw.lower()
            supported_types = ('mjpeg', 'dfile', 'dynamicfile', 'dynamic_file', 'shmem', '')
            if pxs_type not in supported_types:
                elem_xml = etree.tostring(elem, encoding='unicode', pretty_print=True).strip()
                comment_text = f' TODO(deprecated): PixelStream Protocol/Type="{pxs_type_raw}" not supported, use shmem or mjpeg\n{elem_xml}\n'
                comment = etree.Comment(comment_text)
                idx = list(parent).index(elem)
                parent.remove(elem)
                parent.insert(idx, comment)
                return all_after

    # Process this element
    after_elements = process_element(elem, parent_tag)
    all_after.extend(after_elements)

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
    parser.add_argument('-f', '--format', action='store_true', default=True,
                        help='Format output with indentation (default: True)')
    parser.add_argument('--no-format', action='store_false', dest='format',
                        help='Disable output formatting')
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
