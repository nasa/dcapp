# dcapp Blink Reference

Flashing child elements on and off for alerts and status indicators.

---

## Overview

The `<Blink>` element makes its children flash on and off at a configurable rate. This is used for visual alerts such as master alarms, caution lights, and status indicators. When active, the blink cycles between showing and hiding its child elements. When inactive, the children are hidden.

```xml
<Blink Frequency="4" DutyCycle="0.5" Duration="0" FireBlink="@trigger">
    <!-- Children flash on/off while the blink is active -->
    <Rectangle X="100" Y="380" Width="120" Height="50" FillColor="1 0 0"/>
    <Text X="100" Y="380" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
          Size="18" FillColor="1 1 1">ALARM</Text>
</Blink>
```

---

## When To Use Blink

Use `Blink` for attention-getting visual state: alarms, cautions, transient
status changes, and acknowledged/unacknowledged indicators. It is a visual
wrapper, not a timer system for application logic.

For logic that should happen once after a delay or on a fixed cadence, use C
logic or variables with `Set`/`If`. Use `Blink` when the output you need is
specifically "show these children on and off."

---

## Attributes

| Attribute | Type | Default | Description |
|-----------|------|---------|-------------|
| `Frequency` | number | -- | Blink rate in Hz (cycles per second). Higher values produce faster flashing. |
| `DutyCycle` | number (0-1) | -- | Fraction of each cycle during which children are visible. 0.5 means equal time on and off. 0.8 means visible 80% of the time. |
| `Duration` | number (seconds) | -- | How long the blink runs after being triggered. Set to `0` for indefinite blinking (continues until retriggered). |
| `FireBlink` | variable ref | -- | Edge-triggered control variable. Increment to start blinking; increment again to stop. |

---

## How FireBlink Works

`FireBlink` is **edge-triggered**, meaning it responds to *changes* in the variable value, not to the value itself.

- **Start blinking:** Increment the trigger variable (e.g., from 0 to 1).
- **Stop blinking:** Increment the trigger variable again (e.g., from 1 to 2).

Each change in the variable's value toggles the blink state. The actual numeric value does not matter -- only the fact that it changed.

A common pattern is to use a `<Set>` with `#_set_add_` inside a `<MousePressed>` event:

```xml
<Variable Type="#_variable_integer_" InitialValue="0">AlarmTrigger</Variable>

<Button Type="#_button_toggle_" X="100" Y="310" Width="100" Height="30"
        LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_">
    <MousePressed>
        <Set Variable="AlarmTrigger" Operator="#_set_add_">1</Set>
    </MousePressed>
    <ButtonIndicatorOn>
        <Rectangle FillColor="0.2 0.5 0.2"/>
        <Text ParentAlignX="#_align_center_" ParentAlignY="#_align_middle_"
              LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_" Size="12">RESET</Text>
    </ButtonIndicatorOn>
    <ButtonIndicatorOff>
        <Rectangle FillColor="0.5 0.2 0.2"/>
        <Text ParentAlignX="#_align_center_" ParentAlignY="#_align_middle_"
              LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_" Size="12">TRIGGER</Text>
    </ButtonIndicatorOff>
</Button>
```

The first click increments `AlarmTrigger` from 0 to 1, starting the blink. The second click increments from 1 to 2, stopping it. The third click starts it again, and so on.

For timed blinks (`Duration` > 0), the blink automatically stops after the specified duration. You can still stop it early by incrementing the trigger variable.

---

## Examples

### Master Alarm (Fast, Indefinite)

A high-urgency alarm that blinks rapidly and continues until manually acknowledged:

```xml
<Variable Type="#_variable_integer_" InitialValue="0">MasterAlarmTrigger</Variable>

<!-- Dim background (always visible) -->
<Rectangle X="100" Y="380" Width="120" Height="50"
           LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
           FillColor="0.3 0 0" LineColor="0.5 0 0" LineWidth="2"/>

<!-- Bright alarm (blinks) -->
<Blink Frequency="4" DutyCycle="0.5" Duration="0" FireBlink="@MasterAlarmTrigger">
    <Rectangle X="100" Y="380" Width="120" Height="50"
               LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
               FillColor="1 0 0" LineColor="1 0.3 0.3" LineWidth="2"/>
    <Text X="100" Y="380" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
          Size="18" FillColor="1 1 1">ALARM</Text>
</Blink>
```

- `Frequency="4"` -- 4 blinks per second.
- `DutyCycle="0.5"` -- visible half the time.
- `Duration="0"` -- blinks forever until the trigger is incremented again.

### Caution Light (Timed, Medium Speed)

A medium-priority warning that blinks for 5 seconds and then stops automatically:

```xml
<Variable Type="#_variable_integer_" InitialValue="0">CautionTrigger</Variable>

<Rectangle X="300" Y="380" Width="120" Height="50"
           LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
           FillColor="0.3 0.2 0" LineColor="0.5 0.4 0" LineWidth="2"/>

<Blink Frequency="2" DutyCycle="0.6" Duration="5" FireBlink="@CautionTrigger">
    <Rectangle X="300" Y="380" Width="120" Height="50"
               LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
               FillColor="1 0.7 0" LineColor="1 0.85 0.3" LineWidth="2"/>
    <Text X="300" Y="380" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
          Size="18" FillColor="0 0 0">CAUTION</Text>
</Blink>
```

- `Frequency="2"` -- 2 blinks per second.
- `DutyCycle="0.6"` -- visible 60% of each cycle (slightly longer on than off).
- `Duration="5"` -- automatically stops after 5 seconds.

### Multiple Indicators on One Trigger

Several blink elements can share the same trigger variable. Each can have its own frequency, producing a panel of status lights that all start and stop together but flash at different rates:

```xml
<Variable Type="#_variable_integer_" InitialValue="0">StatusTrigger</Variable>

<!-- Green: slow -->
<Blink Frequency="0.5" DutyCycle="0.5" Duration="0" FireBlink="@StatusTrigger">
    <Ellipse X="660" Y="380" Radius="12" FillColor="0 1 0"/>
</Blink>

<!-- Yellow: medium -->
<Blink Frequency="1.5" DutyCycle="0.5" Duration="0" FireBlink="@StatusTrigger">
    <Ellipse X="700" Y="380" Radius="12" FillColor="1 0.8 0"/>
</Blink>

<!-- Red: fast -->
<Blink Frequency="3" DutyCycle="0.5" Duration="0" FireBlink="@StatusTrigger">
    <Ellipse X="740" Y="380" Radius="12" FillColor="1 0 0"/>
</Blink>
```

---

## Tips

- Place a dim "background" version of the indicator behind the `<Blink>` so the element location is always visible, even when the blink is in its off phase.
- Use `DutyCycle` values above 0.5 for caution-level alerts (mostly visible, brief off-flashes) and 0.5 for urgent alarms (equal on/off).
- A `Duration` of `0` means the blink never stops on its own. Use this for critical alarms that require manual acknowledgment.

---

## Legacy Migration

The following attribute names changed from the legacy format:

| Legacy | Current | Notes |
|--------|---------|-------|
| `FnStartBlink` | `FireBlink` | Same edge-triggered behavior. The `@` prefix on the variable reference is preserved. |
| `Duration="-1"` | `Duration="0"` | Both mean indefinite. The conversion script updates `-1` to `0`. |

The legacy conversion script (`scripts/convert-legacy-xml.py`) handles both renames automatically.

---

## See Also

- [samples.md](samples.md) -- Full list of sample projects (`samples/blink/`).
