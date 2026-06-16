# External Integration Overview

dcapp has three external data/display integration paths. Each has its own
explicit reference doc:

| Integration | Use it for | Details |
|-------------|------------|---------|
| TrickIO | Trick Variable Server simulation variables | [trick.md](trick.md) |
| EdgeIO | Edge RCS command/value exchange | [edge.md](edge.md) |
| PixelStream | Video/image stream display | [pixelstream.md](pixelstream.md) |

Use these integrations when the display needs live external state or imagery.
Keep local presentation state in dcapp variables, then map only the values that
must cross the process boundary. That keeps the XML readable and makes it clear
which variables are internal display state versus external IO.

Integration elements are normally declared near the top of the XML, alongside
`Variable` declarations and before `Window` content.

```xml
<DCAPP>
    <Variable Type="#_variable_double_">simTime</Variable>

    <TrickIO Host="localhost" Port="7000" DataRate="0.1">
        <TrickFrom>
            <TrickVariable Name="dyn.cannon.time">simTime</TrickVariable>
        </TrickFrom>
    </TrickIO>

    <Window Title="Display" Width="900" Height="700">
        ...
    </Window>
</DCAPP>
```

All integration variable mapping children use the dcapp variable name as text
content. Do not prefix those names with `@`; the mapping wants the variable
name, not a dereferenced variable value.
