# TrickIO

`TrickIO` connects dcapp variables to a running Trick simulation through the
Trick Variable Server.

Use `TrickIO` when dcapp is acting as a live display or control panel for a
Trick simulation. It lets XML and logic read simulation values as normal dcapp
variables, and optionally send command variables back to Trick when they change.

If the data is only display-local state, keep it as a dcapp variable. If the
value belongs to the simulation or must command the simulation, map it through
`TrickFrom` or `TrickTo`.

## XML Shape

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

`TrickFrom` receives values from Trick into dcapp variables. `TrickTo` sends
dcapp variable changes back to Trick. The text content of each `TrickVariable`
is the dcapp variable name.

## Attributes

| Element | Attribute | Meaning |
|---------|-----------|---------|
| `TrickIO` | `Host` | Trick Variable Server host |
| `TrickIO` | `Port` | Trick Variable Server port |
| `TrickIO` | `DataRate` | Update period in seconds |
| `TrickIO` | `ConnectedVariable` | Optional dcapp boolean updated with connection status |
| `TrickVariable` | `Name` | Trick variable path |
| `TrickVariable` | `Units` | Optional Trick unit conversion string |

## Sample

Run the Trick simulation first:

```bash
cd samples/trick/sim
./S_main_*.exe RUN_test/input.py
```

Then run the display:

```bash
./bin/dcapp.sh samples/trick/trick.xml
```

See also [integration.md](integration.md) for the cross-protocol overview.
