# dcapp Constants

A guide to using built-in and user-defined constants in dcapp displays.

---

## Overview

Constants provide named, fixed values that make XML more readable and maintainable. They are resolved at load time — the constant name is substituted with its value before the display runs.

Constants are referenced using the `#` prefix:

```xml
<Text LocalAlignX="#_align_center_">Centered Text</Text>
<Button Type="#_button_toggle_" .../>
<If Operation="#_conditional_gt_" .../>
```

---

## Constant Syntax

### Basic Reference

```xml
<Text LocalAlignX="#_align_center_"/>
```

### Braced Reference

Use braces when constants are adjacent to other text or nested:

```xml
<Rectangle FillColor="#{myRed},0,0,1"/>
```

### Environment Variables

Use `$` to reference environment variables:

```xml
<Include>$dcappDisplayHome/includes/header.xml</Include>
```

The `dcappDisplayHome` environment variable is automatically set to the directory containing your main XML file.

### Escaping

Use backslash to include literal `#` or `$` characters:

```xml
<Text>Cost: \$100</Text>
<Text>Issue \#42</Text>
```

---

## User-Defined Constants

Define your own constants using the `<Constant>` element:

```xml
<dcapp>
    <Constant Name="mainColor">0.2,0.5,0.8,1</Constant>
    <Constant Name="buttonWidth">120</Constant>
    <Constant Name="appTitle">Flight Display</Constant>
    
    <Window Title="#appTitle" Width="800" Height="600">
        <Rectangle FillColor="#mainColor" Width="#buttonWidth" Height="40"/>
    </Window>
</dcapp>
```

### Constant Element Attributes

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `Name` | string | **Yes** | Constant name (used with `#` prefix) |
| `Immutable` | boolean | No | If `true`, cannot be overridden by command-line args (default: `false`) |

**Content:** The constant value

### Immutable Constants

Mark constants as immutable to prevent command-line overrides:

```xml
<Constant Name="version" Immutable="true">1.0.0</Constant>
```

---

## Command-Line Constant Overrides

Constants can be set or overridden from the command line when launching dcapp:

```bash
dcapp myDisplay.xml buttonColor="1,0,0,1" serverHost="192.168.1.100"
```

This allows the same display XML to be configured differently at runtime.

**Note:** Constants marked with `Immutable="true"` cannot be overridden from the command line.

---

## Constants Can Reference Other Constants

Constants are resolved recursively, so they can reference each other:

```xml
<Constant Name="baseRed">0.8</Constant>
<Constant Name="alertColor">#baseRed,0.1,0.1,1</Constant>

<Rectangle FillColor="#alertColor"/>
<!-- Resolves to: FillColor="0.8,0.1,0.1,1" -->
```

---

## Built-in Constants

dcapp provides many predefined constants for common values.

### Alignment Constants

**Horizontal Alignment** (for `LocalAlignX`, `ParentAlignX`, `PivotLocalAlignX`):

| Constant | Value | Description |
|----------|-------|-------------|
| `#_align_left_` | 0 | Left edge (default) |
| `#_align_center_` | 1 | Horizontal center |
| `#_align_right_` | 2 | Right edge |

**Vertical Alignment** (for `LocalAlignY`, `ParentAlignY`, `PivotLocalAlignY`):

| Constant | Value | Description |
|----------|-------|-------------|
| `#_align_bottom_` | 0 | Bottom edge (default) |
| `#_align_middle_` | 1 | Vertical center |
| `#_align_top_` | 2 | Top edge |

**Example:**
```xml
<Text X="400" Y="300" 
      LocalAlignX="#_align_center_" 
      LocalAlignY="#_align_middle_">
    Centered Text
</Text>
```

### Button Type Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `#_button_standard_` | 0 | Sets target to "on" value when clicked (default) |
| `#_button_momentary_` | 1 | "On" while pressed, "off" when released |
| `#_button_toggle_` | 2 | Alternates between "on" and "off" each click |

**Example:**
```xml
<Button Type="#_button_toggle_" Variable="power" On="1" Off="0">
    ...
</Button>
```

### Conditional Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `#_conditional_true_` | 0 | Check if value is truthy |
| `#_conditional_false_` | 1 | Check if value is falsy |
| `#_conditional_eq_` | 2 | Equal to |
| `#_conditional_ne_` | 3 | Not equal to |
| `#_conditional_lt_` | 4 | Less than |
| `#_conditional_gt_` | 5 | Greater than |
| `#_conditional_lte_` | 6 | Less than or equal |
| `#_conditional_gte_` | 7 | Greater than or equal |

**Example:**
```xml
<If Value="@altitude" Value2="10000" Operation="#_conditional_gt_">
    <True>
        <Text FillColor="1,0,0,1">HIGH ALTITUDE WARNING</Text>
    </True>
</If>
```

### Set Operator Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `#_set_equal_` | 0 | Direct assignment (default) |
| `#_set_add_` | 1 | Add to current value |
| `#_set_subtract_` | 2 | Subtract from current value |
| `#_set_multiply_` | 3 | Multiply current value |
| `#_set_divide_` | 4 | Divide current value |

