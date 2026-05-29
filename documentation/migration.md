# dcapp Legacy to New Syntax Conversion Guide

This document describes all changes required to convert legacy dcapp XML files and C++ logic files to the new dcapp syntax. It serves as a comprehensive migration reference covering element renames, attribute changes, constant values, alignment system overhaul, logic file porting, and behavioral differences between the legacy and new engines.

## Section 1: XML Element Name Changes

The following XML element names have been renamed:

| Legacy | New |
|--------|-----|
| `<String>` | `<Text>` |
| `<Defaults>` | `<Default>` |
| `<DisplayLogic>` | `<Logic File="..."/>` |

[Button](buttons.md) child elements for visual states:

| Legacy | New |
|--------|-----|
| `<Active>` | `<ButtonEnabled>` |
| `<Inactive>` | `<ButtonDisabled>` |
| `<On>` | `<ButtonIndicatorOn>` |
| `<Off>` | `<ButtonIndicatorOff>` |
| `<Transition>` | `<ButtonTransition>` |

Button event handlers:

| Legacy | New |
|--------|-----|
| `<OnPress>` | `<MousePressed>` |
| `<OnRelease>` | `<MouseReleased>` |

Conditional branches:

| Legacy | New |
|--------|-----|
| `<True>` | Still supported (no change needed) |
| `<False>` | Still supported (no change needed) |

Note: `<True>` and `<False>` remain valid children of `<If>`. The preprocessor also wraps any implicit children (elements directly inside `<If>`) in `<True>` blocks.

## Section 2: XML Attribute Name Changes

Text color attribute:

| Legacy | New |
|--------|-----|
| `Color` | `FillColor` |
| `BackgroundColor` | `BackgroundColor` |

Conditional operator attribute (inside `<If>` elements):

| Legacy | New |
|--------|-----|
| `Operation` | `Operator` |

Single-value conditional attribute:

| Legacy | New |
|--------|-----|
| `Value` | `Value1` |

[Button](buttons.md) attributes (enable/disable state):

| Legacy | New |
|--------|-----|
| `ActiveVariable` | `EnableVariable` |
| `ActiveOn` | `EnableOn` |

Button attributes (target variable):

| Legacy | New |
|--------|-----|
| `SwitchVariable` | `TargetVariable` |
| `SwitchOn` | `TargetOn` |
| `SwitchOff` | `TargetOff` |

The following display-selection attributes are still supported:

| Legacy | New |
|--------|-----|
| `DisplayIndex` | `DisplayIndex` |
| `ActiveDisplay` | `ActiveDisplay` |

## Section 3: Variable Type Constants

Variable types now use [constants](constants.md) instead of plain strings.

Legacy syntax:

```xml
<Variable Type="#_variable_double_">myVar</Variable>
<Variable Type="#_variable_integer_">myVar</Variable>
<Variable Type="#_variable_string_">myVar</Variable>
<Variable Type="#_variable_boolean_">myVar</Variable>
```

New syntax:

```xml
<Variable Type="#_variable_double_">myVar</Variable>
<Variable Type="#_variable_integer_">myVar</Variable>
<Variable Type="#_variable_string_">myVar</Variable>
<Variable Type="#_variable_boolean_">myVar</Variable>
```

Note: "Decimal" and "Float" were aliases for double in legacy. Both become `#_variable_double_` in the new syntax.

## Section 4: Alignment System (Important Change)

The legacy system had a single pair of alignment attributes:

    HorizontalAlign / VerticalAlign

These did DIFFERENT things depending on whether X/Y was set:

- WITH X/Y set: Alignment controlled the anchor point on the element
- WITHOUT X/Y: Alignment controlled BOTH position within parent AND anchor

The new system splits this into TWO SEPARATE attribute pairs:

    LocalAlignX / LocalAlignY     - Anchor point on the element itself
    ParentAlignX / ParentAlignY   - Anchor point in the parent container

