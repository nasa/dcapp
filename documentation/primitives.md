# dcapp XML Reference

A comprehensive reference for all XML elements and attributes in the dcapp display framework.

---

## Document Structure

```xml
<dcapp>
    <Window ...>
        <!-- Display elements go here -->
    </Window>
</dcapp>
```

---

## Root Elements

### `<dcapp>`

The root element that wraps the entire display definition. Contains `<Window>`, `<Variable>`, `<Constant>`, `<Style>`, `<TrickIO>`, and `<Logic>` elements.

---

### `<Window>`

Defines the application window.

| Attribute | Aliases | Type | Required | Description |
|-----------|---------|------|----------|-------------|
| `Title` | — | string | No | Window title (default: "dcapp") |
| `PositionX` | `X` | number | No | Initial X position (default: 0) |
| `PositionY` | `Y` | number | No | Initial Y position (default: 0) |
| `DimensionX` | `Width` | number | **Yes** | Window width in pixels |
| `DimensionY` | `Height` | number | **Yes** | Window height in pixels |
| `VirtualDimensionX` | `VirtualWidth` | number/var | No | Virtual coordinate width (default: actual width) |
| `VirtualDimensionY` | `VirtualHeight` | number/var | No | Virtual coordinate height (default: actual height) |

**Example:**
```xml
<Window Title="Flight Display" Width="1920" Height="1080" VirtualWidth="1920" VirtualHeight="1080">
    ...
</Window>
```

---

## Data Elements

### `<Variable>`

Declares a runtime variable that can be referenced and modified.

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `Type` | string | No | Data type: `string`, `integer`, `double` (default: `string`) |
| `InitialValue` | string | No | Initial value (default: empty string) |

**Content:** Variable name

**Example:**
```xml
<Variable Type="#_variable_double_" InitialValue="0.0">altitude</Variable>
<Variable Type="#_variable_string_" InitialValue="OFF">systemStatus</Variable>
```

---

### `<Constant>`

Declares a constant value (processed at parse time, currently ignored at runtime).

---

### `<Style>`

Defines reusable styles (currently ignored at runtime).

---

## Layout Elements

### `<Container>`

A grouping element that establishes a coordinate space for child elements.

| Attribute | Aliases | Type | Required | Description |
|-----------|---------|------|----------|-------------|
| `PositionX` | `X` | number/var | No | X position relative to parent |
| `PositionY` | `Y` | number/var | No | Y position relative to parent |
| `DimensionX` | `Width` | number/var | No | Container width (default: parent width) |
| `DimensionY` | `Height` | number/var | No | Container height (default: parent height) |
| `VirtualDimensionX` | `VirtualWidth` | number/var | No | Virtual coordinate width |
| `VirtualDimensionY` | `VirtualHeight` | number/var | No | Virtual coordinate height |
| `LocalAlignX` | `HorizontalAlign` | align | No | Horizontal alignment of this element |
| `LocalAlignY` | `VerticalAlign` | align | No | Vertical alignment of this element |
| `ParentAlignX` | — | align | No | Anchor point on parent (horizontal) |
| `ParentAlignY` | — | align | No | Anchor point on parent (vertical) |
| `Rotation` | `Rotate` | number/var | No | Rotation in degrees |
| `PivotPositionX` | `PivotX` | number/var | No | Pivot point X (absolute) |
| `PivotPositionY` | `PivotY` | number/var | No | Pivot point Y (absolute) |
| `PivotLocalAlignX` | — | align | No | Pivot alignment (horizontal) |
| `PivotLocalAlignY` | — | align | No | Pivot alignment (vertical) |

**Note:** Pivot position and pivot alignment are mutually exclusive. Use one pair or the other.

---

### `<Panel>`

A simpler container that only sets virtual dimensions.

| Attribute | Aliases | Type | Required | Description |
|-----------|---------|------|----------|-------------|
| `VirtualDimensionX` | `VirtualWidth` | number/var | No | Virtual coordinate width |
| `VirtualDimensionY` | `VirtualHeight` | number/var | No | Virtual coordinate height |

---

### `<Include>`

Includes content from another XML file.

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `File` | string | **Yes** | Path to XML file to include |

---

## Drawing Primitives

### `<Rectangle>`

Draws a rectangle.

