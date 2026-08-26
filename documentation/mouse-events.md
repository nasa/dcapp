# Mouse events

Mouse event elements are conditional containers. Their children render while
the corresponding mouse state is active. They can be children of `Rectangle`,
`Ellipse`, `Polygon`, `Image`, `PixelStream`, `Container`, or `Button`.

Use a [button](buttons.md) when you need its target/indicator state machine.
Mouse events are better suited to hover effects, one-shot actions, and custom
drag controls.

## Event elements

| Element | Active when |
|---------|-------------|
| `MouseHovered` | The pointer is over the parent, pressed or not |
| `MouseActive` | A mouse button is held down on the parent |
| `MouseInactive` | The parent is neither hovered nor pressed/active |
| `MousePressed` | The button goes down; active for one frame |
| `MouseReleased` | The button is released; active for one frame |

Several event children can share a parent and are evaluated independently:

```xml
<Variable Type="#_variable_integer_" InitialValue="0">clicks</Variable>

<Rectangle X="30" Y="10" Width="250" Height="70" FillColor="0.2 0.22 0.28 1">
    <Text X="125" Y="35" LocalAlignX="#_align_center_"
          LocalAlignY="#_align_middle_">@clicks clicks</Text>
    <MouseHovered>
        <Rectangle Width="250" Height="70" LineColor="0.6 0.8 1 1" LineWidth="2"/>
    </MouseHovered>
    <MouseActive>
        <Text X="125" Y="15" LocalAlignX="#_align_center_">held</Text>
    </MouseActive>
    <MousePressed>
        <Set Variable="clicks" Operator="#_set_add_">1</Set>
    </MousePressed>
</Rectangle>
```

## `MouseMotion`

`MouseMotion` writes the pointer position in its parent container's virtual
coordinate space.

| Attribute | Type | Description |
|-----------|------|-------------|
| `VariableX` | variable name | Receives the X coordinate |
| `VariableY` | variable name | Receives the Y coordinate |

Either attribute may be omitted. The receiving variables must be integers or
doubles; integer variables truncate the coordinate. A 400-pixel container with
`VirtualWidth="100"` maps horizontal motion to 0–100:

```xml
<Variable Type="#_variable_double_" InitialValue="50">cursor_x</Variable>

<Container X="100" Y="100" Width="400" Height="30" VirtualWidth="100">
    <MouseMotion VariableX="cursor_x"/>
</Container>
```

## Slider pattern

A draggable slider gates `MouseMotion` with a momentary button variable. The
container's virtual size establishes the value range.

```xml
<Variable Type="#_variable_double_" InitialValue="50">slider_x</Variable>
<Variable Type="#_variable_integer_" InitialValue="0">slider_selected</Variable>

<Container X="300" Y="500" Width="400" Height="30" VirtualWidth="100">
    <Rectangle Width="100" Height="30" FillColor="0.2 0.2 0.2 1"
               LineColor="0.5 0.5 0.5 1" LineWidth="1"/>

    <If Value="@slider_selected" Value2="1" Operator="#_if_eq_">
        <MouseMotion VariableX="slider_x"/>
    </If>

    <Set Variable="slider_x" Operator="#_set_max_">0</Set>
    <Set Variable="slider_x" Operator="#_set_min_">100</Set>

    <Button Type="#_button_momentary_" X="@slider_x" Y="15"
            Width="6" Height="40" Variable="slider_selected"
            LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_">
        <ButtonIndicatorOn>
            <Rectangle FillColor="0.75 1 0 1"/>
        </ButtonIndicatorOn>
        <ButtonIndicatorOff>
            <Rectangle FillColor="0.5 0.5 0.5 1"/>
        </ButtonIndicatorOff>
    </Button>
</Container>
```

`#_set_max_` applies `max(variable, value)`, so it sets the lower bound.
`#_set_min_` applies `min(variable, value)`, so it sets the upper bound. Keep
those `Set` elements before the button so its position is clamped before it is
drawn.

For a vertical slider, set `VirtualHeight` and use `VariableY`. A different
range only requires matching virtual dimensions and clamp values. The
`welcome` sample contains a horizontal slider, and `buttons` shows the other
pointer-event states.
