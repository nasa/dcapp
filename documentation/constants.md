# Constants

Constants are named values resolved while the display loads. The preprocessor
substitutes the value before the runtime display is built, so a constant is not
a live binding.

Constants are referenced using the `#` prefix:

```xml
<Text LocalAlignX="#_align_center_">Centered Text</Text>
<Button Type="#_button_toggle_" .../>
<If Operator="#_if_gt_" .../>
```

Use a [variable](variables.md) for a value that must change after the display
starts.

## References

```xml
<Text LocalAlignX="#_align_center_"/>
```

Use braces when constants are adjacent to other text or nested:

```xml
<Rectangle FillColor="#{myRed},0,0,1"/>
```

Use `$` to reference environment variables:

```xml
<Include>$DCAPP_DISPLAY_HOME/includes/header.xml</Include>
```

dcapp exports two path variables before preprocessing a display:

- `DCAPP_HOME` is the dcapp installation directory.
- `DCAPP_DISPLAY_HOME` is the directory containing the main XML file.

The legacy `dcappHome` and `dcappDisplayHome` names remain available as aliases.

Use backslash to include literal `#` or `$` characters:

```xml
<Text>Cost: \$100</Text>
<Text>Issue \#42</Text>
```

## User-Defined Constants

Define your own constants using the `<Constant>` element:

```xml
<DCAPP>
    <Constant Name="mainColor">0.2,0.5,0.8,1</Constant>
    <Constant Name="buttonWidth">120</Constant>
    <Constant Name="appTitle">Flight Display</Constant>
    
    <Window Title="#appTitle" Width="800" Height="600">
        <Rectangle FillColor="#mainColor" Width="#buttonWidth" Height="40"/>
    </Window>
</DCAPP>
```

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `Name` | string | **Yes** | Constant name (used with `#` prefix) |
| `Immutable` | boolean | No | If `true`, cannot be overridden by command-line args (default: `false`) |

**Content:** The constant value

Mark constants as immutable to prevent command-line overrides:

```xml
<Constant Name="version" Immutable="true">1.0.0</Constant>
```

## Command-Line Constant Overrides

Constants can be set or overridden from the command line when launching dcapp:

```bash
./bin/dcapp.sh myDisplay.xml buttonColor="1,0,0,1" serverHost="192.168.1.100"
```

Constants marked `Immutable="true"` cannot be overridden this way.

## Recursive references

Constants are resolved recursively, so they can reference each other:

```xml
<Constant Name="baseRed">0.8</Constant>
<Constant Name="alertColor">#baseRed,0.1,0.1,1</Constant>

<Rectangle FillColor="#alertColor"/>
<!-- Resolves to: FillColor="0.8,0.1,0.1,1" -->
```

## Built-in Constants

These constants are available without a `Constant` declaration.

### Alignment Constants

