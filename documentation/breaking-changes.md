BREAKING CHANGES
================

Source and ABI changes that may require edits outside XML display files.


[Unreleased]
------------

### 2026-07-27 - Double-Precision Planet Coordinates

#### Affected Code
- Logic code that passes absolute planet positions or point arrays through
  `dc_draw` or `dc_planet`.
- Logic code that reads `DcPlanetBreadcrumbsPoints.points`.

#### Changed
- Generated Logic headers now define `DcVec2d`, `DcVec3d`, and `DcVec4d` with
  the same component aliases as their float equivalents.
- Absolute geodetic and cartesian planet vectors now use `DcVec3d`.
  `DcVec3` remains the float type for attitude and light direction.
- Planet coordinate conversions and overlay projection preserve double
  precision until the float-based planet renderer boundary.

#### Migration
- Regenerate `logic/dcapp.h` and rebuild logic modules.
- Replace `DcVec3` with `DcVec3d` for planet world positions and point arrays.
  Do not change `DcVec3` values passed as RPY attitude or light direction.

### 2026-07-27 - Planet Line-Width Scaling

#### Affected Code
- Non-container planet XML and GeoJSON outlines.
- Logic planet line, polygon, and ellipse calls that manually multiplied their
  line width by `1.2f` to match XML rendering.

#### Changed
- Planet outlines no longer use `DCAPP_LINE_WIDTH_FACTOR`; it remains limited
  to 2D drawing.
- Non-container planet widths are submitted directly as world-space values.
  Local-container widths continue to scale only with the container.

#### Migration
- Pass the intended planet line width directly and remove any caller-side
  `* 1.2f` compensation.

### 2026-07-27 - Planet Ellipse Draw API Split

#### Affected Code
- Logic code that calls `planet_ellipse_geodetic` or
  `planet_ellipse_cartesian`.

#### Changed
- Planet ellipse outline and fill rendering now use separate calls.
  `planet_ellipse_*` draws only the outline, and
  `planet_ellipse_filled_*` draws only the fill.
- A call always submits its requested pass; color alpha no longer selects
  which pass a combined call performs.

#### Migration
- Regenerate `logic/dcapp.h` and rebuild logic modules.
- Replace each combined ellipse call with a filled call followed by an outline
  call when both passes are wanted.

### 2026-07-27 - Planet Polygon Draw API Split

#### Affected Code
- Logic code that calls `planet_polygon_local`, `planet_polygon_geodetic`, or
  `planet_polygon_cartesian`.
- Plugins or tools that fetch `plPlanetI` by version or call
  `draw_polygon_filled`.

#### Changed
- Planet polygon outline and fill rendering now use separate calls.
  `planet_polygon_*` draws only the outline, and
  `planet_convex_polygon_filled_*` draws only the convex fill.
- A call always submits its requested pass; color alpha no longer selects
  which pass a combined call performs.
- `plPlanetI.draw_polygon_filled` was renamed to
  `draw_convex_polygon_filled`, and `plPlanetI_version` changed from
  `{0, 8, 0}` to `{0, 9, 0}`.

#### Migration
- Regenerate `logic/dcapp.h` and rebuild logic modules.
- Replace each combined polygon call with a convex-fill call followed by an
  outline call when both passes are wanted.
- Rebuild `plPlanetI` consumers, request version `{0, 9, 0}`, and rename calls
  to `draw_convex_polygon_filled`.

### 2026-07-24 - Current Generated Logic ABI

#### Affected Code
- Logic libraries built from an older generated `logic/dcapp.h`.
- Logic code that calls `dc_planet->clear_texture`.
- Logic code that calls the generated `dc_place_*`,
  `dc_planet_geojson_style_default`, or
  `dc_planet_view_options_default` convenience functions.
- Multi-file logic builds that define the old `_DCAPP_LOGIC_EXTERN_` macro.

#### Changed
- Generated headers now expose only the explicitly curated short-name draw,
  mouse, texture, planet, and initialization contracts. Internal `DcApp*`
  declarations are no longer copied into logic headers.
