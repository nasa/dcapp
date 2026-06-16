# EdgeIO

`EdgeIO` connects dcapp variables to an Edge RCS server. The shape mirrors
`TrickIO`, but variables are identified by Edge command strings instead of
Trick variable paths.

Use `EdgeIO` when the display needs to exchange command/value data with an Edge
server. The dcapp side still works with normal variables; the Edge mapping is
only the bridge between those variables and Edge command strings.

Use `EdgeFrom` for values the server owns and dcapp displays. Use `EdgeTo` for
values dcapp owns or edits and the server should receive.

## XML Shape

```xml
<Variable Type="#_variable_double_">temperature</Variable>
<Variable Type="#_variable_double_">throttleCommand</Variable>

<EdgeIO Host="localhost" Port="5451" DataRate="1.0">
    <EdgeFrom>
        <EdgeVariable Command="GET_TEMPERATURE">temperature</EdgeVariable>
    </EdgeFrom>
    <EdgeTo>
        <EdgeVariable Command="SET_THROTTLE">throttleCommand</EdgeVariable>
    </EdgeTo>
</EdgeIO>
```

`EdgeFrom` receives command values into dcapp variables. `EdgeTo` sends dcapp
variables when their values change. The text content of each `EdgeVariable` is
the dcapp variable name.

## Attributes

| Element | Attribute | Meaning |
|---------|-----------|---------|
| `EdgeIO` | `Host` | Edge server host; runtime default is `localhost` |
| `EdgeIO` | `Port` | Edge server port; runtime default is `5451` |
| `EdgeIO` | `DataRate` | Update period in seconds; defaults to `1.0` |
| `EdgeIO` | `ConnectedVariable` | Optional dcapp boolean updated with connection status |
| `EdgeVariable` | `Command` | Edge command string |

See also [integration.md](integration.md) for the cross-protocol overview.

Note: the runtime has defaults for `Host` and `Port`, but the validator still
warns when they are omitted. Set them explicitly in checked-in displays.
