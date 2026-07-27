# dcapp 3D Planet Rendering

A guide to rendering 3D planetary terrain in dcapp using preprocessed heightmap tiles.

---

## Overview

dcapp can render 3D planetary terrain from real digital elevation model (DEM) data. The terrain is displayed as a tiled, level-of-detail mesh on a spherical body, with support for custom shaders, texture overlays, and multiple simultaneous viewports.

The system has a two-part architecture:

- **`<Planet>`** -- Defines the terrain data, texture overlays, and shader programs. This element is declared at the top level of your XML, before any `<Window>`.
- **`<PlanetView>`** -- Renders a viewport into a planet. This element is placed inside a `<Panel>` like any other visual element.

Multiple `<PlanetView>` elements can reference the same `<Planet>` definition, allowing you to show the same terrain from different camera angles, with different projection modes, or at different sizes -- all sharing a single set of terrain data.

---

## When To Use Planet Rendering

Use planet rendering when the display needs real terrain geometry, camera
movement over a spherical body, level-of-detail terrain, or geospatial overlays.
It is designed for DEM-backed terrain views, not simple decorative globes.

Use `Image` for a static map, `Sphere` for a simple textured globe, and
`Planet`/`PlanetView` when terrain height, camera frame, streamed chunks, or
planet overlays matter.

---

## Data Preparation

Before dcapp can render a planet, the raw DEM data must be preprocessed into a chunked tile cache. This is a one-time offline step.

### Source Data

The input is a GeoTIFF or PDS-compatible DEM file. For example, the included planet sample uses LOLA's south polar DEM:

- **LDEM_45S_400M** -- Lunar south pole at 400 meters per pixel, available from the MIT LOLA GDR archive.

The repository-level planet data script downloads both the `.IMG` raster and its `.LBL` label file automatically:

```bash
./scripts/download-planet-data.sh
```

This downloads the DEM and writes generated chunks directly under `data/`.

### The `dcapp-planet-chunkgen` Tool

The `dcapp-planet-chunkgen` command preprocesses a DEM into the chunked tile format that dcapp's planet renderer expects. It reads the raster with GDAL, slices it into square tiles, normalizes elevations to 16-bit PNGs, and then processes each tile into a `.chu` chunk file with a CDLOD quadtree mesh. Rectangular DEM extents are supported as rectangular grids of square tiles; partial edge tiles are padded to the full tile size before processing.

**Usage:**

```
dcapp-planet-chunkgen <input_dem> <output_dir> [options]
```

**Options:**

| Option | Default | Description |
|--------|---------|-------------|
| `--radius N` | Auto-detect from DEM | Planet radius in meters |
| `--tile-size N` | 4096 | Tile dimensions in pixels |
| `--min-height N` | Auto-detect | Minimum elevation in meters |
| `--max-height N` | Auto-detect | Maximum elevation in meters |
| `--meters-per-pixel N` | Auto-detect from DEM | Meters per pixel |
| `--tree-depth N` | 6 | CDLOD quadtree depth |
| `--max-base-error N` | 0.15 * meters_per_pixel | LOD error threshold |
| `--prefix NAME` | Input filename stem | Output naming prefix |
| `--keep-tiles` | Off | Keep intermediate PNG tiles (not deleted after processing) |
| `-h`, `--help` | | Show help |

A convenience wrapper script is provided at `./bin/dcapp-planet-chunkgen.sh`, which handles path resolution automatically:

```bash
./bin/dcapp-planet-chunkgen.sh /path/to/LDEM_45S_400M.LBL /path/to/output_dir
```

### Output

The tool produces:

1. **`<prefix>.planet.json`** -- A metadata file recording the planet radius, meters per pixel, tile grid dimensions, elevation range, tree depth, projection parameters, and the list of tile files with their projected `originX`/`originY` center positions.
2. **`<prefix>_<col>_<row>.chu`** -- One chunk file per tile, containing the CDLOD quadtree mesh data.

The `.planet.json` file is what you reference from the `<PlanetData>` element in your XML.

Chunk generation supports north- and south-polar stereographic/UPS-style DEMs with non-rotated, square-pixel geotransforms. The generated metadata stores latitude of origin, central meridian, scale factor, false easting, and false northing so the baked terrain and runtime texture placement use the same projected-meter convention. GDAL band scale metadata is applied to terrain heights. Rotated/skewed geotransforms, arbitrary CRS reprojection, and ellipsoid/geoid terrain baking are intentionally rejected or deferred.

---

## Snapshot Utility

`dcapp-planet-snapshot` renders a planet directly from preprocessed chunk data and writes a PNG. It does not use XML. `CRS` is explicit, matching `<PlanetView>` camera rules.

**Geodetic camera:**

```bash
./bin/dcapp-planet-snapshot.sh \
  --planet-data data/LDEM_45S_400M.planet.json \
  --crs geodetic \
  --attitude-frame local-ned \
  --lat -58.62 --lon 345.27 --elevation 2000000 \
  --yaw 0 --pitch 0 --roll 0 \
  --fov 60 \
  --output snapshot.png
```

**Cartesian camera:**

```bash
./bin/dcapp-planet-snapshot.sh \
  --planet-data data/LDEM_45S_400M.planet.json \
  --crs cartesian \
  --attitude-frame cartesian-rpy \
  --x 1000000 --y -1000000 --z 2000000 \
  --roll 0 --pitch -30 --yaw 45 \
  --output snapshot.png
```

**Options:**

| Option | Default | Description |
|--------|---------|-------------|
| `--planet-data FILE` | Required | Preprocessed `.planet.json` metadata |
| `--output FILE` | Required | Output PNG |
| `--crs geodetic|cartesian` | Required | Camera coordinate reference system |
| `--attitude-frame local-ned|cartesian-rpy` | Required | Frame used to interpret attitude angles |
| `--lat N --lon N --elevation N` | Required for geodetic | Geodetic camera position |
| `--yaw DEG --pitch DEG --roll DEG` | 0 | Camera attitude in the selected attitude frame |
| `--x N --y N --z N --roll DEG --pitch DEG --yaw DEG` | Required for cartesian | Cartesian camera pose |
| `--width N` | 1024 | Output width |
| `--height N` | 1024 | Output height |
| `--fov DEG` | 60 | Vertical field of view |
| `--vertex-shader FILE` | Built-in | Custom vertex shader |
| `--fragment-shader FILE` | Built-in | Custom fragment shader |

