# EdgeIO

`EdgeIO` maps dcapp variables to Edge RCS command strings. `EdgeFrom` receives
values from the server; `EdgeTo` sends a dcapp variable when its value changes.

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

The text inside `EdgeVariable` is the dcapp variable name. It does not use the
`@` prefix.

## Attributes

| Element | Attribute | Meaning |
|---------|-----------|---------|
| `EdgeIO` | `Host` | Edge server host; runtime default is `localhost` |
| `EdgeIO` | `Port` | Edge server port; runtime default is `5451` |
| `EdgeIO` | `DataRate` | Update period in seconds; defaults to `1.0` |
| `EdgeIO` | `ConnectedVariable` | Optional dcapp boolean updated with connection status |
| `EdgeVariable` | `Command` | Edge command string |

The runtime has defaults for `Host` and `Port`, but the validator still
warns when they are omitted. Set them explicitly in checked-in displays.
