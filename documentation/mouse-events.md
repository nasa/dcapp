# dcapp Mouse Events Reference

Handling mouse interaction on drawable elements and containers.

---

## Overview

dcapp provides mouse event child elements that respond to cursor interaction. These events can be placed inside any of the following parent elements:

- `<Rectangle>`
- `<Ellipse>`
- `<Polygon>`
- `<Image>`
- `<PixelStream>`
- `<Container>`
- `<Button>`

Each mouse event element acts as a conditional container: its children are rendered only while the corresponding mouse state is active.

---

## Mouse Event Elements

### `<MouseHovered>`

Renders its children when the mouse cursor is positioned over the parent element, regardless of whether a mouse button is pressed.

```xml
<Rectangle X="100" Y="100" Width="160" Height="80" FillColor="0.2 0.25 0.35">
    <Text ParentAlignX="#_align_center_" ParentAlignY="#_align_middle_"
          LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
          Size="11" FillColor="0.6 0.6 0.6">Hover me</Text>
    <MouseHovered>
        <Text ParentAlignX="#_align_center_" ParentAlignY="#_align_middle_"
              LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
              Size="12" FillColor="0.3 1 0.3">HOVERED!</Text>
    </MouseHovered>
</Rectangle>
```

### `<MouseActive>`

Renders its children while the mouse button is held down on the parent element. The content is visible for the entire duration of the press.

```xml
<Rectangle X="30" Y="280" Width="160" Height="80" FillColor="0.25 0.2 0.3">
    <Text ParentAlignX="#_align_center_" ParentAlignY="#_align_middle_"
          LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
          Size="11" FillColor="0.6 0.6 0.6">Hold click</Text>
    <MouseActive>
        <Text ParentAlignX="#_align_center_" ParentAlignY="#_align_middle_"
              LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
              Size="12" FillColor="1 0.3 0.3">ACTIVE!</Text>
    </MouseActive>
</Rectangle>
```

### `<MouseInactive>`

Renders its children when the mouse button is **not** pressed on the parent element. This is the complement of `<MouseActive>`.

### `<MousePressed>`

Fires on the instant the mouse button goes down on the parent element. Children are rendered (and any `<Set>` actions executed) on that single frame. Useful for triggering one-shot actions like incrementing a variable.

```xml
<Ellipse X="290" Y="180" Radius="40" FillColor="0.2 0.25 0.2">
    <MousePressed>
        <Text ParentAlignX="#_align_center_" ParentAlignY="#_align_middle_"
              LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
              Size="11" FillColor="0.2 1 0.5">PRESSED!</Text>
    </MousePressed>
</Ellipse>
```

### `<MouseReleased>`

Fires on the instant the mouse button is released on the parent element. Like `<MousePressed>`, its children are rendered for a single frame.

```xml
<Rectangle X="380" Y="140" Width="160" Height="80" FillColor="0.2 0.2 0.25">
    <MouseReleased>
        <Text ParentAlignX="#_align_center_" ParentAlignY="#_align_middle_"
              LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
              Size="12" FillColor="0.6 0.2 1">RELEASED!</Text>
    </MouseReleased>
</Rectangle>
```

---

## Combining Multiple Events

A single element can have several mouse event children. They activate independently based on the current mouse state. This is useful for providing layered visual feedback:

```xml
<Rectangle X="30" Y="10" Width="250" Height="70" FillColor="0.2 0.22 0.28">
    <Text ParentAlignX="#_align_center_" ParentAlignY="#_align_middle_"
          LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
          Size="10" FillColor="0.5 0.5 0.5">Interact with me</Text>
    <MouseHovered>
        <Text ParentAlignX="#_align_center_" ParentAlignY="#_align_middle_"
              LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
              Size="11" FillColor="0.6 0.8 1">Hovered</Text>
    </MouseHovered>
    <MouseActive>
        <Text ParentAlignX="#_align_center_" ParentAlignY="#_align_middle_"
              LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
              Size="12" FillColor="1 0.5 0.2">Active!</Text>
    </MouseActive>
    <MousePressed>
        <Set Variable="clickCount" Operator="#_set_add_">1</Set>
    </MousePressed>
</Rectangle>
```

---

## MouseMotion Element

`<MouseMotion>` tracks the cursor position while the mouse is moving and writes the coordinates into variables. It operates in the virtual coordinate space of its parent container.

### Attributes

| Attribute | Type | Description |
|-----------|------|-------------|
| `VariableX` | variable name | Variable to receive the X coordinate of the cursor |
| `VariableY` | variable name | Variable to receive the Y coordinate of the cursor |

You can specify `VariableX`, `VariableY`, or both, depending on whether you need horizontal tracking, vertical tracking, or full 2D tracking.

### Basic Usage

```xml
<Variable Type="#_variable_double_" InitialValue="50">CursorX</Variable>

<Container X="100" Y="100" Width="400" Height="30" VirtualWidth="100">
    <MouseMotion VariableX="CursorX"/>
</Container>
```

The cursor position is mapped to the container's virtual coordinate space. In the example above, the container is 400 pixels wide with a `VirtualWidth` of 100, so `CursorX` ranges from 0 to 100 as the mouse moves across the container.

---

## Building Sliders with MouseMotion