The snapshot utility renders until the current camera view stops queuing tile loads and the output texture has settled, then captures the stable result. Snapshot LOD tau is fixed at `0.05`.

### Snapshot Position And Attitude Frames

`--crs` describes how the camera position is expressed. `--attitude-frame` describes the coordinate frame used for attitude. They are separate because spacecraft position and spacecraft attitude are usually reported in different frames.

For geodetic positions, use `--attitude-frame local-ned`. The local frame is built at the camera latitude/longitude:

| Axis | Meaning |
|------|---------|
| `+N` | Local north, tangent to the planet surface |
| `+E` | Local east, tangent to the planet surface |
| `+D` | Local down, toward nadir |

With `local-ned`, `--yaw 0 --pitch 0 --roll 0` points the camera along `+D` at nadir, with image up aligned to `+N` and image right aligned to `+E`. Positive yaw rotates the local north/east image basis about `+D`, positive pitch tilts the boresight toward the yawed image-up direction, and positive roll rotates the image about the boresight.

For Cartesian positions, use `--attitude-frame cartesian-rpy`. The existing `--roll`, `--pitch`, and `--yaw` values are interpreted by the renderer in body-centered Cartesian coordinates.

---

## XML Elements

### Coordinate Reference Systems

Planet positioning can be expressed in one of these coordinate reference systems:

| Constant | Meaning |
|----------|---------|
| `#_planet_crs_geodetic_` | Latitude/longitude/elevation on the loaded planet body. Latitude and longitude are degrees; heights and sizes are meters. |
| `#_planet_crs_cartesian_` | Renderer-native body-centered Cartesian coordinates in meters. |

Geodetic coordinates use the convention of the loaded planet data. For the included lunar DEM, use the east-positive PDS/IAU longitude from the source map data. The current planet body model is spherical and uses the radius from `<PlanetData>`.

Cartesian coordinates use dcapp's planet renderer frame:

```text
x = R * cos(latitude) * sin(longitude)
y = R * sin(latitude)
z = R * cos(latitude) * cos(longitude)
```

CRS inheritance follows the scene structure:

- `<Planet>` provides the default CRS for child `<PlanetTexture>` elements.
- `<PlanetView>` must set `CRS` explicitly.
- `<PlanetView>` children inherit from their containing `<PlanetView>`.
- A child can override inherited CRS with its own `CRS` attribute.

For `<PlanetView>`, `CRS` controls which camera attributes are valid. Geodetic views require `CameraLatitude`/`CameraLongitude`/`CameraElevation`; cartesian views require `CameraX`/`CameraY`/`CameraZ` plus `CameraRoll`/`CameraPitch`/`CameraYaw`. Child overlays inherit the view CRS unless they set their own `CRS`.

### `<Planet>`

The top-level planet definition. It must be a direct child of `<DCAPP>` and should appear before any `<Window>` element.

```xml
<Planet Name="Moon" CRS="#_planet_crs_geodetic_"
    LightDirectionX="-1" LightDirectionY="-1" LightDirectionZ="-1">
    <PlanetData File="../../data/LDEM_45S_400M.planet.json"/>
    <PlanetTexture File="assets/circle.png" MetersPerPixel="@TexMpp"
        Latitude="-90" Longitude="180" Enabled="@ShowHazard0"
        FireRefresh="@TextureRefresh"/>
    <PlanetTexture File="assets/square.png" MetersPerPixel="@TexMpp"
        Latitude="-90" Longitude="180" Enabled="@ShowHazard1"
        FireRefresh="@TextureRefresh"/>
    <PlanetShader Index="1" FragmentShader="shaders/planet_elevation.frag"/>
    <PlanetShader Index="2" FragmentShader="shaders/planet_slope.frag"/>
</Planet>
```

**Attributes:**

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `Name` | string | Yes | A unique name used by `<PlanetView>` elements to reference this planet |
| `CRS` | enum | No | Coordinate reference system inherited by child `<PlanetTexture>` elements. Defaults to `#_planet_crs_geodetic_`. |
| `LightDirectionX` | double/var | No | X component of the light direction vector. Default -1. Can be variable-driven. |
| `LightDirectionY` | double/var | No | Y component of the light direction vector. Default -1. Can be variable-driven. |
| `LightDirectionZ` | double/var | No | Z component of the light direction vector. Default -1. Can be variable-driven. |
| `MeshCacheSize` | integer | No | Combined vertex/index cache size in MiB. Logic uses bytes in `DcPlanetCreateInfo.mesh_cache_size`. |

**Children:** `<PlanetData>`, `<PlanetTexture>`, `<PlanetShader>`

Logic-created planets can update the same runtime lighting with
`dc_planet->set_light_direction(planet, direction)`.

### `<PlanetData>`

Specifies the preprocessed terrain data for a planet. Must be a child of `<Planet>`.

```xml
<PlanetData File="../../data/LDEM_45S_400M.planet.json"/>
```

**Attributes:**

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `File` | string | Yes | Path to the `.planet.json` metadata file produced by `dcapp-planet-chunkgen`. Resolved relative to the XML file's directory. |

### `<PlanetTexture>`

Overlays an image onto the planet surface at a specific geographic location. Must be a child of `<Planet>`. A planet may contain up to five texture overlays. Their internal slots are assigned by declaration order; there is no XML slot/index attribute. Overlapping textures are combined additively.

```xml
<PlanetTexture File="assets/circle.png" MetersPerPixel="@TexMpp"
    Latitude="-90" Longitude="180" Enabled="@ShowHazard0"
    FireRefresh="@TextureRefresh"/>
```