| Attribute | Aliases | Type | Required | Description |
|-----------|---------|------|----------|-------------|
| `PositionX` | `X` | number/var | No | X position |
| `PositionY` | `Y` | number/var | No | Y position |
| `DimensionX` | `Width` | number/var | No | Rectangle width |
| `DimensionY` | `Height` | number/var | No | Rectangle height |
| `LocalAlignX` | `HorizontalAlign` | align | No | Horizontal alignment |
| `LocalAlignY` | `VerticalAlign` | align | No | Vertical alignment |
| `ParentAlignX` | — | align | No | Parent anchor (horizontal) |
| `ParentAlignY` | — | align | No | Parent anchor (vertical) |
| `Rotation` | `Rotate` | number/var | No | Rotation in degrees |
| `PivotPositionX` | `PivotX` | number/var | No | Pivot point X |
| `PivotPositionY` | `PivotY` | number/var | No | Pivot point Y |
| `PivotLocalAlignX` | — | align | No | Pivot alignment (horizontal) |
| `PivotLocalAlignY` | — | align | No | Pivot alignment (vertical) |
| `FillColor` | — | color | No | Fill color (RGBA) |
| `LineColor` | — | color | No | Border color (RGBA) |
| `LineWidth` | — | number/var | No | Border width |

**Children:** `<Pressed>`, `<Released>`, `<Active>`, `<Inactive>`, `<Hovered>` (mouse events)

---

### `<Circle>`

Draws a circle.

| Attribute | Aliases | Type | Required | Description |
|-----------|---------|------|----------|-------------|
| `PositionX` | `X` | number/var | No | Center X position |
| `PositionY` | `Y` | number/var | No | Center Y position |
| `Radius` | — | number/var | No | Circle radius |
| `Segments` | — | number/var | No | Number of segments for rendering |
| `ParentAlignX` | — | align | No | Parent anchor (horizontal) |
| `ParentAlignY` | — | align | No | Parent anchor (vertical) |
| `Rotation` | `Rotate` | number/var | No | Rotation in degrees |
| `PivotPositionX` | `PivotX` | number/var | No | Pivot point X |
| `PivotPositionY` | `PivotY` | number/var | No | Pivot point Y |
| `PivotLocalAlignX` | — | align | No | Pivot alignment (horizontal) |
| `PivotLocalAlignY` | — | align | No | Pivot alignment (vertical) |
| `FillColor` | — | color | No | Fill color (RGBA) |
| `LineColor` | — | color | No | Border color (RGBA) |
| `LineWidth` | — | number/var | No | Border width |

**Children:** `<Pressed>`, `<Released>`, `<Active>`, `<Inactive>`, `<Hovered>` (mouse events)

---

### `<Ellipse>`

Draws an ellipse.

| Attribute | Aliases | Type | Required | Description |
|-----------|---------|------|----------|-------------|
| `PositionX` | `X` | number/var | No | Center X position |
| `PositionY` | `Y` | number/var | No | Center Y position |
| `RadiusX` | — | number/var | No | Ellipse radius X |
| `RadiusY` | — | number/var | No | Ellipse radius Y |
| `Segments` | — | number/var | No | Number of segments for rendering |
| `ParentAlignX` | — | align | No | Parent anchor (horizontal) |
| `ParentAlignY` | — | align | No | Parent anchor (vertical) |
| `Rotation` | `Rotate` | number/var | No | Rotation in degrees |
| `PivotPositionX` | `PivotX` | number/var | No | Pivot point X |
| `PivotPositionY` | `PivotY` | number/var | No | Pivot point Y |
| `PivotLocalAlignX` | — | align | No | Pivot alignment (horizontal) |
| `PivotLocalAlignY` | — | align | No | Pivot alignment (vertical) |
| `FillColor` | — | color | No | Fill color (RGBA) |
| `LineColor` | — | color | No | Border color (RGBA) |
| `LineWidth` | — | number/var | No | Border width |

**Children:** `<Pressed>`, `<Released>`, `<Active>`, `<Inactive>`, `<Hovered>` (mouse events)

---

### `<Line>`

Draws a polyline through a series of vertices.

| Attribute | Aliases | Type | Required | Description |
|-----------|---------|------|----------|-------------|
| `PositionX` | `X` | number/var | No | Base X position |
| `PositionY` | `Y` | number/var | No | Base Y position |
| `Rotation` | `Rotate` | number/var | No | Rotation in degrees |
| `PivotPositionX` | `PivotX` | number/var | No | Pivot point X |
| `PivotPositionY` | `PivotY` | number/var | No | Pivot point Y |

