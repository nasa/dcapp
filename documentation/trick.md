# TrickIO

`TrickIO` maps dcapp variables to a Trick Variable Server. `TrickFrom` receives
simulation values; `TrickTo` sends a dcapp variable when its value changes.

```xml
<Variable Type="#_variable_double_">posX</Variable>
<Variable Type="#_variable_boolean_" InitialValue="false">trickConnected</Variable>

<TrickIO Host="localhost" Port="7000" DataRate="0.1" ConnectedVariable="trickConnected">
    <TrickFrom>
        <TrickVariable Name="dyn.cannon.pos[0]" Units="m">posX</TrickVariable>
    </TrickFrom>
    <TrickTo>
        <TrickVariable Name="dyn.input.command">posX</TrickVariable>
    </TrickTo>
</TrickIO>
```

The text inside `TrickVariable` is the dcapp variable name. It does not use the
`@` prefix.

## Attributes

| Element | Attribute | Meaning |
|---------|-----------|---------|
| `TrickIO` | `Host` | Required Trick Variable Server host |
| `TrickIO` | `Port` | Required Trick Variable Server port |
| `TrickIO` | `DataRate` | Update period in seconds; defaults to `0.1` |
| `TrickIO` | `ConnectedVariable` | Optional dcapp boolean updated with connection status |
| `TrickVariable` | `Name` | Trick variable path |
| `TrickVariable` | `Units` | Optional Trick unit conversion string |

## Running the sample

Build and run the Trick simulation first:

```bash
cd samples/trick/sim
trick-CP
./S_main_*.exe RUN_test/input.py
```

From the repository root in a second terminal, run the display:

```bash
./bin/dcapp.sh samples/trick/trick.xml
```