**Attributes:**

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `File` | string | Yes | Path to the image file (PNG, etc.) to overlay on the terrain |
| `CRS` | enum | No | Coordinate reference system for the texture center. Inherits from `<Planet>`. |
| `MetersPerPixel` | double/variable | Yes | Resolution of the texture in meters per pixel. Controls how large the image appears on the surface. |
| `Latitude` | double/variable | Yes for geodetic CRS | Latitude of the texture center in degrees |
| `Longitude` | double/variable | Yes for geodetic CRS | Longitude of the texture center in degrees |
| `X`, `Y`, `Z` | double/variable | Yes for cartesian CRS | Texture center in renderer-native Cartesian meters |
| `OriginX`, `OriginY` | double/variable | Optional override | Texture center in projected terrain meters. If both are set, they override `Latitude`/`Longitude` and `X`/`Y`/`Z`. Both attributes must be provided together. |
| `Enabled` | boolean/variable | No | Loads or removes this overlay independently. Defaults to `true`. Re-enabling rebuilds and uploads the texture. |
| `FireRefresh` | integer/variable | No | Edge-triggered texture reload. When this value changes (e.g., incremented by a button), the texture path, scale, and position are re-read. Useful for dynamically updating the overlay image at runtime. |

`MetersPerPixel` must be greater than zero. Disabling an overlay releases its texture resources, so it no longer consumes texture VRAM; turning it back on performs the normal texture build and upload again. `FireRefresh` affects only its own overlay and is ignored while that overlay is disabled. Runtime texture placement uses the same polar stereographic projected-meter convention as the generated `.planet.json` tile origins. XML `Latitude`/`Longitude` use the same user-facing longitude convention as cameras and overlays; dcapp converts that to terrain projection longitude before calling the planet extension. New chunk metadata uses `originX`/`originY`; older metadata with per-tile `lat`/`lon` is still accepted and converted at load time.

Logic modules can address the same five slots through the generated `dc_planet` API:

```c
dc_planet->set_texture_geodetic_slot(
    app_ctx, planet, 3, "hazard-ring.png", lat, lon, meters_per_pixel);

// Releases slot 3's texture resources.
dc_planet->clear_texture(planet, 3);
```

`set_texture_cartesian_slot()` provides the cartesian equivalent, while
`set_texture_projected_slot()` accepts the same projected-meter `OriginX` and
`OriginY` coordinates as XML. The original `set_texture_geodetic()` and
`set_texture_cartesian()` functions remain compatible and target slot 0. Valid
slot values are `0` through `DC_PLANET_TEXTURE_SLOT_COUNT - 1`. Texture setters
clear the selected slot before attempting a replacement, once the planet and
slot are known to be valid. They return `false` when the replacement cannot be
resolved, validated, or decoded.

### `<PlanetShader>`

Defines a custom GLSL shader program that can be applied to the terrain. Must be a child of `<Planet>`. Multiple `<PlanetShader>` elements can be defined, each at a different index.

```xml
<PlanetShader Index="1" FragmentShader="shaders/planet_elevation.frag"/>
<PlanetShader Index="2" FragmentShader="shaders/planet_slope.frag"/>
```

**Attributes:**

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `Index` | integer | Yes | The shader slot number. Index 0 is reserved for the built-in default shader. Custom shaders start at index 1. |
| `VertexShader` | string | No | Path to a custom GLSL vertex shader file. If omitted, the built-in vertex shader is used. |
| `FragmentShader` | string | No | Path to a custom GLSL fragment shader file. If omitted, the built-in fragment shader is used. |

### `<PlanetView>`

Renders a viewport into a planet. This element is placed inside a `<Panel>`, just like any other visual element. It references a `<Planet>` by name.

```xml
<PlanetView Planet="Moon" CRS="#_planet_crs_geodetic_" AttitudeFrame="#_planet_attitude_frame_local_ned_"
    X="15" Y="200" Width="450" Height="450"
    CameraLatitude="@Latitude" CameraLongitude="@Longitude"
    CameraElevation="@Elevation" CameraYaw="@Heading"
    CameraOrthographic="@UseOrtho" ShaderIndex="@ActiveShader"/>
```

**Attributes:**

| Attribute | Aliases | Type | Required | Description |
|-----------|---------|------|----------|-------------|
| `Planet` | — | string | Yes | The `Name` of the `<Planet>` element to render |
| `CRS` | — | enum | Yes | Coordinate reference system for the camera position and inherited child overlays. Must be `#_planet_crs_geodetic_` or `#_planet_crs_cartesian_`. |
| `AttitudeFrame` | — | enum | No | Frame used to interpret camera roll/pitch/yaw. Defaults to `#_planet_attitude_frame_local_ned_` for geodetic CRS and `#_planet_attitude_frame_cartesian_rpy_` for cartesian CRS. |
| `ShaderIndex` | — | integer/var | No | Index of the active shader from the parent `<Planet>`'s `<PlanetShader>` library. Defaults to 0 (built-in shader). Each view can independently select its shader. |
| `Tau` | — | double/var | No | LOD error threshold controlling chunk resolution. Default 0.3; non-positive values also use the default. Lower positive values load higher-resolution chunks sooner (more aggressive). Can be variable-driven for runtime adjustment. |
| `PositionX` | `X` | number/var | No | X position relative to parent |
| `PositionY` | `Y` | number/var | No | Y position relative to parent |
| `DimensionX` | `Width` | number/var | No | Viewport width |
| `DimensionY` | `Height` | number/var | No | Viewport height |
| `LocalAlignX` | `HorizontalAlign` | align | No | Horizontal alignment of this element |
| `LocalAlignY` | `VerticalAlign` | align | No | Vertical alignment of this element |
| `ParentAlignX` | — | align | No | Anchor point on parent (horizontal) |
| `ParentAlignY` | — | align | No | Anchor point on parent (vertical) |
| `Rotation` | `Rotate` | number/var | No | Rotation in degrees |
| `PivotPositionX` | `PivotX` | number/var | No | Pivot point X (absolute) |
| `PivotPositionY` | `PivotY` | number/var | No | Pivot point Y (absolute) |
| `PivotParentAlignX` | — | align | No | Pivot parent alignment (horizontal) |
| `PivotParentAlignY` | — | align | No | Pivot parent alignment (vertical) |
| `PivotLocalAlignX` | — | align | No | Pivot alignment (horizontal) |
| `PivotLocalAlignY` | — | align | No | Pivot alignment (vertical) |
| `NegateX` | — | boolean | No | Negate the resolved X position offset |
| `NegateY` | — | boolean | No | Negate the resolved Y position offset |
| `CameraFOV` | — | number/var | No | Vertical field of view in degrees for perspective rendering and orthographic scale derivation. Defaults to 60. |
| `CameraOrthographic` | — | integer/var | No | Set to 1 for orthographic projection, 0 for perspective. Can be variable-driven for runtime toggling. |