**Children:** `<Vertex>` elements defining the line points

---

### `<Polygon>`

Draws a filled or outlined polygon.

| Attribute | Aliases | Type | Required | Description |
|-----------|---------|------|----------|-------------|
| `PositionX` | `X` | number/var | No | Base X position |
| `PositionY` | `Y` | number/var | No | Base Y position |
| `Rotation` | `Rotate` | number/var | No | Rotation in degrees |
| `PivotPositionX` | `PivotX` | number/var | No | Pivot point X |
| `PivotPositionY` | `PivotY` | number/var | No | Pivot point Y |
| `FillColor` | — | color | No | Fill color (RGBA) |
| `LineColor` | — | color | No | Border color (RGBA) |
| `LineWidth` | — | number/var | No | Border width |

**Children:** `<Vertex>` elements, `<Pressed>`, `<Released>`, `<Active>`, `<Inactive>`, `<Hovered>`

---

### `<Vertex>`

Defines a point for `<Line>` or `<Polygon>` elements.

| Attribute | Aliases | Type | Required | Description |
|-----------|---------|------|----------|-------------|
| `PositionX` | `X` | number/var | **Yes** | X coordinate |
| `PositionY` | `Y` | number/var | **Yes** | Y coordinate |

**Example:**
```xml
<Polygon FillColor="1,0,0,1">
    <Vertex X="0" Y="0"/>
    <Vertex X="100" Y="0"/>
    <Vertex X="50" Y="100"/>
</Polygon>
```

---

## Media Elements

### `<Image>`

Displays an image file.

| Attribute | Aliases | Type | Required | Description |
|-----------|---------|------|----------|-------------|
| `File` | — | string | **Yes** | Path to image file |
| `PositionX` | `X` | number/var | No | X position |
| `PositionY` | `Y` | number/var | No | Y position |
| `DimensionX` | `Width` | number/var | No | Display width |
| `DimensionY` | `Height` | number/var | No | Display height |
| `LocalAlignX` | `HorizontalAlign` | align | No | Horizontal alignment |
| `LocalAlignY` | `VerticalAlign` | align | No | Vertical alignment |
| `ParentAlignX` | — | align | No | Parent anchor (horizontal) |
| `ParentAlignY` | — | align | No | Parent anchor (vertical) |
| `Rotation` | `Rotate` | number/var | No | Rotation in degrees |
| `PivotPositionX` | `PivotX` | number/var | No | Pivot point X |
| `PivotPositionY` | `PivotY` | number/var | No | Pivot point Y |
| `PivotLocalAlignX` | — | align | No | Pivot alignment (horizontal) |
| `PivotLocalAlignY` | — | align | No | Pivot alignment (vertical) |

**Children:** `<Pressed>`, `<Released>`, `<Active>`, `<Inactive>`, `<Hovered>` (mouse events)

---

### `<Pixelstream>`

Displays streaming video content (e.g., MJPEG).

| Attribute | Aliases | Type | Required | Description |
|-----------|---------|------|----------|-------------|
| `Type` | `Protocol` | integer | **Yes** | Stream type |
| `URL` | — | string | Required for MJPEG | Stream URL |
| `Timeout` | — | integer | No | Connection timeout in seconds (default: 5) |

**Stream Type Constants:**

| Constant | Value | Description |
|----------|-------|-------------|
| `#_pixelstream_dynamic_file_` | 0 | Dynamic file source |
| `#_pixelstream_mjpeg_` | 1 | MJPEG stream |
| `PositionX` | `X` | number/var | No | X position |
| `PositionY` | `Y` | number/var | No | Y position |
| `DimensionX` | `Width` | number/var | No | Display width |
| `DimensionY` | `Height` | number/var | No | Display height |
| `LocalAlignX` | `HorizontalAlign` | align | No | Horizontal alignment |
| `LocalAlignY` | `VerticalAlign` | align | No | Vertical alignment |
| `ParentAlignX` | — | align | No | Parent anchor (horizontal) |
| `ParentAlignY` | — | align | No | Parent anchor (vertical) |
| `Rotation` | `Rotate` | number/var | No | Rotation in degrees |
| `PivotPositionX` | `PivotX` | number/var | No | Pivot point X |
| `PivotPositionY` | `PivotY` | number/var | No | Pivot point Y |
| `PivotLocalAlignX` | — | align | No | Pivot alignment (horizontal) |
| `PivotLocalAlignY` | — | align | No | Pivot alignment (vertical) |

