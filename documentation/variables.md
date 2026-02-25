# dcapp Variables

A guide to declaring and using variables in dcapp displays.

---

## Overview

Variables store runtime values that can change during display execution. They enable dynamic content, user interaction, and data binding with external systems.

```xml
<Variable Type="#_variable_double_" InitialValue="0">altitude</Variable>
```

Variables are referenced using the `@` prefix:

```xml
<Text>Altitude: @altitude ft</Text>
```

---

## Declaring Variables

Variables are declared at the top of your dcapp file, before the `<Window>` element:

```xml
<dcapp>
    <Variable Type="#_variable_double_" InitialValue="0">altitude</Variable>
    <Variable Type="#_variable_double_" InitialValue="0">speed</Variable>
    <Variable Type="#_variable_string_" InitialValue="OFF">engineStatus</Variable>
    
    <Window Title="My Display" Width="800" Height="600">
        <!-- Display content here -->
    </Window>
</dcapp>
```

### Variable Element Attributes

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `Type` | string | No | Data type (default: `String`) |
| `InitialValue` | string | No | Starting value (default: empty) |

**Content:** The variable name (used for referencing with `@`)

---

## Variable Types

| Type Constant | Description | Example Values |
|---------------|-------------|----------------|
| `#_variable_string_` | Text data | `"ON"`, `"Hello"`, `"123"` |
| `#_variable_integer_` | Whole numbers | `0`, `42`, `-17` |
| `#_variable_double_` | Decimal numbers | `0.0`, `3.14159`, `-273.15` |
| `#_variable_boolean_` | True/false values | `true`, `false`, `1`, `0` |

```xml
<Variable Type="#_variable_string_" InitialValue="STANDBY">status</Variable>
<Variable Type="#_variable_integer_" InitialValue="100">health</Variable>
<Variable Type="#_variable_double_" InitialValue="0.0">temperature</Variable>
<Variable Type="#_variable_boolean_" InitialValue="false">isActive</Variable>
```

---

## Referencing Variables

### In Attributes

Use the `@` prefix to reference a variable's value in any numeric attribute:

```xml
<Variable Type="#_variable_double_" InitialValue="100">xPos</Variable>
<Variable Type="#_variable_double_" InitialValue="50">yPos</Variable>
<Variable Type="#_variable_double_" InitialValue="45">angle</Variable>

<Rectangle X="@xPos" Y="@yPos" Width="50" Height="50" Rotation="@angle"/>
```

### In Text Content

Variables can be interpolated directly into text:

```xml
<Variable Type="#_variable_string_" InitialValue="World">name</Variable>
<Text>Hello, @name!</Text>
<!-- Output: Hello, World! -->
```

### Braced Syntax

Use braces when the variable name is adjacent to other text:

```xml
<Variable Type="#_variable_double_" InitialValue="75">temp</Variable>
<Text>Temperature: @{temp}°F</Text>
<!-- Output: Temperature: 75°F -->
```

Without braces, `@temp°F` would look for a variable named `temp°F`.

---

## Format Specifiers

Use printf-style format specifiers for number formatting in text:

```xml
<Variable Type="#_variable_double_" InitialValue="1234.5678">value</Variable>

<Text>Default: @value</Text>           <!-- 1234.5678 -->
<Text>Integer: @value(%.0f)</Text>     <!-- 1235 -->
<Text>2 decimals: @value(%.2f)</Text>  <!-- 1234.57 -->
<Text>Padded: @value(%08.2f)</Text>    <!-- 01234.57 -->
<Text>Signed: @value(%+.1f)</Text>     <!-- +1234.6 -->
```

### Common Format Specifiers

| Specifier | Description | Input | Output |
|-----------|-------------|-------|--------|
| `%.0f` | No decimals | `123.456` | `123` |
| `%.1f` | One decimal | `123.456` | `123.5` |
| `%.2f` | Two decimals | `123.456` | `123.46` |
| `%05.1f` | Padded to 5 chars | `12.3` | `012.3` |
| `%+.1f` | Always show sign | `12.3` | `+12.3` |
| `%03d` | Zero-padded integer | `7` | `007` |
| `%e` | Scientific notation | `1234.5` | `1.234500e+03` |

---

## Modifying Variables