**Geodetic Position + Local-NED Attitude**:

| Attribute | Type | Description |
|-----------|------|-------------|
| `CameraLatitude` | double/variable | Camera latitude in degrees |
| `CameraLongitude` | double/variable | Camera longitude in degrees |
| `CameraElevation` | double/variable | Camera elevation above the surface in meters |
| `CameraRoll` | double/variable | Roll in the local-NED attitude frame. Defaults to 0. |
| `CameraPitch` | double/variable | Pitch in the local-NED attitude frame. Defaults to 0. |
| `CameraYaw` | double/variable | Yaw in the local-NED attitude frame. Replaces the legacy `CameraHeading` alias. |
| `CameraFOV` | double/variable | Vertical field of view in degrees. Defaults to 60. |

**Cartesian Position + Cartesian-RPY Attitude**:

| Attribute | Type | Description |
|-----------|------|-------------|
| `CameraX` | double/variable | Camera X position in meters (body-centered Cartesian) |
| `CameraY` | double/variable | Camera Y position in meters |
| `CameraZ` | double/variable | Camera Z position in meters |
| `CameraRoll` | double/variable | Camera roll angle in degrees |
| `CameraPitch` | double/variable | Camera pitch angle in degrees |
| `CameraYaw` | double/variable | Camera yaw angle in degrees |
| `CameraFOV` | double/variable | Vertical field of view in degrees. Defaults to 60. |

Use one position CRS and the matching attitude frame on a given `<PlanetView>`. Initially supported pairs are geodetic + local-NED and cartesian + cartesian-RPY. `CameraHeading` is accepted as a legacy alias for geodetic `CameraYaw`.

---

## Camera Frames

### Geodetic Position + Local-NED Attitude

Geodetic position places the camera using latitude, longitude, and elevation. Local-NED attitude interprets roll, pitch, and yaw in the camera's local north/east/down frame. This is the most intuitive frame pair for exploring terrain interactively or setting up views at known geographic locations.
Longitude follows the loaded planet data's geodetic convention. For the included lunar DEM, use the east-positive PDS/IAU longitude from the source map data; dcapp converts that into the renderer's native Cartesian frame.

```xml
<PlanetView Planet="Moon" CRS="#_planet_crs_geodetic_" AttitudeFrame="#_planet_attitude_frame_local_ned_"
    X="15" Y="200" Width="450" Height="450"
    CameraLatitude="@Latitude" CameraLongitude="@Longitude"
    CameraElevation="@Elevation" CameraYaw="@Heading"/>
```

- **Latitude/Longitude** place the camera above a specific point on the surface.
- **Elevation** controls the height above the surface in meters. Higher values zoom out; lower values bring the camera closer to the terrain.
- **Yaw** rotates the local north/east image basis about the local down vector. Pitch tilts away from nadir, and roll rotates around the camera boresight.

Use geodetic + local-NED when:
- Building interactive terrain browsers with sliders for lat/lon/elevation
- Positioning cameras at known geographic features (craters, landing sites)
- Displaying overhead or oblique views of a specific region

### Cartesian Position + Cartesian-RPY Attitude

Cartesian position places the camera using the renderer-native body-centered Cartesian coordinate system. Cartesian-RPY attitude applies roll, pitch, and yaw in that same cartesian camera frame. This frame pair is typically driven by an external simulation or a logic file that computes camera state.

```xml
<PlanetView Planet="Moon" CRS="#_planet_crs_cartesian_" AttitudeFrame="#_planet_attitude_frame_cartesian_rpy_"
    X="535" Y="200" Width="450" Height="450"
    CameraX="@CamX" CameraY="@CamY" CameraZ="@CamZ"
    CameraRoll="@CamRoll" CameraPitch="@CamPitch" CameraYaw="@CamYaw"/>
```

- **X, Y, Z** specify the camera position in the planet's body-centered coordinate frame (meters).
- **Roll, Pitch, Yaw** specify the camera orientation as Euler angles (degrees).

Use cartesian + cartesian-RPY when:
- The camera state comes from an external simulation (e.g., a vehicle dynamics model via Trick)
- You need precise control over orientation that does not map cleanly to heading
- Implementing chase cameras, cockpit views, or other vehicle-relative perspectives

### Orthographic Projection

Both camera modes support an orthographic projection toggle:

```xml
<PlanetView ... CameraOrthographic="@UseOrtho"/>
```

When `CameraOrthographic` is set to 1, the view uses orthographic (parallel) projection instead of the default perspective projection. This is useful for top-down map-style views where you want consistent scale across the viewport.

---

## Planet Overlays

`<PlanetView>` supports child elements that render geographic overlays on the terrain surface.

### `<PlanetContainer>`

Establishes a movable geodetic frame for lines and polygons authored once in
local 2D meters. It must be a direct child of `<PlanetView>` and may contain
`<PlanetLine>` and `<PlanetPolygon>` children.

