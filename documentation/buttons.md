# dcapp Button Reference

A comprehensive guide to creating interactive buttons in dcapp.

---

## Overview

The `<Button>` element creates interactive UI controls with multiple visual states. Buttons can toggle variables, respond to clicks, and display different content based on their current state.

```xml
<Button X="100" Y="100" Width="80" Height="40" Variable="myVar" On="1" Off="0">
    <Enabled>
        <On><!-- Content when indicator matches "on" value --></On>
        <Off><!-- Content when indicator matches "off" value --></Off>
    </Enabled>
</Button>
```

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

**Alignment Constants:**

| Horizontal | Vertical | Value |
|------------|----------|-------|
| `#_align_left_` | `#_align_bottom_` | 0 (default) |
| `#_align_center_` | `#_align_middle_` | 1 |
| `#_align_right_` | `#_align_top_` | 2 |

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

| Constant | Description |
|----------|-------------|
| `#_button_standard_` | Standard button - sets target to "on" value when clicked (default) |
| `#_button_momentary_` | Momentary button - "on" while pressed, "off" when released |
| `#_button_toggle_` | Toggle button - alternates between "on" and "off" each click |

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
| `EnabledOn` | Value that means button is enabled/interactive | `1` |

### Value Inheritance Diagram

```
On ─────────┬──────────► TargetOn ──────► IndicatorOn
            │
Off ────────┴──────────► TargetOff

EnabledOn (independent, defaults to 1)
```

---

## Variable Attributes

These attributes define which variables the button reads from and writes to.

| Attribute | Description | Default |
|-----------|-------------|---------|
| `Variable` | Default variable (used by all others if not specified) | Creates anonymous variable |
| `TargetVariable` | Variable modified when button is clicked | Inherits from `Variable` |
| `IndicatorVariable` | Variable checked for visual indicator state | Inherits from `TargetVariable` |
| `EnabledVariable` | Variable checked to determine if button is enabled | Creates anonymous variable (always enabled) |

### Variable Inheritance Diagram

```
Variable ──────┬──────────► TargetVariable ──────► IndicatorVariable
               │
               └──────────► (EnabledVariable is independent)
```

---

## Child Elements (Visual States)

Buttons use child elements to define their appearance in different states.

### State Hierarchy

```
<Button>
    <Enabled>           <!-- Shown when button is interactive -->
        <On>            <!-- Shown when indicator matches "on" value -->
        <Off>           <!-- Shown when indicator matches "off" value -->
        <Transition>    <!-- Shown during state change (optional) -->
    </Enabled>
    <Disabled>          <!-- Shown when button is not interactive -->
    <Pressed>           <!-- Actions/content when mouse down -->
    <Released>          <!-- Actions/content when mouse up -->
</Button>
```

### Child Element Reference

| Element | Also Known As | Description |
|---------|---------------|-------------|
| `<Enabled>` | — | Container for enabled state visuals |
| `<Disabled>` | — | Content shown when button is disabled |
| `<IndicatorOn>` | `<On>` | Content shown when indicator variable equals IndicatorOn value |
| `<IndicatorOff>` | `<Off>` | Content shown when indicator variable does not equal IndicatorOn value |
| `<Transition>` | — | Content shown during state transition |
| `<Pressed>` | — | Content/actions executed on mouse button down |
| `<Released>` | — | Content/actions executed on mouse button up |

---

## Examples

### Simple Toggle Button

A basic on/off toggle that controls a single variable:

```xml
<Variable Type="string" InitialValue="OFF">power</Variable>

<Button X="100" Y="100" Width="100" Height="50" 
        Variable="power" On="ON" Off="OFF" Type="#_button_toggle_">
    <Enabled>
        <On>
            <Rectangle FillColor="0,0.7,0,1" Width="100" Height="50"/>
            <Text X="50" Y="25" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
                  FillColor="1,1,1,1" Size="18">POWER ON</Text>
        </On>
        <Off>
            <Rectangle FillColor="0.5,0,0,1" Width="100" Height="50"/>
            <Text X="50" Y="25" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
                  FillColor="1,1,1,1" Size="18">POWER OFF</Text>
        </Off>
    </Enabled>
</Button>
```

