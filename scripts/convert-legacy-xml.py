#!/usr/bin/env python3
"""
DCAPP Legacy to New XML Syntax Converter

Converts legacy dcapp XML files to the new syntax based on the rules
documented in documentation/notes.txt.

Usage:
    python convert-legacy-xml.py input.xml [output.xml]

If output.xml is not specified, outputs to stdout.
"""

import copy
import re
import sys
import argparse
from typing import Optional, Tuple
from lxml import etree

# ============================================================================
# SECTION 0: PYTHON3 VERSION CHECK
# ============================================================================
MIN_PY = (3, 9)  # adjust as needed
if sys.version_info < MIN_PY:
    print(
        f"[ERROR] Python {MIN_PY[0]}.{MIN_PY[1]}+ required, "
        f"but found {sys.version_info.major}.{sys.version_info.minor}.{sys.version_info.micro}\n"
        f"Reason: this script uses features introduced in Python {MIN_PY[0]}.{MIN_PY[1]} "
        f"(e.g., built-in generics like tuple[int, int])."
    )
    sys.exit(1)

# ============================================================================
# SECTION 1: ELEMENT NAME MAPPINGS
# ============================================================================

ELEMENT_RENAMES = {
    'String': 'Text',
    'Defaults': 'Default',
    'DisplayLogic': 'Logic',
    # Button event handlers (global - can appear inside Button or other elements)
    'OnPress': 'MousePressed',
    'OnRelease': 'MouseReleased',
    # Mask child elements - handled by process_mask_element, NOT here
    # TrickIO renames
    'TrickIo': 'TrickIO',
    'FromTrick': 'TrickFrom',
    'ToTrick': 'TrickTo',
    # EdgeIO renames
    'EdgeIo': 'EdgeIO',
    'FromEdge': 'EdgeFrom',
    'ToEdge': 'EdgeTo',
    # Case fix
    'Pixelstream': 'PixelStream',
}

# Button-specific child element renames
BUTTON_CHILD_RENAMES = {
    'On': 'ButtonIndicatorOn',
    'Off': 'ButtonIndicatorOff',
    'Active': 'ButtonEnabled',
    'Inactive': 'ButtonDisabled',
    'Transition': 'ButtonTransition',
}

# Button-specific attribute renames
BUTTON_ATTRIBUTE_RENAMES = {
    'ActiveVariable': 'EnableVariable',
    'ActiveOn': 'EnableOn',
    # SwitchVariable/SwitchOn/SwitchOff handled by custom logic in process_element
}


# ============================================================================
# SECTION 2: ATTRIBUTE NAME MAPPINGS
# ============================================================================

# Reference: attribute renames handled inline per element type
# - If: Operation → Operator, Value → Value1 (lines 607-612)
# - Text/String: Color → FillColor (via ELEMENT_ATTRIBUTE_RENAMES)
# - Line: Color → LineColor (via ELEMENT_ATTRIBUTE_RENAMES)

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
    'Hagstrom': ('TODO(deprecated)', 'Hagstrom bezel keyboard not yet supported'),
    'KeyboardEvent': ('TODO(migration)', 'KeyboardEvent not yet supported'),
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


def _is_inside_event_block(elem: etree._Element) -> bool:
    """Check if element is inside a MousePressed/MouseReleased event block."""
    parent = elem.getparent()
    while parent is not None:
        if isinstance(parent.tag, str) and parent.tag in ('MousePressed', 'MouseReleased'):
            return True
        parent = parent.getparent()
    return False