```xml
<PlanetContainer Latitude="@LandingLat" Longitude="@LandingLon"
    HeightAboveTerrain="60000" Rotation="@Heading" Scale="2000">
    <PlanetPolygon LineWidth="2.1"
        LineColor="1 0.8 0 1" FillColor="1 0.5 0 0.2">
        <Vertex X="-40" Y="-30"/>
        <Vertex X="40" Y="-30"/>
        <Vertex X="40" Y="10"/>
        <Vertex X="0" Y="50"/>
        <Vertex X="-40" Y="10"/>
    </PlanetPolygon>
</PlanetContainer>
```

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `Latitude` | double/var | Yes | Frame anchor latitude in degrees |
| `Longitude` | double/var | Yes | Frame anchor longitude in degrees |
| `HeightAboveTerrain` | double/var | No | Radial height above the reference sphere in meters. Defaults to 0. |
| `Rotation` | double/var | No | Rotation in degrees. Defaults to 0. |
| `Scale` | double/var | No | Uniform scale applied to local coordinates and line widths. Defaults to 1. |

Within the container, vertex `X` and `Y` are local meters: `+X` points east
and `+Y` points north at the anchor. `LineWidth` is expressed in the same local
units and scales with the container. Scale is applied before rotation, and
positive rotation turns east toward north. Container nesting is not supported.
The container height applies to the entire shape; child `CRS` and
`HeightAboveTerrain` attributes and vertex `Latitude`, `Longitude`, `Altitude`,
and `Z` attributes are invalid in this local scope.

The C equivalent is a draw-context scope. A successful push establishes the
frame used by subsequent local line and polygon calls; pop restores the
previous frame:

```c
typedef struct _DcPlanetLocalTransform {
    float scale;
    float rotation_degrees;
} DcPlanetLocalTransform;

static const DcVec2 doghouse[] = {
    {-40.0f, -30.0f},
    { 40.0f, -30.0f},
    { 40.0f,  10.0f},
    {  0.0f,  50.0f},
    {-40.0f,  10.0f},
};

DcPlanetLocalTransform transform = {
    .scale = 2000.0f,
    .rotation_degrees = heading,
};

if (dc_draw->planet_container_push_geodetic(
        draw_ctx, view, latitude, longitude, height, transform)) {
    dc_draw->planet_convex_polygon_filled_local(
        draw_ctx, doghouse,
        (uint32_t)(sizeof(doghouse) / sizeof(doghouse[0])),
        fill_color);
    dc_draw->planet_polygon_local(
        draw_ctx, doghouse,
        (uint32_t)(sizeof(doghouse) / sizeof(doghouse[0])),
        line_width, line_color);
    dc_draw->planet_container_pop(draw_ctx);
}
```

The available scoped calls are:

```c
bool (*planet_container_push_geodetic)(
    DcDrawContext *draw_ctx, DcDrawPlanetViewHandle view,
    double latitude, double longitude, double height,
    DcPlanetLocalTransform transform);
void (*planet_container_pop)(DcDrawContext *draw_ctx);
void (*planet_line_local)(
    DcDrawContext *draw_ctx, const DcVec2 *points, uint32_t point_count,
    float line_width, DcVec4 color);
void (*planet_polygon_local)(
    DcDrawContext *draw_ctx, const DcVec2 *points, uint32_t point_count,
    float line_width, DcVec4 color);
void (*planet_convex_polygon_filled_local)(
    DcDrawContext *draw_ctx, const DcVec2 *points, uint32_t point_count,
    DcVec4 color);
```

Scale must be initialized explicitly; a zero-initialized transform collapses
every point to the anchor and scales line width to zero. The frame is bound to
the supplied draw view and lives only for the current draw context. Pop should
only be called after a successful push.

Each transformed point is mapped onto the sphere independently:

```text
d = sqrt(x*x + y*y)
tangent = normalize(east*x + north*y)
angle = d / planet_radius
direction = up*cos(angle) + tangent*sin(angle)
world = direction * (planet_radius + height)
```

Local `(0, 0)` maps directly to the anchor. The mapping crosses longitude
boundaries naturally, but remains a local chart; shapes should stay well below
antipodal scale.

The initial implementation maps only the authored vertices to the reference
sphere and then uses the existing line and polygon renderer unchanged. Lines
and outlines remain straight chords, and convex fills remain triangle fans;
there is no automatic subdivision or terrain elevation sampling. Filled
polygons must be convex with vertices in perimeter order. Add authored vertices
when a smoother large curve is needed.

### `<PlanetLine>`

Draws a line strip on the terrain surface.

```xml
<PlanetLine HeightAboveTerrain="1000" LineColor="1 0 0 1" LineWidth="2000">
    <Vertex Latitude="28.6" Longitude="-80.6"/>
    <Vertex Latitude="32.3" Longitude="-64.8"/>
</PlanetLine>
```

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `CRS` | enum | No | Coordinate reference system for child vertices. Inherits from `<PlanetView>`. |
| `HeightAboveTerrain` | double/var | No | Height above the surface in meters |
| `LineColor` | color | No | Line color (RGBA) |
| `LineWidth` | double/var | No | Line width in meters |

**Children:** `<Vertex>` elements with either `Latitude`/`Longitude` or cartesian `X`/`Y`/`Z` attributes.

### `<PlanetBreadcrumbs>`

Records and draws a live breadcrumb trail from input position variables. The trail is independent of the `<PlanetView>` camera position.

```xml
<PlanetBreadcrumbs Latitude="@VehicleLat" Longitude="@VehicleLon"
    HeightAboveTerrain="500" PointSpacing="25" MaxPoints="2000"
    Clear="@ClearTrail" Enabled="@ShowTrail"
    LineColor="1 0 0 0.5" LineWidth="200"/>
```

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `CRS` | enum | No | Coordinate reference system for the sampled position. Inherits from `<PlanetView>`. |
| `Latitude` | double/var | Yes for geodetic CRS | Source latitude in degrees |
| `Longitude` | double/var | Yes for geodetic CRS | Source longitude in degrees |
| `Altitude` | double/var | No | Source altitude in meters. If omitted, `HeightAboveTerrain` is used. |
| `X`, `Y`, `Z` | double/var | Yes for cartesian CRS | Source position in native body-centered Cartesian meters |
| `HeightAboveTerrain` | double/var | No | Geodetic fallback height above the surface in meters |
| `PointSpacing` | double/var | No | Minimum cartesian distance in meters between stored breadcrumb points. Defaults to 1. |
| `MaxPoints` | integer/var | No | Maximum stored points. Defaults to 4096. |
| `Clear` | integer/var | No | Edge-triggered: changing this value clears the stored trail |
| `Enabled` | boolean/var | No | Enables sampling and drawing. Defaults to true. |
| `LineColor` | color | No | Trail color (RGBA). Defaults to semi-transparent red. |
| `LineWidth` | double/var | No | Line width in meters |