**Example:**
```xml
<Set Variable="score" Operator="#_set_add_">100</Set>
```

### Pixelstream Type Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `#_pixelstream_dynamic_file_` | 0 | Dynamic file source |
| `#_pixelstream_mjpeg_` | 1 | MJPEG network stream |

**Example:**
```xml
<Pixelstream Type="#_pixelstream_mjpeg_" URL="http://camera:8080/stream" 
             X="100" Y="100" Width="640" Height="480"/>
```

---

## Built-in Color Constants

dcapp includes a comprehensive palette of named colors. Color constants use RGB format with values from 0.0 to 1.0 (no alpha - add your own alpha value).

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

### Using Color Constants

Color constants use space-separated RGB values. Add alpha when using:

```xml
<!-- Add alpha value (1 = opaque) -->
<Rectangle FillColor="#_color_red_ 1"/>

<!-- Or define a custom constant with alpha -->
<Constant Name="alertRed">#_color_red_ 1</Constant>
<Rectangle FillColor="#alertRed"/>
```

**Note:** Color constants are *not* immutable by default, so they can be overridden from the command line or by defining your own constant with the same name.

---

## Complete Built-in Constants Reference

| Category | Constant | Value |
|----------|----------|-------|
| **Alignment (H)** | `#_align_left_` | 0 |
| | `#_align_center_` | 1 |
| | `#_align_right_` | 2 |
| **Alignment (V)** | `#_align_bottom_` | 0 |
| | `#_align_middle_` | 1 |
| | `#_align_top_` | 2 |
| **Button Type** | `#_button_standard_` | 0 |
| | `#_button_momentary_` | 1 |
| | `#_button_toggle_` | 2 |
| **Conditional** | `#_conditional_true_` | 0 |
| | `#_conditional_false_` | 1 |
| | `#_conditional_eq_` | 2 |
| | `#_conditional_ne_` | 3 |
| | `#_conditional_lt_` | 4 |
| | `#_conditional_gt_` | 5 |
| | `#_conditional_lte_` | 6 |
| | `#_conditional_gte_` | 7 |
| **Set Operation** | `#_set_equal_` | 0 |
| | `#_set_add_` | 1 |
| | `#_set_subtract_` | 2 |
| | `#_set_multiply_` | 3 |
| | `#_set_divide_` | 4 |
| **Pixelstream** | `#_pixelstream_dynamic_file_` | 0 |
| | `#_pixelstream_mjpeg_` | 1 |

---

## Examples

### Themed Display with Constants

```xml
<dcapp>
    <!-- Theme colors -->
    <Constant Name="bgColor">0.1,0.1,0.15,1</Constant>
    <Constant Name="primaryColor">0.2,0.6,0.9,1</Constant>
    <Constant Name="dangerColor">0.9,0.2,0.2,1</Constant>
    <Constant Name="successColor">0.2,0.8,0.3,1</Constant>
    
    <!-- Layout -->
    <Constant Name="margin">20</Constant>
    <Constant Name="buttonWidth">100</Constant>
    <Constant Name="buttonHeight">40</Constant>
    
    <Window Title="Themed Display" Width="800" Height="600">
        <Rectangle FillColor="#bgColor" Width="800" Height="600"/>
        
        <Rectangle X="#margin" Y="#margin" 
                   Width="#buttonWidth" Height="#buttonHeight"
                   FillColor="#primaryColor"/>
    </Window>
</dcapp>
```

### Configurable Display

```xml
<dcapp>
    <!-- These can be overridden from command line -->
    <Constant Name="serverHost">localhost</Constant>
    <Constant Name="serverPort">7000</Constant>
    <Constant Name="updateRate">0.1</Constant>
    
    <!-- This cannot be overridden -->
    <Constant Name="version" Immutable="true">2.1.0</Constant>
    
    <Variable Type="#_variable_double_" InitialValue="0">altitude</Variable>
    
    <TrickIO Host="#serverHost" Port="#serverPort" DataRate="#updateRate">
        <FromTrick>
            <TrickVariable Name="vehicle.altitude">altitude</TrickVariable>
        </FromTrick>
    </TrickIO>
    
    <Window Title="Display v#version" Width="800" Height="600">
        <Text X="10" Y="10" Size="12" FillColor="0.5,0.5,0.5,1">
            Connected to #serverHost:#serverPort
        </Text>
    </Window>
</dcapp>
```

Run with custom server:
```bash
dcapp display.xml serverHost="192.168.1.50" serverPort="8000"
```

---

## Tips & Best Practices

1. **Use constants for repeated values** — Colors, sizes, and spacing used multiple times should be constants.

2. **Use meaningful names** — `#headerHeight` is clearer than `#h1`.

3. **Group related constants** — Put theme colors together, layout values together, etc.

4. **Use built-in constants** — `Operation="#_conditional_gt_"` is more readable than `Operation="5"`.

5. **Mark version/config constants as immutable** — Prevent accidental command-line overrides.

6. **Leverage command-line overrides for deployment** — Same XML can work in different environments.

7. **Use environment variables for paths** — `$dcappDisplayHome` makes includes portable.
