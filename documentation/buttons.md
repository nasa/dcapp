# dcapp Button Reference

A comprehensive guide to creating interactive buttons in dcapp.

---

## Overview

The `<Button>` element creates interactive UI controls with multiple visual states. Buttons can toggle variables, respond to clicks, and display different content based on their current state.

```xml
<Button X="100" Y="100" Width="80" Height="40" Variable="myVar" On="1" Off="0">
    <ButtonEnabled>
        <ButtonIndicatorOn><!-- Content when indicator matches "on" value --></ButtonIndicatorOn>
        <ButtonIndicatorOff><!-- Content when indicator matches "off" value --></ButtonIndicatorOff>
    </ButtonEnabled>
</Button>
```

---

## When To Use Buttons

Use `Button` when the display needs a control with built-in state behavior:

- momentary controls that are on only while pressed
- standard action buttons that write an on value when clicked
- toggles that alternate between on/off values
- visual state branches for enabled, disabled, pressed, released, on, and off

Use lower-level [mouse events](mouse-events.md) instead when you only need a
drawable region to react to hover/press/release, or when the interaction is a
custom drag gesture such as a slider.

---

## Positioning & Transformation Attributes

These attributes control where and how the button is rendered.

### Position

| Attribute | Alias | Type | Description |
|-----------|-------|------|-------------|
| `PositionX` | `X` | number/var | X position relative to parent |
| `PositionY` | `Y` | number/var | Y position relative to parent |

### Dimensions

| Attribute | Alias | Type | Description |
|-----------|-------|------|-------------|
| `DimensionX` | `Width` | number/var | Button width (default: parent width) |
| `DimensionY` | `Height` | number/var | Button height (default: parent height) |
| `VirtualDimensionX` | `VirtualWidth` | number/var | Virtual coordinate width for children |
| `VirtualDimensionY` | `VirtualHeight` | number/var | Virtual coordinate height for children |

### Alignment

| Attribute | Alias | Type | Description |
|-----------|-------|------|-------------|
| `LocalAlignX` | `HorizontalAlign` | align | Which point on the button is placed at its position (horizontal) |
| `LocalAlignY` | `VerticalAlign` | align | Which point on the button is placed at its position (vertical) |
| `ParentAlignX` | — | align | Which point on the parent to align to (horizontal) |
| `ParentAlignY` | — | align | Which point on the parent to align to (vertical) |

