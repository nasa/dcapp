# Variables

Variables hold values that can change while a display is running. Declare them
as direct children of `DCAPP`, then refer to them with `@`:

```xml
<DCAPP>
    <Variable Type="#_variable_double_" InitialValue="0">altitude</Variable>

    <Window Title="Flight Display" Width="800" Height="600">
        <Text>Altitude: @altitude(%.0f) ft</Text>
    </Window>
</DCAPP>
```

Use a [constant](constants.md) instead when the value is fixed at load time.
State private to a C or C++ logic module can stay in that module's `user_data`.

## Declaration

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `Type` | constant | No | Variable type. Defaults to `#_variable_string_`. |
| `InitialValue` | string | No | Initial value. Defaults to an empty string. |

The element content is the variable name.

| Type | Values |
|------|--------|
| `#_variable_string_` | Text such as `STANDBY` or `Hello` |
| `#_variable_integer_` | Whole numbers such as `0`, `42`, or `-17` |
| `#_variable_double_` | Decimal numbers such as `3.14159` or `-273.15` |
| `#_variable_boolean_` | `true`, `false`, `1`, or `0` |

```xml
<Variable Type="#_variable_string_" InitialValue="STANDBY">status</Variable>
<Variable Type="#_variable_integer_" InitialValue="100">health</Variable>
<Variable Type="#_variable_double_" InitialValue="0.0">temperature</Variable>
<Variable Type="#_variable_boolean_" InitialValue="false">active</Variable>
```

## References

Attributes that accept variables use a direct reference:

```xml
<Rectangle X="@x" Y="@y" Rotation="@angle" Width="50" Height="50"/>
```

Text can contain a reference, a braced reference, or a printf-style format:

```xml
<Text>Hello, @name</Text>
<Text>Temperature: @{temperature} F</Text>
<Text>Altitude: @altitude(%.1f) ft</Text>
```

Braces delimit the name when more text follows it. For example,
`@{temperature}F` refers to `temperature`, while `@temperatureF` refers to a
variable named `temperatureF`.

Common numeric formats are:

| Format | Input | Output |
|--------|-------|--------|
| `%.0f` | `123.456` | `123` |
| `%.1f` | `123.456` | `123.5` |
| `%.2f` | `123.456` | `123.46` |
| `%05.1f` | `12.3` | `012.3` |
| `%+.1f` | `12.3` | `+12.3` |
| `%03d` | `7` | `007` |
| `%e` | `1234.5` | `1.234500e+03` |

Text expansion recognizes these escapes:

| Sequence | Result |
|----------|--------|
| `\n` | Newline |
| `\t` | Tab |
| `\\` | Backslash |
| `\@` | Literal `@` |
| `\"` | Double quote |
| `\'` | Single quote |
| `\#` | Literal `#` |
| `\$` | Literal `$` |

## Changing a variable with `Set`

```xml
<Set Variable="counter" Operator="#_set_add_">1</Set>
```

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `Variable` | string | Yes | Name of the variable to modify |
| `Operator` | integer | No | Operation to apply. Defaults to `#_set_equal_`. |
| `Defer` | boolean | No | Apply after the current draw pass. Kept for legacy compatibility. |

The element content supplies the operand. It is still required for unary and
stack operators, although those operations do not use its value.

### Set operators

| Constant | Value | Effect |
|----------|-------|--------|
| `#_set_equal_` | 1 | `var = value` |
| `#_set_add_` | 2 | `var = var + value` |
| `#_set_subtract_` | 3 | `var = var - value` |
| `#_set_multiply_` | 4 | `var = var * value` |
| `#_set_divide_` | 5 | `var = var / value` |
| `#_set_min_` | 6 | `var = min(var, value)` |
| `#_set_max_` | 7 | `var = max(var, value)` |
| `#_set_push_` | 8 | Save the current value on the variable's stack |
| `#_set_pop_` | 9 | Restore the saved value |
| `#_set_negate_` | 10 | `var = -var` |
| `#_set_reciprocal_` | 11 | `var = 1 / var` |
| `#_set_absolute_` | 12 | `var = abs(var)` |
| `#_set_square_` | 13 | `var = var * var` |
| `#_set_sqrt_` | 14 | `var = sqrt(var)` |
| `#_set_modulo_` | 15 | `var = var % value` |
| `#_set_power_` | 16 | `var = var ^ value` |
| `#_set_log_` | 17 | `var = ln(var)` |
| `#_set_exp_` | 18 | `var = e ^ var` |
| `#_set_round_` | 19 | `var = round(var)` |
| `#_set_sign_` | 20 | Set to `-1`, `0`, or `1` according to the sign |

The `min` and `max` names describe the operation, not the bound. To keep `x`
between 0 and 100:

```xml
<Set Variable="x" Operator="#_set_max_">0</Set>
<Set Variable="x" Operator="#_set_min_">100</Set>
```

`Defer="true"` collects the operation during drawing and applies it after the
draw completes. It exists to preserve legacy event behavior; new displays
normally leave it unset. The [migration guide](migration.md) covers converted
uses.

`Set` is often used inside an event:

```xml
<Variable Type="#_variable_integer_" InitialValue="0">clicks</Variable>

<Rectangle X="20" Y="20" Width="100" Height="40" FillColor="0.2 0.3 0.5 1">
    <MousePressed>
        <Set Variable="clicks" Operator="#_set_add_">1</Set>
    </MousePressed>
    <Text X="50" Y="20" LocalAlignX="#_align_center_"
          LocalAlignY="#_align_middle_">@clicks</Text>
</Rectangle>
```

## Testing variables with `If`

`If` accepts a variable reference in `Value` or `Value2`:

```xml
<If Value="@fuel" Value2="20" Operator="#_if_lt_">
    <True>
        <Text FillColor="1 0 0 1">LOW FUEL</Text>
    </True>
</If>
```

| Constant | Test |
|----------|------|
| `#_if_true_` | Value is truthy |
| `#_if_false_` | Value is falsy |
| `#_if_eq_` | Equal |
| `#_if_ne_` | Not equal |
| `#_if_lt_` | Less than |
| `#_if_gt_` | Greater than |
| `#_if_lte_` | Less than or equal |
| `#_if_gte_` | Greater than or equal |

Trick, Edge, and logic libraries can also read or update declared variables.
Their setup is documented in [TrickIO](trick.md), [EdgeIO](edge.md), and
[logic files](logic.md).