The standard pattern for a draggable slider combines four pieces:

1. A **Container** with a virtual coordinate space defining the slider range.
2. A **momentary Button** that the user drags (its variable tracks whether it is held).
3. A **MouseMotion** element gated by an `<If>` so it only tracks while the button is held.
4. **Clamping** via `<Set>` with `#_set_max_` and `#_set_min_` to keep the value in bounds.

### Horizontal Slider (0-100)

```xml
<Variable Type="#_variable_double_" InitialValue="50">SliderX</Variable>
<Variable Type="#_variable_integer_">SliderXSelected</Variable>

<Container X="300" Y="500" Width="400" Height="30" VirtualWidth="100">
    <Rectangle FillColor="#_color_dark_gray_" LineColor="#_color_gray_" LineWidth="1"/>

    <!-- Only track motion while the thumb is held down -->
    <If Value="@SliderXSelected" Operator="#_if_eq_" Value2="1">
        <MouseMotion VariableX="SliderX"/>
    </If>

    <!-- Clamp value to 0-100 range -->
    <Set Variable="SliderX" Operator="#_set_max_">0</Set>
    <Set Variable="SliderX" Operator="#_set_min_">100</Set>

    <!-- Draggable thumb -->
    <Button Type="#_button_momentary_" X="@SliderX" Y="15" Width="6" Height="40"
            Variable="SliderXSelected"
            LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_">
        <ButtonIndicatorOn>
            <Rectangle FillColor="#_color_lime_"/>
        </ButtonIndicatorOn>
        <ButtonIndicatorOff>
            <Rectangle FillColor="#_color_gray_"/>
        </ButtonIndicatorOff>
    </Button>
</Container>
```

**How it works:**

- The `Container` maps its 400-pixel width to a virtual width of 100, so the slider range is 0 to 100.
- The `Button` is momentary: its variable `SliderXSelected` is 1 while held, 0 otherwise.
- The `<If>` gate ensures `<MouseMotion>` only writes to `SliderX` while the thumb is being dragged.
- `#_set_max_` ensures the value is at least 0; `#_set_min_` ensures it is at most 100.
- The button positions itself at `X="@SliderX"`, so it follows the variable value.

### Vertical Slider (0-100)

For a vertical slider, use `VariableY` instead and set `VirtualHeight` on the container:

```xml
<Variable Type="#_variable_double_" InitialValue="50">SliderY</Variable>
<Variable Type="#_variable_integer_">SliderYSelected</Variable>

<Container X="740" Y="100" Width="30" Height="280" VirtualHeight="100">
    <Rectangle FillColor="#_color_dark_gray_" LineColor="#_color_gray_" LineWidth="1"/>

    <If Value="@SliderYSelected" Operator="#_if_eq_" Value2="1">
        <MouseMotion VariableY="SliderY"/>
    </If>

    <Set Variable="SliderY" Operator="#_set_max_">0</Set>
    <Set Variable="SliderY" Operator="#_set_min_">100</Set>

    <Button Type="#_button_momentary_" X="15" Y="@SliderY" Width="40" Height="6"
            Variable="SliderYSelected"
            LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_">
        <ButtonIndicatorOn>
            <Rectangle FillColor="#_color_lime_"/>
        </ButtonIndicatorOn>
        <ButtonIndicatorOff>
            <Rectangle FillColor="#_color_gray_"/>
        </ButtonIndicatorOff>
    </Button>
</Container>
```

### Custom Range Slider (0-255)

Change the clamping bounds and `VirtualWidth` to match any desired range:

```xml
<Variable Type="#_variable_double_" InitialValue="128">SliderRed</Variable>
<Variable Type="#_variable_integer_">SliderRedSelected</Variable>

<Container X="220" Y="380" Width="300" Height="20" VirtualWidth="255">
    <Rectangle FillColor="0.3 0 0" LineColor="#_color_red_" LineWidth="1"/>

    <If Value="@SliderRedSelected" Operator="#_if_eq_" Value2="1">
        <MouseMotion VariableX="SliderRed"/>
    </If>

    <Set Variable="SliderRed" Operator="#_set_max_">0</Set>
    <Set Variable="SliderRed" Operator="#_set_min_">255</Set>

    <Button Type="#_button_momentary_" X="@SliderRed" Y="10" Width="12" Height="26"
            Variable="SliderRedSelected"
            LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_">
        <ButtonIndicatorOn>
            <Rectangle FillColor="#_color_red_"/>
        </ButtonIndicatorOn>
        <ButtonIndicatorOff>
            <Rectangle FillColor="0.5 0 0"/>
        </ButtonIndicatorOff>
    </Button>
</Container>
```

---

## Clamping Notes

The `#_set_max_` and `#_set_min_` operators have intentionally inverted names:

- `#_set_max_` ensures the variable is **at least** the given value (clamps to a minimum).
- `#_set_min_` ensures the variable is **at most** the given value (clamps to a maximum).

Always place the clamping `<Set>` elements before the `<Button>` draw so the thumb position stays within bounds on every frame.

---

## See Also

- [buttons.md](buttons.md) -- Button element reference, including momentary buttons used for slider thumbs.
- [samples.md](samples.md) -- Full list of sample projects (`samples/events/`, `samples/slider/`).