### `<PlanetEllipse>`

Draws an ellipse on the terrain surface at a geographic location.

```xml
<PlanetEllipse Latitude="@Lat" Longitude="@Lon" Radius="5000"
    HeightAboveTerrain="500" FillColor="1 0 0 0.3" LineColor="1 0 0 1" LineWidth="200"/>
```

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `CRS` | enum | No | Coordinate reference system for the center. Inherits from `<PlanetView>`. |
| `Latitude` | double/var | Yes for geodetic CRS | Center latitude in degrees |
| `Longitude` | double/var | Yes for geodetic CRS | Center longitude in degrees |
| `X`, `Y`, `Z` | double/var | Yes for cartesian CRS | Center in native body-centered Cartesian meters |
| `Radius` | double/var | No | Radius in meters (shorthand for both RadiusX and RadiusY) |
| `RadiusX` | double/var | No | X radius in meters (overrides Radius) |
| `RadiusY` | double/var | No | Y radius in meters (overrides Radius) |
| `Rotation` | double/var | No | Rotation in degrees |
| `HeightAboveTerrain` | double/var | No | Height above the surface in meters |
| `Segments` | integer/var | No | Number of segments for the ellipse approximation |
| `FillColor` | color | No | Fill color (RGBA) |
| `LineColor` | color | No | Line color (RGBA) |
| `LineWidth` | double/var | No | Line width in meters |

Logic uses separate calls for the two passes:
`planet_ellipse_geodetic()` and `planet_ellipse_cartesian()` draw outlines,
while `planet_ellipse_filled_geodetic()` and
`planet_ellipse_filled_cartesian()` draw fills. Call the fill first and the
outline second to render both like an XML element with `FillColor` and
`LineColor`. Passing `segments == 0` selects the XML default of 64; values are
limited to `DC_PLANET_ELLIPSE_MAX_SEGMENTS`.

### `<PlanetSphere>`

Draws a sphere at a geographic location on the terrain surface.

```xml
<PlanetSphere Latitude="@Lat" Longitude="@Lon" Radius="1000"
    HeightAboveTerrain="500" FillColor="0 1 0 1"/>
```

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `CRS` | enum | No | Coordinate reference system for the center. Inherits from `<PlanetView>`. |
| `Latitude` | double/var | Yes for geodetic CRS | Latitude in degrees |
| `Longitude` | double/var | Yes for geodetic CRS | Longitude in degrees |
| `X`, `Y`, `Z` | double/var | Yes for cartesian CRS | Center in native body-centered Cartesian meters |
| `Radius` | double/var | No | Sphere radius in meters |
| `HeightAboveTerrain` | double/var | No | Height above the surface in meters |
| `FillColor` | color | No | Sphere color (RGBA) |

### `<PlanetText>`

Displays text at a geographic location on the terrain surface.

```xml
<PlanetText Latitude="@Lat" Longitude="@Lon" Size="5000"
    HeightAboveTerrain="1000" FillColor="1 1 1 1">Landing Site</PlanetText>
```

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `CRS` | enum | No | Coordinate reference system for the text position. Inherits from `<PlanetView>`. |
| `Latitude` | double/var | Yes for geodetic CRS | Latitude in degrees |
| `Longitude` | double/var | Yes for geodetic CRS | Longitude in degrees |
| `X`, `Y`, `Z` | double/var | Yes for cartesian CRS | Text position in native body-centered Cartesian meters |
| `Size` | double/var | No | Text size in meters |
| `HeightAboveTerrain` | double/var | No | Height above the surface in meters |
| `FillColor` | color | No | Text color (RGBA) |

**Content:** Text string with variable interpolation (same syntax as `<Text>`).

### `<PlanetPolygon>`

Draws a filled or outlined polygon on the terrain surface.

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `CRS` | enum | No | Coordinate reference system for child vertices. Inherits from `<PlanetView>`. |
| `HeightAboveTerrain` | double/var | No | Height above the surface in meters |
| `FillColor` | color | No | Fill color (RGBA) |
| `LineColor` | color | No | Line color (RGBA) |
| `LineWidth` | double/var | No | Line width in meters |

**Children:** `<Vertex>` elements with either `Latitude`/`Longitude` or cartesian `X`/`Y`/`Z` attributes.

Filled `<PlanetPolygon>` elements must be convex with vertices in perimeter
order. Outline-only polygons do not have that convexity restriction.

Logic uses separate calls for the two passes:
`planet_polygon_geodetic()` and `planet_polygon_cartesian()` draw outlines,
while `planet_convex_polygon_filled_geodetic()` and
`planet_convex_polygon_filled_cartesian()` draw convex fills. Call the fill
first and the outline second to render both like an XML element with
`FillColor` and `LineColor`.

### `<Vertex>`

Defines a point inside `<PlanetLine>` or `<PlanetPolygon>`. The containing line
or polygon determines the CRS unless the primitive explicitly overrides it.
Inside `<PlanetContainer>`, use `X` and `Y` for local east/north meters instead.

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `Latitude` | double/var | Yes for geodetic CRS | Vertex latitude in degrees |
| `Longitude` | double/var | Yes for geodetic CRS | Vertex longitude in degrees |
| `Altitude` | double/var | No | Vertex altitude in meters. If omitted, the parent primitive's `HeightAboveTerrain` is used. |
| `X`, `Y`, `Z` | double/var | Yes for cartesian CRS | Vertex position in native body-centered Cartesian meters |

### `<PlanetGeoJSON>`