---

### `<Text>`

Displays text with variable interpolation.

| Attribute | Aliases | Type | Required | Description |
|-----------|---------|------|----------|-------------|
| `PositionX` | `X` | number/var | No | X position |
| `PositionY` | `Y` | number/var | No | Y position |
| `Size` | — | number/var | No | Font size |
| `LocalAlignX` | `HorizontalAlign` | align | No | Horizontal alignment |
| `LocalAlignY` | `VerticalAlign` | align | No | Vertical alignment |
| `ParentAlignX` | — | align | No | Parent anchor (horizontal) |
| `ParentAlignY` | — | align | No | Parent anchor (vertical) |
| `Rotation` | `Rotate` | number/var | No | Rotation in degrees |
| `PivotPositionX` | `PivotX` | number/var | No | Pivot point X |
| `PivotPositionY` | `PivotY` | number/var | No | Pivot point Y |
| `PivotLocalAlignX` | — | align | No | Pivot alignment (horizontal) |
| `PivotLocalAlignY` | — | align | No | Pivot alignment (vertical) |
| `FillColor` | — | color | No | Text fill color |
| `LineColor` | — | color | No | Text outline color |

**Content:** Text string with variable interpolation

**Variable Interpolation:**
- `@variableName` - Insert variable value
- `@{variableName}` - Insert variable with braces (for adjacent text)
- `@variableName(%format)` - Format specifier (e.g., `@altitude(%.1f)`)

**Escape Sequences:**
- `\n` - Newline
- `\t` - Tab
- `\\` - Backslash
- `\@` - Literal @ symbol
- `\"` - Quote
- `\'` - Single quote

**Example:**
```xml
<Text X="100" Y="50" Size="24" FillColor="1,1,1,1">
    Altitude: @altitude(%.0f) ft
</Text>
```

---

## Interactive Elements

### `<Button>`

Creates an interactive button with multiple visual states.

| Attribute | Aliases | Type | Required | Description |
|-----------|---------|------|----------|-------------|
| `PositionX` | `X` | number/var | No | X position |
| `PositionY` | `Y` | number/var | No | Y position |
| `DimensionX` | `Width` | number/var | No | Button width |
| `DimensionY` | `Height` | number/var | No | Button height |
| `VirtualDimensionX` | `VirtualWidth` | number/var | No | Virtual coordinate width |
| `VirtualDimensionY` | `VirtualHeight` | number/var | No | Virtual coordinate height |
| `LocalAlignX` | `HorizontalAlign` | align | No | Horizontal alignment |
| `LocalAlignY` | `VerticalAlign` | align | No | Vertical alignment |
| `ParentAlignX` | — | align | No | Parent anchor (horizontal) |
| `ParentAlignY` | — | align | No | Parent anchor (vertical) |
| `Rotation` | `Rotate` | number/var | No | Rotation in degrees |
| `PivotPositionX` | `PivotX` | number/var | No | Pivot point X |
| `PivotPositionY` | `PivotY` | number/var | No | Pivot point Y |
| `PivotLocalAlignX` | — | align | No | Pivot alignment (horizontal) |
| `PivotLocalAlignY` | — | align | No | Pivot alignment (vertical) |
| `Type` | — | integer | No | Button type (default: standard) |

**Button Type Constants:**

| Constant | Value | Description |
|----------|-------|-------------|
| `#_button_standard_` | 0 | Standard button (default) |
| `#_button_momentary_` | 1 | Momentary button (active only while pressed) |
| `#_button_toggle_` | 2 | Toggle button (alternates between on/off) |

**Value Attributes (control what values are used):**

| Attribute | Description |
|-----------|-------------|
| `On` | Default "on" value for target and indicator |
| `Off` | Default "off" value for target and indicator |
| `TargetOn` | Value to set target variable when pressed (inherits from `On`) |
| `TargetOff` | Value to set target variable when released (inherits from `Off`) |
| `IndicatorOn` | Value that means indicator shows "on" state (inherits from `TargetOn`) |
| `EnabledOn` | Value that means button is enabled (default: 1) |

**Variable Attributes (control which variables are used):**

