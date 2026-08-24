# XML element reference

This page lists the elements accepted by a dcapp display. Detailed behavior
for buttons, events, variables, integrations, and planet rendering is linked
from the relevant entry.

```xml
<DCAPP>
    <Window ...>
        <!-- Display elements go here -->
    </Window>
</DCAPP>
```

`Window` owns the application window. `Panel` and `Container` establish local
coordinate spaces. Drawing elements, interaction, and logic live below them;
declarations such as `Variable`, `TrickIO`, `EdgeIO`, `Logic`, and `Planet`
live directly under `DCAPP`.

## Root elements

### `<DCAPP>`

The root element that wraps the entire display definition. After preprocessing,
its children may be `<Window>`, `<Variable>`, `<TrickIO>`, `<EdgeIO>`,
`<Planet>`, and `<Logic>`. Authoring declarations such as `<Constant>` and
`<Style>` also begin here but disappear before runtime validation.

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
| `UpdateRate` | — | number/var | No | Fixed logic updates per second; rendering still syncs to the display |
| `Fullscreen` | — | boolean | No | Start the window in fullscreen mode |

```xml
<Window Title="Flight Display" Width="1920" Height="1080" VirtualWidth="1920" VirtualHeight="1080" UpdateRate="60" Fullscreen="false">
    ...
</Window>
```

## Data elements

### `<Variable>`

Declares a runtime variable. See [Variables](variables.md) for text expansion,
`Set` operators, and external bindings.

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `Type` | string | No | `#_variable_string_`, `#_variable_integer_`, `#_variable_double_`, or `#_variable_boolean_` (default: string) |
| `InitialValue` | string | No | Initial value (default: empty string) |

The element content is the variable name.

```xml
<Variable Type="#_variable_double_" InitialValue="0.0">altitude</Variable>
<Variable Type="#_variable_string_" InitialValue="OFF">systemStatus</Variable>
```

### `<Constant>`

Declares a value substituted during XML preprocessing. See
[Constants](constants.md) for its attributes and the built-in constants.

### `<Style>`

Defines a reusable style. Styles are handled during preprocessing rather than
as runtime display nodes.

## Layout elements

### `<Container>`

A grouping element that establishes a coordinate space for child elements.
Use a container when a group of elements should move, rotate, scale to a virtual
coordinate system, or share an interaction region.

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

Pivot position and pivot alignment are mutually exclusive. Use one pair or the
other.

### `<Panel>`

A simpler container that sets virtual dimensions and can draw an optional background.
Use a panel when you mostly need a display area with known virtual dimensions,
not the full transform and interaction behavior of a container.

| Attribute | Aliases | Type | Required | Description |
|-----------|---------|------|----------|-------------|
| `VirtualDimensionX` | `VirtualWidth` | number/var | No | Virtual coordinate width |
| `VirtualDimensionY` | `VirtualHeight` | number/var | No | Virtual coordinate height |
| `BackgroundColor` | — | color | No | Panel background fill color |

### `<Include>`

Includes another XML file. Put the path in the element content; the `File`
attribute remains as a fallback. Included content keeps its own `_Directory`
context, so relative paths remain local to the included file.

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `File` | string | No | Path used when the element content is empty |
| `Optional` | boolean | No | Skip a missing file instead of reporting an error. Defaults to false. |

```xml
<Include Optional="true">includes/debug.xml</Include>
```

## Drawing primitives

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
| `LinePattern` | — | integer/var | No | 8-bit dash pattern for the outline, such as `0xFF` solid or `0xAA` dashed |
| `Rounded` | — | boolean/var | No | Round corners (radius = 10% of smaller dimension) |

Mouse event elements may be nested here: `<MousePressed>`, `<MouseReleased>`,
`<MouseActive>`, `<MouseInactive>`, and `<MouseHovered>`.

### `<Circle>` *(Deprecated)*

Deprecated. Use `<Arc>` for line-only circles or `<Ellipse>` for filled circles.

