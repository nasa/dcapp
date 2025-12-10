# Button Element

The `<Button>` element creates an interactive button with configurable behavior, visual states, and variable bindings.

## Button Types

Set via the `Type` attribute:

| Type | Behavior |
|------|----------|
| `Standard` | Press sets switch variable to on-value. No release behavior. No state checking. |
| `Momentary` | Press sets switch to on-value, release sets it to off-value. Like a push-to-talk button. |
| `Toggle` | Each press flips between on and off states. Checks indicator state before acting. |

Default is `Standard` if not specified.

## Variable System

Buttons use up to four variable concepts, each with associated comparison values:

### 1. Switch Variable (command)
The variable that gets *written* when you interact with the button.

| Attribute | Description |
|-----------|-------------|
| `SwitchVariable` | Variable name to write to |
| `SwitchOn` | Value written when turning on (default: `1`) |
| `SwitchOff` | Value written when turning off (default: `0`) |

### 2. Indicator Variable (feedback)
The variable that determines the button's *visual state* (on/off appearance).

| Attribute | Description |
|-----------|-------------|
| `IndicatorVariable` | Variable name to read from |
| `IndicatorOn` | Value that means "on" state (default: `1`) |

### 3. Active Variable (enabled/disabled)
Controls whether the button accepts input.

| Attribute | Description |
|-----------|-------------|
| `ActiveVariable` | Variable name to check |
| `ActiveOn` | Value that means "enabled" (default: `1`) |

If `ActiveVariable` is not specified, the button is always enabled.

### 4. Base Variable (shorthand)
Convenience attributes that feed into Switch and Indicator:

| Attribute | Description |
|-----------|-------------|
| `Variable` | Base variable name |
| `On` | Base on-value |
| `Off` | Base off-value |

## Variable Inheritance

The system uses a cascading inheritance pattern to reduce repetition:

```
Variable    →  SwitchVariable    →  IndicatorVariable
On          →  SwitchOn          →  IndicatorOn
Off         →  SwitchOff
```

**Inheritance rules:**

1. If `SwitchVariable` is not set, it inherits from `Variable`
2. If `SwitchOn` is not set, it inherits from `On`
3. If `SwitchOff` is not set, it inherits from `Off`
4. If `IndicatorVariable` is not set, it inherits from `SwitchVariable`
5. If `IndicatorOn` is not set, it inherits from `SwitchOn`

**Final defaults** (if still unset after inheritance):
- `SwitchOn` → `1`
- `SwitchOff` → `0`
- `IndicatorOn` → `1`
- `ActiveOn` → `1` (only if `ActiveVariable` is specified)

**Anonymous variable creation:**
- If no `SwitchVariable` is specified (even after inheritance), an anonymous internal variable is created automatically

### Examples

**Minimal usage** - all variables are the same:
```xml
<Button Type="Toggle" Variable="my_switch" On="1" Off="0">
```
Equivalent to setting `SwitchVariable="my_switch"`, `IndicatorVariable="my_switch"`, etc.

**Separate command and feedback:**
```xml
<Button Type="Toggle" 
        SwitchVariable="cmd_light" SwitchOn="1" SwitchOff="0"
        IndicatorVariable="status_light" IndicatorOn="1">
```
Writes to `cmd_light`, but visual state is determined by `status_light`.

**With enable/disable control:**
```xml
<Button Type="Toggle" Variable="pump" ActiveVariable="system_ready">
```
Button only works when `system_ready == 1`.

## Transition State

When `SwitchVariable` and `IndicatorVariable` are *different*, an internal transition variable is automatically created. This tracks the "pending" state:

| Transition Value | Meaning |
|------------------|---------|
| `1` | Commanded ON, waiting for indicator to confirm |
| `-1` | Commanded OFF, waiting for indicator to confirm |
| `0` | Stable - indicator matches last command |

This is useful for hardware control where there's latency between sending a command and receiving confirmation.

## Child Elements

Visual content for different button states:

| Element | When Displayed |
|---------|----------------|
| `<On>` | Indicator shows on state |
| `<Off>` | Indicator shows off state |
| `<Enabled>` | Button is enabled (accepts input) |
| `<Disabled>` | Button is disabled (ignores input) |
| `<Transition>` | Waiting for indicator to catch up to switch |
| `<MousePressed>` | Mouse button is held down |
| `<MouseReleased>` | Mouse button released (momentary display) |

Child elements can contain any visual elements (images, shapes, text, etc.).

## Transform Attributes

Standard positioning and transformation:

| Attribute | Aliases | Description |
|-----------|---------|-------------|
| `PositionX` | `X` | X position |
| `PositionY` | `Y` | Y position |
| `DimensionX` | `Width` | Width |
| `DimensionY` | `Height` | Height |
| `VirtualDimensionX` | `VirtualWidth` | Virtual width for scaling |
| `VirtualDimensionY` | `VirtualHeight` | Virtual height for scaling |
| `LocalAlignX` | `HorizontalAlign` | Horizontal alignment |
| `LocalAlignY` | `VerticalAlign` | Vertical alignment |
| `ParentAlignX` | | Parent horizontal alignment |
| `ParentAlignY` | | Parent vertical alignment |
| `Rotation` | `Rotate` | Rotation angle |
| `PivotPositionX` | `PivotX` | Pivot X position |
| `PivotPositionY` | `PivotY` | Pivot Y position |
| `PivotLocalAlignX` | | Pivot horizontal alignment |
| `PivotLocalAlignY` | | Pivot vertical alignment |

## Complete Example

```xml
<Button Type="Toggle" 
        X="100" Y="200" Width="80" Height="40"
        SwitchVariable="cmd_pump" 
        IndicatorVariable="status_pump"
        ActiveVariable="system_armed" ActiveOn="1">
    
    <Enabled>
        <On>
            <Image File="pump_on.png"/>
        </On>
        <Off>
            <Image File="pump_off.png"/>
        </Off>
        <Transition>
            <Image File="pump_pending.png"/>
        </Transition>
    </Enabled>
    
    <Disabled>
        <Image File="pump_disabled.png"/>
    </Disabled>
    
</Button>
```

This creates a toggle button that:
- Writes to `cmd_pump` when clicked
- Shows on/off state based on `status_pump`
- Only works when `system_armed == 1`
- Shows a "pending" image while waiting for confirmation
- Shows a grayed-out image when disabled