| Attribute | Description |
|-----------|-------------|
| `Variable` | Default variable for target, indicator, and enabled |
| `TargetVariable` | Variable to modify when clicked (inherits from `Variable`) |
| `IndicatorVariable` | Variable to check for indicator state (inherits from `TargetVariable`) |
| `EnabledVariable` | Variable to check for enabled state |

**Children (visual states):**
- `<Enabled>` - Content shown when button is enabled (interactive)
- `<Disabled>` - Content shown when button is disabled
- `<IndicatorOn>` or `<On>` - Content shown when indicator matches "on" value
- `<IndicatorOff>` or `<Off>` - Content shown when indicator matches "off" value
- `<Transition>` - Content shown during state transition
- `<Pressed>` - Content/actions when mouse button down
- `<Released>` - Content/actions when mouse button up

**Example:**
```xml
<Button X="100" Y="100" Width="80" Height="40" Variable="systemPower" On="ON" Off="OFF">
    <Enabled>
        <On>
            <Rectangle FillColor="0,1,0,1" Width="80" Height="40"/>
            <Text X="40" Y="20" LocalAlignX="1">ON</Text>
        </On>
        <Off>
            <Rectangle FillColor="0.3,0.3,0.3,1" Width="80" Height="40"/>
            <Text X="40" Y="20" LocalAlignX="1">OFF</Text>
        </Off>
    </Enabled>
    <Disabled>
        <Rectangle FillColor="0.2,0.2,0.2,1" Width="80" Height="40"/>
    </Disabled>
</Button>
```

---

## Mouse Event Elements

These elements can be children of `<Rectangle>`, `<Circle>`, `<Image>`, `<Polygon>`, `<Pixelstream>`, or `<Button>`.

### `<Pressed>`
Content/actions executed when mouse button is pressed down on the element.

### `<Released>`
Content/actions executed when mouse button is released on the element.

### `<Active>`
Content shown while mouse button is held down on the element.

### `<Inactive>`
Content shown when mouse is not pressed on the element.

### `<Hovered>`
Content shown when mouse cursor is over the element.

---

## Logic Elements

### `<If>`

Conditional rendering based on variable comparison.

| Attribute | Aliases | Type | Required | Description |
|-----------|---------|------|----------|-------------|
| `Operation` | — | integer | No | Comparison operation |
| `Value` | `Value1` | string/var | **Yes** | First value to compare |
| `Value2` | — | string/var | No | Second value to compare |

**Operation Constants:**

| Constant | Value | Description |
|----------|-------|-------------|
| `#_conditional_true_` | 0 | Boolean truthy check (default when `Value2` not provided) |
| `#_conditional_false_` | 1 | Boolean falsy check |
| `#_conditional_eq_` | 2 | Equal |
| `#_conditional_ne_` | 3 | Not equal |
| `#_conditional_lt_` | 4 | Less than |
| `#_conditional_gt_` | 5 | Greater than |
| `#_conditional_lte_` | 6 | Less than or equal |
| `#_conditional_gte_` | 7 | Greater than or equal |

**Children:**
- `<True>` - Content shown when condition is true
- `<False>` - Content shown when condition is false
- Direct children (without `<True>` wrapper) are treated as `<True>` content

**Example:**
```xml
<If Value="@altitude" Value2="1000" Operation="#_conditional_gt_">
    <True>
        <Text FillColor="1,0,0,1">HIGH ALTITUDE</Text>
    </True>
    <False>
        <Text FillColor="0,1,0,1">Normal altitude</Text>
    </False>
</If>
```

---

### `<StaticIf>`

Parse-time conditional that evaluates once during XML parsing and includes only the matching branch's children.

| Attribute | Aliases | Type | Required | Description |
|-----------|---------|------|----------|-------------|
| `Operation` | — | integer | No | Comparison operation |
| `Value` | `Value1` | string/var | **Yes** | First value to compare |
| `Value2` | — | string/var | No | Second value to compare |

**Operation Constants:**

| Constant | Value | Description |
|----------|-------|-------------|
| `#_conditional_true_` | 0 | Boolean truthy check (default when `Value2` not provided) |
| `#_conditional_false_` | 1 | Boolean falsy check |
| `#_conditional_eq_` | 2 | Equal |
| `#_conditional_ne_` | 3 | Not equal |
| `#_conditional_lt_` | 4 | Less than |
| `#_conditional_gt_` | 5 | Greater than |
| `#_conditional_lte_` | 6 | Less than or equal |
| `#_conditional_gte_` | 7 | Greater than or equal |