### The `<Set>` Element

Use `<Set>` to modify variable values at runtime:

```xml
<Set Variable="counter">0</Set>
```

### Set Attributes

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `Variable` | string | **Yes** | Name of variable to modify |
| `Operator` | integer | No | Operation type (default: assignment) |

**Content:** The value or expression to apply

### Set Operators

| Constant | Value | Description | Effect |
|----------|-------|-------------|--------|
| `#_set_equal_` | 0 | Direct assignment (default) | `var = value` |
| `#_set_add_` | 1 | Addition | `var = var + value` |
| `#_set_subtract_` | 2 | Subtraction | `var = var - value` |
| `#_set_multiply_` | 3 | Multiplication | `var = var * value` |
| `#_set_divide_` | 4 | Division | `var = var / value` |

### Examples

```xml
<!-- Direct assignment -->
<Set Variable="status">ACTIVE</Set>

<!-- Increment by 1 -->
<Set Variable="counter" Operator="#_set_add_">1</Set>

<!-- Decrease health by 10 -->
<Set Variable="health" Operator="#_set_subtract_">10</Set>

<!-- Double the score -->
<Set Variable="score" Operator="#_set_multiply_">2</Set>

<!-- Halve the speed -->
<Set Variable="speed" Operator="#_set_divide_">2</Set>
```

### Using Set in Event Handlers

`<Set>` is commonly used inside button press/release handlers:

```xml
<Variable Type="#_variable_double_" InitialValue="50">volume</Variable>

<Button X="100" Y="100" Width="40" Height="40">
    <Pressed>
        <Set Variable="volume" Operator="#_set_add_">5</Set>
    </Pressed>
    <Enabled>
        <Off><Rectangle FillColor="0.3,0.5,0.3,1" Width="40" Height="40"/></Off>
        <On><Rectangle FillColor="0.4,0.7,0.4,1" Width="40" Height="40"/></On>
    </Enabled>
</Button>
```

---

## Escape Sequences in Text

| Sequence | Result |
|----------|--------|
| `\n` | Newline |
| `\t` | Tab |
| `\\` | Backslash |
| `\@` | Literal @ symbol |
| `\"` | Double quote |
| `\'` | Single quote |
| `\#` | Literal # symbol |
| `\$` | Literal $ symbol |

```xml
<Text>Line 1\nLine 2</Text>
<!-- Output:
Line 1
Line 2
-->

<Text>Email: user\@example.com</Text>
<!-- Output: Email: user@example.com -->

<Text>Cost: \$100</Text>
<!-- Output: Cost: $100 -->
```

---

## Conditionals with Variables

Use `<If>` to show/hide content based on variable values:

```xml
<Variable Type="#_variable_double_" InitialValue="100">fuel</Variable>

<If Value="@fuel" Value2="20" Operator="#_if_lt_">
    <True>
        <Text FillColor="1,0,0,1">LOW FUEL WARNING</Text>
    </True>
</If>
```

### Conditional Operations

| Constant | Description |
|----------|-------------|
| `#_if_true_` | Check if value is truthy (non-zero, non-empty) |
| `#_if_false_` | Check if value is falsy |
| `#_if_eq_` | Equal to |
| `#_if_ne_` | Not equal to |
| `#_if_lt_` | Less than |
| `#_if_gt_` | Greater than |
| `#_if_lte_` | Less than or equal |
| `#_if_gte_` | Greater than or equal |

See the [Constants documentation](dcapp_constants.md) for the complete list.

---

## External Data Integration

### TrickIO

Variables can be bound to Trick simulation variables for real-time data exchange:

```xml
<TrickIO Host="localhost" Port="7000" DataRate="0.1">
    <TrickFrom>
        <TrickVariable Name="rocket.altitude" Units="ft">altitude</TrickVariable>
        <TrickVariable Name="rocket.velocity" Units="fps">velocity</TrickVariable>
    </TrickFrom>
    <TrickTo>
        <TrickVariable Name="rocket.throttle">throttleCommand</TrickVariable>
    </TrickTo>
</TrickIO>
```

### Logic Files

For complex variable manipulation beyond what XML can express, you can use external C/C++ logic files. Logic files receive direct pointers to all declared variables, allowing arbitrary computation each frame.

