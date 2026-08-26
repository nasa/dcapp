# Migrating legacy displays

The converter requires Python 3 and `lxml`:

```bash
python3 -m pip install lxml
```

It handles the mechanical XML changes:

```bash
python3 scripts/convert-legacy-xml.py old.xml new.xml
python3 scripts/convert-legacy-xml.py --in-place display.xml
python3 scripts/convert-legacy-xml.py --directory displays/
```

Directory mode updates every XML file below the directory. Keep a copy or use
version control before running an in-place conversion. The output still needs
review: unsupported fields are preserved with an underscore, unsupported
elements become TODO comments, and a few legacy layout rules cannot be mapped
without knowing the author's intent.

## Element names

| Legacy | Current |
|--------|---------|
| `String` | `Text` |
| `Defaults` | `Default` |
| `DisplayLogic` | `Logic` |
| `OnPress` | `MousePressed` |
| `OnRelease` | `MouseReleased` |
| `TrickIo` | `TrickIO` |
| `FromTrick` | `TrickFrom` |
| `ToTrick` | `TrickTo` |
| `EdgeIo` | `EdgeIO` |
| `FromEdge` | `EdgeFrom` |
| `ToEdge` | `EdgeTo` |
| `Pixelstream` | `PixelStream` |

Button state children have their own names:

| Legacy child | Current child |
|--------------|---------------|
| `Active` | `ButtonEnabled` |
| `Inactive` | `ButtonDisabled` |
| `On` | `ButtonIndicatorOn` |
| `Off` | `ButtonIndicatorOff` |
| `Transition` | `ButtonTransition` |

`True` and `False` remain valid children of `If`. Children placed directly
inside an `If` are treated as its true branch.

Legacy masks are rewritten as:

| Legacy | Current |
|--------|---------|
| outer `Mask` | `Stencil` |
| child `Stencil` | `StencilAdd` |
| child `StencilSub` | `StencilRemove` |
| child `Projection` | `StencilDraw` |

The converter also applies `#_stencil_color_` to mask geometry so the stencil
write is fully opaque.

## Attribute names

| Element | Legacy | Current |
|---------|--------|---------|
| `Text`/`String` | `Color` | `FillColor` |
| `Line` | `Color` | `LineColor` |
| `If` | `Operation` | `Operator` |
| `If` | `Value` | `Value1` |
| `Button` | `ActiveVariable` | `EnableVariable` |
| `Button` | `ActiveOn` | `EnableOn` |
| `MouseMotion` | `XVariable` | `VariableX` |
| `MouseMotion` | `YVariable` | `VariableY` |
| `EdgeVariable` | `RcsCommand` | `Command` |

`Image` paths move from text content to `File`:

```xml
<!-- legacy -->
<Image Width="100" Height="100">images/map.tga</Image>

<!-- current -->
<Image Width="100" Height="100" File="images/map.tga"/>
```

`DisplayLogic` accepts either a legacy `File` attribute or text content. Both
forms become `<Logic File="..."/>`.

`DisplayIndex` and `ActiveDisplay` are preserved. `ForceUpdate` is removed.
The converter turns a panel's `BackgroundColor` into a first-child
`Rectangle`, preserving the old draw order.

## Constants

### Variable types

| Legacy literal | Current constant |
|----------------|------------------|
| `Decimal`, `Float`, `Double` | `#_variable_double_` |
| `Integer`, `Int` | `#_variable_integer_` |
| `String` | `#_variable_string_` |
| `Boolean`, `Bool` | `#_variable_boolean_` |

Already-converted constants are left alone.

### Button types

| Legacy | Current |
|--------|---------|
| `Standard` | `#_button_standard_` |
| `Momentary` | `#_button_momentary_` |
| `Toggle` | `#_button_toggle_` |

### Conditional operators

| Legacy | Current |
|--------|---------|
| `gt` or `>` | `#_if_gt_` |
| `lt` or `<` | `#_if_lt_` |
| `eq` or `==` | `#_if_eq_` |
| `ne` or `!=` | `#_if_ne_` |
| `gte`, `ge`, or `>=` | `#_if_gte_` |
| `lte`, `le`, or `<=` | `#_if_lte_` |

For a single boolean value, use `#_if_true_` or `#_if_false_`. The converter
adds `Static="true"` when neither operand contains an `@` variable reference.

### Set operators

| Legacy | Current |
|--------|---------|
| omitted or `=` | `#_set_equal_` |
| `+=` | `#_set_add_` |
| `-=` | `#_set_subtract_` |
| `*=` | `#_set_multiply_` |
| `/=` | `#_set_divide_` |