**Children:**
- `<True>` - Content included when condition is true
- `<False>` - Content included when condition is false
- Direct children (without `<True>` wrapper) are treated as `<True>` content

**How it works:**
- Unlike `<If>`, which evaluates every frame during rendering, `<StaticIf>` evaluates once during XML parsing
- The matching branch's children are "spliced" directly into the parent, as if the `<StaticIf>` never existed
- No conditional node is created in the scene graph
- Useful for conditional variable registration, TrickIO setup, or build-time configuration

**Example:**
```xml
<!-- Conditionally register debug variables -->
<StaticIf Value="@debugMode" Value2="1" Operation="#_conditional_eq_">
    <Variable Type="#_variable_double_" InitialValue="0">debugCounter</Variable>
    <Variable Type="#_variable_string_" InitialValue="">debugMessage</Variable>
</StaticIf>

<!-- Conditionally include TrickVariables -->
<TrickIO Host="localhost" Port="7000">
    <FromTrick>
        <TrickVariable Name="rocket.altitude">altitude</TrickVariable>
        <StaticIf Value="@useAdvancedTelemetry" Value2="1" Operation="#_conditional_eq_">
            <TrickVariable Name="rocket.fuel_temp">fuelTemp</TrickVariable>
            <TrickVariable Name="rocket.chamber_pressure">chamberPressure</TrickVariable>
        </StaticIf>
    </FromTrick>
</TrickIO>
```

---

### `<Set>`

Sets a variable to a value.

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `Variable` | string | **Yes** | Variable name to set |
| `Operator` | integer | No | Operation type |

**Content:** Value or expression to assign

**Operator Constants:**

| Constant | Value | Description |
|----------|-------|-------------|
| `#_set_equal_` | 0 | Direct assignment (default) |
| `#_set_add_` | 1 | Add |
| `#_set_subtract_` | 2 | Subtract |
| `#_set_multiply_` | 3 | Multiply |
| `#_set_divide_` | 4 | Divide |

**Example:**
```xml
<Set Variable="counter" Operator="#_set_add_">1</Set>  <!-- counter += 1 -->
<Set Variable="status">ACTIVE</Set>                    <!-- status = "ACTIVE" -->
```

---

## External Integration

### `<Logic>`

Loads custom C logic from a shared library.

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `File` | string | **Yes** | Path to shared library (.so/.dylib/.dll) |

**Expected Functions in Library:**
- `display_pre_init(GetVariableValueAddr)` - Called before init
- `display_init()` - Called at startup
- `display_draw()` - Called each frame
- `display_close()` - Called at shutdown

**Cross-Platform:** dcapp automatically tries `.so`, `.dylib`, and `.dll` extensions, so the same XML works on Linux, macOS, and Windows.

---

### `<Function>`

Calls a function from the loaded logic library.

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `Name` | string | **Yes** | Name of the function to call |

The function must have the signature `void function_name(void)`.

**Example:**
```xml
<Function Name="on_button_click"/>
```

See the [Logic Files documentation](logic.md) for details on using `<Function>` with buttons and conditionals.

---

### `<TrickIO>`

Establishes connection to Trick simulation.

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `Host` | string | **Yes** | Trick host address |
| `Port` | integer | **Yes** | Trick port number |
| `DataRate` | number | No | Data rate in Hz (default: 0.1) |

**Children:** `<FromTrick>`, `<ToTrick>`

---

### `<FromTrick>`

Container for variables received from Trick.

**Children:** `<TrickVariable>`

---

### `<ToTrick>`

Container for variables sent to Trick.

**Children:** `<TrickVariable>`

---

### `<TrickVariable>`

Maps a Trick variable to a dcapp variable.

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `Name` | string | **Yes** | Trick variable path |
| `Units` | string | No | Unit conversion |

**Content:** dcapp variable name

**Example:**
```xml
<TrickIO Host="localhost" Port="7000">
    <FromTrick>
        <TrickVariable Name="vehicle.altitude">altitude</TrickVariable>
        <TrickVariable Name="vehicle.speed" Units="kn">airspeed</TrickVariable>
    </FromTrick>
    <ToTrick>
        <TrickVariable Name="controls.throttle">throttlePosition</TrickVariable>
    </ToTrick>
</TrickIO>
```

---

## Terrain Elements

