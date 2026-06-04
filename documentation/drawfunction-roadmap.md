# DrawFunction Roadmap Notes

These notes capture the current design direction for exposing more dcapp drawing
capability to user logic C code.

This is a roadmap, not the authoritative API reference. Some snippets are
planned shorthand and may not match the current generated `logic/dcapp.h`
exactly.

## Current Intent

`DrawFunction` should let XML stay useful as the display/layout structure while C
handles drawing that is procedural, repetitive, stateful, or awkward to express
as XML.

There are two supported patterns:

- Hybrid XML/C: XML owns layout; small draw functions fill in procedural pieces.
- Full procedural panel: XML creates the shell; one draw function owns the panel.

## Do Not Clone XML In C

Avoid building a second retained-mode XML scene graph in C. Prefer immediate-mode
drawing scopes and helpers.

Good:

```c
if (dc_draw->container_push_area(ctx, &area)) {
    dc_draw->rect_filled(ctx, position, size, fill_color);
    dc_draw->rect(ctx, position, size, stroke);
    dc_draw->container_pop(ctx);
}
```

Avoid:

```c
DcContainer c = dc_draw->container(...);
dc_draw->container_add_child(c, ...);
dc_draw->draw_container(c);
```

## Missing 2D Features To Add

### Arc

Expose XML-like arc drawing directly.

```c
dc_draw->arc(ctx, radius, angle, segments, placement, stroke);
dc_draw->arc_filled(ctx, radius, angle, segments, placement, fill_color);
```

This does not need to be a perfect XML mirror, but should cover the useful arc
feature set without forcing users to sample arc points by hand.

### Ellipse And Pie Wedge

Expose full ellipse and partial/pie modes.

```c
dc_draw->ellipse(ctx, radius_x, radius_y, angle, segments, placement, stroke);
dc_draw->ellipse_filled(ctx, radius_x, radius_y, angle, segments, placement, fill_color);
```

This should support fill, line, and partial-angle wedge behavior.

### Text

Text is required even if the first API is not a full XML text clone.

Recommended first pass:

```c
dc_draw->text(ctx, "Hello", placement, text_style);
dc_draw->text_size(ctx, "Hello", text_style, &width, &height);
```

C users can handle formatting themselves with `snprintf`, so the first version
does not need XML variable interpolation or update-rate caching.

### Image

The low-level image quad calls exist, but user logic needs a friendly way to get
a texture handle.

Possible shape:

```c
uint32_t image = dc_draw->image_load(ctx, "foo.png");
dc_draw->image(ctx, image, width, height, placement, tint);
```

## Draw State And Scopes

### Container-Like Scope

Expose container behavior as immediate-mode state, not as a retained node tree.

```c
dc_draw->container_push(ctx, position, size, virtual_size);
dc_draw->container_pop(ctx);
```

`container_push` should eventually handle local coordinate space, alignment,
virtual dimensions, transforms, local mouse coordinates, and optional clipping.

### Basic Transforms

Also expose lower-level building blocks:

```c
dc_draw->push(ctx);
dc_draw->translate(ctx, x, y);
dc_draw->rotate(ctx, degrees);
dc_draw->pop(ctx);
```

### Stencil / Clip

Stenciling should be exposed as scoped immediate-mode clipping/masking.

```c
if (dc_draw->stencil_begin(ctx)) {
    dc_draw->stencil_add(ctx);
    dc_draw->rect_filled(ctx, position, size, dc_stencil_color());
    dc_draw->stencil_draw(ctx);
    dc_draw->rect_filled(ctx, position, size, color);
    dc_draw->stencil_end(ctx);
}
```

Stencil phase calls currently reuse normal draw calls, with `dc_stencil_color()`
as the visible-color placeholder for stencil-only geometry.

## 3D Direction

3D drawing should be planned for, but should probably use a separate context or
expanded context rather than overloading the 2D panel API too much.

Possible future shape:

```c
void draw_3d(DcDraw3DContext *ctx, const DcDrawFuncArgs *args);
```

`Sphere` support belongs here.

## Planet Direction

Planet drawing is one of the important long-term reasons for `DrawFunction`.
Planet primitives need planet-specific render context:

- `PlanetView`
- CRS interpretation
- camera/view state
- terrain-relative height
- planet radius and extension state

Prefer a dedicated planet draw callback/API:

```xml
<PlanetView Name="EarthView" ...>
    <PlanetDrawFunction Name="draw_trajectory"/>
</PlanetView>
```

```c
void draw_trajectory(DcPlanetDrawContext *ctx, const DcDrawFuncArgs *args) {
    dc_planet_draw->line(ctx, points, count, ...);
    dc_planet_draw->sphere(ctx, ...);
    dc_planet_draw->text(ctx, ...);
}
```

## Initialization Model

Keep `display_pre_init()` as runtime wiring:

- variable lookup
- draw API pointer
- app/user data handle

Use `display_init()` for durable user setup:

- cache image handles
- initialize procedural state
- resolve named resources/views once an API exists

Per-view rendering state should still come from draw contexts, not from global
init. For example, planet-specific state should arrive through
`DcPlanetDrawContext`, not through a global `display_init()` parameter.
