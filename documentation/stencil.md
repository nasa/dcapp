# dcapp Stencil Reference

Clipping rendered content through mask shapes.

---

## Overview

Stencils let you define an arbitrary mask shape and then render content only within (or outside of) that mask. This is used for effects like shaped viewports, donut cutouts, text-shaped windows, and clipped HUD overlays.

The stencil system works in three steps:

1. **Add** geometry to the mask (defines the visible region).
2. Optionally **remove** geometry from the mask (cuts holes).
3. **Draw** content that is clipped to the resulting mask.

Content outside the mask is not rendered. Content between or after `<Stencil>` blocks renders normally, unaffected by any stencil.

---

## Elements

### `<Stencil>`

The top-level container that groups a mask definition and the content drawn through it. Each `<Stencil>` block is independent; multiple stencils on the same panel do not interfere with each other.

**Children:** `<StencilAdd>`, `<StencilRemove>`, `<StencilDraw>` (in any order, and multiple of each are allowed).

### `<StencilAdd>`

Adds geometry to the mask. Any area covered by shapes inside this element becomes part of the visible region. You can have multiple `<StencilAdd>` blocks within one `<Stencil>` to build up a complex mask incrementally.

### `<StencilRemove>`

Subtracts geometry from the mask. Any area covered by shapes inside this element is cut away from the visible region. This is how you create holes (for example, the inner circle of a donut).

### `<StencilDraw>`

Renders content clipped to the current mask. Only the portions of child elements that overlap with the mask are visible. You can draw any elements here: rectangles, ellipses, text, images, and so on.

---

## The `#_stencil_color_` Constant

Shapes inside `<StencilAdd>` and `<StencilRemove>` must use the special fill color constant `#_stencil_color_` for their `FillColor` attribute. This ensures the mask is written to the stencil buffer correctly.

```xml
<StencilAdd>
    <Ellipse X="600" Y="750" Radius="100" FillColor="#_stencil_color_"/>
</StencilAdd>
```

If you use a regular color instead, the mask shape may not register in the stencil buffer. The legacy conversion script (`scripts/convert-legacy-xml.py`) automatically sets this constant when converting old `<Mask>` elements.

---

## Patterns

### Basic Shape Mask

Clip content to a single shape, such as a circle filled with horizontal stripes:

```xml
<Stencil>
    <StencilAdd>
        <Ellipse X="950" Y="380" Radius="80" FillColor="#_stencil_color_"/>
    </StencilAdd>
    <StencilDraw>
        <Rectangle X="820" Y="460" Width="260" Height="20" FillColor="1 0 0"/>
        <Rectangle X="820" Y="440" Width="260" Height="20" FillColor="1 1 1"/>
        <Rectangle X="820" Y="420" Width="260" Height="20" FillColor="1 0 0"/>
        <Rectangle X="820" Y="400" Width="260" Height="20" FillColor="1 1 1"/>
        <Rectangle X="820" Y="380" Width="260" Height="20" FillColor="1 0 0"/>
    </StencilDraw>
</Stencil>
```

### Donut (Add Outer, Remove Inner)

Create a ring shape by adding a large circle and then removing a smaller concentric circle:

```xml
<Stencil>
    <StencilAdd>
        <!-- Outer circle -->
        <Ellipse X="600" Y="750" Radius="100" FillColor="#_stencil_color_"/>
    </StencilAdd>
    <StencilRemove>
        <!-- Inner circle creates the hole -->
        <Ellipse X="600" Y="750" Radius="50" FillColor="#_stencil_color_"/>
    </StencilRemove>
    <StencilDraw>
        <!-- Colored bands visible only in the ring -->
        <Rectangle X="450" Y="850" Width="300" Height="50" FillColor="1 0.2 0.2"/>
        <Rectangle X="450" Y="800" Width="300" Height="50" FillColor="0.2 1 0.2"/>
        <Rectangle X="450" Y="750" Width="300" Height="50" FillColor="0.2 0.2 1"/>
        <Text X="520" Y="740" Size="20" FillColor="1 1 1">RING</Text>
    </StencilDraw>
</Stencil>
```

### Text as Mask

Use text to form the mask shape. The SDF text renderer integrates with the stencil system, so letter outlines define the visible region:

```xml
<Stencil>
    <StencilAdd>
        <Text X="20" Y="750" Size="80" FillColor="#_stencil_color_">MASKED</Text>
    </StencilAdd>
    <StencilDraw>
        <!-- Rainbow stripes show through the text shape -->
        <Rectangle X="0" Y="820" Width="500" Height="30" FillColor="1 0 0"/>
        <Rectangle X="0" Y="790" Width="500" Height="30" FillColor="1 0.5 0"/>
        <Rectangle X="0" Y="760" Width="500" Height="30" FillColor="1 1 0"/>
        <Rectangle X="0" Y="730" Width="500" Height="30" FillColor="0 1 0"/>
        <Rectangle X="0" Y="700" Width="500" Height="30" FillColor="0 0 1"/>
    </StencilDraw>
</Stencil>
```

