# Buttons

`Button` provides momentary, action, and toggle behavior. It can use separate
variables for the commanded state and the indicated state, which is useful
when an external system acknowledges commands asynchronously. For a clickable
drawing region without a button state machine, use [mouse events](mouse-events.md).

## Attributes

### Position and transform

| Attribute | Alias | Type | Description |
|-----------|-------|------|-------------|
| `PositionX` | `X` | number/var | X position relative to the parent |
| `PositionY` | `Y` | number/var | Y position relative to the parent |
| `DimensionX` | `Width` | number/var | Width. Defaults to the parent width. |
| `DimensionY` | `Height` | number/var | Height. Defaults to the parent height. |
| `VirtualDimensionX` | `VirtualWidth` | number/var | Virtual width used by children |
| `VirtualDimensionY` | `VirtualHeight` | number/var | Virtual height used by children |
| `LocalAlignX` | `HorizontalAlign` | align | Horizontal anchor on the button |
| `LocalAlignY` | `VerticalAlign` | align | Vertical anchor on the button |
| `ParentAlignX` | — | align | Horizontal anchor on the parent |
| `ParentAlignY` | — | align | Vertical anchor on the parent |
| `Rotation` | `Rotate` | number/var | Rotation in degrees |
| `PivotPositionX` | `PivotX` | number/var | Absolute X coordinate of the pivot |
| `PivotPositionY` | `PivotY` | number/var | Absolute Y coordinate of the pivot |
| `PivotParentAlignX` | — | align | Horizontal pivot alignment in the parent |
| `PivotParentAlignY` | — | align | Vertical pivot alignment in the parent |
| `PivotLocalAlignX` | — | align | Horizontal pivot alignment |
| `PivotLocalAlignY` | — | align | Vertical pivot alignment |
| `NegateX` | — | boolean/var | Negate the X offset |
| `NegateY` | — | boolean/var | Negate the Y offset |

`PivotPositionX` and `PivotPositionY` must be specified together. Pivot modes
take precedence in this order: position, parent alignment, local alignment.
[Positioning and alignment](primitives.md#positioning-and-alignment) describes
the coordinate rules.

### Behavior

| Attribute | Type | Description |
|-----------|------|-------------|
| `Type` | integer | Button behavior. Defaults to `#_button_standard_`. |

| Type | Value | Behavior |
|------|-------|----------|
| `#_button_momentary_` | 1 | Writes the on value while pressed and the off value when released |
| `#_button_standard_` | 2 | Writes the on value when clicked |
| `#_button_toggle_` | 3 | Alternates between the on and off values |

### Values

| Attribute | Default | Description |
|-----------|---------|-------------|
| `On` | `1` | Base on value |
| `Off` | `0` | Base off value |
| `TargetOn` | `On` | Value written when the target activates |
| `TargetOff` | `Off` | Value written when the target deactivates |
| `IndicatorOn` | `TargetOn` | Value that selects the on indicator |
| `EnableOn` | `1` | Value that makes the button interactive |

The inheritance is:

```text
On  -> TargetOn  -> IndicatorOn
Off -> TargetOff

EnableOn is independent.
```

### Variables

| Attribute | Default | Description |
|-----------|---------|-------------|
| `Variable` | Anonymous variable | Base variable used by the button |
| `TargetVariable` | `Variable` | Variable written by the button |
| `IndicatorVariable` | `TargetVariable` | Variable read for the visual state |
| `EnableVariable` | Anonymous, enabled variable | Variable read to decide whether the button is interactive |

`EnableVariable` does not inherit from `Variable`.

## State children

| Element | Active when |
|---------|-------------|
| `ButtonEnabled` | The enable variable matches `EnableOn` |
| `ButtonDisabled` | The button is not enabled |
| `ButtonIndicatorOn` | The indicator variable matches `IndicatorOn` |
| `ButtonIndicatorOff` | The indicator variable does not match `IndicatorOn` |
| `ButtonTransition` | The target and indicator variables differ |
| `ButtonPressed` | Mouse button goes down |
| `ButtonReleased` | Mouse button goes up |

Indicator children may be placed under `ButtonEnabled`:

```xml
<Button>
    <ButtonEnabled>
        <ButtonIndicatorOn>...</ButtonIndicatorOn>
        <ButtonIndicatorOff>...</ButtonIndicatorOff>
        <ButtonTransition>...</ButtonTransition>
    </ButtonEnabled>
    <ButtonDisabled>...</ButtonDisabled>
    <ButtonPressed>...</ButtonPressed>
    <ButtonReleased>...</ButtonReleased>
</Button>
```

While the target and indicator variables differ, `ButtonTransition` replaces
both indicator branches. Standard and toggle buttons also stop accepting mouse
input until the indicator catches up; momentary buttons remain interactive.

## Toggle example

```xml
<Variable Type="#_variable_string_" InitialValue="OFF">power</Variable>

<Button X="100" Y="100" Width="100" Height="50"
        Variable="power" On="ON" Off="OFF" Type="#_button_toggle_">
    <ButtonIndicatorOn>
        <Rectangle Width="100" Height="50" FillColor="0 0.7 0 1"/>
        <Text X="50" Y="25" LocalAlignX="#_align_center_"
              LocalAlignY="#_align_middle_">POWER ON</Text>
    </ButtonIndicatorOn>
    <ButtonIndicatorOff>
        <Rectangle Width="100" Height="50" FillColor="0.5 0 0 1"/>
        <Text X="50" Y="25" LocalAlignX="#_align_center_"
              LocalAlignY="#_align_middle_">POWER OFF</Text>
    </ButtonIndicatorOff>
</Button>
```

For a momentary control, change `Type` to `#_button_momentary_`; `power` then
contains `ON` only while the control is held.

## Separate command and indication

This button writes `engine_command`, reads `engine_status` for its lamp, and is
disabled unless `system_ready` is `1`:

```xml
<Variable Type="#_variable_string_" InitialValue="OFF">engine_command</Variable>
<Variable Type="#_variable_string_" InitialValue="OFF">engine_status</Variable>
<Variable Type="#_variable_integer_" InitialValue="1">system_ready</Variable>

<Button X="100" Y="100" Width="120" Height="60"
        TargetVariable="engine_command"
        IndicatorVariable="engine_status"
        EnableVariable="system_ready" EnableOn="1"
        On="ON" Off="OFF" Type="#_button_standard_">
    <ButtonEnabled>
        <ButtonIndicatorOn>
            <Rectangle Width="120" Height="60" FillColor="0 0.6 0 1"/>
            <Text X="60" Y="30" LocalAlignX="#_align_center_"
                  LocalAlignY="#_align_middle_">RUNNING</Text>
        </ButtonIndicatorOn>
        <ButtonIndicatorOff>
            <Rectangle Width="120" Height="60" FillColor="0.3 0.3 0.3 1"/>
            <Text X="60" Y="30" LocalAlignX="#_align_center_"
                  LocalAlignY="#_align_middle_">START</Text>
        </ButtonIndicatorOff>
    </ButtonEnabled>
    <ButtonDisabled>
        <Rectangle Width="120" Height="60" FillColor="0.15 0.15 0.15 1"/>
        <Text X="60" Y="30" LocalAlignX="#_align_center_"
              LocalAlignY="#_align_middle_">OFFLINE</Text>
    </ButtonDisabled>
</Button>
```

`ButtonPressed` and `ButtonReleased` can contain actions such as `Set` or
`Function`. See [variables](variables.md#changing-a-variable-with-set) for the
`Set` operators.