`MinimumValue` and `MaximumValue` become separate clamp operations:

```xml
<Set Variable="value" Operator="#_set_add_">10</Set>
<Set Variable="value" Operator="#_set_max_">0</Set>
<Set Variable="value" Operator="#_set_min_">100</Set>
```

`#_set_max_` enforces a lower bound; `#_set_min_` enforces an upper bound.

## Variable names and values

An `@` reads a value. Attributes that identify which variable to modify use
the bare name:

```xml
<Text>Count: @counter</Text>
<Set Variable="counter" Operator="#_set_add_">1</Set>
<Button Variable="enabled"/>
```

The converter strips a leading `@` from `Variable`, button variable fields,
`ConnectedVariable`, and mouse-motion variable fields. It leaves `@` in value
expressions and edge-triggered fields such as `FireBlink`.

## Alignment

Legacy `HorizontalAlign` and `VerticalAlign` did two jobs. With an explicit
coordinate they selected the element's anchor. Without a coordinate they also
placed that anchor within the parent. Current XML separates those jobs:

| Attribute | Meaning |
|-----------|---------|
| `LocalAlignX`, `LocalAlignY` | anchor on the element |
| `ParentAlignX`, `ParentAlignY` | anchor in the parent |
| `AlignX`, `AlignY` | shorthand that sets matching local and parent anchors |

Parent alignment and position are additive. In an 800-pixel parent,
`ParentAlignX="#_align_center_" X="10"` resolves to X=410.

### Explicit coordinates

When an axis has an explicit coordinate, the converter keeps that coordinate
absolute and writes only the local alignment for that axis:

```xml
<!-- legacy -->
<Text X="100" Y="200"
      HorizontalAlign="Center" VerticalAlign="Middle"/>

<!-- converted -->
<Text X="100" Y="200"
      ParentAlignX="#_align_left_" ParentAlignY="#_align_bottom_"
      LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
      PivotLocalAlignX="#_align_center_"
      PivotLocalAlignY="#_align_middle_"/>
```

The explicit left/bottom parent anchors prevent an inherited `AlignX` or
`AlignY` from turning those coordinates into offsets.

### No coordinate on an axis

Without a coordinate, the converter uses `AlignX` or `AlignY`:

```xml
<!-- legacy -->
<Text HorizontalAlign="Center" VerticalAlign="Middle"/>

<!-- converted -->
<Text AlignX="#_align_center_" AlignY="#_align_middle_"
      PivotLocalAlignX="#_align_center_"
      PivotLocalAlignY="#_align_middle_"/>
```

For mixed positioning, each axis is handled independently:

```xml
<!-- legacy -->
<Button Y="100" HorizontalAlign="Center" VerticalAlign="Middle"/>

<!-- converted -->
<Button Y="100"
        AlignX="#_align_center_"
        ParentAlignY="#_align_bottom_"
        LocalAlignY="#_align_middle_"
        PivotLocalAlignX="#_align_center_"
        PivotLocalAlignY="#_align_middle_"/>
```

Alignment literals map to the corresponding constants:

| Legacy | Current |
|--------|---------|
| `Left`, `Center`, `Right` | `#_align_left_`, `#_align_center_`, `#_align_right_` |
| `Bottom`, `Middle`, `Top` | `#_align_bottom_`, `#_align_middle_`, `#_align_top_` |

### Legacy origins

`OriginX="Right"` becomes a right parent anchor. Literal X offsets are negated;
variable offsets use `NegateX="true"`. `OriginX="Center"` becomes a center
parent anchor. `OriginY="Top"` and `OriginY="Middle"` follow the same rule with
Y and `NegateY`. Left and bottom are the defaults.

### Defaults and explicit positions

A default `ParentAlignX` changes every explicit X into an offset from that
anchor. Remove it when the children use absolute positions:

```xml
<Default>
    <Button LocalAlignX="#_align_center_"/>
</Default>

<Button X="10">...</Button>
<Button X="50">...</Button>
```

### Circles, arcs, and ellipses

A legacy `Circle` with `FillColor` becomes `Ellipse`; a line-only circle
becomes `Arc`. In a `Style`, the converter emits both forms because it cannot
know which kind of circle will use the style.

Legacy circle X/Y always referred to the center, regardless of alignment.
Converted arcs and ellipses therefore use their center-aligned defaults and
discard legacy circle alignment. A circle that combines non-default alignment
with rotation gets a TODO comment because its legacy parent-relative pivot
cannot be reproduced mechanically.