See also [How Alignment and Position Work Together](primitives.md#how-alignment-and-position-work-together) for the full reference.

### Legacy Behavior (single HorizontalAlign/VerticalAlign)

Legacy only had `HorizontalAlign` and `VerticalAlign`. Their behavior depended on whether X/Y coordinates were specified:

**WITH X/Y set:**
- X/Y gave absolute position
- Alignment only affected the element's own anchor point

**WITHOUT X/Y:**
- Alignment controlled position within parent (centered, etc.)
- Alignment ALSO affected the element's own anchor point

### New Behavior (separate ParentAlign and LocalAlign)

The new system separates these two concepts:

    ParentAlignX/Y  - Where in the parent to anchor (LEFT=0, CENTER=width/2, etc.)
    LocalAlignX/Y   - Where on the element to anchor (left edge, center, etc.)

IMPORTANT: ParentAlign and X/Y now work TOGETHER as anchor + offset:

    position = parent_anchor + offset

Example with `ParentAlignX="#_align_center_"` and `X="10"` in an 800px wide parent:

    final_x = 400 (center) + 10 = 410

This allows powerful positioning like "10 pixels right of center" or "20 pixels above the bottom edge".

### Conversion Rules

**CASE 1:** Element HAS X and Y attributes set (absolute positioning)

Legacy:

```xml
<Text X="100" Y="200" HorizontalAlign="Center" VerticalAlign="Middle">
```

New (LocalAlign only, X/Y are absolute positions):

```xml
<Text X="100" Y="200" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_">
```

DO NOT add ParentAlign if you want to preserve legacy absolute positioning! Adding `ParentAlignX="#_align_center_"` would make X an offset from center.

**CASE 2:** Element has NO X and NO Y attributes (pure alignment)

Legacy:

```xml
<Text HorizontalAlign="Center" VerticalAlign="Middle">
```

New (BOTH ParentAlign AND LocalAlign needed):

```xml
<Text ParentAlignX="#_align_center_" ParentAlignY="#_align_middle_"
      LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_">
```

WHY BOTH? The legacy behavior was:
1. Position the element at the center of the parent (ParentAlign)
2. Use the element's center as the anchor point (LocalAlign)

If you only set ParentAlign, the element positions at parent center but anchors from its default corner, resulting in off-center placement.

**CASE 3:** Element has only X OR only Y (mixed)

Legacy:

```xml
<Button Y="100" HorizontalAlign="Center" VerticalAlign="Middle">
```

New:

```xml
<Button Y="100"
        ParentAlignX="#_align_center_" LocalAlignX="#_align_center_"
        LocalAlignY="#_align_middle_">
```

For the axis WITH a position (Y), use LocalAlign only.
For the axis WITHOUT a position (X), use BOTH ParentAlign and LocalAlign.

### Common Pitfall -- Defaults with ParentAlign + Absolute X Positions

If you have a Default that sets ParentAlignX, but individual elements use absolute X positions, those X values will become OFFSETS from the parent anchor instead of absolute positions.

WRONG (buttons end up offset from center instead of at absolute positions):

```xml
<Default>
    <Button ParentAlignX="#_align_center_" LocalAlignX="#_align_center_"/>
</Default>

<Button X="10">...</Button>   <!-- Ends up at center+10, not at X=10! -->
<Button X="50">...</Button>   <!-- Ends up at center+50, not at X=50! -->
```

CORRECT (remove ParentAlignX from Default if using absolute X positions):

```xml
<Default>
    <Button LocalAlignX="#_align_center_"/>
</Default>

<Button X="10">...</Button>   <!-- Correctly at X=10 -->
<Button X="50">...</Button>   <!-- Correctly at X=50 -->
```

### Special Case -- Arcs, Ellipses, and Spheres

Arc, Ellipse, and Sphere elements default to `LocalAlignX="#_align_center_"` and `LocalAlignY="#_align_middle_"` because their natural anchor point is their center. If you need corner-based positioning, explicitly set LocalAlign:

```xml
<Ellipse X="100" Y="100" Radius="50"
         LocalAlignX="#_align_left_" LocalAlignY="#_align_bottom_"/>
```

### Alignment Value Constants

Horizontal alignment values:

| Legacy | LocalAlignX | ParentAlignX |
|--------|-------------|--------------|
| `HorizontalAlign="Left"` | `#_align_left_` | `#_align_left_` |
| `HorizontalAlign="Center"` | `#_align_center_` | `#_align_center_` |
| `HorizontalAlign="Right"` | `#_align_right_` | `#_align_right_` |

Vertical alignment values:

| Legacy | LocalAlignY | ParentAlignY |
|--------|-------------|--------------|
| `VerticalAlign="Bottom"` | `#_align_bottom_` | `#_align_bottom_` |
| `VerticalAlign="Middle"` | `#_align_middle_` | `#_align_middle_` |
| `VerticalAlign="Top"` | `#_align_top_` | `#_align_top_` |

## Section 5: Button Type Constants

[Button](buttons.md) types now use [constants](constants.md) instead of plain strings.

Legacy syntax:

```xml
<Button Type="Standard">
<Button Type="Momentary">
<Button Type="Toggle">
```

New syntax:

```xml
<Button Type="#_button_standard_">
<Button Type="#_button_momentary_">
<Button Type="#_button_toggle_">
```

### Button Child Element Renames

Button visual state children:

| Legacy | New |
|--------|-----|
| `<Active>` | `<ButtonEnabled>` |
| `<Inactive>` | `<ButtonDisabled>` |
| `<On>` | `<ButtonIndicatorOn>` |
| `<Off>` | `<ButtonIndicatorOff>` |
| `<Transition>` | `<ButtonTransition>` |

Button event handlers:

| Legacy | New |
|--------|-----|
| `<OnPress>` | `<MousePressed>` |
| `<OnRelease>` | `<MouseReleased>` |

### Button Attribute Renames

Enable/disable state:

| Legacy | New |
|--------|-----|
| `ActiveVariable` | `EnableVariable` |
| `ActiveOn` | `EnableOn` |

Target variable (what gets set when button is pressed):

| Legacy | New |
|--------|-----|
| `SwitchVariable` | `TargetVariable` |
| `SwitchOn` | `TargetOn` |
| `SwitchOff` | `TargetOff` |

## Section 6: Conditional Operator Constants

Conditional operators in `<If>` elements now use [constants](constants.md).

Legacy syntax:

```xml
<If Value1="@x" Operation="gt" Value2="100">
<If Value1="@x" Operation="lt" Value2="0">
<If Value1="@x" Operation="eq" Value2="50">
<If Value1="@x" Operation="ne" Value2="50">
<If Value1="@x" Operation="gte" Value2="100">
<If Value1="@x" Operation="lte" Value2="0">
```

New syntax:

```xml
<If Value1="@x" Operator="#_if_gt_" Value2="100">
<If Value1="@x" Operator="#_if_lt_" Value2="0">
<If Value1="@x" Operator="#_if_eq_" Value2="50">
<If Value1="@x" Operator="#_if_ne_" Value2="50">
<If Value1="@x" Operator="#_if_gte_" Value2="100">
<If Value1="@x" Operator="#_if_lte_" Value2="0">
```

Note: The attribute name changed from `Operation` to `Operator`.

### Boolean Conditionals

`<True>` and `<False>` are STILL valid child elements of `<If>`. No conversion needed:

```xml
<If Value1="@myBool">
    <True>
        <!-- content when true -->
    </True>
    <False>
        <!-- content when false -->
    </False>
</If>
```

The preprocessor also wraps implicit children (elements directly inside `<If>`) in `<True>` blocks automatically:

```xml
<If Value1="@x" Operator="#_if_gt_" Value2="10">
    <Rectangle .../>   <!-- implicitly wrapped in <True> -->
</If>
```

### Single-Value Boolean Tests

For testing a single boolean value (not comparing two values), use the `#_if_true_` or `#_if_false_` operators:

```xml
<If Value1="@isEnabled" Operator="#_if_true_">
    <!-- shown when isEnabled is truthy (non-zero) -->
</If>

<If Value1="@isEnabled" Operator="#_if_false_">
    <!-- shown when isEnabled is falsy (zero) -->
</If>
```

Note: The attribute name changed from `Value` to `Value1`.

### Static Conditionals

If an `<If>` element has no runtime variable references (no `@` prefix or `#{}` macro in Value1 or Value2), the conversion script adds `Static="true"`. Static conditionals are evaluated at load time and their branch is baked into the display tree.

Legacy (no variables -- static):

```xml
<If Value1="1" Operation="eq" Value2="1">
    <Rectangle .../>
</If>
```

New:

```xml
<If Static="true" Value1="1" Operator="#_if_eq_" Value2="1">
    <True>
        <Rectangle .../>
    </True>
</If>
```

Runtime variables are detected by `@` or `#{}` patterns:

| Pattern | Result |
|---------|--------|
| `@VARNAME` | stays as `<If>` (no Static attribute) |
| `@#{MACRO}_SUFFIX` | stays as `<If>` (no Static attribute) |
| `#{MACRO}` | stays as `<If>` (no Static attribute) |
| literal values | becomes `<If Static="true">` |

## Section 7: Set Operator Constants

The `<Set>` element's Operator attribute now uses [constants](constants.md). See also [Set Operators](variables.md#set-operators).

Legacy syntax:

```xml
<Set Variable="@x">100</Set>           (or no Operator attribute)
<Set Variable="@x" Operator="+=">10</Set>
<Set Variable="@x" Operator="-=">10</Set>
<Set Variable="@x" Operator="*=">2</Set>
<Set Variable="@x" Operator="/=">2</Set>
```

New syntax:

```xml
<Set Variable="x" Operator="#_set_equal_">100</Set>
<Set Variable="x" Operator="#_set_add_">10</Set>
<Set Variable="x" Operator="#_set_subtract_">10</Set>
<Set Variable="x" Operator="#_set_multiply_">2</Set>
<Set Variable="x" Operator="#_set_divide_">2</Set>
```

IMPORTANT: Note that the Variable attribute NO LONGER uses the `@` prefix when referring to the variable itself. See Section 8.

## Section 8: Variable Reference Syntax

There are two contexts for referencing variables:

1. **READING a variable's value** (to display or use in expressions):
   - Use the `@` prefix: `@varname` or `@varname(%.2f)`
   - This has NOT changed from legacy

2. **REFERENCING which variable to modify** (in `Variable=""` attributes):
   - Legacy: `Variable="@varname"`
   - New: `Variable="varname"` (NO `@` prefix)

Examples:

Legacy:

```xml
<Set Variable="@counter" Operator="+=">1</Set>
<Text>Count: @counter</Text>
<Button Variable="@isEnabled">
```

New:

```xml
<Set Variable="counter" Operator="#_set_add_">1</Set>
<Text>Count: @counter</Text>
<Button Variable="isEnabled">
```

## Section 9: Image Element Syntax

The `<Image>` element now uses a `File` attribute instead of element content.

Legacy syntax:

```xml
<Image Width="100" Height="100">path/to/image.tga</Image>
```

New syntax:

```xml
<Image Width="100" Height="100" File="path/to/image.png"/>
```

Also recommended: Convert TGA files to PNG for smaller file sizes.

## Section 10: Logic File Changes (C/C++)

[Logic files](logic.md) have been simplified and now use plain C instead of C++.

### File Extension

    .cc or .cpp  -->  .c

### Function Names

| Legacy | New |
|--------|-----|
| `extern "C" void DisplayInit(void)` | `void display_init(void)` |
| `extern "C" void DisplayLogic(void)` | `void display_draw(void)` |
| `extern "C" void DisplayClose(void)` | `void display_close(void)` |

The `extern "C"` wrapper is no longer needed since files are now plain C.

### Header Macro for Multi-File Projects

    #define _DCAPP_EXTERNALS_  -->  #define _DCAPP_LOGIC_EXTERN_

Usage remains the same:
- Main logic file: Do NOT define the macro (gets actual variable definitions)
- Additional files: Define the macro before `#include` (gets extern declarations)

### Example Conversion

Legacy (`mylogic.cc`):

```c
#include "dcapp.h"

extern "C" void DisplayInit(void) {
    // initialization
}

extern "C" void DisplayLogic(void) {
    *MY_VARIABLE += 1.0;
}

extern "C" void DisplayClose(void) {
    // cleanup
}
```

New (`logic.c`):

```c
#include "dcapp.h"

void display_init(void) {
    // initialization
}

void display_draw(void) {
    *MY_VARIABLE += 1.0;
}

void display_close(void) {
    // cleanup
}
```

### String Variable Handling

String variables are now char arrays instead of C++ `std::string`.

Legacy (C++):

```c
*CURRENT_TIME = someString;
if (!CURRENT_TIME->empty()) CURRENT_TIME->pop_back();
```

New (C):

```c
strncpy(*CURRENT_TIME, someString, 255);
size_t len = strlen(*CURRENT_TIME);
if (len > 0 && (*CURRENT_TIME)[len-1] == '\n') {
    (*CURRENT_TIME)[len-1] = '\0';
}
```

## Section 11: Set Element Min/Max Changes

Legacy `<Set>` had `MinimumValue` and `MaximumValue` attributes for clamping:

```xml
<Set Variable="@value" Operator="+=" MinimumValue="0" MaximumValue="100">10</Set>
```

New syntax uses separate [Set operations](variables.md#set-operators) for clamping:

```xml
<Set Variable="value" Operator="#_set_add_">10</Set>
<Set Variable="value" Operator="#_set_max_">0</Set>
<Set Variable="value" Operator="#_set_min_">100</Set>
```

Note: `#_set_max_` clamps to a minimum value (ensures value >= operand).
`#_set_min_` clamps to a maximum value (ensures value <= operand).

## Section 12: Blink Element Changes

The `<Blink>` element's trigger attribute changed:

Legacy syntax:

```xml
<Blink Frequency=".8" DutyCycle=".70" Duration="-1" FnStartBlink="@TRIGGER_VAR">
```

New syntax:

```xml
<Blink Frequency="0.8" DutyCycle="0.7" Duration="0" FireBlink="@TriggerVar">
```

Changes:

| Legacy | New |
|--------|-----|
| `FnStartBlink="@varname"` | `FireBlink="@varname"` |
| `Duration="-1"` | `Duration="0"` (0 means indefinite, same behavior) |

Note: `FireBlink` uses the `Fire*` prefix convention for edge-triggered attributes. All `Fire*` attributes use `DcAppValIndex` and fire on value change via `dc_value_is_equal()`. See also: `FireRefresh` on `<PlanetTexture>`, `FireCall` on `<Function>`.

## Section 13: Panel BackgroundColor

Legacy Panel elements had a `BackgroundColor` attribute. New dcapp supports this directly on `<Panel>`.

Legacy syntax:

```xml
<Panel BackgroundColor="0 0 0" VirtualWidth="800" VirtualHeight="600">
    <!-- panel content -->
</Panel>
```

New syntax:

```xml
<Panel BackgroundColor="0 0 0" VirtualWidth="800" VirtualHeight="600">
    <!-- panel content -->
</Panel>
```

The background is drawn before panel children.

## Section 14: Legacy Attribute Status

The following legacy attributes either need rewrites or should be checked during migration:

Text/String attributes:

| Legacy | Status |
|--------|--------|
| `ShadowOffset` | Supported |
| `BackgroundColor` | Supported |
| `UpdateRate` | Supported |
| `ForceMono` | Removed (no monospace forcing) |

Window attributes:

| Legacy | Status |
|--------|--------|
| `ForceUpdate` | Removed (refresh rate handled differently) |
| `ActiveDisplay` | Supported |

Panel attributes:

| Legacy | Status |
|--------|--------|
| `DisplayIndex` | Supported |

Set attributes:

| Legacy | Replacement |
|--------|-------------|
| `MinimumValue` | Use separate `<Set>` with `Operator="#_set_max_"` |
| `MaximumValue` | Use separate `<Set>` with `Operator="#_set_min_"` |

## Section 15: Line Element Changes

The `<Line>` element's color attribute changed:

Legacy syntax:

```xml
<Line Color="0 1 0">
    <Vertex X="0" Y="0"/>
    <Vertex X="100" Y="100"/>
</Line>
```

New syntax:

```xml
<Line LineColor="0 1 0">
    <Vertex X="0" Y="0"/>
    <Vertex X="100" Y="100"/>
</Line>
```

Change: `Color` --> `LineColor`

## Section 16: DisplayLogic Element Variations

Legacy `<DisplayLogic>` had two syntax forms:

Form 1 (attribute):

```xml
<DisplayLogic File="logic/mylogic.so"/>
```

Form 2 (content):

```xml
<DisplayLogic>logic/mylogic.so</DisplayLogic>
```

New syntax (only one form):

```xml
<Logic File="logic/logic.so"/>
```

Both legacy forms convert to the new `<Logic File="..."/>` syntax. See [Logic Files](logic.md) for details.

## Section 17: Mask/Stencil Element Changes

Legacy syntax:

```xml
<Mask>
    <Stencil>
        <!-- shapes that define the mask -->
    </Stencil>
    <Projection>
        <!-- content shown through the mask -->
    </Projection>
</Mask>
```

New syntax:

```xml
<Stencil>
    <StencilAdd>
        <!-- shapes that define the mask -->
    </StencilAdd>
    <StencilDraw>
        <!-- content shown through the mask -->
    </StencilDraw>
</Stencil>
```

Changes:

| Legacy | New |
|--------|-----|
| `<Mask>` | Removed (use `<Stencil>` directly) |
| `<Stencil>` | `<StencilAdd>` |
| `<Projection>` | `<StencilDraw>` |

## Section 18: Elements Not Yet Implemented

The following legacy elements are not yet available in the new dcapp and should be commented out with a TODO note:

| Element | Description |
|---------|-------------|
| `<ADI>` | Attitude Direction Indicator (use samples/adi as reference -- partially implemented) |
| `<Animation>` | Animated value transitions |
| `<Map>` | Map display with `<UPSMapTexture>` |
| `<Hagstrom>` | Bezel keyboard hardware support |
| `<KeyboardEvent>` | Keyboard input handling |
| `BezelKey` attribute | Hardware bezel button binding |
| `<PixelStream>` with VSM protocol | |

The following elements ARE now implemented (removed from this list):

| Element | Notes |
|---------|-------|
| `<Function>` | Calling C functions from XML |
| `<Mask>`/`<Stencil>` | Stencil masking (see samples/stencil, samples/mask) |
| `<Blink>` | Blinking/flashing elements (see samples/blink) |
| `<MouseMotion>` | Mouse drag tracking |

## Section 19: Complete Conversion Example

Legacy XML:

```xml
<?xml version="1.0"?>
<DCAPP>
    <Variable Type="#_variable_double_" InitialValue="0.5">value</Variable>
    <Variable Type="#_variable_integer_">buttonState</Variable>

    <DisplayLogic File="logic/mylogic.so"/>

    <Defaults>
        <String Size="20" Color="1 1 1" HorizontalAlign="Center" VerticalAlign="Middle"/>
        <Button Type="Momentary" Width="100" Height="50" HorizontalAlign="Center"/>
    </Defaults>

    <Window X="0" Y="0" Width="800" Height="600" ActiveDisplay="1">
        <Panel DisplayIndex="1" BackgroundColor="0 0 0" VirtualWidth="800" VirtualHeight="600">

            <String X="400" Y="500">Value: @value(%.2f)</String>

            <Button X="400" Y="300" Variable="@buttonState">
                <OnPress>
                    <Set Variable="@value" Operator="+=">0.1</Set>
                    <If Value1="@value" Operator="gt" Value2="1">
                        <Set Variable="@value">0</Set>
                    </If>
                </OnPress>
                <On><Rectangle FillColor="0 1 0"/></On>
                <Off><Rectangle FillColor="0.5 0.5 0.5"/></Off>
                <String>Click Me</String>
            </Button>

        </Panel>
    </Window>
</DCAPP>
```

New XML:

```xml
<?xml version="1.0"?>
<DCAPP>
    <Variable Type="#_variable_double_" InitialValue="0.5">value</Variable>
    <Variable Type="#_variable_integer_">buttonState</Variable>

    <Logic File="logic/logic.so"/>

    <Default>
        <Text Size="20" FillColor="1 1 1"
              ParentAlignX="#_align_center_" ParentAlignY="#_align_middle_"
              LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"/>
        <Button Type="#_button_momentary_" Width="100" Height="50"
                ParentAlignX="#_align_center_" LocalAlignX="#_align_center_"/>
    </Default>

    <Window X="0" Y="0" Width="800" Height="600">
        <Panel BackgroundColor="0 0 0" VirtualWidth="800" VirtualHeight="600">

            <Text X="400" Y="500" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_">Value: @value(%.2f)</Text>

            <Button X="400" Y="300" LocalAlignX="#_align_center_" Variable="buttonState">
                <MousePressed>
                    <Set Variable="value" Operator="#_set_add_">0.1</Set>
                    <If Value1="@value" Operator="#_if_gt_" Value2="1">
                        <Set Variable="value" Operator="#_set_equal_">0</Set>
                    </If>
                </MousePressed>
                <ButtonIndicatorOn><Rectangle FillColor="0 1 0"/></ButtonIndicatorOn>
                <ButtonIndicatorOff><Rectangle FillColor="0.5 0.5 0.5"/></ButtonIndicatorOff>
                <Text>Click Me</Text>
            </Button>

        </Panel>
    </Window>
</DCAPP>
```

## Section 20: Deferred Set Operations (Legacy Compatibility)

The new dcapp engine executes `<Set>` operations immediately during the draw pass. The legacy engine had a different execution model with two separate code paths:

1. **Draw pass:** Sets executed immediately via `draw()`
2. **Event pass:** Sets inside OnPress/OnRelease were DEFERRED to a list and executed atomically after all event handlers finished

This means legacy Sets inside event blocks (OnPress/OnRelease) were deferred -- other elements drawn later in the same frame still saw the OLD variable values. The new engine's immediate execution changes this ordering, which can break converted legacy displays that depend on the deferred behavior.

### The Defer Attribute

To support legacy conversion, `<Set>` elements accept an optional `Defer` attribute:

```xml
<Set Variable="myVar" Operator="#_set_equal_" Defer="true">newValue</Set>
```

When `Defer` is `"true"`, the Set operation is collected during the draw pass and applied atomically AFTER the entire draw pass completes. This matches the legacy engine's deferred execution behavior. `Defer` is a standard `DcAppValIndex` boolean, parsed via `dc_app_create_and_register_typed_value_from_string`.

Without `Defer`, Sets execute immediately as before. Modern XML should NOT use `Defer` -- it exists solely for legacy conversion compatibility.

### Legacy Execution Model (from legacy-dcapp source)

The legacy engine (`primitives/setvalue.cc`) had two methods:

- `dcSetValue::draw()` -- Called during draw pass, executes immediately
- `dcSetValue::handleEvent()` -- Called during event handling, defers to `AppData.events` list for deferred execution

State blocks (Active/Inactive/On/Off/Transition) used `draw()`, so any Sets inside them executed immediately. Only event blocks (OnPress/OnRelease) used `handleEvent()`, which deferred Sets for deferred execution.

After event propagation completed, `ProcessEvents()` (`handle_utils.cc`) iterated the defer, called `updateData()` on each entry, cleared the defer, and triggered a display refresh.

This means:

**IMMEDIATE (during draw pass):**
- Sets inside `<Active>`/`<Inactive>` (now ButtonEnabled/ButtonDisabled)
- Sets inside `<On>`/`<Off>` (now ButtonIndicatorOn/ButtonIndicatorOff)
- Sets inside `<Transition>` (now ButtonTransition)
- Sets at the top level or inside `<If>` blocks outside events
- Any Set reached via the `draw()` code path

**DEFERRED (deferred until end of event handling):**
- Sets inside `<OnPress>` (now MousePressed)
- Sets inside `<OnRelease>` (now MouseReleased)
- Nested Sets (e.g., OnPress > If > Set) -- `handleEvent()` propagated recursively through the entire child tree

### Conversion Script Behavior

The conversion script (`convert-legacy-xml.py`) automatically adds `Defer="true"` to all `<Set>` elements that are descendants of MousePressed or MouseReleased blocks. This includes deeply nested Sets (e.g., MousePressed > If > True > Set).

Sets outside event blocks are left as immediate execution, matching legacy.

### SwitchVariable Conversion

Legacy `SwitchVariable` was a [button](buttons.md) attribute that set a variable when the button was pressed. The legacy engine implemented this internally by creating event handlers (`dcMouseEvent`) with Sets in the PressList/ReleaseList.

The conversion script converts `SwitchVariable` to explicit deferred Set elements rather than using `TargetVariable`. This is because `TargetVariable` is handled by the engine's built-in button logic, which would bypass the `Defer` mechanism.

Legacy:

```xml
<Button SwitchVariable="@CAMERA_SELECT" On="#Camera02Target" Type="Standard">
    <OnPress>
        <Set Variable="@PIXELSTREAMID">0</Set>
    </OnPress>
</Button>
```

Converted:

```xml
<Button IndicatorVariable="CAMERA_SELECT" IndicatorOn="#Camera02Target"
        Type="#_button_standard_">
    <MousePressed>
        <Set Variable="CAMERA_SELECT" Operator="#_set_equal_"
             Defer="true">#Camera02Target</Set>
    </MousePressed>
    <MousePressed>
        <Set Variable="PIXELSTREAMID" Operator="#_set_equal_"
             Defer="true">0</Set>
    </MousePressed>
</Button>
```

The first MousePressed (generated for SwitchVariable) is inserted as the first child. The second (original OnPress content) retains its position. Both use `Defer="true"` so all Sets are deferred and applied atomically after the draw pass.

`IndicatorVariable` replaces `SwitchVariable` for visual indicator state. No `TargetVariable` is set, so the button's built-in auto-set writes to an internal anonymous variable (harmless).

Fallback chain for switch values (matches legacy `xml_parse.cc` lines 713-714):

| Value | Fallback Chain |
|-------|---------------|
| Switch On value | SwitchOn -> On -> `"1"` |
| Switch Off value | SwitchOff -> Off -> `"0"` |

Button type handling:

| Type | Behavior |
|------|----------|
| Standard | MousePressed with Set to On value |
| Toggle | MousePressed with If/True/False conditional to flip between On and Off values based on IndicatorVariable state |
| Momentary | MousePressed with Set to On value, PLUS MouseReleased with Set to Off value (resets on release) |

### Implementation Details

C engine files modified:

| File | Changes |
|------|---------|
| `apps/dcapp.h` | `_NodeSet.deferred` flag, `_DeferredSetOp` struct, `_AppData.sb_deferred_sets` stretchy buffer |
| `apps/_dcapp_process_xml.c` | Parses `Defer` attribute (presence-based) |
| `apps/_dcapp_draw.c` | `_apply_set_operation()` helper, defer check in `_draw_node_set()`, `_flush_deferred_sets()` function |
| `apps/dcapp.c` | Calls `_flush_deferred_sets()` after `_draw_node()`, `sbfree()` in shutdown cleanup |

Defer mechanism:

1. During draw pass, if a Set has `deferred=true`, its operation type and operand value are snapshotted into a `_DeferredSetOp` struct and pushed onto the `sb_deferred_sets` stretchy buffer. The Set returns without modifying the variable.
2. After `_draw_node()` returns, `_flush_deferred_sets()` iterates the buffer, applies each operation via `_apply_set_operation()`, and clears the buffer.
3. Multiple deferred Sets to the same variable are applied in defer order (last write wins for EQUAL operations).

## Section 21: Arc/Ellipse Starting Angle -- Legacy Compatibility

Legacy Circle elements start drawing at 3 o'clock (standard math convention: angle=0 produces x=r, y=0). The modern Arc and Ellipse elements originally started at 12 o'clock by adding a `M_PI_2` (90-degree) offset. This was changed to match legacy behavior -- arcs and ellipse wedges now start at 3 o'clock.

Changes made:

- `_dcapp_draw.c`: Removed `+ (float)M_PI_2` from arc vertex generation and ellipse pie/wedge vertex generation. Full ellipse mode was already correct (no offset).
- `_dcapp_process_xml.c`: Changed Arc default Angle from 90 to 360 to match legacy Circle default (full circle). Ellipse already defaulted to 360 (`DC_APP_VAL_INDEX_UNDEFINED` treated as 360).

No conversion script changes needed -- Circle->Arc/Ellipse renaming works correctly since the C code now matches legacy behavior directly.