- `DcInit` is the exact current six-field initialization aggregate. The old
  `size`, `version`, and duplicate direct `get_variable` fields were removed.
- `DcPlanetApi.clear_texture` now takes only `(planet, slot)`; the planet handle
  already identifies the owning resource.
- Every XML `Function` and `DrawFunction` receives a typed, C-linked exported
  declaration. One name cannot be used for both callback kinds.
- The generated placement convenience functions were removed. A zeroed
  `DcPlacement` is already the default; use a designated initializer when
  alignment or pivot fields are needed.
- `dc_planet_geojson_style_default` was removed because a zeroed
  `DcPlanetGeojsonStyle` already selects the renderer's fallback behavior.
- `dc_planet_view_options_default` was removed. A non-positive `tau` now
  selects the renderer's default value of `0.3`.
- `DCAPP_LOGIC_EXTERN` is the public multi-translation-unit macro. The old
  spelling remains accepted as a compatibility alias.

#### Migration
- Regenerate `logic/dcapp.h` and rebuild the complete logic library.
- Change `dc_planet->clear_texture(dc_app_ctx, planet, slot)` to
  `dc_planet->clear_texture(planet, slot)`.
- Replace `dc_place_default()` with `(DcPlacement){0}`. The directional
  placement helpers remain available.
- Replace `dc_planet_geojson_style_default()` with
  `(DcPlanetGeojsonStyle){0}` before setting any desired fallback flags.
- Replace `dc_planet_view_options_default()` with
  `(DcPlanetViewOptions){0}` before setting any desired flags.
- Define `DCAPP_LOGIC_EXTERN` before including `dcapp.h` in additional logic
  translation units.

### 2026-07-20 - Planet Extension API 0.7.0

#### Affected Code
- Plugins or tools that fetch `plPlanetI` by version.
- Custom planet fragment shaders that want to render texture slots 1 through 4.
- Logic modules that want to configure texture slots 1 through 4.

#### Changed
- `plPlanetI_version` changed from `{0, 6, 0}` to `{0, 7, 0}`.
- The existing `plPlanetI.set_texture(..., index)` parameter now selects one
  of five independent slots instead of being ignored.
- `plGpuDynPlanetData` adds four texture indices and four UV transforms.
- `DcPlanetApi` adds slot-aware geodetic/cartesian texture setters and a
  per-slot clear function, grouped with the existing texture controls.

#### Migration
- Rebuild consumers and request `plPlanetI` version `{0, 7, 0}`.
- Existing custom shaders continue to render slot 0. To render every overlay,
  sample `uTextureIndex1` through `uTextureIndex4` with their matching
  `tUVInfo1` through `tUVInfo4` values.
- Regenerate `logic/dcapp.h` and rebuild logic modules. The original setters
  remain source-compatible and continue to target slot 0.

### 2026-06-12 - Planet Extension API 0.6.0

#### Affected Code
- Plugins or tools that fetch `plPlanetI` by version or compile against the
  `plPlanetI` function table.

#### Changed
- `plPlanetI_version` changed from `{0, 5, 0}` to `{0, 6, 0}`.
- `plPlanetStreamStats` was added.
- `plPlanetI.get_stream_stats(plPlanet*)` was added to expose pending request,
  resident chunk, and total chunk counts.

#### Migration
- Rebuild consumers that compile against `plPlanetI`.
- Consumers using strict API version checks should request `plPlanetI` version
  `{0, 6, 0}` when they need stream statistics.

### 2026-06-09 - PlanetView CRS Is Required

#### Affected XML
- Displays with `<PlanetView>` elements that omitted `CRS`.
- Displays that relied on camera attributes to infer geodetic or cartesian mode.

