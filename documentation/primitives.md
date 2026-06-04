# dcapp XML Reference

A comprehensive reference for all XML elements and attributes in the dcapp display framework.

---

## Document Structure

```xml
<DCAPP>
    <Window ...>
        <!-- Display elements go here -->
    </Window>
</DCAPP>
```

---

## Root Elements

### `<DCAPP>`

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
| `FrameRateLimit` | `MaxFPS`, `MaxFrameRate` | number/var | No | Maximum frames per second; values <= 0 disable limiting |

**Example:**
```xml
<Window Title="Flight Display" Width="1920" Height="1080" VirtualWidth="1920" VirtualHeight="1080" FrameRateLimit="30">
    ...
</Window>
```

---

## Data Elements

### `<Variable>`

Declares a runtime variable that can be referenced and modified.

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `Type` | string | No | Data type constant: `#_variable_string_`, `#_variable_integer_`, `#_variable_double_` (default: string) |
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

A simpler container that sets virtual dimensions and can draw an optional background.

| Attribute | Aliases | Type | Required | Description |
|-----------|---------|------|----------|-------------|
| `VirtualDimensionX` | `VirtualWidth` | number/var | No | Virtual coordinate width |
| `VirtualDimensionY` | `VirtualHeight` | number/var | No | Virtual coordinate height |
| `BackgroundColor` | — | color | No | Panel background fill color |

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
| `Rounded` | — | boolean/var | No | Round corners (radius = 10% of smaller dimension) |

**Children:** `<MousePressed>`, `<MouseReleased>`, `<MouseActive>`, `<MouseInactive>`, `<MouseHovered>` (mouse events)

---

### `<Circle>` *(Deprecated)*

**Deprecated:** Use `<Arc>` for line-only circles or `<Ellipse>` for filled circles.

The legacy conversion script (`scripts/convert-legacy-xml.py`) automatically converts:
- `<Circle>` with `Angle` and `FillColor` → `<Ellipse>` (pie/wedge shape)
- `<Circle>` with `Angle` only → `<Arc>` (line-only arc)
- `<Circle>` in `<Style>` → both `<Arc>` and `<Ellipse>` styles

---

### `<Arc>`

Draws an arc (partial circle outline). Arc is a **line-only** element and does not support fill.

| Attribute | Aliases | Type | Required | Description |
|-----------|---------|------|----------|-------------|
| `PositionX` | `X` | number/var | No | Center X position |
| `PositionY` | `Y` | number/var | No | Center Y position |
| `Radius` | — | number/var | No | Arc radius |
| `Angle` | — | number/var | No | Arc angle in degrees (default: 360 = full circle) |
| `Segments` | — | number/var | No | Number of segments for rendering |
| `LocalAlignX` | `HorizontalAlign` | align | No | Horizontal alignment |
| `LocalAlignY` | `VerticalAlign` | align | No | Vertical alignment |
| `ParentAlignX` | — | align | No | Parent anchor (horizontal) |
| `ParentAlignY` | — | align | No | Parent anchor (vertical) |
| `Rotation` | `Rotate` | number/var | No | Rotation in degrees (0° = top, clockwise) |
| `PivotPositionX` | `PivotX` | number/var | No | Pivot point X |
| `PivotPositionY` | `PivotY` | number/var | No | Pivot point Y |
| `PivotLocalAlignX` | — | align | No | Pivot alignment (horizontal) |
| `PivotLocalAlignY` | — | align | No | Pivot alignment (vertical) |
| `LineColor` | — | color | No | Arc line color (RGBA) |
| `LineWidth` | — | number/var | No | Line width |