### Momentary Button

A button that's only "on" while being held:

```xml
<Variable Type="integer" InitialValue="0">firing</Variable>

<Button X="200" Y="300" Width="80" Height="80"
        Variable="firing" On="1" Off="0" Type="#_button_momentary_">
    <Enabled>
        <On>
            <Circle X="40" Y="40" Radius="38" FillColor="1,0,0,1"/>
            <Text X="40" Y="40" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
                  FillColor="1,1,1,1" Size="14">FIRING</Text>
        </On>
        <Off>
            <Circle X="40" Y="40" Radius="38" FillColor="0.3,0,0,1"/>
            <Text X="40" Y="40" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
                  FillColor="0.7,0.7,0.7,1" Size="14">FIRE</Text>
        </Off>
    </Enabled>
</Button>
```

### Button with Separate Indicator

Control one variable while displaying another's state:

```xml
<Variable Type="string" InitialValue="OFF">engine_command</Variable>
<Variable Type="string" InitialValue="OFF">engine_status</Variable>

<Button X="100" Y="100" Width="120" Height="60"
        TargetVariable="engine_command"
        IndicatorVariable="engine_status"
        On="ON" Off="OFF"
        Type="#_button_standard_">
    <Enabled>
        <On>
            <Rectangle FillColor="0,0.6,0,1" Width="120" Height="60"/>
            <Text X="60" Y="30" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
                  FillColor="1,1,1,1">ENGINE RUNNING</Text>
        </On>
        <Off>
            <Rectangle FillColor="0.4,0.4,0.4,1" Width="120" Height="60"/>
            <Text X="60" Y="30" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
                  FillColor="1,1,1,1">START ENGINE</Text>
        </Off>
    </Enabled>
</Button>
```

### Button with Disabled State

A button that can be disabled based on a condition:

```xml
<Variable Type="integer" InitialValue="1">systemReady</Variable>
<Variable Type="string" InitialValue="SAFE">armState</Variable>

<Button X="100" Y="100" Width="100" Height="50"
        TargetVariable="armState"
        EnabledVariable="systemReady"
        EnabledOn="1"
        On="ARMED" Off="SAFE"
        Type="#_button_toggle_">
    <Enabled>
        <On>
            <Rectangle FillColor="1,0,0,1" Width="100" Height="50"/>
            <Text X="50" Y="25" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
                  FillColor="1,1,1,1" Size="16">ARMED</Text>
        </On>
        <Off>
            <Rectangle FillColor="0,0.5,0,1" Width="100" Height="50"/>
            <Text X="50" Y="25" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
                  FillColor="1,1,1,1" Size="16">SAFE</Text>
        </Off>
    </Enabled>
    <Disabled>
        <Rectangle FillColor="0.2,0.2,0.2,1" Width="100" Height="50"/>
        <Text X="50" Y="25" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
              FillColor="0.5,0.5,0.5,1" Size="16">OFFLINE</Text>
    </Disabled>
</Button>
```

### Button with Press/Release Actions

A button that executes actions on press and release:

```xml
<Variable Type="double" InitialValue="0">throttle</Variable>

<Button X="100" Y="100" Width="60" Height="40" Type="#_button_momentary_">
    <Pressed>
        <Set Variable="throttle" Operator="#_set_add_">10</Set>
    </Pressed>
    <Enabled>
        <Off>
            <Rectangle FillColor="0.3,0.3,0.6,1" Width="60" Height="40"/>
            <Text X="30" Y="20" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
                  FillColor="1,1,1,1">+10</Text>
        </Off>
        <On>
            <Rectangle FillColor="0.5,0.5,0.9,1" Width="60" Height="40"/>
            <Text X="30" Y="20" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
                  FillColor="1,1,1,1">+10</Text>
        </On>
    </Enabled>
</Button>
```

### Rotated Button

A button with rotation around its center:

```xml
<Button X="200" Y="200" Width="100" Height="40"
        Rotation="45"
        PivotLocalAlignX="#_align_center_" PivotLocalAlignY="#_align_middle_"
        Variable="diagonal" On="1" Off="0" Type="#_button_toggle_">
    <Enabled>
        <Off>
            <Rectangle FillColor="0.4,0.4,0.4,1" Width="100" Height="40"/>
            <Text X="50" Y="20" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
                  FillColor="1,1,1,1">DIAGONAL</Text>
        </Off>
        <On>
            <Rectangle FillColor="0.2,0.6,0.8,1" Width="100" Height="40"/>
            <Text X="50" Y="20" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
                  FillColor="1,1,1,1">ACTIVE</Text>
        </On>
    </Enabled>
</Button>
```

---

## Common Patterns

### Radio Button Group

Use separate buttons with a shared variable:

```xml
<Variable Type="string" InitialValue="A">selection</Variable>

<Button X="10" Y="100" Width="60" Height="30" Variable="selection" On="A" Type="#_button_standard_">
    <Enabled>
        <On><Rectangle FillColor="0,0.6,0,1" Width="60" Height="30"/></On>
        <Off><Rectangle FillColor="0.3,0.3,0.3,1" Width="60" Height="30"/></Off>
    </Enabled>
</Button>

<Button X="80" Y="100" Width="60" Height="30" Variable="selection" On="B" Type="#_button_standard_">
    <Enabled>
        <On><Rectangle FillColor="0,0.6,0,1" Width="60" Height="30"/></On>
        <Off><Rectangle FillColor="0.3,0.3,0.3,1" Width="60" Height="30"/></Off>
    </Enabled>
</Button>

<Button X="150" Y="100" Width="60" Height="30" Variable="selection" On="C" Type="#_button_standard_">
    <Enabled>
        <On><Rectangle FillColor="0,0.6,0,1" Width="60" Height="30"/></On>
        <Off><Rectangle FillColor="0.3,0.3,0.3,1" Width="60" Height="30"/></Off>
    </Enabled>
</Button>
```

### Increment/Decrement Pair

```xml
<Variable Type="double" InitialValue="50">value</Variable>

<Button X="100" Y="100" Width="40" Height="40">
    <Pressed><Set Variable="value" Operator="#_set_subtract_">1</Set></Pressed>
    <Enabled>
        <Off><Rectangle FillColor="0.4,0.2,0.2,1" Width="40" Height="40"/>
             <Text X="20" Y="20" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_" 
                   FillColor="1,1,1,1" Size="24">−</Text></Off>
        <On><Rectangle FillColor="0.6,0.3,0.3,1" Width="40" Height="40"/>
            <Text X="20" Y="20" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_" 
                  FillColor="1,1,1,1" Size="24">−</Text></On>
    </Enabled>
</Button>

<Text X="160" Y="120" LocalAlignX="#_align_center_" Size="20">@value(%.0f)</Text>

<Button X="200" Y="100" Width="40" Height="40">
    <Pressed><Set Variable="value" Operator="#_set_add_">1</Set></Pressed>
    <Enabled>
        <Off><Rectangle FillColor="0.2,0.4,0.2,1" Width="40" Height="40"/>
             <Text X="20" Y="20" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_" 
                   FillColor="1,1,1,1" Size="24">+</Text></Off>
        <On><Rectangle FillColor="0.3,0.6,0.3,1" Width="40" Height="40"/>
            <Text X="20" Y="20" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_" 
                  FillColor="1,1,1,1" Size="24">+</Text></On>
    </Enabled>
</Button>
```

---

## Tips & Best Practices

1. **Always define both `<On>` and `<Off>` states** - Even if they look similar, having both ensures predictable behavior.

2. **Use `Type="#_button_toggle_"` for on/off switches** - It automatically handles state toggling.

3. **Use `Type="#_button_momentary_"` for continuous actions** - Like thrust controls or horn buttons.

4. **Separate TargetVariable and IndicatorVariable** when the command and status are different - Common in hardware interfaces where commanded state differs from actual state.

5. **Use meaningful On/Off values** - Strings like `"ARMED"`/`"SAFE"` are more readable in XML than `1`/`0`.

6. **Center text with alignment constants** - `LocalAlignX="#_align_center_"` and `LocalAlignY="#_align_middle_"` for centered button labels.

7. **Include a `<Disabled>` state** - Provides visual feedback when the button can't be used.
