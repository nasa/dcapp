# Blink

`Blink` alternates its children between visible and hidden while active. Before
it is triggered and after it stops, the children are visible normally. It is a
rendering effect; it does not run application logic on a timer.

| Attribute | Type | Default | Description |
|-----------|------|---------|-------------|
| `Frequency` | number/var | `1` | Cycles per second |
| `DutyCycle` | number/var, 0–1 | `0.5` | Fraction of each cycle for which the children are visible |
| `Duration` | seconds/var | indefinite | Time before the blink stops. Values at or below `0` run indefinitely. |
| `FireBlink` | integer/var | — | Edge-triggered value that starts the blink |

`FireBlink` responds to a change in the referenced value, not to its numeric
value. With an indefinite duration, each change toggles flashing on or off.
With a positive duration, each change starts or restarts the timer; the blink
returns to steady drawing when the timer expires.

This example starts on the first press and stops on the next. The dim rectangle
remains visible during the blink's off phase.

```xml
<Variable Type="#_variable_integer_" InitialValue="0">alarm_trigger</Variable>

<Rectangle X="100" Y="380" Width="120" Height="50"
           LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
           FillColor="0.3 0 0 1"/>

<Blink Frequency="4" DutyCycle="0.5" Duration="0"
       FireBlink="@alarm_trigger">
    <Rectangle X="100" Y="380" Width="120" Height="50"
               LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
               FillColor="1 0 0 1"/>
    <Text X="100" Y="380" LocalAlignX="#_align_center_"
          LocalAlignY="#_align_middle_" Size="18" FillColor="1 1 1 1">ALARM</Text>
</Blink>

<Rectangle X="50" Y="300" Width="100" Height="30"
           FillColor="0.3 0.3 0.3 1">
    <Text X="50" Y="15" LocalAlignX="#_align_center_"
          LocalAlignY="#_align_middle_">ALARM</Text>
    <MousePressed>
        <Set Variable="alarm_trigger" Operator="#_set_add_">1</Set>
    </MousePressed>
</Rectangle>
```

Several `Blink` elements may share one trigger and still use different
frequencies or durations.

## Legacy names

| Legacy | Current |
|--------|---------|
| `FnStartBlink` | `FireBlink` |
| `Duration="-1"` | `Duration="0"` |

`scripts/convert-legacy-xml.py` applies both conversions.