**Note:** Arc starts drawing from the top (12 o'clock position) and proceeds clockwise. Use `Rotation` to change the starting position.

**Example:**
```xml
<!-- 90-degree arc starting from top -->
<Arc X="100" Y="100" Radius="50" Angle="90" LineColor="1,1,1,1" LineWidth="2"/>

<!-- Arc rotated to start from the right (3 o'clock) -->
<Arc X="200" Y="100" Radius="50" Angle="90" Rotation="90" LineColor="0,1,0,1"/>
```

---

### `<Ellipse>`

Draws a filled ellipse or pie/wedge shape.

| Attribute | Aliases | Type | Required | Description |
|-----------|---------|------|----------|-------------|
| `PositionX` | `X` | number/var | No | Center X position |
| `PositionY` | `Y` | number/var | No | Center Y position |
| `Radius` | — | number/var | No | Circle radius (sets both RadiusX and RadiusY) |
| `RadiusX` | — | number/var | No | Horizontal radius |
| `RadiusY` | — | number/var | No | Vertical radius |
| `Angle` | — | number/var | No | Wedge angle in degrees (default: 360 = full ellipse) |
| `Segments` | — | number/var | No | Number of segments for rendering |
| `LocalAlignX` | `HorizontalAlign` | align | No | Horizontal alignment |
| `LocalAlignY` | `VerticalAlign` | align | No | Vertical alignment |
| `ParentAlignX` | — | align | No | Parent anchor (horizontal) |
| `ParentAlignY` | — | align | No | Parent anchor (vertical) |
| `Rotation` | `Rotate` | number/var | No | Rotation in degrees (0° = top, clockwise) |
| `PivotPositionX` | `PivotX` | number/var | No | Pivot point X |
| `PivotPositionY` | `PivotY` | number/var | No | Pivot point Y |
| `PivotLocalAlignX` | — | align | No | Pivot alignment (horizontal) |
| `PivotLocalAlignY` | — | align | No | Pivot alignment (vertical) |
| `FillColor` | — | color | No | Fill color (RGBA) |
| `LineColor` | — | color | No | Border color (RGBA) |
| `LineWidth` | — | number/var | No | Border width |

**Note:** When `Angle` is less than 360, Ellipse draws a pie/wedge shape (filled sector). The wedge starts from the top (12 o'clock position) and proceeds clockwise. Use `Rotation` to change the starting position.

**Children:** `<MousePressed>`, `<MouseReleased>`, `<MouseActive>`, `<MouseInactive>`, `<MouseHovered>` (mouse events)

**Example:**
```xml
<!-- Full ellipse -->
<Ellipse X="100" Y="100" RadiusX="80" RadiusY="50" FillColor="0,0,1,1"/>

<!-- Pie wedge (quarter circle) -->
<Ellipse X="200" Y="100" Radius="50" Angle="90" FillColor="1,0,0,1"/>

<!-- Pie wedge rotated to start from right side -->
<Ellipse X="300" Y="100" Radius="50" Angle="90" Rotation="90" FillColor="0,1,0,1"/>
```

---

### `<Sphere>`

Draws a 3D sphere with optional texture mapping and internal rotation. Useful for attitude indicators (ADI balls).

| Attribute | Aliases | Type | Required | Description |
|-----------|---------|------|----------|-------------|
| `PositionX` | `X` | number/var | No | X position |
| `PositionY` | `Y` | number/var | No | Y position |
| `Radius` | — | number/var | No | Sphere radius |
| `FillColor` | — | color | No | Fill color (RGBA) |
| `Roll` | — | number/var | No | Internal roll rotation (degrees) |
| `Pitch` | — | number/var | No | Internal pitch rotation (degrees) |
| `Yaw` | — | number/var | No | Internal yaw rotation (degrees) |
| `Image` | — | string | No | Path to texture image file |
| `NegateX` | — | boolean | No | Negate X axis orientation |
| `NegateY` | — | boolean | No | Negate Y axis orientation |
| `Rotation` | — | number/var | No | External 2D rotation in the orthographic view |
| `LocalAlignX` | `HorizontalAlign` | align | No | Horizontal alignment (default: center) |
| `LocalAlignY` | `VerticalAlign` | align | No | Vertical alignment (default: middle) |
| `ParentAlignX` | — | align | No | Parent anchor (horizontal) |
| `ParentAlignY` | — | align | No | Parent anchor (vertical) |
| `PivotPositionX` | `PivotX` | number/var | No | Pivot point X |
| `PivotPositionY` | `PivotY` | number/var | No | Pivot point Y |
| `PivotLocalAlignX` | — | align | No | Pivot alignment (horizontal) |
| `PivotLocalAlignY` | — | align | No | Pivot alignment (vertical) |

**Example:**
```xml
<!-- ADI ball driven by vehicle attitude -->
<Sphere X="200" Y="200" Radius="100"
        Roll="@roll" Pitch="@pitch" Yaw="@yaw"
        Image="textures/adi_ball.png"/>

<!-- Simple colored sphere -->
<Sphere X="400" Y="300" Radius="50" FillColor="0.3 0.3 0.8 1"/>
```

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
| `Rounded` | — | boolean/var | No | Round corners (radius = 10% of bounding box's smaller dimension) |

**Children:** `<Vertex>` elements, `<MousePressed>`, `<MouseReleased>`, `<MouseActive>`, `<MouseInactive>`, `<MouseHovered>`

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

**Children:** `<MousePressed>`, `<MouseReleased>`, `<MouseActive>`, `<MouseInactive>`, `<MouseHovered>` (mouse events)

---

### `<PixelStream>`

Displays streaming video content (MJPEG or dynamic file). Standard positioning/alignment attributes apply. See [Integration — PixelStream](integration.md#pixelstream) for full details.

| Attribute | Aliases | Type | Required | Description |
|-----------|---------|------|----------|-------------|
| `Type` | `Protocol` | integer | **Yes** | `#_pixelstream_shmem_` or `#_pixelstream_mjpeg_` |
| `URL` | — | string | Required for MJPEG | Stream URL |
| `Timeout` | — | integer | No | Connection timeout in seconds (default: 5) |

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
| `FillColor` | `Color` | color | No | Text fill color |
| `LineColor` | — | color | No | Text outline color |
| `BackgroundColor` | — | color | No | Text background fill color |
| `Font` | — | string | No | Path to a TTF font file (relative to XML directory or absolute). Defaults to Bitstream Vera Sans. |
| `UpdateRate` | — | number/var | No | Minimum seconds between variable-expansion refreshes |

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

Creates an interactive button with multiple visual states. See [Buttons](buttons.md) for the full reference including value/variable inheritance, visual states, and examples.

### Mouse Events

Mouse event children (`<MousePressed>`, `<MouseReleased>`, `<MouseActive>`, `<MouseInactive>`, `<MouseHovered>`, `<MouseMotion>`) can be added to `<Rectangle>`, `<Ellipse>`, `<Image>`, `<Polygon>`, `<PixelStream>`, or `<Button>`. See [Mouse Events](mouse-events.md) for details.

### `<Blink>`

Wraps child elements in a flashing container with configurable frequency, duty cycle, and duration. See [Blink](blink.md) for the full reference.

### `<Stencil>`

Defines a stencil mask region for clipping child content. Contains `<StencilAdd>`, `<StencilRemove>`, and `<StencilDraw>` children. See [Stencil](stencil.md) for the full reference.

---

## Logic Elements

### `<If>`

Conditional rendering based on variable comparison.

| Attribute | Aliases | Type | Required | Description |
|-----------|---------|------|----------|-------------|
| `Operator` | — | integer | No | Comparison operator |
| `Value` | `Value1` | string/var | **Yes** | First value to compare |
| `Value2` | — | string/var | No | Second value to compare |
| `Static` | — | boolean | No | If "true", evaluates once at parse time (default: false) |

**Operator Constants:**

| Constant | Value | Description |
|----------|-------|-------------|
| `#_if_true_` | 1 | Boolean truthy check (default when `Value2` not provided) |
| `#_if_false_` | 2 | Boolean falsy check |
| `#_if_eq_` | 3 | Equal |
| `#_if_ne_` | 4 | Not equal |
| `#_if_lt_` | 5 | Less than |
| `#_if_gt_` | 6 | Greater than |
| `#_if_lte_` | 7 | Less than or equal |
| `#_if_gte_` | 8 | Greater than or equal |

**Children:**
- `<True>` - Content shown when condition is true
- `<False>` - Content shown when condition is false
- Direct children (without `<True>` wrapper) are treated as `<True>` content

**Example:**
```xml
<If Value="@altitude" Value2="1000" Operator="#_if_gt_">
    <True>
        <Text FillColor="1,0,0,1">HIGH ALTITUDE</Text>
    </True>
    <False>
        <Text FillColor="0,1,0,1">Normal altitude</Text>
    </False>
</If>
```

---

### `<If Static="true">` (Parse-Time Conditional)

When `Static="true"` is set on an `<If>` element, it evaluates once during XML parsing and includes only the matching branch's children. This replaces the deprecated `<StaticIf>` element.

**How it works:**
- Unlike runtime `<If>`, which evaluates every frame during rendering, `<If Static="true">` evaluates once during XML parsing
- The matching branch's children are "spliced" directly into the parent, as if the `<If>` never existed
- No conditional node is created in the scene graph
- Useful for conditional variable registration, TrickIO setup, or build-time configuration
- **Important:** When `Static="true"`, `Value`/`Value1` and `Value2` cannot use runtime variables (`@`). Only constants (`#`) and literal values are allowed.

**Example:**
```xml
<!-- Conditionally register debug variables -->
<If Static="true" Value="#debugMode" Value2="1" Operator="#_if_eq_">
    <Variable Type="#_variable_double_" InitialValue="0">debugCounter</Variable>
    <Variable Type="#_variable_string_" InitialValue="">debugMessage</Variable>
</If>

<!-- Conditionally include TrickVariables -->
<TrickIO Host="localhost" Port="7000">
    <TrickFrom>
        <TrickVariable Name="rocket.altitude">altitude</TrickVariable>
        <If Static="true" Value="#useAdvancedTelemetry" Value2="1" Operator="#_if_eq_">
            <TrickVariable Name="rocket.fuel_temp">fuelTemp</TrickVariable>
            <TrickVariable Name="rocket.chamber_pressure">chamberPressure</TrickVariable>
        </If>
    </TrickFrom>
</TrickIO>
```

### `<Set>`

Sets a variable to a value using an operator. See [Variables — Set Operators](variables.md#set-operators) for the full operator list.

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `Variable` | string | **Yes** | Variable name to set |
| `Operator` | integer | No | Operation type (default: `#_set_equal_`) |

**Content:** Value or expression to assign

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
- `display_pre_init(const DcInit *init)` - Auto-generated by the `dcapp.h` header; the user does not implement this
- `display_init(void)` - Called at startup (user-implemented)
- `display_draw(void)` - Called each frame (user-implemented)
- `display_close(void)` - Called at shutdown (user-implemented)

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

### `<TrickIO>`, `<EdgeIO>`, `<PixelStream>`

See [Integration](integration.md) for full documentation on TrickIO, EdgeIO, and PixelStream elements.

---

## Planet Elements

### `<Planet>`

Top-level resource definition for 3D planetary terrain. Defines the planet's data files, texture overlay, and shader overrides. Multiple `<PlanetView>` elements can reference the same `<Planet>` by name.

**Parent:** `<DCAPP>` (top-level only)

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `Name` | string | **Yes** | Unique name used by `<PlanetView>` to reference this planet |
| `ShaderIndex` | integer/var | No | Active shader index (selects from child `<PlanetShader>` elements) |

**Children:** `<PlanetData>`, `<PlanetShader>`, `<PlanetTexture>`

---

### `<PlanetView>`

Renders a view of a named planet. Supports two camera modes: LLE (latitude/longitude/elevation) and XYZ/RPY (position/orientation). Multiple views can reference the same planet.

**Parent:** `<Window>`, `<Panel>`, `<Container>`, or any drawable parent

| Attribute | Aliases | Type | Required | Description |
|-----------|---------|------|----------|-------------|
| `Planet` | — | string | **Yes** | Name of the `<Planet>` definition to render |
| `PositionX` | `X` | number/var | No | Screen X position |
| `PositionY` | `Y` | number/var | No | Screen Y position |
| `DimensionX` | `Width` | number/var | No | Display width |
| `DimensionY` | `Height` | number/var | No | Display height |
| `LocalAlignX` | `HorizontalAlign` | align | No | Horizontal alignment |
| `LocalAlignY` | `VerticalAlign` | align | No | Vertical alignment |
| `ParentAlignX` | — | align | No | Parent anchor (horizontal) |
| `ParentAlignY` | — | align | No | Parent anchor (vertical) |
| `Rotation` | `Rotate` | number/var | No | 2D rotation in degrees |
| `PivotPositionX` | `PivotX` | number/var | No | Pivot point X |
| `PivotPositionY` | `PivotY` | number/var | No | Pivot point Y |
| `PivotLocalAlignX` | — | align | No | Pivot alignment (horizontal) |
| `PivotLocalAlignY` | — | align | No | Pivot alignment (vertical) |
| `CameraLatitude` | — | number/var | No | Camera latitude (LLE mode) |
| `CameraLongitude` | — | number/var | No | Camera longitude (LLE mode) |
| `CameraElevation` | — | number/var | No | Camera elevation in meters (LLE mode) |
| `CameraHeading` | — | number/var | No | Camera heading (LLE mode, degrees CW from north) |
| `CameraX` | — | number/var | No | Camera X position (XYZ mode) |
| `CameraY` | — | number/var | No | Camera Y position (XYZ mode) |
| `CameraZ` | — | number/var | No | Camera Z position (XYZ mode) |
| `CameraRoll` | — | number/var | No | Camera roll angle (XYZ mode) |
| `CameraPitch` | — | number/var | No | Camera pitch angle (XYZ mode) |
| `CameraYaw` | — | number/var | No | Camera yaw angle (XYZ mode) |
| `CameraOrthographic` | — | integer/var | No | 1 for orthographic projection, 0 for perspective |

**Children:** `<PlanetEllipse>`, `<PlanetGeoJSON>`, `<PlanetText>`, and other planet overlay elements

---

### `<PlanetData>`

Specifies a planet terrain JSON file containing chunked heightmap data. Child of `<Planet>`.

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `File` | string | **Yes** | Path to `.planet.json` file |

---

### `<PlanetShader>`

Registers a custom shader for the planet, selectable at runtime via the parent `ShaderIndex` attribute. Child of `<Planet>`.

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `Index` | integer | **Yes** | Shader index (matched against `ShaderIndex`) |
| `VertexShader` | string | No | Path to custom vertex shader (`.vert`) |
| `FragmentShader` | string | No | Path to custom fragment shader (`.frag`) |

---

### `<PlanetTexture>`

Configures a texture overlay on the planet surface. The `File` attribute is a static file path; all other attributes are dynamic (can be bound to variables). Changing the value of `FireRefresh` at runtime triggers a re-read of the texture file from disk. Child of `<Planet>`.

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `File` | string | No | Path to texture image file |
| `MetersPerPixel` | number/var | No | Scale of the texture in meters per pixel; must be greater than zero |
| `Latitude` | number/var | No | Latitude of texture center for geodetic CRS |
| `Longitude` | number/var | No | Longitude of texture center for geodetic CRS |
| `X`, `Y`, `Z` | number/var | No | Texture center for cartesian CRS |
| `OriginX`, `OriginY` | number/var | No | Projected terrain-meter center override; both must be specified together |
| `FireRefresh` | integer/var | No | Edge-triggered: changing this value re-reads the texture file, scale, and position |

---

## Positioning and Alignment

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

- **Arc, Ellipse, and Sphere** elements default to center-aligned (`LocalAlignX="#_align_center_"`, `LocalAlignY="#_align_middle_"`) since their natural anchor is their center
- **Container, Panel, Window, and Button** reset the parent_position to {0,0} for their children, so children position relative to the container's top-left corner

**Example:**
```xml
<Text X="100" Y="50" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_">
    Centered Text
</Text>
```

See [Constants](constants.md) for the full built-in constants reference.

### Color Format

Colors are specified as comma-separated or space-separated RGBA values, each from 0.0 to 1.0:
```
"R,G,B,A"   or   "R G B A"
```

**Examples:**
- `"1,0,0,1"` - Red (fully opaque)
- `"0 1 0 0.5"` - Green (50% transparent)
- `"0.2,0.2,0.2,1"` - Dark gray

### Variable References

Attributes marked as `number/var` can contain:
- A literal number: `100`, `3.14`
- A variable reference: `@variableName`

---

## Complete Example

```xml
<DCAPP>
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
        <If Value="@altitude" Value2="10000" Operator="#_if_gt_">
            <True>
                <Text X="400" Y="300" Size="48" FillColor="1,0,0,1" LocalAlignX="#_align_center_">
                    ⚠ HIGH ALTITUDE
                </Text>
            </True>
        </If>
        
    </Window>
</DCAPP>
```
