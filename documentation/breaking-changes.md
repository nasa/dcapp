# Breaking changes

Changes on this page may require edits to display XML or Logic code. For Logic API
changes, regenerate `logic/dcapp.h` from the display XML and rebuild the whole
shared library; dcapp does not provide a binary compatibility layer between
generated-header versions.

## Unreleased

### 2026-07-28: planet strokes use `DcStroke` and pixel widths

The six Logic outline calls now take one `DcStroke` instead of separate width
and color arguments:

- `planet_line_local` and `planet_polygon_local`
- `planet_line_geodetic` and `planet_polygon_geodetic`
- `planet_line_cartesian` and `planet_polygon_cartesian`

Planet outline width is now measured in logical display pixels. It stays stable
while zooming and is not affected by `PlanetContainer` scale. `PlanetLine` and
`PlanetPolygon` accept `LinePattern`. GeoJSON `LineWidth` and simplestyle
`stroke-width` affect line and polygon outlines, not the fixed 1000-meter point
markers.

Replace trailing `line_width, color` arguments with:

```c
(DcStroke){
    .color = color,
    .width = 2.0f,
    .pattern = 0xAA,
}
```

Use an explicit planet sphere when a GeoJSON point needs a different
world-space radius.

### 2026-07-27: mouse click semantics

`dc_mouse->clicked()` now requires the captured press to be released while the
pointer is over the target. `released()` still reports a captured release after
the pointer leaves. Code that treated any captured release as a click should
use `released()`.

### 2026-07-27: projected slot-zero texture setter

`DcPlanetApi` adds `set_texture_projected()`. It delegates to
`set_texture_projected_slot()` with slot 0. Code that declares or copies the
API table must rebuild against a regenerated header.

### 2026-07-27: mesh-cache units

`DcPlanetCreateInfo.mesh_cache_size` is now
`DcPlanetCreateInfo.mesh_cache_size_mb`, measured as the combined vertex/index
cache size in MiB, matching XML `MeshCacheSize`.

```c
// old
.mesh_cache_size = 128u * 1024u * 1024u,

// current
.mesh_cache_size_mb = 128u,
```

### 2026-07-27: breadcrumb update results

`update_breadcrumbs_geodetic()` and `update_breadcrumbs_cartesian()` return
`true` only when they append the supplied position. Invalid arguments,
mismatched CRS, non-finite coordinates, and positions below the configured
spacing return `false`. Existing callers may ignore the result.

### 2026-07-27: double-precision planet coordinates

Absolute geodetic and cartesian planet positions and point arrays now use
`DcVec3d`. This includes cartesian cameras, texture centers, overlays,
breadcrumb inputs, and `DcPlanetBreadcrumbsPoints.points`. The generated header
also defines `DcVec2d` and `DcVec4d` with the aliases available on their float
counterparts.

Replace `DcVec3` with `DcVec3d` for absolute planet positions. Keep `DcVec3`
for attitude and light direction; `DcVec2` remains the screen-space and
planet-local type.

### 2026-07-27: planet line-width scaling

Planet outlines no longer use `DCAPP_LINE_WIDTH_FACTOR`. Remove caller-side
`* 1.2f` compensation. This factor remains part of 2D drawing.

This change preceded the screen-space stroke API above. Current planet outline
widths should be supplied directly in logical display pixels.

### 2026-07-27: ellipse draw calls split

`planet_ellipse_geodetic()` and `planet_ellipse_cartesian()` now draw outlines
only. The matching `planet_ellipse_filled_*()` calls draw fills. Color alpha no
longer chooses a pass inside one combined call.

To reproduce an XML ellipse with both colors, submit the fill first and the
outline second.

### 2026-07-27: polygon draw calls split

`planet_polygon_local()`, `planet_polygon_geodetic()`, and
`planet_polygon_cartesian()` now draw outlines only. The
`planet_convex_polygon_filled_*()` calls draw convex fills. Submit the fill
first and outline second when both are needed.

### 2026-07-24: generated Logic ABI cleanup

Generated headers now expose the curated short-name draw, mouse, texture,
planet, and initialization contracts. Internal `DcApp*` declarations are no
longer copied into them.

Other source changes in this ABI revision:

- `DcInit` is the current six-field initialization aggregate. Its old `size`,
  `version`, and duplicate direct `get_variable` fields were removed.
- `dc_planet->clear_texture` takes `(planet, slot)`; the app context argument
  was removed.
- Every XML `Function` and `DrawFunction` gets a typed, C-linked, exported
  declaration. One symbol cannot serve as both callback kinds.
- `dc_place_default()` was removed. Use `(DcPlacement){0}` or a designated
  initializer; directional placement helpers remain.
- `dc_planet_geojson_style_default()` was removed. A zeroed
  `DcPlanetGeojsonStyle` selects renderer fallbacks.
- `dc_planet_view_options_default()` was removed. A zeroed
  `DcPlanetViewOptions` uses the default tau of 0.3.
- `DCAPP_LOGIC_EXTERN` is the multi-translation-unit macro. The previous
  spelling remains a compatibility alias.

Typical replacements:

```c
dc_planet->clear_texture(planet, slot);

DcPlacement placement = {0};
DcPlanetGeojsonStyle style = {0};
DcPlanetViewOptions options = {0};
```

Define `DCAPP_LOGIC_EXTERN` before `#include "dcapp.h"` in additional Logic
translation units.

### 2026-07-20: Logic planet texture slots

`DcPlanetApi` adds slot-aware geodetic and cartesian setters plus per-slot
clear. The original setters remain source-compatible and target slot 0. Rebuild
against the generated header to use slots 1 through 4.

### 2026-06-15: callback `user_data`

Lifecycle callbacks now have these signatures:

```c
void display_init(DcAppContext *app_ctx, void **user_data);
void display_draw(DcAppContext *app_ctx, void *user_data);
void display_close(DcAppContext *app_ctx, void *user_data);
```

`display_init` may assign per-display state through `*user_data`. dcapp passes
that value to later lifecycle callbacks, `Function` callbacks, and
`DrawFunction` callbacks.

`Function`:

```c
void name(DcAppContext *app_ctx, void *user_data);
```

`DrawFunction`:

```c
void name(DcDrawContext *draw_ctx,
          const DcDrawFuncArgs *args,
          void *user_data);
```

### 2026-06-09: `PlanetView CRS` is required

`PlanetView` no longer inherits or infers its camera CRS:

- Geodetic views set `CRS="#_planet_crs_geodetic_"` and provide
  `CameraLatitude`, `CameraLongitude`, and `CameraElevation`.
- Cartesian views set `CRS="#_planet_crs_cartesian_"` and provide `CameraX`,
  `CameraY`, and `CameraZ`.
- Geodetic and cartesian camera attributes cannot be mixed.

`AttitudeFrame` defaults to local-NED for geodetic views and cartesian-RPY for
cartesian views. `CameraHeading` remains a geodetic alias, but `CameraYaw` is
preferred.

### 2026-06-04: generated-header initialization

The generated initialization hook changed from:

```c
display_pre_init(_GetVariableValueAddr);
```

to:

```c
void display_pre_init(const DcInit *init);
```

The old `_GetVariableValueAddr` and `get_pointer` lookup path is not part of
the generated contract. Use:

```c
double *value =
    (double *)dc_app->get_variable(app_ctx, "VariableName");
```

Do not implement `display_pre_init` in ordinary Logic code; the generated
header owns it. Custom replacements must use the current `const DcInit *`
signature.