### `<Terrain>`

Renders 3D terrain with camera positioning.

| Attribute | Aliases | Type | Required | Description |
|-----------|---------|------|----------|-------------|
| `PositionX` | `X` | number/var | No | Screen X position |
| `PositionY` | `Y` | number/var | No | Screen Y position |
| `DimensionX` | `Width` | number/var | No | Display width |
| `DimensionY` | `Height` | number/var | No | Display height |
| `LocalAlignX` | `HorizontalAlign` | align | No | Horizontal alignment |
| `LocalAlignY` | `VerticalAlign` | align | No | Vertical alignment |
| `ParentAlignX` | — | align | No | Parent anchor (horizontal) |
| `ParentAlignY` | — | align | No | Parent anchor (vertical) |
| `Rotation` | `Rotate` | number/var | No | Rotation in degrees |
| `PivotPositionX` | `PivotX` | number/var | No | Pivot point X |
| `PivotPositionY` | `PivotY` | number/var | No | Pivot point Y |
| `PivotLocalAlignX` | — | align | No | Pivot alignment (horizontal) |
| `PivotLocalAlignY` | — | align | No | Pivot alignment (vertical) |
| `Latitude` | — | number/var | **Yes** | Camera latitude |
| `Longitude` | — | number/var | **Yes** | Camera longitude |
| `Elevation` | — | number/var | **Yes** | Camera elevation |
| `Roll` | — | number/var | **Yes** | Camera roll angle |
| `Pitch` | — | number/var | **Yes** | Camera pitch angle |
| `Yaw` | — | number/var | **Yes** | Camera yaw/heading angle |

**Children:** `<TerrainDEM>`

---

### `<TerrainDEM>`

Specifies a Digital Elevation Model file for terrain.

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `File` | string | **Yes** | Path to DEM file |

---

## Data Types

### Alignment Values

Integer attributes can use built-in constants with the `#` prefix (e.g., `ParentAlignX="#_align_left_"`).

**Horizontal Alignment** (for `LocalAlignX`, `ParentAlignX`, `PivotLocalAlignX`):

| Constant | Value | Description |
|----------|-------|-------------|
| `#_align_left_` | 0 | Left alignment (default) |
| `#_align_center_` | 1 | Center alignment |
| `#_align_right_` | 2 | Right alignment |

**Vertical Alignment** (for `LocalAlignY`, `ParentAlignY`, `PivotLocalAlignY`):

| Constant | Value | Description |
|----------|-------|-------------|
| `#_align_bottom_` | 0 | Bottom alignment (default) |
| `#_align_middle_` | 1 | Middle alignment |
| `#_align_top_` | 2 | Top alignment |

---

### How Alignment and Position Work Together

The alignment system has two separate concepts:

- **LocalAlign** - The anchor point on the element itself (where on the element is "the position")
- **ParentAlign** - The anchor point in the parent container (where in the parent to position)

**Position Calculation:**
```
final_position = parent_anchor + offset
```

Where:
- `parent_anchor` is determined by `ParentAlignX`/`ParentAlignY` (LEFT=0, CENTER=width/2, RIGHT=width, etc.)
- `offset` is the `X`/`Y` value

**Examples in an 800x600 parent:**

```xml
<!-- Absolute positioning: X=100 means 100 pixels from left edge -->
<Text X="100" Y="50" LocalAlignX="#_align_center_">Hello</Text>

<!-- Offset from center: X=10 means 10 pixels right of center (400+10=410) -->
<Text ParentAlignX="#_align_center_" X="10" LocalAlignX="#_align_center_">Hello</Text>

<!-- Offset from right edge: X=-20 means 20 pixels left of right edge (800-20=780) -->
<Text ParentAlignX="#_align_right_" X="-20" LocalAlignX="#_align_right_">Hello</Text>

<!-- Centered (no offset): element centered in parent -->
<Text ParentAlignX="#_align_center_" ParentAlignY="#_align_middle_"
      LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_">Hello</Text>
```

**Common Pitfall - Defaults with ParentAlign:**

If a `<Default>` sets `ParentAlignX`, any X values on child elements become offsets from that anchor, not absolute positions:

```xml
<!-- WRONG: Buttons end up offset from center instead of at absolute X positions -->
<Default>
    <Button ParentAlignX="#_align_center_" LocalAlignX="#_align_center_"/>
</Default>
<Button X="10">...</Button>   <!-- Ends up at center+10, not X=10! -->

<!-- CORRECT: Remove ParentAlignX from Default when using absolute positions -->
<Default>
    <Button LocalAlignX="#_align_center_"/>
</Default>
<Button X="10">...</Button>   <!-- Correctly at X=10 -->
```

**Special Cases:**

- **Circle and Sphere** elements default to center-aligned (`LocalAlignX="#_align_center_"`, `LocalAlignY="#_align_middle_"`) since their natural anchor is their center
- **Container, Panel, Window, and Button** reset the parent_position to {0,0} for their children, so children position relative to the container's top-left corner

**Example:**
```xml
<Text X="100" Y="50" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_">
    Centered Text
</Text>
```

### Built-in Constants Reference

All built-in constants use the `#` prefix followed by the constant name:

| Category | Constants |
|----------|-----------|
| **Alignment (Horizontal)** | `#_align_left_`, `#_align_center_`, `#_align_right_` |
| **Alignment (Vertical)** | `#_align_bottom_`, `#_align_middle_`, `#_align_top_` |
| **Button Types** | `#_button_standard_`, `#_button_momentary_`, `#_button_toggle_` |
| **Conditionals** | `#_conditional_true_`, `#_conditional_false_`, `#_conditional_eq_`, `#_conditional_ne_`, `#_conditional_lt_`, `#_conditional_gt_`, `#_conditional_lte_`, `#_conditional_gte_` |
| **Set Operations** | `#_set_equal_`, `#_set_add_`, `#_set_subtract_`, `#_set_multiply_`, `#_set_divide_` |
| **Pixelstream Types** | `#_pixelstream_dynamic_file_`, `#_pixelstream_mjpeg_` |

### Color Format

Colors are specified as comma-separated RGBA values, each from 0.0 to 1.0:
```
"R,G,B,A"
```

**Examples:**
- `"1,0,0,1"` - Red (fully opaque)
- `"0,1,0,0.5"` - Green (50% transparent)
- `"0.2,0.2,0.2,1"` - Dark gray

### Variable References

Attributes marked as `number/var` can contain:
- A literal number: `100`, `3.14`
- A variable reference: `@variableName`

---

## Complete Example

```xml
<dcapp>
    <Variable Type="#_variable_double_" InitialValue="0">altitude</Variable>
    <Variable Type="#_variable_double_" InitialValue="0">speed</Variable>
    <Variable Type="#_variable_string_" InitialValue="OFF">engineStatus</Variable>

    <Window Title="Flight Display" Width="800" Height="600" VirtualWidth="800" VirtualHeight="600">
        
        <!-- Background -->
        <Rectangle Width="800" Height="600" FillColor="0.1,0.1,0.2,1"/>
        
        <!-- Altitude Display -->
        <Container X="50" Y="50" Width="200" Height="100">
            <Rectangle FillColor="0,0,0,0.7" Width="200" Height="100"/>
            <Text X="100" Y="70" Size="16" FillColor="1,1,1,1" LocalAlignX="#_align_center_">ALTITUDE</Text>
            <Text X="100" Y="30" Size="32" FillColor="0,1,0,1" LocalAlignX="#_align_center_">@altitude(%.0f) ft</Text>
        </Container>
        
        <!-- Engine Control Button -->
        <Button X="350" Y="500" Width="100" Height="50" 
                Variable="engineStatus" On="ON" Off="OFF" Type="#_button_toggle_">
            <Enabled>
                <On>
                    <Rectangle FillColor="0,0.6,0,1" Width="100" Height="50"/>
                    <Text X="50" Y="25" Size="18" FillColor="1,1,1,1" 
                          LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_">ENGINE ON</Text>
                </On>
                <Off>
                    <Rectangle FillColor="0.5,0,0,1" Width="100" Height="50"/>
                    <Text X="50" Y="25" Size="18" FillColor="1,1,1,1" 
                          LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_">ENGINE OFF</Text>
                </Off>
            </Enabled>
        </Button>
        
        <!-- Altitude Warning -->
        <If Value="@altitude" Value2="10000" Operation="#_conditional_gt_">
            <True>
                <Text X="400" Y="300" Size="48" FillColor="1,0,0,1" LocalAlignX="#_align_center_">
                    ⚠ HIGH ALTITUDE
                </Text>
            </True>
        </If>
        
    </Window>
</dcapp>
```