See [Positioning and Alignment](primitives.md#positioning-and-alignment) for alignment constants.

### Rotation & Pivot

| Attribute | Alias | Type | Description |
|-----------|-------|------|-------------|
| `Rotation` | `Rotate` | number/var | Rotation angle in degrees |
| `PivotPositionX` | `PivotX` | number/var | Absolute X coordinate of rotation pivot |
| `PivotPositionY` | `PivotY` | number/var | Absolute Y coordinate of rotation pivot |
| `PivotLocalAlignX` | — | align | Pivot point as alignment (horizontal) |
| `PivotLocalAlignY` | — | align | Pivot point as alignment (vertical) |

**Note:** Use either `PivotPositionX`/`PivotPositionY` (both required together) OR `PivotLocalAlignX`/`PivotLocalAlignY`, not both.

---

## Button Type

| Attribute | Type | Description |
|-----------|------|-------------|
| `Type` | integer | Button behavior type |

**Type Constants:**

| Constant | Value | Description |
|----------|-------|-------------|
| `#_button_momentary_` | 1 | Momentary button - "on" while pressed, "off" when released |
| `#_button_standard_` | 2 | Standard button - sets target to "on" value when clicked (default) |
| `#_button_toggle_` | 3 | Toggle button - alternates between "on" and "off" each click |

---

## Value Attributes

These attributes define what values represent the "on" and "off" states.

| Attribute | Description | Default |
|-----------|-------------|---------|
| `On` | Default "on" value (used by TargetOn and IndicatorOn if not specified) | `1` |
| `Off` | Default "off" value (used by TargetOff if not specified) | `0` |
| `TargetOn` | Value written to target variable when button activates | Inherits from `On` |
| `TargetOff` | Value written to target variable when button deactivates | Inherits from `Off` |
| `IndicatorOn` | Value that causes indicator to show "on" state | Inherits from `TargetOn` |
| `EnableOn` | Value that means button is enabled/interactive | `1` |

### Value Inheritance Diagram

```
On ─────────┬──────────► TargetOn ──────► IndicatorOn
            │
Off ────────┴──────────► TargetOff

EnableOn (independent, defaults to 1)
```

---

## Variable Attributes

These attributes define which variables the button reads from and writes to.

| Attribute | Description | Default |
|-----------|-------------|---------|
| `Variable` | Default variable (used by all others if not specified) | Creates anonymous variable |
| `TargetVariable` | Variable modified when button is clicked | Inherits from `Variable` |
| `IndicatorVariable` | Variable checked for visual indicator state | Inherits from `TargetVariable` |
| `EnableVariable` | Variable checked to determine if button is enabled | Creates anonymous variable (always enabled) |

### Variable Inheritance Diagram

```
Variable ──────┬──────────► TargetVariable ──────► IndicatorVariable
               │
               └──────────► (EnableVariable is independent)
```

---

## Child Elements (Visual States)

Buttons use child elements to define their appearance in different states.

### State Hierarchy

```
<Button>
    <ButtonEnabled>           <!-- Shown when button is interactive -->
        <ButtonIndicatorOn>   <!-- Shown when indicator matches "on" value -->
        <ButtonIndicatorOff>  <!-- Shown when indicator matches "off" value -->
        <ButtonTransition>    <!-- Shown during state change (optional) -->
    </ButtonEnabled>
    <ButtonDisabled>          <!-- Shown when button is not interactive -->
    <ButtonPressed>           <!-- Actions/content when mouse down -->
    <ButtonReleased>          <!-- Actions/content when mouse up -->
</Button>
```

### Child Element Reference

| Element | Description |
|---------|-------------|
| `<ButtonEnabled>` | Container for enabled state visuals |
| `<ButtonDisabled>` | Content shown when button is disabled |
| `<ButtonIndicatorOn>` | Content shown when indicator variable equals IndicatorOn value |
| `<ButtonIndicatorOff>` | Content shown when indicator variable does not equal IndicatorOn value |
| `<ButtonTransition>` | Content shown during state transition |
| `<ButtonPressed>` | Content/actions executed on mouse button down |
| `<ButtonReleased>` | Content/actions executed on mouse button up |

---

## Examples

### Simple Toggle Button

A basic on/off toggle that controls a single variable:

```xml
<Variable Type="#_variable_string_" InitialValue="OFF">power</Variable>

<Button X="100" Y="100" Width="100" Height="50"
        Variable="power" On="ON" Off="OFF" Type="#_button_toggle_">
    <ButtonEnabled>
        <ButtonIndicatorOn>
            <Rectangle FillColor="0,0.7,0,1" Width="100" Height="50"/>
            <Text X="50" Y="25" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
                  FillColor="1,1,1,1" Size="18">POWER ON</Text>
        </ButtonIndicatorOn>
        <ButtonIndicatorOff>
            <Rectangle FillColor="0.5,0,0,1" Width="100" Height="50"/>
            <Text X="50" Y="25" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
                  FillColor="1,1,1,1" Size="18">POWER OFF</Text>
        </ButtonIndicatorOff>
    </ButtonEnabled>
</Button>
```

### Momentary Button

A button that's only "on" while being held:

```xml
<Variable Type="#_variable_integer_" InitialValue="0">firing</Variable>

<Button X="200" Y="300" Width="80" Height="80"
        Variable="firing" On="1" Off="0" Type="#_button_momentary_">
    <ButtonEnabled>
        <ButtonIndicatorOn>
            <Ellipse X="40" Y="40" Radius="38" FillColor="1,0,0,1"/>
            <Text X="40" Y="40" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
                  FillColor="1,1,1,1" Size="14">FIRING</Text>
        </ButtonIndicatorOn>
        <ButtonIndicatorOff>
            <Ellipse X="40" Y="40" Radius="38" FillColor="0.3,0,0,1"/>
            <Text X="40" Y="40" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
                  FillColor="0.7,0.7,0.7,1" Size="14">FIRE</Text>
        </ButtonIndicatorOff>
    </ButtonEnabled>
</Button>
```

### Button with Separate Indicator

Control one variable while displaying another's state:

```xml
<Variable Type="#_variable_string_" InitialValue="OFF">engine_command</Variable>
<Variable Type="#_variable_string_" InitialValue="OFF">engine_status</Variable>

<Button X="100" Y="100" Width="120" Height="60"
        TargetVariable="engine_command"
        IndicatorVariable="engine_status"
        On="ON" Off="OFF"
        Type="#_button_standard_">
    <ButtonEnabled>
        <ButtonIndicatorOn>
            <Rectangle FillColor="0,0.6,0,1" Width="120" Height="60"/>
            <Text X="60" Y="30" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
                  FillColor="1,1,1,1">ENGINE RUNNING</Text>
        </ButtonIndicatorOn>
        <ButtonIndicatorOff>
            <Rectangle FillColor="0.4,0.4,0.4,1" Width="120" Height="60"/>
            <Text X="60" Y="30" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
                  FillColor="1,1,1,1">START ENGINE</Text>
        </ButtonIndicatorOff>
    </ButtonEnabled>
</Button>
```

### Button with Disabled State

A button that can be disabled based on a condition:

```xml
<Variable Type="#_variable_integer_" InitialValue="1">systemReady</Variable>
<Variable Type="#_variable_string_" InitialValue="SAFE">armState</Variable>

<Button X="100" Y="100" Width="100" Height="50"
        TargetVariable="armState"
        EnableVariable="systemReady"
        EnableOn="1"
        On="ARMED" Off="SAFE"
        Type="#_button_toggle_">
    <ButtonEnabled>
        <ButtonIndicatorOn>
            <Rectangle FillColor="1,0,0,1" Width="100" Height="50"/>
            <Text X="50" Y="25" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
                  FillColor="1,1,1,1" Size="16">ARMED</Text>
        </ButtonIndicatorOn>
        <ButtonIndicatorOff>
            <Rectangle FillColor="0,0.5,0,1" Width="100" Height="50"/>
            <Text X="50" Y="25" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
                  FillColor="1,1,1,1" Size="16">SAFE</Text>
        </ButtonIndicatorOff>
    </ButtonEnabled>
    <ButtonDisabled>
        <Rectangle FillColor="0.2,0.2,0.2,1" Width="100" Height="50"/>
        <Text X="50" Y="25" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
              FillColor="0.5,0.5,0.5,1" Size="16">OFFLINE</Text>
    </ButtonDisabled>
</Button>
```

### Button with Press/Release Actions

A button that executes actions on press and release:

```xml
<Variable Type="#_variable_double_" InitialValue="0">throttle</Variable>

<Button X="100" Y="100" Width="60" Height="40" Type="#_button_momentary_">
    <ButtonPressed>
        <Set Variable="throttle" Operator="#_set_add_">10</Set>
    </ButtonPressed>
    <ButtonEnabled>
        <ButtonIndicatorOff>
            <Rectangle FillColor="0.3,0.3,0.6,1" Width="60" Height="40"/>
            <Text X="30" Y="20" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
                  FillColor="1,1,1,1">+10</Text>
        </ButtonIndicatorOff>
        <ButtonIndicatorOn>
            <Rectangle FillColor="0.5,0.5,0.9,1" Width="60" Height="40"/>
            <Text X="30" Y="20" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
                  FillColor="1,1,1,1">+10</Text>
        </ButtonIndicatorOn>
    </ButtonEnabled>
</Button>
```

### Rotated Button

A button with rotation around its center:

```xml
<Button X="200" Y="200" Width="100" Height="40"
        Rotation="45"
        PivotLocalAlignX="#_align_center_" PivotLocalAlignY="#_align_middle_"
        Variable="diagonal" On="1" Off="0" Type="#_button_toggle_">
    <ButtonEnabled>
        <ButtonIndicatorOff>
            <Rectangle FillColor="0.4,0.4,0.4,1" Width="100" Height="40"/>
            <Text X="50" Y="20" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
                  FillColor="1,1,1,1">DIAGONAL</Text>
        </ButtonIndicatorOff>
        <ButtonIndicatorOn>
            <Rectangle FillColor="0.2,0.6,0.8,1" Width="100" Height="40"/>
            <Text X="50" Y="20" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
                  FillColor="1,1,1,1">ACTIVE</Text>
        </ButtonIndicatorOn>
    </ButtonEnabled>
</Button>
```

---

## Common Patterns

### Radio Button Group

Use separate buttons with a shared variable:

```xml
<Variable Type="#_variable_string_" InitialValue="A">selection</Variable>

<Button X="10" Y="100" Width="60" Height="30" Variable="selection" On="A" Type="#_button_standard_">
    <ButtonEnabled>
        <ButtonIndicatorOn><Rectangle FillColor="0,0.6,0,1" Width="60" Height="30"/></ButtonIndicatorOn>
        <ButtonIndicatorOff><Rectangle FillColor="0.3,0.3,0.3,1" Width="60" Height="30"/></ButtonIndicatorOff>
    </ButtonEnabled>
</Button>

<Button X="80" Y="100" Width="60" Height="30" Variable="selection" On="B" Type="#_button_standard_">
    <ButtonEnabled>
        <ButtonIndicatorOn><Rectangle FillColor="0,0.6,0,1" Width="60" Height="30"/></ButtonIndicatorOn>
        <ButtonIndicatorOff><Rectangle FillColor="0.3,0.3,0.3,1" Width="60" Height="30"/></ButtonIndicatorOff>
    </ButtonEnabled>
</Button>

<Button X="150" Y="100" Width="60" Height="30" Variable="selection" On="C" Type="#_button_standard_">
    <ButtonEnabled>
        <ButtonIndicatorOn><Rectangle FillColor="0,0.6,0,1" Width="60" Height="30"/></ButtonIndicatorOn>
        <ButtonIndicatorOff><Rectangle FillColor="0.3,0.3,0.3,1" Width="60" Height="30"/></ButtonIndicatorOff>
    </ButtonEnabled>
</Button>
```

### Increment/Decrement Pair

```xml
<Variable Type="#_variable_double_" InitialValue="50">value</Variable>

<Button X="100" Y="100" Width="40" Height="40">
    <ButtonPressed><Set Variable="value" Operator="#_set_subtract_">1</Set></ButtonPressed>
    <ButtonEnabled>
        <ButtonIndicatorOff><Rectangle FillColor="0.4,0.2,0.2,1" Width="40" Height="40"/>
             <Text X="20" Y="20" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
                   FillColor="1,1,1,1" Size="24">−</Text></ButtonIndicatorOff>
        <ButtonIndicatorOn><Rectangle FillColor="0.6,0.3,0.3,1" Width="40" Height="40"/>
            <Text X="20" Y="20" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
                  FillColor="1,1,1,1" Size="24">−</Text></ButtonIndicatorOn>
    </ButtonEnabled>
</Button>

<Text X="160" Y="120" LocalAlignX="#_align_center_" Size="20">@value(%.0f)</Text>

<Button X="200" Y="100" Width="40" Height="40">
    <ButtonPressed><Set Variable="value" Operator="#_set_add_">1</Set></ButtonPressed>
    <ButtonEnabled>
        <ButtonIndicatorOff><Rectangle FillColor="0.2,0.4,0.2,1" Width="40" Height="40"/>
             <Text X="20" Y="20" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
                   FillColor="1,1,1,1" Size="24">+</Text></ButtonIndicatorOff>
        <ButtonIndicatorOn><Rectangle FillColor="0.3,0.6,0.3,1" Width="40" Height="40"/>
            <Text X="20" Y="20" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
                  FillColor="1,1,1,1" Size="24">+</Text></ButtonIndicatorOn>
    </ButtonEnabled>
</Button>
```

---

## Tips & Best Practices

1. **Always define both `<ButtonIndicatorOn>` and `<ButtonIndicatorOff>` states** - Even if they look similar, having both ensures predictable behavior.

2. **Use `Type="#_button_toggle_"` for on/off switches** - It automatically handles state toggling.

3. **Use `Type="#_button_momentary_"` for continuous actions** - Like thrust controls or horn buttons.

4. **Separate TargetVariable and IndicatorVariable** when the command and status are different - Common in hardware interfaces where commanded state differs from actual state.

5. **Use meaningful On/Off values** - Strings like `"ARMED"`/`"SAFE"` are more readable in XML than `1`/`0`.

6. **Center text with alignment constants** - `LocalAlignX="#_align_center_"` and `LocalAlignY="#_align_middle_"` for centered button labels.

7. **Include a `<ButtonDisabled>` state** - Provides visual feedback when the button can't be used.

---

## See Also

- [Mouse Events](mouse-events.md) — MousePressed, MouseReleased, MouseMotion, and slider patterns
- [Variables](variables.md) — Variable types and Set operators used in button actions
- [Constants](constants.md) — All button type and alignment constants