def convert_alignment(elem: etree._Element, has_x: bool, has_y: bool) -> None:
    """
    Convert HorizontalAlign/VerticalAlign to LocalAlign/ParentAlign (or AlignX/AlignY shorthand).

    Rules:
    - If axis has explicit position (X/Y), use only LocalAlign
    - If axis has no position, use AlignX/AlignY shorthand (expands to both Local and Parent)
    - Preserve constants (#) and variables (@) as-is
    - Convert literal values (Left, Center, Right, etc.) to constants
    - Also set PivotLocalAlignX/Y to match, since legacy used alignment for both
      the anchor point and the rotation pivot
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
        # Legacy used alignment as the rotation pivot too
        elem.set('PivotLocalAlignX', new_value)
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
        # Legacy used alignment as the rotation pivot too
        elem.set('PivotLocalAlignY', new_value)
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
    """Convert If element legacy Operation to Operator with constant."""
    attr_name = 'Operation'
    operation = elem.get(attr_name)
    if not operation:
        attr_name = 'Operator'
        operation = elem.get(attr_name)
    if operation:
        new_value = CONDITIONAL_OP_MAP.get(operation, operation)
        elem.set('Operator', new_value)
        if elem.get('Operation'):
            del elem.attrib['Operation']


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
    # FnStartBlink -> FireBlink (keep @ prefix — FireBlink uses ValIndex)
    fn_start = elem.get('FnStartBlink')
    if fn_start:
        # Ensure @ prefix: legacy had @varname, new syntax also needs @varname
        if not fn_start.startswith('@'):
            fn_start = '@' + fn_start
        elem.set('FireBlink', fn_start)
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


def _is_inside_default_or_style(elem: etree._Element) -> bool:
    """Check if element is inside a Default/Defaults or Style template."""
    parent = elem.getparent()
    while parent is not None:
        if isinstance(parent.tag, str) and parent.tag in ('Default', 'Defaults', 'Style'):
            return True
        parent = parent.getparent()
    return False


def flag_circles_with_rotation_pivot(root: etree._Element) -> None:
    """
    Pre-pass: add TODO comments for Circle elements with non-default alignment
    AND rotation. Legacy circles use parent dimensions for rotation pivot, which
    doesn't translate directly to modern Arc/Ellipse. Flag for manual review.
    """
    for circle in list(root.iter('Circle')):
        parent = circle.getparent()
        if parent is None:
            continue

        h_align = circle.get('HorizontalAlign')
        v_align = circle.get('VerticalAlign')
        rotation = circle.get('Rotation') or circle.get('Rotate')

        if not rotation:
            continue

        has_nondefault_h = h_align and h_align != 'Left'
        has_nondefault_v = v_align and v_align != 'Bottom'

        if not has_nondefault_h and not has_nondefault_v:
            continue

        comment = etree.Comment(
            ' TODO: Circle with alignment+rotation. Legacy rotation pivot is '
            'relative to parent dimensions, not circle center. May need manual '
            'adjustment. '
        )
        idx = list(parent).index(circle)
        parent.insert(idx, comment)


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
        # Strip alignment from Ellipse copy (not re-processed through convert)
        if 'HorizontalAlign' in ellipse_elem.attrib:
            del ellipse_elem.attrib['HorizontalAlign']
        if 'VerticalAlign' in ellipse_elem.attrib:
            del ellipse_elem.attrib['VerticalAlign']
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

    # Strip alignment from Circle->Arc/Ellipse conversions.
    # Legacy circles use containerw/containerh (parent dimensions) for alignment
    # math, not the circle's own diameter. X,Y is always the circle center
    # regardless of alignment. Modern Arc/Ellipse default to CENTER alignment,
    # which correctly makes X,Y the center. Converting legacy alignment would
    # override this default and break positioning.
    if tag in ('Arc', 'Ellipse'):
        if 'HorizontalAlign' in elem.attrib:
            del elem.attrib['HorizontalAlign']
        if 'VerticalAlign' in elem.attrib:
            del elem.attrib['VerticalAlign']

    # ---- Element-specific attribute conversions ----

    # Variable element
    if tag == 'Variable':
        convert_variable_type(elem)

    # Button element
    elif tag == 'Button':
        convert_button_type(elem)

        # Handle SwitchVariable -> IndicatorVariable + explicit deferred Set
        switch_var_raw = elem.get('SwitchVariable')
        if switch_var_raw:
            switch_var = strip_at_prefix(switch_var_raw)
            switch_on = elem.get('SwitchOn')
            switch_off = elem.get('SwitchOff')
            on_value = elem.get('On')
            btn_type_raw = elem.get('Type') or ''

            # Determine the on/off values for the deferred set
            set_on_value = switch_on or on_value or '1'
            set_off_value = switch_off or elem.get('Off') or '0'

            # Determine indicator variable and on value for toggle conditional
            indicator_var = strip_at_prefix(elem.get('IndicatorVariable') or switch_var_raw)
            indicator_on = elem.get('IndicatorOn') or switch_on or on_value or '1'

            # Set IndicatorVariable and IndicatorOn (only if not already explicitly provided)
            if 'IndicatorVariable' not in elem.attrib:
                elem.set('IndicatorVariable', switch_var)
            if 'IndicatorOn' not in elem.attrib:
                elem.set('IndicatorOn', set_on_value)

            # Remove old attributes — SwitchVariable consumed by explicit Set,
            # On/Off consumed by IndicatorOn. Leaving On/Off would cause the
            # C parser to create an anonymous TargetVariable, which breaks
            # the transition check (anonymous var != indicator var = always transitioning).
            del elem.attrib['SwitchVariable']
            if 'SwitchOn' in elem.attrib:
                del elem.attrib['SwitchOn']
            if 'SwitchOff' in elem.attrib:
                del elem.attrib['SwitchOff']
            if 'On' in elem.attrib:
                del elem.attrib['On']
            if 'Off' in elem.attrib:
                del elem.attrib['Off']

            # Store metadata for post-children generation in process_tree
            elem.set('_dcapp_switch_var', switch_var)
            elem.set('_dcapp_switch_on', set_on_value)
            elem.set('_dcapp_switch_off', set_off_value)
            elem.set('_dcapp_switch_btn_type', btn_type_raw)
            elem.set('_dcapp_switch_ind_var', indicator_var)
            elem.set('_dcapp_switch_ind_on', indicator_on)

        # Rename remaining Button-specific attributes
        for old_attr, new_attr in BUTTON_ATTRIBUTE_RENAMES.items():
            if old_attr in elem.attrib:
                elem.set(new_attr, elem.get(old_attr))
                del elem.attrib[old_attr]
        # remove leading @s
        convert_variable_reference(elem, 'Variable')
        convert_variable_reference(elem, 'IndicatorVariable')
        convert_variable_reference(elem, 'TargetVariable')
        convert_variable_reference(elem, 'EnableVariable')

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

        # Add Defer="true" to Sets inside event blocks (legacy queuing behavior)
        if _is_inside_event_block(elem):
            elem.set('Defer', 'true')

        # Generate additional Set elements for min/max clamping
        var_name = elem.get('Variable')
        inside_event = _is_inside_event_block(elem)
        if var_name:
            if min_val:
                # min_val means "minimum allowed value" -> use #_set_max_
                clamp_elem = etree.Element('Set')
                clamp_elem.set('Variable', var_name)
                clamp_elem.set('Operator', '#_set_max_')
                clamp_elem.text = min_val
                if inside_event:
                    clamp_elem.set('Defer', 'true')
                after_elements.append(clamp_elem)
            if max_val:
                # max_val means "maximum allowed value" -> use #_set_min_
                clamp_elem = etree.Element('Set')
                clamp_elem.set('Variable', var_name)
                clamp_elem.set('Operator', '#_set_min_')
                clamp_elem.text = max_val
                if inside_event:
                    clamp_elem.set('Defer', 'true')
                after_elements.append(clamp_elem)

    # Blink element
    elif tag == 'Blink':
        convert_blink_attributes(elem)

    # MouseMotion element
    elif tag == 'MouseMotion':
        convert_variable_reference(elem, 'XVariable')
        convert_variable_reference(elem, 'YVariable')
        # Rename XVariable → VariableX, YVariable → VariableY
        if 'XVariable' in elem.attrib:
            elem.set('VariableX', elem.get('XVariable'))
            del elem.attrib['XVariable']
        if 'YVariable' in elem.attrib:
            elem.set('VariableY', elem.get('YVariable'))
            del elem.attrib['YVariable']

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
               'Sphere', 'Terrain', 'PixelStream', 'Pixelstream'):
        has_x = 'X' in elem.attrib or 'PositionX' in elem.attrib
        has_y = 'Y' in elem.attrib or 'PositionY' in elem.attrib
        convert_alignment(elem, has_x, has_y)

    # Origin conversion for positionable elements
    if tag in ('Text', 'String', 'Button', 'Rectangle', 'Circle', 'Image',
               'Container', 'Line', 'Polygon', 'Ellipse', 'Arc', 'Vertex',
               'Sphere', 'Terrain', 'PixelStream', 'Pixelstream'):
        convert_origin_attributes(elem)

    # Prevent Default/Style alignment inheritance from making explicit positions relative.
    # ParentAlignX="#_align_left_" and ParentAlignY="#_align_bottom_" are the defaults, so
    # this is harmless for elements not inheriting from Default/Style. But it blocks an
    # inherited AlignX/AlignY (which expands to ParentAlignX + LocalAlignX) from turning
    # absolute positions into relative offsets from the parent anchor.
    if tag in ('Text', 'String', 'Button', 'Rectangle', 'Circle', 'Image',
               'Container', 'Line', 'Polygon', 'Ellipse', 'Arc', 'Vertex',
               'Sphere', 'Terrain', 'PixelStream', 'Pixelstream'):
        if not _is_inside_default_or_style(elem):
            has_x = 'X' in elem.attrib or 'PositionX' in elem.attrib
            has_y = 'Y' in elem.attrib or 'PositionY' in elem.attrib
            if has_x and 'ParentAlignX' not in elem.attrib:
                elem.set('ParentAlignX', '#_align_left_')
            if has_y and 'ParentAlignY' not in elem.attrib:
                elem.set('ParentAlignY', '#_align_bottom_')

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
        elif child.tag == 'StencilSub':
            child.tag = 'StencilRemove'


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

    # Post-children: generate deferred Set for SwitchVariable buttons
    if isinstance(elem.tag, str) and elem.tag == 'Button' and '_dcapp_switch_var' in elem.attrib:
        switch_var = elem.get('_dcapp_switch_var')
        switch_on = elem.get('_dcapp_switch_on')
        switch_off = elem.get('_dcapp_switch_off')
        btn_type = elem.get('_dcapp_switch_btn_type')
        indicator_var = elem.get('_dcapp_switch_ind_var')
        indicator_on = elem.get('_dcapp_switch_ind_on')

        # Clean up temporary attributes
        del elem.attrib['_dcapp_switch_var']
        del elem.attrib['_dcapp_switch_on']
        del elem.attrib['_dcapp_switch_off']
        del elem.attrib['_dcapp_switch_btn_type']
        del elem.attrib['_dcapp_switch_ind_var']
        del elem.attrib['_dcapp_switch_ind_on']

        is_toggle = btn_type.lower() in ('toggle', '#_button_toggle_') if btn_type else False
        is_momentary = btn_type.lower() in ('momentary', '#_button_momentary_') if btn_type else False

        if is_toggle:
            # Toggle: conditional set — flip based on indicator state
            mouse_pressed = etree.Element('MousePressed')
            if_elem = etree.SubElement(mouse_pressed, 'If')
            if_elem.set('Value1', '@' + indicator_var)
            if_elem.set('Operator', '#_if_eq_')
            if_elem.set('Value2', indicator_on)
            # If indicator is On -> set to Off
            true_elem = etree.SubElement(if_elem, 'True')
            set_off_elem = etree.SubElement(true_elem, 'Set')
            set_off_elem.set('Variable', switch_var)
            set_off_elem.set('Operator', '#_set_equal_')
            set_off_elem.set('Defer', 'true')
            set_off_elem.text = switch_off
            # If indicator is Off -> set to On
            false_elem = etree.SubElement(if_elem, 'False')
            set_on_elem = etree.SubElement(false_elem, 'Set')
            set_on_elem.set('Variable', switch_var)
            set_on_elem.set('Operator', '#_set_equal_')
            set_on_elem.set('Defer', 'true')
            set_on_elem.text = switch_on
            elem.insert(0, mouse_pressed)
        else:
            # Standard/Momentary: set to On value on press
            mouse_pressed = etree.Element('MousePressed')
            set_elem = etree.SubElement(mouse_pressed, 'Set')
            set_elem.set('Variable', switch_var)
            set_elem.set('Operator', '#_set_equal_')
            set_elem.set('Defer', 'true')
            set_elem.text = switch_on
            elem.insert(0, mouse_pressed)

            # Momentary: also set to Off value on release
            if is_momentary:
                mouse_released = etree.Element('MouseReleased')
                set_rel_elem = etree.SubElement(mouse_released, 'Set')
                set_rel_elem.set('Variable', switch_var)
                set_rel_elem.set('Operator', '#_set_equal_')
                set_rel_elem.set('Defer', 'true')
                set_rel_elem.text = switch_off
                elem.insert(1, mouse_released)

    return all_after


def force_stencil_mask_colors(root: etree._Element) -> None:
    """
    Post-pass: set FillColor/LineColor to #_stencil_color_ on elements inside
    StencilAdd/StencilRemove so stencil masks are fully opaque.
    """
    STENCIL_COLOR = '#_stencil_color_'
    FILL_ELEMENTS = {'Rectangle', 'Ellipse', 'Polygon', 'Text'}

    for stencil in root.iter('StencilAdd', 'StencilRemove'):
        for elem in stencil.iter():
            if not isinstance(elem.tag, str):
                continue
            if elem.tag in FILL_ELEMENTS:
                elem.set('FillColor', STENCIL_COLOR)
            if 'LineColor' in elem.attrib:
                elem.set('LineColor', STENCIL_COLOR)


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

    # Pre-pass: flag circles with alignment+rotation for manual review
    flag_circles_with_rotation_pivot(root)

    # Process tree
    additional = process_tree(root)

    # Insert any top-level additional elements
    # (shouldn't normally happen, but handle it)
    for add_elem in additional:
        root.append(add_elem)

    # Post-pass: force stencil mask colors
    force_stencil_mask_colors(root)

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