### Image as Mask

Images with transparency can serve as masks. The opaque portions of the image define the visible region:

```xml
<Stencil>
    <StencilAdd>
        <Image X="1100" Y="650" Width="500" Height="166" File="assets/nasa-worm.png"/>
    </StencilAdd>
    <StencilDraw>
        <Rectangle X="1050" Y="820" Width="600" Height="25" FillColor="1 0 0"/>
        <Rectangle X="1050" Y="795" Width="600" Height="25" FillColor="1 0.5 0"/>
        <Rectangle X="1050" Y="770" Width="600" Height="25" FillColor="1 1 0"/>
    </StencilDraw>
</Stencil>
```

### Multiple Add/Remove Passes

You can alternate `<StencilAdd>` and `<StencilRemove>` blocks to sculpt the mask. The operations are cumulative:

```xml
<Stencil>
    <StencilAdd>
        <!-- Three overlapping circles -->
        <Ellipse X="880" Y="760" Radius="60" FillColor="#_stencil_color_"/>
        <Ellipse X="960" Y="760" Radius="60" FillColor="#_stencil_color_"/>
        <Ellipse X="920" Y="700" Radius="60" FillColor="#_stencil_color_"/>
    </StencilAdd>
    <StencilRemove>
        <!-- Cut a hole from the center -->
        <Ellipse X="920" Y="740" Radius="25" FillColor="#_stencil_color_"/>
    </StencilRemove>
    <StencilAdd>
        <!-- Add a small dot back in the very center -->
        <Ellipse X="920" Y="740" Radius="10" FillColor="#_stencil_color_"/>
    </StencilAdd>
    <StencilDraw>
        <!-- Checkerboard visible through the sculpted mask -->
        <Rectangle X="780" Y="800" Width="40" Height="40" FillColor="1 0 0"/>
        <Rectangle X="820" Y="800" Width="40" Height="40" FillColor="0 1 0"/>
    </StencilDraw>
</Stencil>
```

### Side-by-Side Independent Stencils

Each `<Stencil>` block is fully independent. Adjacent stencils do not affect each other, and normal content between them renders without clipping:

```xml
<!-- First stencil -->
<Stencil>
    <StencilAdd>
        <Rectangle X="1150" Y="380" Width="80" Height="80" FillColor="#_stencil_color_"/>
    </StencilAdd>
    <StencilDraw>
        <Ellipse X="1190" Y="420" Radius="60" FillColor="0 0 1"/>
        <Text X="1160" Y="410" Size="14" FillColor="1 1 1">BOX1</Text>
    </StencilDraw>
</Stencil>

<!-- Normal content between stencils renders unclipped -->
<Text X="20" Y="600" Size="18" FillColor="0.2 0.8 0.2">Regular content</Text>

<!-- Second stencil (independent) -->
<Stencil>
    <StencilAdd>
        <Ellipse X="1350" Y="420" Radius="50" FillColor="#_stencil_color_"/>
    </StencilAdd>
    <StencilDraw>
        <Rectangle X="1280" Y="360" Width="140" Height="120" FillColor="0 1 0"/>
        <Text X="1310" Y="410" Size="14" FillColor="0 0 0">CIRCLE</Text>
    </StencilDraw>
</Stencil>
```

### Animated Stencil Masks

Mask shapes can be driven by variables, allowing animated reveal effects. Bind the mask geometry attributes (radius, position, etc.) to variables updated by logic or Trick:

```xml
<Variable Type="#_variable_double_" InitialValue="50">RADIUS1</Variable>

<Stencil>
    <StencilAdd>
        <Ellipse X="750" Y="600" Radius="@RADIUS1" FillColor="#_stencil_color_"/>
    </StencilAdd>
    <StencilDraw>
        <Rectangle FillColor="1 1 1"/>
    </StencilDraw>
</Stencil>
```

---

## Legacy Migration: Mask to Stencil

The old `<Mask>` element has been replaced by `<Stencil>`. The conversion is:

| Legacy Element | New Element |
|----------------|-------------|
| `<Mask>` | `<Stencil>` |
| `<Stencil>` (child of Mask) | `<StencilAdd>` |
| `<Projection>` | `<StencilDraw>` |

The legacy conversion script (`scripts/convert-legacy-xml.py`) performs this rename automatically and sets `FillColor="#_stencil_color_"` on all shapes inside `<StencilAdd>` and `<StencilRemove>`.

See [migration.md](migration.md) for full details on legacy XML conversion.

---

## See Also

- [samples.md](samples.md) -- Full list of sample projects (`samples/stencil/`).
