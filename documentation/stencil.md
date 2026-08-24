# Stencils

A stencil clips drawing to an arbitrary mask. Geometry can be added to the
mask, removed from it to cut holes, and then used to clip normal drawing.

| Element | Purpose |
|---------|---------|
| `Stencil` | Owns one independent mask and its clipped content |
| `StencilAdd` | Adds its child geometry to the visible region |
| `StencilRemove` | Removes its child geometry from the visible region |
| `StencilDraw` | Draws its children through the current mask |

`StencilAdd`, `StencilRemove`, and `StencilDraw` may appear in any order, and a
`Stencil` may contain more than one of each. Operations are cumulative within
that block. A later `Stencil` starts with a separate mask, and content outside
stencil blocks draws normally.

Shapes in `StencilAdd` and `StencilRemove` must use
`FillColor="#_stencil_color_"` so they write the stencil buffer correctly.

## Ring mask

This mask adds an outer ellipse, removes the inner ellipse, and draws colored
bands through the resulting ring:

```xml
<Stencil>
    <StencilAdd>
        <Ellipse X="600" Y="750" Radius="100"
                 FillColor="#_stencil_color_"/>
    </StencilAdd>
    <StencilRemove>
        <Ellipse X="600" Y="750" Radius="50"
                 FillColor="#_stencil_color_"/>
    </StencilRemove>
    <StencilDraw>
        <Rectangle X="450" Y="850" Width="300" Height="50"
                   FillColor="1 0.2 0.2 1"/>
        <Rectangle X="450" Y="800" Width="300" Height="50"
                   FillColor="0.2 1 0.2 1"/>
        <Rectangle X="450" Y="750" Width="300" Height="50"
                   FillColor="0.2 0.2 1 1"/>
    </StencilDraw>
</Stencil>
```

Text can define a mask as well:

```xml
<Stencil>
    <StencilAdd>
        <Text X="20" Y="750" Size="80"
              FillColor="#_stencil_color_">MASKED</Text>
    </StencilAdd>
    <StencilDraw>
        <Rectangle X="0" Y="800" Width="500" Height="40" FillColor="1 0 0 1"/>
        <Rectangle X="0" Y="760" Width="500" Height="40" FillColor="1 1 0 1"/>
        <Rectangle X="0" Y="720" Width="500" Height="40" FillColor="0 0 1 1"/>
    </StencilDraw>
</Stencil>
```

Transparent images can serve as masks; their opaque portions define the
visible region. Mask geometry also accepts variables, so position, radius, and
other supported attributes can be animated:

```xml
<Variable Type="#_variable_double_" InitialValue="50">mask_radius</Variable>

<Stencil>
    <StencilAdd>
        <Ellipse X="750" Y="600" Radius="@mask_radius"
                 FillColor="#_stencil_color_"/>
    </StencilAdd>
    <StencilDraw>
        <Rectangle FillColor="1 1 1 1"/>
    </StencilDraw>
</Stencil>
```

## Legacy masks

| Legacy element | Current element |
|----------------|-----------------|
| `Mask` | `Stencil` |
| `Stencil` inside `Mask` | `StencilAdd` |
| `Projection` | `StencilDraw` |

`scripts/convert-legacy-xml.py` performs these renames and adds
`FillColor="#_stencil_color_"` to mask shapes. The [migration guide](migration.md)
has the remaining conversion details.