**See the [Logic Files documentation](dcapp_logic_files.md) for details on:**
- Setting up logic files
- The generated `dcapp.h` header
- Accessing variables from C/C++ code
- The `display_init()`, `display_draw()`, and `display_close()` callbacks

---

## Examples

### Flight Instruments Display

```xml
<dcapp>
    <Variable Type="#_variable_double_" InitialValue="0">altitude</Variable>
    <Variable Type="#_variable_double_" InitialValue="0">speed</Variable>
    <Variable Type="#_variable_double_" InitialValue="0">heading</Variable>
    <Variable Type="#_variable_string_" InitialValue="NORMAL">flightMode</Variable>

    <Window Title="Flight Display" Width="400" Height="300">
        <Rectangle FillColor="0.1,0.1,0.15,1" Width="400" Height="300"/>
        
        <Text X="20" Y="250" Size="12" FillColor="0.6,0.6,0.6,1">ALT</Text>
        <Text X="20" Y="220" Size="28" FillColor="0,1,0,1">@altitude(%.0f) ft</Text>
        
        <Text X="20" Y="170" Size="12" FillColor="0.6,0.6,0.6,1">SPD</Text>
        <Text X="20" Y="140" Size="28" FillColor="0,1,0,1">@speed(%.1f) kts</Text>
        
        <Text X="20" Y="90" Size="12" FillColor="0.6,0.6,0.6,1">HDG</Text>
        <Text X="20" Y="60" Size="28" FillColor="0,1,0,1">@heading(%03.0f)°</Text>
        
        <Text X="20" Y="20" Size="16" FillColor="0,0.8,1,1">@flightMode</Text>
    </Window>
</dcapp>
```

### Counter with Buttons

```xml
<dcapp>
    <Variable Type="#_variable_integer_" InitialValue="0">counter</Variable>

    <Window Title="Counter" Width="300" Height="100">
        <Rectangle FillColor="0.2,0.2,0.2,1" Width="300" Height="100"/>
        
        <!-- Decrement -->
        <Button X="20" Y="25" Width="50" Height="50">
            <Pressed>
                <Set Variable="counter" Operator="#_set_subtract_">1</Set>
            </Pressed>
            <Enabled>
                <Off><Rectangle FillColor="0.5,0.2,0.2,1" Width="50" Height="50"/>
                     <Text X="25" Y="25" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
                           FillColor="1,1,1,1" Size="24">−</Text></Off>
                <On><Rectangle FillColor="0.7,0.3,0.3,1" Width="50" Height="50"/>
                    <Text X="25" Y="25" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
                          FillColor="1,1,1,1" Size="24">−</Text></On>
            </Enabled>
        </Button>
        
        <!-- Display -->
        <Text X="150" Y="50" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
              FillColor="0,1,0,1" Size="36">@counter</Text>
        
        <!-- Increment -->
        <Button X="230" Y="25" Width="50" Height="50">
            <Pressed>
                <Set Variable="counter" Operator="#_set_add_">1</Set>
            </Pressed>
            <Enabled>
                <Off><Rectangle FillColor="0.2,0.5,0.2,1" Width="50" Height="50"/>
                     <Text X="25" Y="25" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
                           FillColor="1,1,1,1" Size="24">+</Text></Off>
                <On><Rectangle FillColor="0.3,0.7,0.3,1" Width="50" Height="50"/>
                    <Text X="25" Y="25" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
                          FillColor="1,1,1,1" Size="24">+</Text></On>
            </Enabled>
        </Button>
    </Window>
</dcapp>
```

---

## Tips & Best Practices

1. **Use meaningful variable names** — `engineTemperature` is clearer than `et` or `temp1`.

2. **Choose appropriate types** — Use `Integer` for counts, `Double` for measurements, `String` for states/labels.

3. **Initialize with sensible defaults** — Prevents undefined behavior on startup.

4. **Group related variables** — Declare related variables together for maintainability.

5. **Use braced syntax `@{var}` when adjacent to text** — Ensures correct parsing.

6. **Format numbers for readability** — Use `%.0f` for whole numbers, `%.2f` for precision values.

7. **Use constants for operators** — `Operator="#_set_add_"` is clearer than `Operator="1"`.