The legacy conversion script (`scripts/convert-legacy-xml.py`) automatically converts:
- `<Circle>` with `Angle` and `FillColor` → `<Ellipse>` (pie/wedge shape)
- `<Circle>` with `Angle` only → `<Arc>` (line-only arc)
- `<Circle>` in `<Style>` → both `<Arc>` and `<Ellipse>` styles

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
| `Rotation` | `Rotate` | number/var | No | Rotation in degrees, applied to the whole arc |
| `PivotPositionX` | `PivotX` | number/var | No | Pivot point X |
| `PivotPositionY` | `PivotY` | number/var | No | Pivot point Y |
| `PivotLocalAlignX` | — | align | No | Pivot alignment (horizontal) |
| `PivotLocalAlignY` | — | align | No | Pivot alignment (vertical) |
| `LineColor` | — | color | No | Arc line color (RGBA) |
| `LineWidth` | — | number/var | No | Line width |
| `LinePattern` | — | integer/var | No | 8-bit dash pattern, such as `0xFF` solid or `0xAA` dashed |

An arc starts on the positive X axis (3 o'clock) and proceeds counterclockwise.
Use `Rotation` to change the starting position.

```xml
<!-- 90-degree arc starting at 3 o'clock -->
<Arc X="100" Y="100" Radius="50" Angle="90" LineColor="1,1,1,1" LineWidth="2"/>

<!-- Rotated to start at 12 o'clock -->
<Arc X="200" Y="100" Radius="50" Angle="90" Rotation="90" LineColor="0,1,0,1"/>
```

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
| `Rotation` | `Rotate` | number/var | No | Rotation in degrees, applied to the whole ellipse |
| `PivotPositionX` | `PivotX` | number/var | No | Pivot point X |
| `PivotPositionY` | `PivotY` | number/var | No | Pivot point Y |
| `PivotLocalAlignX` | — | align | No | Pivot alignment (horizontal) |
| `PivotLocalAlignY` | — | align | No | Pivot alignment (vertical) |
| `FillColor` | — | color | No | Fill color (RGBA) |
| `LineColor` | — | color | No | Border color (RGBA) |
| `LineWidth` | — | number/var | No | Border width |
| `LinePattern` | — | integer/var | No | 8-bit dash pattern for the outline, such as `0xFF` solid or `0xAA` dashed |

When `Angle` is less than 360, `Ellipse` draws a filled wedge. It starts on
the positive X axis (3 o'clock) and proceeds counterclockwise. Use `Rotation`
to change the starting position.

Mouse event elements may be nested here: `<MousePressed>`, `<MouseReleased>`,
`<MouseActive>`, `<MouseInactive>`, and `<MouseHovered>`.

```xml
<!-- Full ellipse -->
<Ellipse X="100" Y="100" RadiusX="80" RadiusY="50" FillColor="0,0,1,1"/>

<!-- Pie wedge (quarter circle) -->
<Ellipse X="200" Y="100" Radius="50" Angle="90" FillColor="1,0,0,1"/>

<!-- Pie wedge rotated to start at 12 o'clock -->
<Ellipse X="300" Y="100" Radius="50" Angle="90" Rotation="90" FillColor="0,1,0,1"/>
```

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

```xml
<!-- ADI ball driven by vehicle attitude -->
<Sphere X="200" Y="200" Radius="100"
        Roll="@roll" Pitch="@pitch" Yaw="@yaw"
        Image="textures/adi_ball.png"/>

<!-- Simple colored sphere -->
<Sphere X="400" Y="300" Radius="50" FillColor="0.3 0.3 0.8 1"/>
```

### `<Line>`

Draws a polyline through a series of vertices.

| Attribute | Aliases | Type | Required | Description |
|-----------|---------|------|----------|-------------|
| `PositionX` | `X` | number/var | No | Base X position |
| `PositionY` | `Y` | number/var | No | Base Y position |
| `Rotation` | `Rotate` | number/var | No | Rotation in degrees |
| `PivotPositionX` | `PivotX` | number/var | No | Pivot point X |
| `PivotPositionY` | `PivotY` | number/var | No | Pivot point Y |
| `LineColor` | — | color | No | Line color (RGBA) |
| `LineWidth` | — | number/var | No | Line width |
| `LinePattern` | — | integer/var | No | 8-bit dash pattern, such as `0xFF` solid or `0xAA` dashed |

Add one `<Vertex>` child for each point in the line.

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
| `LinePattern` | — | integer/var | No | 8-bit dash pattern for the outline, such as `0xFF` solid or `0xAA` dashed |
| `Rounded` | — | boolean/var | No | Round corners (radius = 10% of bounding box's smaller dimension) |

Add `<Vertex>` children for the polygon points. Mouse event elements may also
be nested here.

Filled polygons must be convex with vertices in perimeter order. Outlined
polygons do not have that convexity restriction.

### `<Vertex>`

Defines a point for `<Line>` or `<Polygon>` elements.

| Attribute | Aliases | Type | Required | Description |
|-----------|---------|------|----------|-------------|
| `PositionX` | `X` | number/var | **Yes** | X coordinate |
| `PositionY` | `Y` | number/var | **Yes** | Y coordinate |

```xml
<Polygon FillColor="1,0,0,1">
    <Vertex X="0" Y="0"/>
    <Vertex X="100" Y="0"/>
    <Vertex X="50" Y="100"/>
</Polygon>
```

## Media elements

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

Mouse event elements may be nested here: `<MousePressed>`, `<MouseReleased>`,
`<MouseActive>`, `<MouseInactive>`, and `<MouseHovered>`.

### `<PixelStream>`

Displays streaming video content. Standard positioning/alignment attributes apply. See [PixelStream](pixelstream.md) for full details.

| Attribute | Aliases | Type | Required | Description |
|-----------|---------|------|----------|-------------|
| `Type` | — | integer | **Yes** | `#_pixelstream_shmem_` or `#_pixelstream_mjpeg_` |
| `URL` | — | string | Required for MJPEG | Stream URL |
| `File` | `SharedMemoryKey` | string | Required for shared memory | Backing-file path used for the System V shared-memory key and RGBA data |
| `Timeout` | — | integer | No | MJPEG connection timeout in seconds (default: 5) |
| `TestPattern` | — | string | No | Fallback image path |

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
| `Bold` | — | boolean/var | No | Render with bold SDF styling |
| `Italic` | — | boolean/var | No | Render with italic slant styling |
| `ShadowOffset` | — | number/var | No | Offset for text shadow rendering |
| `NegateX` | — | boolean/var | No | Flip text horizontally |
| `NegateY` | — | boolean/var | No | Flip text vertically |
| `UpdateRate` | — | number/var | No | Minimum seconds between variable-expansion refreshes |

The element content is the displayed text.

Variable interpolation:

- `@variableName` - Insert variable value
- `@{variableName}` - Insert variable with braces (for adjacent text)
- `@variableName(%format)` - Format specifier (e.g., `@altitude(%.1f)`)

Escapes:

- `\n` - Newline
- `\t` - Tab
- `\\` - Backslash
- `\@` - Literal @ symbol
- `\"` - Quote
- `\'` - Single quote

```xml
<Text X="100" Y="50" Size="24" FillColor="1,1,1,1">
    Altitude: @altitude(%.0f) ft
</Text>
```

## Interactive elements

### `<Button>`

Creates an interactive button with multiple visual states. See [Buttons](buttons.md) for the full reference including value/variable inheritance, visual states, and examples.

### Mouse events

Mouse event children (`<MousePressed>`, `<MouseReleased>`, `<MouseActive>`, `<MouseInactive>`, `<MouseHovered>`, `<MouseMotion>`) can be added to `<Rectangle>`, `<Ellipse>`, `<Image>`, `<Polygon>`, `<PixelStream>`, or `<Button>`. See [Mouse Events](mouse-events.md) for details.

### `<Blink>`

Wraps child elements in a flashing container with configurable frequency, duty cycle, and duration. See [Blink](blink.md) for the full reference.

### `<Stencil>`

Defines a stencil mask region for clipping child content. Contains `<StencilAdd>`, `<StencilRemove>`, and `<StencilDraw>` children. See [Stencil](stencil.md) for the full reference.

## Logic elements

### `<If>`

Conditional rendering based on variable comparison.

| Attribute | Aliases | Type | Required | Description |
|-----------|---------|------|----------|-------------|
| `Operator` | — | integer | No | Comparison operator |
| `Value` | `Value1` | string/var | **Yes** | First value to compare |
| `Value2` | — | string/var | No | Second value to compare |
| `Static` | — | boolean | No | If "true", evaluates once at parse time (default: false) |

Operators:

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

The element accepts:

- `<True>` - Content shown when condition is true
- `<False>` - Content shown when condition is false
- Direct children (without `<True>` wrapper) are treated as `<True>` content

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

### `<If Static="true">` (parse-time conditional)

With `Static="true"`, `If` is evaluated once while the XML is parsed. The
matching branch is inserted directly into its parent and no runtime conditional
node is created. This replaces the deprecated `StaticIf` element.

Static conditions accept constants and literals, but not runtime `@` variable
references. They can select declarations as well as visible elements:

```xml
<If Static="true" Value="#debugMode" Value2="1" Operator="#_if_eq_">
    <Variable Type="#_variable_double_" InitialValue="0">debugCounter</Variable>
    <Variable Type="#_variable_string_" InitialValue="">debugMessage</Variable>
</If>
```

### `<Set>`

Sets a variable to a value using an operator. See [Variables — Set Operators](variables.md#set-operators) for the full operator list.

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `Variable` | string | **Yes** | Variable name to set |
| `Operator` | integer | No | Operation type (default: `#_set_equal_`) |

The element content is the value or expression to assign.

```xml
<Set Variable="counter" Operator="#_set_add_">1</Set>  <!-- counter += 1 -->
<Set Variable="status">ACTIVE</Set>                    <!-- status = "ACTIVE" -->
```

## External integration

### `<Logic>`

Loads custom C logic from a shared library.

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `File` | string | **Yes** | Path to shared library (.so/.dylib/.dll) |

`Logic` is a declaration and must be a direct child of `<DCAPP>`. Its position
among the root children does not matter.

The library exports:

- `display_pre_init(const DcInit *init)` - Auto-generated by the `dcapp.h` header; the user does not implement this
- `display_init(DcAppContext *app_ctx, void **user_data)` - Called at startup (user-implemented)
- `display_draw(DcAppContext *app_ctx, void *user_data)` - Called once per render by default, or at the fixed `Window` `UpdateRate` when that attribute is set (user-implemented)
- `display_close(DcAppContext *app_ctx, void *user_data)` - Called at shutdown (user-implemented)

dcapp tries `.so`, `.dylib`, and `.dll` extensions, so the same XML works on
Linux, macOS, and Windows.

### `<Function>`

Calls a function from the loaded logic library.

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `Name` | string | **Yes** | Name of the function to call |

The function must have the signature `void function_name(DcAppContext *app_ctx, void *user_data)`.

`Function` must be inside the `<Window>` render tree, optionally nested in a
panel, container, conditional branch, or event element.

```xml
<Function Name="on_button_click"/>
```

See the [Logic Files documentation](logic.md) for details on using `<Function>` with buttons and conditionals.

### `<DrawFunction>`

Calls a draw callback from the loaded logic library. Use this when a display needs
procedural C/C++ drawing inside the normal XML scene graph.

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `Name` | string | **Yes** | Name of the C draw callback to call |

The function must have this signature:

```c
void function_name(DcDrawContext *ctx, const DcDrawFuncArgs *args, void *user_data);
```

Optional `<Arg>` children pass values to the callback.

`DrawFunction` must be inside the `<Window>` render tree.

### `<Arg>`

Passes a typed value into a parent `<DrawFunction>`.

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `Type` | integer | **Yes** | Value type, such as `#_variable_string_`, `#_variable_integer_`, `#_variable_double_`, or `#_variable_boolean_` |
| `Value` | string/var | **Yes** | Literal, constant, or variable-backed value passed to the draw callback |

```xml
<DrawFunction Name="draw_widget">
    <Arg Type="#_variable_string_" Value="primary"/>
    <Arg Type="#_variable_double_" Value="@scale"/>
</DrawFunction>
```

See [Logic files](logic.md#drawfunction) for the `DrawFunction` C API.

### `<TrickIO>`, `<EdgeIO>`, `<PixelStream>`

See [Integration](integration.md) for full documentation on TrickIO, EdgeIO, and PixelStream elements.

## Planet elements

### `<Planet>`

Top-level resource definition for 3D planetary terrain. Defines the planet's data files, up to five texture overlays, and shader overrides. Multiple `<PlanetView>` elements can reference the same `<Planet>` by name.

Place `Planet` directly under `<DCAPP>`.

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `Name` | string | **Yes** | Unique name used by `<PlanetView>` to reference this planet |
| `ShaderIndex` | integer/var | No | Active shader index (selects from child `<PlanetShader>` elements) |

Its children are `<PlanetData>`, `<PlanetShader>`, and `<PlanetTexture>`.

### `<PlanetView>`

Renders a view of a named planet. Supports geodetic camera positions with local-NED attitude and cartesian camera positions with cartesian-RPY attitude. Multiple views can reference the same planet.

Place `PlanetView` under `<Window>`, `<Panel>`, `<Container>`, or another
drawable parent.

| Attribute | Aliases | Type | Required | Description |
|-----------|---------|------|----------|-------------|
| `Planet` | — | string | **Yes** | Name of the `<Planet>` definition to render |
| `CRS` | — | enum | **Yes** | Camera coordinate reference system: `#_planet_crs_geodetic_` or `#_planet_crs_cartesian_` |
| `AttitudeFrame` | — | enum | No | Camera attitude frame: `#_planet_attitude_frame_local_ned_` for geodetic CRS or `#_planet_attitude_frame_cartesian_rpy_` for cartesian CRS. Defaults from `CRS` if omitted |
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
| `CameraLatitude` | — | number/var | Yes for geodetic CRS | Camera latitude |
| `CameraLongitude` | — | number/var | Yes for geodetic CRS | Camera longitude |
| `CameraElevation` | — | number/var | Yes for geodetic CRS | Camera elevation in meters |
| `CameraHeading` | — | number/var | No | Legacy alias for geodetic `CameraYaw` |
| `CameraFOV` | — | number/var | No | Vertical camera field of view in degrees. Defaults to 60 |
| `CameraX` | — | number/var | Yes for cartesian CRS | Camera X position in renderer-native Cartesian meters |
| `CameraY` | — | number/var | Yes for cartesian CRS | Camera Y position in renderer-native Cartesian meters |
| `CameraZ` | — | number/var | Yes for cartesian CRS | Camera Z position in renderer-native Cartesian meters |
| `CameraRoll` | — | number/var | No | Camera roll angle in the selected attitude frame |
| `CameraPitch` | — | number/var | No | Camera pitch angle in the selected attitude frame |
| `CameraYaw` | — | number/var | No | Camera yaw angle in the selected attitude frame |
| `CameraOrthographic` | — | integer/var | No | 1 for orthographic projection, 0 for perspective |

Its children are `<PlanetBreadcrumbs>`, `<PlanetContainer>`, `<PlanetEllipse>`,
`<PlanetGeoJSON>`, `<PlanetImage>`, `<PlanetLine>`, `<PlanetPolygon>`,
`<PlanetSphere>`, and `<PlanetText>`

The following drawable planet overlays support `Enabled`:
`<PlanetBreadcrumbs>`, `<PlanetContainer>`, `<PlanetEllipse>`,
`<PlanetGeoJSON>`, `<PlanetImage>`, `<PlanetLine>`, `<PlanetPolygon>`,
`<PlanetSphere>`, and `<PlanetText>`. `<PlanetTexture>` supports it separately
on the planet definition.

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `Enabled` | boolean/var | No | Enables drawing. Defaults to true. |

### `<PlanetBreadcrumbs>`

Records and draws a live breadcrumb trail from input position variables. The sampled position is independent of the `<PlanetView>` camera.

Place `PlanetBreadcrumbs` under `<PlanetView>`.

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `CRS` | enum | No | Coordinate reference system for the sampled position. Inherits from `<PlanetView>`. |
| `Latitude` | number/var | Yes for geodetic CRS | Source latitude in degrees |
| `Longitude` | number/var | Yes for geodetic CRS | Source longitude in degrees |
| `Altitude` | number/var | No | Source altitude in meters. If omitted, `HeightAboveTerrain` is used. |
| `X`, `Y`, `Z` | number/var | Yes for cartesian CRS | Source position in native body-centered Cartesian meters |
| `HeightAboveTerrain` | number/var | No | Geodetic fallback height above the surface in meters |
| `PointSpacing` | number/var | No | Minimum cartesian distance in meters between stored breadcrumb points. Defaults to 1. |
| `MaxPoints` | integer/var | No | Maximum stored points. Defaults to 4096. |
| `Clear` | integer/var | No | Edge-triggered: changing this value clears the stored trail |
| `Enabled` | boolean/var | No | Enables sampling and drawing. Defaults to true. |
| `LineColor` | color | No | Trail color (RGBA). Defaults to semi-transparent red. |
| `LineWidth` | number/var | No | Line width in logical display pixels |
| `LinePattern` | integer/var | No | 8-bit dash pattern, such as `0xF0` dashed. Defaults to solid. |

```xml
<PlanetBreadcrumbs Latitude="@VehicleLat" Longitude="@VehicleLon"
    HeightAboveTerrain="500" PointSpacing="25" MaxPoints="2000"
    Clear="@ClearTrail" Enabled="@ShowTrail"
    LineColor="1 0 0 0.5" LineWidth="2" LinePattern="0xF0"/>
```

### `<PlanetData>`

Specifies a planet terrain JSON file containing chunked heightmap data. Child of `<Planet>`.

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `File` | string | **Yes** | Path to `.planet.json` file |

### `<PlanetShader>`

Registers a custom shader for the planet, selectable at runtime via the parent `ShaderIndex` attribute. Child of `<Planet>`.

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `Index` | integer | **Yes** | Shader index (matched against `ShaderIndex`) |
| `VertexShader` | string | No | Path to custom vertex shader (`.vert`) |
| `FragmentShader` | string | No | Path to custom fragment shader (`.frag`) |

### `<PlanetTexture>`

Configures a texture overlay on the planet surface. A planet accepts up to five of these elements. Internal slots are assigned by declaration order, and overlapping overlays combine additively. The `File` attribute and the other runtime attributes can be bound to variables. Changing `FireRefresh` re-reads only this texture from disk. Child of `<Planet>`.

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `File` | string/var | No | Path to the texture image file. Passed through unchanged; use `$DCAPP_HOME` or `$DCAPP_DISPLAY_HOME` in XML to supply an absolute path. |
| `MetersPerPixel` | number/var | No | Scale of the texture in meters per pixel; must be greater than zero |
| `Latitude` | number/var | No | Latitude of texture center for geodetic CRS |
| `Longitude` | number/var | No | Longitude of texture center for geodetic CRS |
| `X`, `Y`, `Z` | number/var | No | Texture center for cartesian CRS |
| `OriginX`, `OriginY` | number/var | No | Projected terrain-meter center override; both must be specified together |
| `Enabled` | boolean/var | No | Loads or removes this overlay independently. Defaults to true. Re-enabling rebuilds the texture. |
| `FireRefresh` | integer/var | No | Edge-triggered: changing this value re-reads the current texture path, scale, and position |

Disabled overlays release their texture resources and therefore their texture VRAM. A `FireRefresh` change while disabled is recorded but does not load the overlay; the current values are used when it is enabled again.

## Positioning and alignment

`LocalAlignX` and `LocalAlignY` choose the anchor on the element itself.
`ParentAlignX` and `ParentAlignY` choose the anchor on its parent. `X` and `Y`
are offsets from that parent anchor:

```text
final position = parent anchor + offset
```

In an 800-by-600 parent:

```xml
<!-- 100 from the left edge -->
<Text X="100" Y="50" LocalAlignX="#_align_center_">Hello</Text>

<!-- 10 right of center: 400 + 10 = 410 -->
<Text ParentAlignX="#_align_center_" X="10" LocalAlignX="#_align_center_">Hello</Text>

<!-- 20 left of the right edge: 800 - 20 = 780 -->
<Text ParentAlignX="#_align_right_" X="-20" LocalAlignX="#_align_right_">Hello</Text>

<!-- Centered with no offset -->
<Text ParentAlignX="#_align_center_" ParentAlignY="#_align_middle_"
      LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_">Hello</Text>
```

If a `Default` sets `ParentAlignX`, every child `X` becomes an offset from that
anchor. Leave `ParentAlignX` out when the child values are intended to be
absolute:

```xml
<Default>
    <Button ParentAlignX="#_align_center_" LocalAlignX="#_align_center_"/>
</Default>
<Button X="10">...</Button>   <!-- center + 10 -->

<Default>
    <Button LocalAlignX="#_align_center_"/>
</Default>
<Button X="10">...</Button>   <!-- absolute X = 10 -->
```

- `Arc`, `Ellipse`, and `Sphere` default to center/middle local alignment.
- `Container`, `Panel`, `Window`, and `Button` give their children a new origin
  at the container's top-left corner.

See [Constants](constants.md) for the full built-in constants reference.

### Color format

Colors are specified as comma-separated or space-separated RGBA values, each from 0.0 to 1.0:
```
"R,G,B,A"   or   "R G B A"
```

Some common values:

- `"1,0,0,1"` - Red (fully opaque)
- `"0 1 0 0.5"` - Green (50% transparent)
- `"0.2,0.2,0.2,1"` - Dark gray

### Variable references

Attributes marked as `number/var` can contain:

- A literal number: `100`, `3.14`
- A variable reference: `@variableName`