Modern arc and ellipse wedges start at 3 o'clock, matching legacy circle angle
zero. `Arc` defaults to 360 degrees.

## Buttons and legacy switch behavior

Most button variable attributes simply lose a leading `@`. `SwitchVariable` is
different: legacy buttons applied the switch through the deferred event queue.
The converter uses `IndicatorVariable` for the visual state and emits explicit
`MousePressed`/`MouseReleased` sets with `Defer="true"`.

Switch values use these fallbacks:

| Value | Fallback order |
|-------|----------------|
| on | `SwitchOn`, then `On`, then `1` |
| off | `SwitchOff`, then `Off`, then `0` |

The generated event behavior is:

| Button type | Conversion |
|-------------|------------|
| standard | set the on value on press |
| toggle | compare the indicator state and set the opposite value on press |
| momentary | set on at press and off at release |

The converter does not use `TargetVariable` here because the built-in button
write would bypass legacy deferred ordering.

## Deferred Set operations

The legacy engine executed ordinary draw-tree `Set` nodes immediately, but
queued sets reached through `OnPress` or `OnRelease` until event handling
finished. Current dcapp normally executes all sets during the draw pass.

`Defer="true"` restores the legacy event behavior:

```xml
<Set Variable="mode" Operator="#_set_equal_" Defer="true">2</Set>
```

The converter adds it to every `Set` below `MousePressed` or `MouseReleased`,
including nested conditionals and generated clamp operations. Deferred sets are
flushed in encounter order after the draw tree; multiple writes to one variable
therefore preserve their legacy order. Sets in button state blocks and other
draw-tree locations remain immediate.

Do not add `Defer` to new displays unless the old frame-ordering behavior is
actually required.

## Other element conversions

### Blink

| Legacy | Current |
|--------|---------|
| `FnStartBlink="@trigger"` | `FireBlink="@trigger"` |
| `Duration="-1"` | `Duration="0"` |

Zero duration means indefinite blinking.

### PixelStream

`Host`, `Port`, and `Path` are combined into an HTTP `URL`. Protocol names map
as follows:

| Legacy protocol/type | Current type |
|----------------------|--------------|
| `MJPEG` | `#_pixelstream_mjpeg_` |
| `DFILE`, `DynamicFile`, `dynamic_file`, `shmem` | `#_pixelstream_shmem_` |

An unsupported literal protocol becomes a TODO comment. Constant- and
variable-backed types are preserved for runtime resolution.

### Logic files

The generated interface supports both C and C++. Include `dcapp.h` before the
callback definitions so C++ builds inherit its C linkage and export
annotations. The lifecycle signatures are:

```c
void display_init(DcAppContext *app_ctx, void **user_data);
void display_draw(DcAppContext *app_ctx, void *user_data);
void display_close(DcAppContext *app_ctx, void *user_data);
```

Additional translation units define `DCAPP_LOGIC_EXTERN` before including
`dcapp.h`. The legacy macro was `_DCAPP_EXTERNALS_`.

Generated string variables are fixed 256-byte arrays rather than
`std::string`:

```c
snprintf(*CURRENT_TIME, 256, "%s", source);
```

Regenerate `logic/dcapp.h` and rebuild the shared library after converting the
XML.

## Unsupported legacy fields

The converter prefixes known, unsupported attributes with `_` so their values
survive for review:

| Attribute | Legacy use |
|-----------|------------|
| `UpdateRate` | throttled text refresh |
| `Pattern`, `LinePattern`, `LineFactor` | legacy line patterns |
| `Key`, `KeyASCII`, `BezelKey` | keyboard or bezel bindings |
| `ForceMono`, `Face`, `ZeroTrim` | legacy text formatting |
| `DisconnectAction` | Trick disconnect behavior |
| `FullScreen` | window fullscreen mode |
| `Camera` | PixelStream/terrain camera binding |

Some names in that list now exist in other current contexts; the underscore is
about the legacy attribute handled by the converter, not a blanket statement
about the name.

These elements are replaced with TODO comments:

| Element | Review note |
|---------|-------------|
| `ADI` | use `<Sphere Image="...">`; `samples/adi` shows the current approach |
| `Animation` | no direct replacement |
| `CAN`, `UEI` | legacy hardware integration is unavailable |
| `Hagstrom`, `KeyboardEvent` | legacy keyboard integration is unavailable |
| `Map` | use current terrain/planet facilities |

After conversion, search for `TODO(` and attributes beginning with `_`. Then
validate the resulting XML and compare its alignment and event timing with the
legacy display.