#### Changed
- `<PlanetView>` no longer inherits or infers CRS.
- `CRS` is required and must be `#_planet_crs_geodetic_` or `#_planet_crs_cartesian_`.
- `AttitudeFrame` is now supported and defaults from `CRS` when omitted.
- Geodetic views require `CameraLatitude`, `CameraLongitude`, and `CameraElevation`.
- Cartesian views require `CameraX`, `CameraY`, and `CameraZ`.
- Mixing geodetic and cartesian camera attributes is now invalid.

#### Migration
- Add `CRS="#_planet_crs_geodetic_"` to geodetic PlanetViews. Add `AttitudeFrame="#_planet_attitude_frame_local_ned_"` if you want to be explicit.
- Add `CRS="#_planet_crs_cartesian_"` to cartesian PlanetViews. Add `AttitudeFrame="#_planet_attitude_frame_cartesian_rpy_"` if you want to be explicit.
- Prefer `CameraYaw` over the legacy `CameraHeading` alias.
- Remove mismatched camera attributes.

### 2026-06-15 - Logic Callback User Data ABI

#### Affected Code
- Logic shared libraries compiled against older generated `logic/dcapp.h` files.
- User code implementing `display_init`, `display_draw`, `display_close`,
  `<Function>` callbacks, or `<DrawFunction>` callbacks.

#### Changed
- `display_init` now receives `DcAppContext *app_ctx` and `void **user_data`.
  Set `*user_data` there to store app-owned logic state.
- `display_draw`, `display_close`, `<Function>` callbacks, and
  `<DrawFunction>` callbacks now receive the stored `void *user_data`.
- `<Function>` callbacks also receive `DcAppContext *app_ctx`.
- `<DrawFunction>` callbacks now receive `void *user_data` after the draw
  context and argument list.

#### Migration
- Regenerate `logic/dcapp.h` and rebuild logic shared libraries.
- Update lifecycle callbacks to:
  `display_init(DcAppContext *app_ctx, void **user_data)`,
  `display_draw(DcAppContext *app_ctx, void *user_data)`, and
  `display_close(DcAppContext *app_ctx, void *user_data)`.
- Update `<Function>` callbacks to
  `void name(DcAppContext *app_ctx, void *user_data)`.
- Update `<DrawFunction>` callbacks to
  `void name(DcDrawContext *draw_ctx, const DcDrawFuncArgs *args, void *user_data)`.

### 2026-06-04 - Logic Header Initialization ABI

This section compares the current logic API against the public logic API that
existed before the DrawFunction logic API work. It intentionally does not list
short-lived intermediate header or bootstrap shapes from that implementation
work.

#### Affected Code
- Logic shared libraries compiled against an older generated `logic/dcapp.h`.
- User code that manually declared or called generated logic internals such as
  `display_pre_init`, `_GetVariableValueAddr`, or `get_pointer`.

#### Changed
- The generated logic initialization hook changed from
  `display_pre_init(_GetVariableValueAddr)` to
  `display_pre_init(const DcInit *)`.
- Generated logic headers replaced the old `_GetVariableValueAddr` /
  `get_pointer` variable lookup path with `dc_app->get_variable(app_ctx, "VariableName")`.
- The generated header no longer declares `get_pointer` as the public/manual
  variable lookup escape hatch. Manual lookups should use `dc_app->get_variable()`.

#### Migration
- Regenerate `logic/dcapp.h` and rebuild logic shared libraries.
- If user code called the old generated lookup pointer directly, update it to `dc_app->get_variable(app_ctx, "VariableName")`.
- Remove user-maintained declarations of `_GetVariableValueAddr` and
  `get_pointer`; those names are no longer part of the generated header
  contract.
- Do not implement `display_pre_init` in user logic code unless you are
  deliberately replacing generated-header initialization. The generated
  `logic/dcapp.h` owns that hook.
- If custom user code really does implement `display_pre_init`, update its
  signature to `void display_pre_init(const DcInit *init)`.

#### Excluded Intermediate Changes
- Earlier same-day DrawFunction mouse and texture helper changes are not listed
  here because they were intermediate implementation states, not migration steps
  from the previous public logic API.