Loads a GeoJSON file and renders its features (points, lines, polygons) on the terrain surface. Supports [simplestyle](https://github.com/mapbox/simplestyle-spec) properties (`stroke`, `stroke-opacity`, `stroke-width`, `fill`, `fill-opacity`) from the GeoJSON file, with fallback to XML attribute defaults.

```xml
<PlanetGeoJSON File="assets/features.geojson" HeightAboveTerrain="1000"
    LineColor="1 1 0 1" LineWidth="2000" FillColor="1 1 0 0.2"/>
```

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `File` | string | Yes | Path to a `.geojson` file (relative to XML directory) |
| `CRS` | enum | No | Only `#_planet_crs_geodetic_` is currently supported for GeoJSON |
| `HeightAboveTerrain` | double/var | No | Height above the surface in meters |
| `LineColor` | color | No | Default line color for features without simplestyle properties |
| `LineWidth` | double/var | No | Default line width in meters |
| `FillColor` | color | No | Default fill color for polygon features |

Logic loads the file once as an app-owned resource, then draws it into any
compatible planet view:

```c
DcPlanetGeojsonHandle features =
    dc_planet->load_geojson(app_ctx, "assets/features.geojson");

DcPlanetGeojsonStyle style = {0};
style.flags = DC_PLANET_GEOJSON_STYLE_FLAGS_LINE_COLOR |
              DC_PLANET_GEOJSON_STYLE_FLAGS_FILL_COLOR |
              DC_PLANET_GEOJSON_STYLE_FLAGS_LINE_WIDTH;
style.height_above_terrain = 1000.0;
style.line_width = 2000.0f;
style.line_color = (DcVec4){.r = 1, .g = 1, .a = 1};
style.fill_color = (DcVec4){.r = 1, .g = 1, .a = 0.2f};

dc_draw->planet_geojson(draw_ctx, view, features, style);
```

Simplestyle values in the file override flagged fallback values. With no
fallback flags, the XML-compatible defaults apply: points are white 1000-meter
spheres, lines are white at the default width, and unstyled polygons are not
drawn. GeoJSON resources remain valid until app shutdown.

Logic planet line, polygon, and ellipse functions accept renderer line widths
directly. XML planet primitives multiply their `LineWidth` by 1.2 before
submission, so multiply by `1.2f` when exact side-by-side width matching matters.
`planet_geojson()` applies that XML-compatible scaling itself.

---

## Custom Shaders

The planet renderer uses GLSL shaders (Vulkan-style, version 450) to control how terrain is drawn. Index 0 is always the built-in default shader, which renders the terrain with simple diffuse lighting. Custom shaders are assigned to index 1 and above.

### Switching Shaders at Runtime

The `ShaderIndex` attribute on `<PlanetView>` controls which shader that view uses. By binding it to a variable, you can switch shaders at runtime. The `<PlanetShader>` definitions on `<Planet>` act as a shared library of available shaders; each view independently selects from that library.

```xml
<Variable Type="#_variable_integer_" InitialValue="0">ActiveShader</Variable>

<Planet Name="Moon">
    <PlanetShader Index="1" FragmentShader="shaders/planet_elevation.frag"/>
    <PlanetShader Index="2" FragmentShader="shaders/planet_slope.frag"/>
</Planet>

<!-- Each view can use a different shader -->
<PlanetView Planet="Moon" ... ShaderIndex="@ActiveShader"/>
<PlanetView Planet="Moon" ... ShaderIndex="2"/>
```

Setting `ShaderIndex` to 0 uses the built-in shader. Setting it to 1 or 2 activates the corresponding custom shader. Different views can use different variables or literal values.

### Fragment Shader Interface

Custom fragment shaders receive the following inputs from the vertex stage:

```glsl
layout(location = 0) in struct plShaderIn {
    vec4 tColor;           // LOD debug color
    vec3 tWorldPosition;   // World-space position of the fragment (meters)
    vec3 tWorldNormal;     // World-space surface normal
    vec2 tUV;              // Texture coordinates
} tShaderIn;
```

The dynamic data uniform provides additional information:

- `tDynamicData.tData.tLightDirection` -- Direction toward the light source
- `tDynamicData.tData.tUVInfo`, `tUVInfo1` ... `tUVInfo4` -- Per-overlay UV scale and offset for texture atlas lookup
- `tDynamicData.tData.uTextureIndex`, `uTextureIndex1` ... `uTextureIndex4` -- Per-overlay bindless texture indices
- `tDynamicData.tData.tFlags` -- Flags for wireframe, LOD level, and chunk visualization
- `tDynamicData.tData.iChunkID` -- Chunk identifier (for debug coloring)
- `tDynamicData.tData.fHazardMapStrength` -- Hazard map overlay intensity (default 0.3)

The original unsuffixed fields represent declaration-order slot 0. Existing custom shaders that use only those fields remain compatible and render only the first overlay; custom shaders must sample the four suffixed field pairs to render all five.

The output is a single `vec4` color:

```glsl
layout(location = 0) out vec4 outColor;
```

### Example: Elevation Gradient

The included `planet_elevation.frag` shader computes elevation above a reference radius and maps it to a six-stop color ramp (deep olive through warm white):

```glsl
float elevation = length(tShaderIn.tWorldPosition) - 1737400.0;
float t = clamp((elevation + 8000.0) / 13000.0, 0.0, 1.0);

vec3 c0 = vec3(0.18, 0.30, 0.08);  // deep olive     (lowest basins)
vec3 c1 = vec3(0.42, 0.52, 0.18);  // sage / moss    (low plains)
vec3 c2 = vec3(0.76, 0.65, 0.22);  // golden amber   (mid elevation)
vec3 c3 = vec3(0.72, 0.40, 0.14);  // burnt sienna   (highlands)
vec3 c4 = vec3(0.45, 0.30, 0.18);  // umber brown    (high ridges)
vec3 c5 = vec3(0.95, 0.94, 0.90);  // warm white     (peaks)
```

### Example: Slope Classification

The included `planet_slope.frag` shader classifies terrain by slope angle into three discrete bands:

```glsl
vec3 radial = normalize(tShaderIn.tWorldPosition);
float cosAngle = dot(normal, radial);
float slopeDeg = degrees(acos(clamp(cosAngle, 0.0, 1.0)));

vec3 cFlat     = vec3(0.55, 0.52, 0.48);  // 0-5 deg:  warm stone gray
vec3 cModerate = vec3(0.85, 0.62, 0.15);  // 5-10 deg: golden amber
vec3 cSteep    = vec3(0.78, 0.12, 0.10);  // 10+ deg:  deep crimson

vec3 color = cFlat;
color = mix(color, cModerate, step(5.0, slopeDeg));
color = mix(color, cSteep,    step(10.0, slopeDeg));
```

### Flattened Views

Use `Flatten="true"` on `<PlanetView>` to render terrain on the reference sphere while still passing the original terrain position to fragment shaders. This means fragment shaders such as `planet_elevation.frag` can still color by real elevation even though the displayed geometry is flat.

```xml
<PlanetView Planet="Moon" Flatten="true" ShaderIndex="1" .../>
```

You can still provide a custom vertex shader through `VertexShader`, but flattening no longer needs one.

---

## Complete Example

The `samples/planet/planet.xml` sample demonstrates the full planet rendering pipeline. Here is a walkthrough of its key parts.

### Variables

The sample declares slider-driven camera values, independent shader selection for each view, and one enabled variable per hazard map:

```xml
<Variable Type="#_variable_double_" InitialValue="-58.62">Latitude</Variable>
<Variable Type="#_variable_double_" InitialValue="345.27">Longitude</Variable>
<Variable Type="#_variable_double_" InitialValue="2000000">Elevation</Variable>
<Variable Type="#_variable_double_" InitialValue="0">Heading</Variable>

<Variable Type="#_variable_integer_" InitialValue="1">LeftShader</Variable>
<Variable Type="#_variable_integer_" InitialValue="2">RightShader</Variable>
<Variable Type="#_variable_integer_" InitialValue="0">UseOrtho</Variable>

<Variable Type="#_variable_integer_" InitialValue="1">HazardMap0Enabled</Variable>
<Variable Type="#_variable_integer_" InitialValue="1">HazardMap1Enabled</Variable>
<Variable Type="#_variable_integer_" InitialValue="1">HazardMap2Enabled</Variable>
<Variable Type="#_variable_integer_" InitialValue="1">HazardMap3Enabled</Variable>
<Variable Type="#_variable_integer_" InitialValue="1">HazardMap4Enabled</Variable>
```

### Logic File

A logic file creates the second planet/view through the public C API so it can be compared with the XML-created view:

```xml
<Logic File="logic/logic.so"/>
```

### Planet Definition

The XML planet defines one data source, five same-center texture overlays, and two custom shaders. Each overlay gets its internal slot from this declaration order:

```xml
<Planet Name="Moon" CRS="#_planet_crs_geodetic_"
    LightDirectionX="-1" LightDirectionY="-1" LightDirectionZ="-1">
    <PlanetData File="../../data/LDEM_45S_400M.planet.json"/>
    <PlanetTexture File="assets/circle.png" MetersPerPixel="@TexMpp"
        Latitude="-58.62" Longitude="345.27"
        Enabled="@HazardMap0Enabled" FireRefresh="@TextureRefresh"/>
    <PlanetTexture File="assets/square.png" MetersPerPixel="@TexMpp"
        Latitude="-58.62" Longitude="345.27"
        Enabled="@HazardMap1Enabled" FireRefresh="@TextureRefresh"/>
    <PlanetTexture File="assets/triangle.png" MetersPerPixel="@TexMpp"
        Latitude="-58.62" Longitude="345.27"
        Enabled="@HazardMap2Enabled" FireRefresh="@TextureRefresh"/>
    <PlanetTexture File="assets/ring.png" MetersPerPixel="@TexMpp"
        Latitude="-58.62" Longitude="345.27"
        Enabled="@HazardMap3Enabled" FireRefresh="@TextureRefresh"/>
    <PlanetTexture File="assets/cross.png" MetersPerPixel="@TexMpp"
        Latitude="-58.62" Longitude="345.27"
        Enabled="@HazardMap4Enabled" FireRefresh="@TextureRefresh"/>
    <PlanetShader Index="1" FragmentShader="shaders/planet_elevation.frag"/>
    <PlanetShader Index="2" FragmentShader="shaders/planet_slope.frag"/>
</Planet>
```

### Two PlanetViews

The sample renders two side-by-side geodetic viewports. The left view and its overlays are XML-defined. The right view is drawn by the logic module using the public planet API:

```xml
<PlanetView Planet="Moon" CRS="#_planet_crs_geodetic_"
    AttitudeFrame="#_planet_attitude_frame_local_ned_"
    X="15" Y="300" Width="450" Height="450"
    CameraLatitude="@Latitude" CameraLongitude="@Longitude"
    CameraElevation="@Elevation" CameraYaw="@Heading"
    CameraOrthographic="@UseOrtho" ShaderIndex="@LeftShader"/>

<Container X="535" Y="300" Width="450" Height="450"
    VirtualWidth="450" VirtualHeight="450">
    <DrawFunction Name="draw_logic_planet_view"/>
</Container>
```

Both views consume the same camera variables and display the same area. The
XML planet and the logic-created planet each demonstrate all five texture
slots, and the same five toggle variables independently clear or rebuild the
corresponding slot in both planets.
They also draw the same doghouse from fixed local points at the moving
`OrbitLat`/`OrbitLon` anchor: the left view uses `<PlanetContainer>`, while the
right view uses the matching C push/draw/pop API.

### Interactive Controls

The sample provides sliders for camera and terrain controls, independent shader toggles, a shared texture refresh control, and separate Circle, Square, Triangle, Ring, and Cross hazard-map toggles.

### Running the Sample

```bash
# 1. Prepare the terrain data (downloads DEM, runs chunkgen)
./scripts/download-planet-data.sh

# 2. Run the display
./bin/dcapp.sh samples/planet/planet.xml
```

---

## See Also

- [logic.md](logic.md) -- Writing logic files for custom C/C++ behavior
- [variables.md](variables.md) -- Declaring and using variables in dcapp XML
- [Coordinate frame reference](coordinate-frame.md) -- Pilotlight coordinate system and planet terrain projection