Used with `LocalAlignX`, `ParentAlignX`, `PivotLocalAlignX` (horizontal) and `LocalAlignY`, `ParentAlignY`, `PivotLocalAlignY` (vertical). See [Positioning and Alignment](primitives.md#positioning-and-alignment) for details.

Horizontal:

| Constant | Value |
|----------|-------|
| `#_align_left_` | 1 (default) |
| `#_align_center_` | 2 |
| `#_align_right_` | 3 |

Vertical:

| Constant | Value |
|----------|-------|
| `#_align_bottom_` | 4 (default) |
| `#_align_middle_` | 5 |
| `#_align_top_` | 6 |

### Button Type Constants

See [Buttons](buttons.md) for full usage details.

| Constant | Value | Description |
|----------|-------|-------------|
| `#_button_momentary_` | 1 | "On" while pressed, "off" when released |
| `#_button_standard_` | 2 | Sets target to "on" value when clicked (default) |
| `#_button_toggle_` | 3 | Alternates between "on" and "off" each click |

### Conditional Constants

Used with the `<If>` element's `Operator` attribute. See [`If`](primitives.md#if)
for details.

| Constant | Value | Description |
|----------|-------|-------------|
| `#_if_true_` | 1 | Check if value is truthy |
| `#_if_false_` | 2 | Check if value is falsy |
| `#_if_eq_` | 3 | Equal to |
| `#_if_ne_` | 4 | Not equal to |
| `#_if_lt_` | 5 | Less than |
| `#_if_gt_` | 6 | Greater than |
| `#_if_lte_` | 7 | Less than or equal |
| `#_if_gte_` | 8 | Greater than or equal |

### Set Operator Constants

Used with the `<Set>` element's `Operator` attribute. See [Set Operators](variables.md#set-operators) for the full list including math functions.

| Constant | Value | Description |
|----------|-------|-------------|
| `#_set_equal_` | 1 | Direct assignment (default) |
| `#_set_add_` | 2 | Add to current value |
| `#_set_subtract_` | 3 | Subtract from current value |
| `#_set_multiply_` | 4 | Multiply current value |
| `#_set_divide_` | 5 | Divide current value |

### PixelStream Type Constants

See [PixelStream](pixelstream.md) for usage details.

| Constant | Value | Description |
|----------|-------|-------------|
| `#_pixelstream_shmem_` | 1 | Shared memory source |
| `#_pixelstream_mjpeg_` | 2 | MJPEG network stream |

### Planet Constants

| Constant | Value |
|----------|-------|
| `#_planet_crs_geodetic_` | 1 |
| `#_planet_crs_cartesian_` | 2 |
| `#_planet_attitude_frame_local_ned_` | 1 |
| `#_planet_attitude_frame_cartesian_rpy_` | 2 |

### Stencil Constant

| Constant | Value |
|----------|-------|
| `#_stencil_color_` | `0 0 0 1` |

## Built-in Color Constants

Color constants contain space-separated RGB values from 0.0 to 1.0. Append an
alpha component when using one as a color attribute.

### Reds & Pinks

| Constant | RGB Value |
|----------|-----------|
| `#_color_red_` | 1.0, 0.0, 0.0 |
| `#_color_crimson_` | 0.86, 0.08, 0.24 |
| `#_color_maroon_` | 0.5, 0.0, 0.0 |
| `#_color_burgundy_` | 0.6, 0.0, 0.13 |
| `#_color_ruby_` | 0.88, 0.07, 0.37 |
| `#_color_cherry_` | 0.87, 0.19, 0.39 |
| `#_color_rose_` | 1.0, 0.0, 0.5 |
| `#_color_pink_` | 1.0, 0.75, 0.8 |
| `#_color_salmon_` | 0.98, 0.5, 0.45 |
| `#_color_coral_` | 1.0, 0.5, 0.31 |
| `#_color_peach_` | 1.0, 0.85, 0.73 |
| `#_color_fuchsia_` | 1.0, 0.0, 1.0 |
| `#_color_hot_pink_` | 1.0, 0.41, 0.71 |
| `#_color_light_pink_` | 1.0, 0.71, 0.76 |
| `#_color_mulberry_` | 0.77, 0.29, 0.55 |
| `#_color_scarlet_` | 1.0, 0.14, 0.0 |
| `#_color_tomato_` | 1.0, 0.39, 0.28 |
| `#_color_wine_` | 0.45, 0.18, 0.22 |
| `#_color_raspberry_` | 0.89, 0.04, 0.36 |
| `#_color_dark_red_` | 0.55, 0.0, 0.0 |

### Oranges

| Constant | RGB Value |
|----------|-----------|
| `#_color_orange_` | 1.0, 0.5, 0.0 |
| `#_color_tangerine_` | 1.0, 0.6, 0.0 |
| `#_color_pumpkin_` | 1.0, 0.46, 0.1 |
| `#_color_apricot_` | 0.98, 0.81, 0.69 |
| `#_color_cantaloupe_` | 1.0, 0.71, 0.55 |
| `#_color_amber_` | 1.0, 0.75, 0.0 |
| `#_color_burnt_orange_` | 0.8, 0.33, 0.0 |
| `#_color_rust_` | 0.72, 0.25, 0.05 |
| `#_color_terracotta_` | 0.89, 0.45, 0.36 |
| `#_color_dark_orange_` | 1.0, 0.55, 0.0 |
| `#_color_mango_` | 1.0, 0.51, 0.26 |
| `#_color_persimmon_` | 0.93, 0.35, 0.0 |

### Yellows

| Constant | RGB Value |
|----------|-----------|
| `#_color_yellow_` | 1.0, 1.0, 0.0 |
| `#_color_lemon_` | 1.0, 1.0, 0.31 |
| `#_color_mustard_` | 1.0, 0.86, 0.35 |
| `#_color_gold_` | 1.0, 0.84, 0.0 |
| `#_color_butter_` | 1.0, 0.94, 0.75 |
| `#_color_champagne_` | 0.97, 0.91, 0.81 |
| `#_color_sunflower_` | 1.0, 0.8, 0.0 |
| `#_color_flax_` | 0.93, 0.87, 0.51 |
| `#_color_cream_` | 1.0, 0.99, 0.82 |
| `#_color_ivory_` | 1.0, 1.0, 0.94 |
| `#_color_saffron_` | 0.96, 0.77, 0.19 |
| `#_color_golden_rod_` | 0.85, 0.65, 0.13 |
| `#_color_canary_` | 1.0, 0.94, 0.0 |

### Greens

| Constant | RGB Value |
|----------|-----------|
| `#_color_green_` | 0.0, 1.0, 0.0 |
| `#_color_lime_` | 0.75, 1.0, 0.0 |
| `#_color_olive_` | 0.5, 0.5, 0.0 |
| `#_color_moss_` | 0.53, 0.6, 0.42 |
| `#_color_forest_green_` | 0.13, 0.55, 0.13 |
| `#_color_emerald_` | 0.31, 0.78, 0.47 |
| `#_color_jade_` | 0.0, 0.66, 0.42 |
| `#_color_mint_` | 0.74, 0.99, 0.79 |
| `#_color_pistachio_` | 0.58, 0.77, 0.45 |
| `#_color_seafoam_` | 0.62, 0.89, 0.76 |
| `#_color_chartreuse_` | 0.5, 1.0, 0.0 |
| `#_color_dark_green_` | 0.0, 0.39, 0.0 |
| `#_color_sage_` | 0.72, 0.72, 0.59 |
| `#_color_spring_green_` | 0.0, 1.0, 0.5 |
| `#_color_hunter_green_` | 0.21, 0.37, 0.23 |
| `#_color_kelly_green_` | 0.3, 0.73, 0.09 |
| `#_color_pine_` | 0.06, 0.32, 0.21 |
| `#_color_fern_` | 0.44, 0.64, 0.26 |
| `#_color_neon_green_` | 0.22, 1.0, 0.08 |

### Blues

| Constant | RGB Value |
|----------|-----------|
| `#_color_blue_` | 0.0, 0.0, 1.0 |
| `#_color_navy_` | 0.0, 0.0, 0.5 |
| `#_color_sky_blue_` | 0.53, 0.81, 0.92 |
| `#_color_baby_blue_` | 0.87, 0.92, 1.0 |
| `#_color_azure_` | 0.0, 0.5, 1.0 |
| `#_color_denim_` | 0.08, 0.38, 0.65 |
| `#_color_sapphire_` | 0.08, 0.15, 0.39 |
| `#_color_steel_blue_` | 0.27, 0.51, 0.71 |
| `#_color_powder_blue_` | 0.69, 0.88, 0.9 |
| `#_color_cerulean_` | 0.0, 0.48, 0.65 |
| `#_color_teal_` | 0.0, 0.5, 0.5 |
| `#_color_royal_blue_` | 0.25, 0.41, 0.88 |
| `#_color_midnight_blue_` | 0.1, 0.1, 0.44 |
| `#_color_cobalt_` | 0.0, 0.28, 0.67 |
| `#_color_cornflower_blue_` | 0.39, 0.58, 0.93 |
| `#_color_turquoise_` | 0.25, 0.88, 0.82 |
| `#_color_cyan_` | 0.0, 1.0, 1.0 |
| `#_color_aquamarine_` | 0.5, 1.0, 0.83 |
| `#_color_electric_blue_` | 0.49, 0.98, 1.0 |
| `#_color_periwinkle_` | 0.8, 0.8, 1.0 |

### Purples & Violets

| Constant | RGB Value |
|----------|-----------|
| `#_color_purple_` | 0.5, 0.0, 0.5 |
| `#_color_indigo_` | 0.29, 0.0, 0.51 |
| `#_color_lavender_` | 0.9, 0.9, 0.98 |
| `#_color_plum_` | 0.56, 0.27, 0.52 |
| `#_color_violet_` | 0.93, 0.51, 0.93 |
| `#_color_amethyst_` | 0.6, 0.4, 0.8 |
| `#_color_orchid_` | 0.85, 0.44, 0.84 |
| `#_color_thistle_` | 0.85, 0.75, 0.85 |
| `#_color_eggplant_` | 0.38, 0.25, 0.32 |
| `#_color_magenta_` | 0.8, 0.0, 0.8 |
| `#_color_mauve_` | 0.88, 0.69, 1.0 |
| `#_color_lilac_` | 0.78, 0.64, 0.78 |
| `#_color_grape_` | 0.44, 0.18, 0.66 |
| `#_color_royal_purple_` | 0.47, 0.32, 0.66 |

### Browns

| Constant | RGB Value |
|----------|-----------|
| `#_color_brown_` | 0.6, 0.4, 0.2 |
| `#_color_chocolate_` | 0.82, 0.41, 0.12 |
| `#_color_saddle_brown_` | 0.55, 0.27, 0.07 |
| `#_color_umber_` | 0.39, 0.32, 0.28 |
| `#_color_mahogany_` | 0.65, 0.19, 0.19 |
| `#_color_copper_` | 0.72, 0.45, 0.2 |
| `#_color_tan_` | 0.82, 0.71, 0.55 |
| `#_color_walnut_` | 0.39, 0.26, 0.13 |
| `#_color_espresso_` | 0.36, 0.25, 0.2 |
| `#_color_caramel_` | 0.87, 0.58, 0.36 |
| `#_color_mocha_` | 0.44, 0.31, 0.22 |
| `#_color_pecan_` | 0.78, 0.52, 0.25 |
| `#_color_wood_` | 0.76, 0.6, 0.42 |
| `#_color_bronze_` | 0.8, 0.5, 0.2 |
| `#_color_russet_` | 0.5, 0.27, 0.23 |
| `#_color_sienna_` | 0.63, 0.32, 0.18 |
| `#_color_cinnamon_` | 0.69, 0.4, 0.24 |
| `#_color_sandy_brown_` | 0.96, 0.64, 0.38 |

### Neutrals & Grays

| Constant | RGB Value |
|----------|-----------|
| `#_color_white_` | 1.0, 1.0, 1.0 |
| `#_color_black_` | 0.0, 0.0, 0.0 |
| `#_color_gray_` | 0.5, 0.5, 0.5 |
| `#_color_light_gray_` | 0.83, 0.83, 0.83 |
| `#_color_dark_gray_` | 0.33, 0.33, 0.33 |
| `#_color_charcoal_` | 0.21, 0.27, 0.31 |
| `#_color_silver_` | 0.75, 0.75, 0.75 |
| `#_color_ash_` | 0.7, 0.75, 0.71 |
| `#_color_slate_` | 0.44, 0.5, 0.56 |
| `#_color_eggshell_` | 0.94, 0.92, 0.84 |
| `#_color_alabaster_` | 0.98, 0.98, 0.95 |
| `#_color_beige_` | 0.96, 0.96, 0.86 |
| `#_color_khaki_` | 0.76, 0.69, 0.57 |
| `#_color_sand_` | 0.94, 0.87, 0.73 |
| `#_color_taupe_` | 0.56, 0.52, 0.51 |
| `#_color_snow_` | 1.0, 0.98, 0.98 |
| `#_color_pearl_` | 0.94, 0.92, 0.88 |
| `#_color_smoke_` | 0.96, 0.96, 0.96 |
| `#_color_bone_` | 0.89, 0.85, 0.79 |
| `#_color_graphite_` | 0.29, 0.29, 0.29 |
| `#_color_iron_` | 0.32, 0.34, 0.36 |
| `#_color_steel_` | 0.5, 0.5, 0.55 |

### Using color constants

```xml
<!-- Add alpha value (1 = opaque) -->
<Rectangle FillColor="#_color_red_ 1"/>

<!-- Or define a custom constant with alpha -->
<Constant Name="alertRed">#_color_red_ 1</Constant>
<Rectangle FillColor="#alertRed"/>
```

Built-in color constants are not immutable. They can be overridden from the
command line or by a user-defined constant with the same name.
