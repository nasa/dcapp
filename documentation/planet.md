# Planet rendering

`Planet` defines DEM-backed terrain, textures, and shaders. `PlanetView` draws
that planet inside a panel. Several views can share one planet and select their
own camera, projection, shader, and viewport.

## Data preparation

Raw DEM data must be converted to dcapp's chunked tile cache before rendering.

### Source data

The input is a GeoTIFF or PDS-compatible DEM. The planet sample uses the
LOLA `LDEM_45S_400M` south-polar DEM at 400 meters per pixel. This script
downloads its `.IMG` raster and `.LBL` label:

```bash
./scripts/download-planet-data.sh
```

Generated chunks are written under `data/`.

### `dcapp-planet-chunkgen`

`dcapp-planet-chunkgen` reads the raster with GDAL, divides it into square
tiles, normalizes elevations to 16-bit PNGs, and writes CDLOD quadtree meshes.
Rectangular extents become a rectangular grid of square tiles; partial edge
tiles are padded.

```
dcapp-planet-chunkgen <input_dem> <output_dir> [options]
```

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

```bash
./bin/dcapp-planet-chunkgen.sh /path/to/LDEM_45S_400M.LBL /path/to/output_dir
```

### Output

The tool writes:

- `<prefix>.planet.json` with radius, resolution, grid, elevation range,
  projection, tree depth, and projected tile centers.
- `<prefix>_<col>_<row>.chu` with one CDLOD mesh per tile.

Reference the `.planet.json` file from `PlanetData`.

Chunk generation supports north- and south-polar stereographic/UPS-style DEMs
with non-rotated, square-pixel geotransforms. Metadata records the latitude of
origin, central meridian, scale, false easting, and false northing so terrain
and texture placement use the same projected meters. GDAL band scale metadata
is applied to heights. Rotated/skewed transforms, arbitrary CRS reprojection,
and ellipsoid/geoid terrain baking are not supported.

## Snapshot utility

`dcapp-planet-snapshot` renders preprocessed chunk data to a PNG without XML.
Its explicit CRS and attitude-frame rules match `PlanetView`.

Geodetic camera:

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

Cartesian camera:

```bash
./bin/dcapp-planet-snapshot.sh \
  --planet-data data/LDEM_45S_400M.planet.json \
  --crs cartesian \
  --attitude-frame cartesian-rpy \
  --x 1000000 --y -1000000 --z 2000000 \
  --roll 0 --pitch -30 --yaw 45 \
  --output snapshot.png
```

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

### Position and attitude frames

`--crs` describes how the camera position is expressed. `--attitude-frame` describes the coordinate frame used for attitude. They are separate because spacecraft position and spacecraft attitude are usually reported in different frames.

For geodetic positions, use `--attitude-frame local-ned`. The local frame is built at the camera latitude/longitude:

| Axis | Meaning |
|------|---------|
| `+N` | Local north, tangent to the planet surface |
| `+E` | Local east, tangent to the planet surface |
| `+D` | Local down, toward nadir |

With `local-ned`, `--yaw 0 --pitch 0 --roll 0` points the camera along `+D` at nadir, with image up aligned to `+N` and image right aligned to `+E`. Positive yaw rotates the local north/east image basis about `+D`, positive pitch tilts the boresight toward the yawed image-up direction, and positive roll rotates the image about the boresight.

For Cartesian positions, use `--attitude-frame cartesian-rpy`. The existing `--roll`, `--pitch`, and `--yaw` values are interpreted by the renderer in body-centered Cartesian coordinates.

## XML elements

### Coordinate reference systems

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

For `<PlanetView>`, `CRS` controls which camera attributes are valid. Geodetic
views require `CameraLatitude`/`CameraLongitude`/`CameraElevation`; cartesian
views require `CameraX`/`CameraY`/`CameraZ`. Roll, pitch, and yaw are optional
and default to zero. Child overlays inherit the view CRS unless they set their
own `CRS`.

### `<Planet>`

The top-level planet definition. It must be a direct child of `<DCAPP>` and
must appear before any `<Window>` that references it.

```xml
<Planet Name="Moon" CRS="#_planet_crs_geodetic_"
    LightDirectionX="-1" LightDirectionY="-1" LightDirectionZ="-1">
    <PlanetData File="$DCAPP_HOME/data/LDEM_45S_400M.planet.json"/>
    <PlanetTexture File="$DCAPP_HOME/assets/circle.png" MetersPerPixel="@TexMpp"
        Latitude="-90" Longitude="180" Enabled="@ShowHazard0"
        FireRefresh="@TextureRefresh"/>
    <PlanetTexture File="$DCAPP_HOME/assets/square.png" MetersPerPixel="@TexMpp"
        Latitude="-90" Longitude="180" Enabled="@ShowHazard1"
        FireRefresh="@TextureRefresh"/>
    <PlanetShader Index="1" FragmentShader="shaders/planet_elevation.frag"/>
    <PlanetShader Index="2" FragmentShader="shaders/planet_slope.frag"/>
</Planet>
```

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `Name` | string | Yes | A unique name used by `<PlanetView>` elements to reference this planet |
| `CRS` | enum | No | Coordinate reference system inherited by child `<PlanetTexture>` elements. Defaults to `#_planet_crs_geodetic_`. |
| `LightDirectionX` | double/var | No | X component of the light direction vector. Default -1. Can be variable-driven. |
| `LightDirectionY` | double/var | No | Y component of the light direction vector. Default -1. Can be variable-driven. |
| `LightDirectionZ` | double/var | No | Z component of the light direction vector. Default -1. Can be variable-driven. |
| `MeshCacheSize` | integer | No | Combined vertex/index cache size in MiB. Logic uses the same unit in `DcPlanetCreateInfo.mesh_cache_size_mb`. |

Children: `PlanetData`, `PlanetTexture`, and `PlanetShader`.

Logic-created planets can update the same runtime lighting with
`dc_planet->set_light_direction(planet, direction)`.

### `<PlanetData>`

Specifies the preprocessed terrain data for a planet. Must be a child of `<Planet>`.

```xml
<PlanetData File="../../data/LDEM_45S_400M.planet.json"/>
```

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `File` | string | Yes | Path to the `.planet.json` metadata file produced by `dcapp-planet-chunkgen`. Resolved relative to the XML file's directory. |

### `<PlanetTexture>`

Overlays an image onto the planet surface at a specific geographic location. Must be a child of `<Planet>`. A planet may contain up to five texture overlays. Their internal slots are assigned by declaration order; there is no XML slot/index attribute. Overlapping textures are combined additively.

```xml
<PlanetTexture File="$DCAPP_HOME/assets/circle.png" MetersPerPixel="@TexMpp"
    Latitude="-90" Longitude="180" Enabled="@ShowHazard0"
    FireRefresh="@TextureRefresh"/>
```

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `File` | string/variable | Yes | Path to the image file (PNG, etc.) to overlay on the terrain. The path is passed through unchanged; use `$DCAPP_HOME` or `$DCAPP_DISPLAY_HOME` in XML to supply an absolute path. |
| `CRS` | enum | No | Coordinate reference system for the texture center. Inherits from `<Planet>`. |
| `MetersPerPixel` | double/variable | Yes | Resolution of the texture in meters per pixel. Controls how large the image appears on the surface. |
| `Latitude` | double/variable | Yes for geodetic CRS | Latitude of the texture center in degrees |
| `Longitude` | double/variable | Yes for geodetic CRS | Longitude of the texture center in degrees |
| `X`, `Y`, `Z` | double/variable | Yes for cartesian CRS | Texture center in renderer-native Cartesian meters |
| `OriginX`, `OriginY` | double/variable | Optional override | Texture center in projected terrain meters. If both are set, they override `Latitude`/`Longitude` and `X`/`Y`/`Z`. Both attributes must be provided together. |
| `Enabled` | boolean/variable | No | Loads or removes this overlay independently. Defaults to `true`. Re-enabling rebuilds and uploads the texture. |
| `FireRefresh` | integer/variable | No | Edge-triggered texture reload. When this value changes (e.g., incremented by a button), the texture path, scale, and position are re-read. Useful for dynamically updating the overlay image at runtime. |

`MetersPerPixel` must be greater than zero. The 256 MiB safety limit applies to each generated per-tile texture; increase `MetersPerPixel` or split the overlay if one exceeds that limit. If `File` is variable-backed, change `FireRefresh` after updating the variable to load the new path. Disabling an overlay releases its texture resources, so it no longer consumes texture VRAM; turning it back on performs the normal texture build and upload again. `FireRefresh` affects only its own overlay and is ignored while that overlay is disabled. Runtime texture placement uses the same polar stereographic projected-meter convention as the generated `.planet.json` tile origins. XML `Latitude`/`Longitude` use the same user-facing longitude convention as cameras and overlays; dcapp converts that to terrain projection longitude before calling the planet extension. New chunk metadata uses `originX`/`originY`; older metadata with per-tile `lat`/`lon` is still accepted and converted at load time.

Logic modules can address the same five slots through the generated `dc_planet` API:

```c
// hazard_path is absolute.
dc_planet->set_texture_geodetic_slot(
    app_ctx, planet, 3, hazard_path, lat, lon, meters_per_pixel);

// Releases slot 3's texture resources.
dc_planet->clear_texture(planet, 3);
```

`set_texture_cartesian_slot()` provides the cartesian equivalent, while
`set_texture_projected_slot()` accepts the same projected-meter `OriginX` and
`OriginY` coordinates as XML. The corresponding `set_texture_geodetic()`,
`set_texture_cartesian()`, and `set_texture_projected()` functions target slot
0. Valid slot values are `0` through `DC_PLANET_TEXTURE_SLOT_COUNT - 1`.
Texture setters pass `path` through unchanged and clear the selected slot before
attempting a replacement, once the planet and slot are known to be valid. Callers
should supply an absolute path. Setters return `false` when the replacement
cannot be validated or decoded.

### `<PlanetShader>`

Defines a custom GLSL shader program that can be applied to the terrain. Must be a child of `<Planet>`. Multiple `<PlanetShader>` elements can be defined, each at a different index.

```xml
<PlanetShader Index="1" FragmentShader="shaders/planet_elevation.frag"/>
<PlanetShader Index="2" FragmentShader="shaders/planet_slope.frag"/>
```

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `Index` | integer | Yes | The shader slot number. Index 0 is reserved for the built-in default shader. Custom shaders start at index 1. |
| `VertexShader` | string | No | Path to a custom GLSL vertex shader file. If omitted, the built-in vertex shader is used. |
| `FragmentShader` | string | No | Path to a custom GLSL fragment shader file. If omitted, the built-in fragment shader is used. |

### `<PlanetView>`

Draws a named planet inside a `Panel`.

```xml
<PlanetView Planet="Moon" CRS="#_planet_crs_geodetic_" AttitudeFrame="#_planet_attitude_frame_local_ned_"
    X="15" Y="200" Width="450" Height="450"
    CameraLatitude="@Latitude" CameraLongitude="@Longitude"
    CameraElevation="@Elevation" CameraYaw="@Heading"
    CameraOrthographic="@UseOrtho" ShaderIndex="@ActiveShader"/>
```

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
| `Flatten` | — | boolean/var | No | Draw terrain on the reference sphere while retaining original terrain positions for fragment shaders. Defaults to false. |

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

## Camera frames

For geodetic positions, local-NED attitude defines yaw about local down, pitch
away from nadir, and roll about the boresight. Longitude follows the loaded
planet data; the lunar sample uses east-positive PDS/IAU longitude.

Cartesian positions and cartesian-RPY attitude use the renderer's body-centered
frame directly. Positions are meters and attitude angles are degrees.

`CameraOrthographic="1"` selects parallel projection for either camera mode;
the default is perspective.

## Planet overlays

### `<PlanetContainer>`

Establishes a movable geodetic frame for lines, polygons, and text authored
once in local 2D meters. It must be a direct child of `<PlanetView>` and may
contain `<PlanetLine>`, `<PlanetPolygon>`, and `<PlanetText>` children.

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
    <PlanetText X="0" Y="65" Size="12"
        FillColor="1 0.8 0 1">Landing Site</PlanetText>
</PlanetContainer>
```

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `Latitude` | double/var | Yes | Frame anchor latitude in degrees |
| `Longitude` | double/var | Yes | Frame anchor longitude in degrees |
| `HeightAboveTerrain` | double/var | No | Radial height above the reference sphere in meters. Defaults to 0. |
| `Rotation` | double/var | No | Rotation in degrees. Defaults to 0. |
| `Scale` | double/var | No | Uniform scale applied to local coordinates. Defaults to 1. |
| `Enabled` | boolean/var | No | Enables the container and all of its children. Defaults to true. |

Within the container, vertex and `<PlanetText>` `X` and `Y` values are local
meters: `+X` points east and `+Y` points north at the anchor. `LineWidth`
remains in logical display pixels and does not scale with the container;
`PlanetText` `Size` is in meters and does scale. Scale is applied before
rotation, and positive rotation turns east toward north. Text remains
screen-facing, so rotation moves its anchor without rotating its glyphs.
Container nesting is not supported. The container height applies to every
child; child `CRS` and `HeightAboveTerrain` attributes and local `Latitude`,
`Longitude`, `Altitude`, and `Z` attributes are invalid.

For line and polygon drawing, the C equivalent is a draw-context scope. A
successful push establishes the frame used by subsequent local calls; pop
restores the previous frame:

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
        (DcStroke){
            .color = line_color,
            .width = line_width,
            .pattern = 0xAA,
        });
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
    DcStroke stroke);
void (*planet_polygon_local)(
    DcDrawContext *draw_ctx, const DcVec2 *points, uint32_t point_count,
    DcStroke stroke);
void (*planet_convex_polygon_filled_local)(
    DcDrawContext *draw_ctx, const DcVec2 *points, uint32_t point_count,
    DcVec4 color);
```

Scale must be initialized explicitly; a zero-initialized transform collapses
every point to the anchor. The frame is bound to the supplied draw view and
lives only for the current draw context. Pop should only be called after a
successful push.

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

Only the authored vertices are mapped to the reference sphere. Lines and
outlines remain straight chords, and convex fills remain triangle fans; there
is no automatic subdivision or terrain elevation sampling. Filled polygons
must be convex with vertices in perimeter order. Add authored vertices when a
smoother large curve is needed.

### `<PlanetLine>`

Draws a line strip on the terrain surface.

```xml
<PlanetLine HeightAboveTerrain="1000" LineColor="1 0 0 1"
    LineWidth="2" LinePattern="0xAA">
    <Vertex Latitude="28.6" Longitude="-80.6"/>
    <Vertex Latitude="32.3" Longitude="-64.8"/>
</PlanetLine>
```

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `CRS` | enum | No | Coordinate reference system for child vertices. Inherits from `<PlanetView>`. |
| `HeightAboveTerrain` | double/var | No | Height above the surface in meters |
| `LineColor` | color | No | Line color (RGBA) |
| `LineWidth` | double/var | No | Line width in logical display pixels. Defaults to 1. |
| `LinePattern` | integer/var | No | 8-bit dash pattern, such as `0xAA` dashed. Defaults to solid. |
| `Enabled` | boolean/var | No | Enables drawing. Defaults to true. |

Children are `Vertex` elements with either `Latitude`/`Longitude` or cartesian
`X`/`Y`/`Z` attributes.

### `<PlanetBreadcrumbs>`

Records and draws a live breadcrumb trail from input position variables. The trail is independent of the `<PlanetView>` camera position.

```xml
<PlanetBreadcrumbs Latitude="@VehicleLat" Longitude="@VehicleLon"
    HeightAboveTerrain="500" PointSpacing="25" MaxPoints="2000"
    Clear="@ClearTrail" Enabled="@ShowTrail"
    LineColor="1 0 0 0.5" LineWidth="2" LinePattern="0xF0"/>
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
| `LineWidth` | double/var | No | Line width in logical display pixels |
| `LinePattern` | integer/var | No | 8-bit dash pattern, such as `0xF0` dashed. Defaults to solid. |

Logic breadcrumb update calls return `true` when the supplied position is
appended and `false` when it is invalid or rejected by the configured point
spacing. Existing callers may ignore the result.

### `<PlanetEllipse>`

Draws an ellipse on the terrain surface at a geographic location.

```xml
<PlanetEllipse Latitude="@Lat" Longitude="@Lon" Radius="5000"
    HeightAboveTerrain="500" FillColor="1 0 0 0.3" LineColor="1 0 0 1" LineWidth="2"/>
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
| `LineWidth` | double/var | No | Line width in logical display pixels |
| `Enabled` | boolean/var | No | Enables drawing. Defaults to true. |

After applying `RadiusX` and `RadiusY` overrides, both effective radii must be
greater than zero; otherwise the ellipse is not drawn. Logic calls likewise
require both components of the radius vector to be positive.

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
| `Enabled` | boolean/var | No | Enables drawing. Defaults to true. |

### `<PlanetImage>`

Displays an image at a geographic location on the terrain surface.

```xml
<PlanetImage File="assets/marker.png" Latitude="@Lat" Longitude="@Lon"
    Width="5000" Height="5000" HeightAboveTerrain="1000"/>
```

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `File` | string | No | Path to the image file. Resolved relative to the XML file's directory. The element's text content may be used instead. |
| `CRS` | enum | No | Coordinate reference system for the image position. Inherits from `<PlanetView>`. |
| `Latitude` | double/var | Yes for geodetic CRS | Latitude in degrees |
| `Longitude` | double/var | Yes for geodetic CRS | Longitude in degrees |
| `X`, `Y`, `Z` | double/var | Yes for cartesian CRS | Image position in native body-centered Cartesian meters |
| `Width` | double/var | No | Image width in meters. `DimensionX` and `Size` are aliases. |
| `Height` | double/var | No | Image height in meters. `DimensionY` is an alias. |
| `HeightAboveTerrain` | double/var | No | Height above the surface in meters |
| `TintColor` | color | No | Image tint color (RGBA). `Color` and `FillColor` are aliases. |
| `Enabled` | boolean/var | No | Enables drawing. Defaults to true. |

An image path and at least one dimension are required. If only width or height
is supplied, the other dimension is inferred from the image's aspect ratio.

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
| `Size` | double/var | No | Text size in meters. Defaults to 14. |
| `HeightAboveTerrain` | double/var | No | Height above the surface in meters |
| `FillColor` | color | No | Text color (RGBA) |
| `Enabled` | boolean/var | No | Enables drawing. Defaults to true. |

Text content uses the same variable interpolation syntax as `Text`.

As a direct child of `<PlanetContainer>`, `X` and `Y` are required local
east/north coordinates. The container supplies the geographic frame and
height, and its scale applies to both the position and `Size`. `CRS`,
`Latitude`, `Longitude`, `Z`, and `HeightAboveTerrain` are invalid in this
local form.

### `<PlanetPolygon>`

Draws a filled or outlined polygon on the terrain surface.

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `CRS` | enum | No | Coordinate reference system for child vertices. Inherits from `<PlanetView>`. |
| `HeightAboveTerrain` | double/var | No | Height above the surface in meters |
| `FillColor` | color | No | Fill color (RGBA) |
| `LineColor` | color | No | Line color (RGBA) |
| `LineWidth` | double/var | No | Line width in logical display pixels. Defaults to 1. |
| `LinePattern` | integer/var | No | 8-bit dash pattern for the outline, such as `0xAA` dashed. Defaults to solid. |
| `Enabled` | boolean/var | No | Enables drawing. Defaults to true. |

Children are `Vertex` elements with either `Latitude`/`Longitude` or cartesian
`X`/`Y`/`Z` attributes.

Filled `<PlanetPolygon>` elements must be convex with vertices in perimeter
order. Outline-only polygons do not have that convexity restriction.

Logic uses separate calls for the two passes:
`planet_polygon_geodetic()` and `planet_polygon_cartesian()` draw outlines,
while `planet_convex_polygon_filled_geodetic()` and
`planet_convex_polygon_filled_cartesian()` draw convex fills. Call the fill
first and the outline second to render both like an XML element with
`FillColor` and `LineColor`.

The six Logic line and polygon outline calls—local, geodetic, and
cartesian—take a `DcStroke`. Its width uses logical display pixels and its
8-bit pattern matches `LinePattern`. One pattern cycle spans 20 logical pixels
independently of line width. Bits are read least-significant first; `0` and
`0xFF` are solid. Pattern phase remains continuous along each line and around
each polygon.

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
    LineColor="1 1 0 1" LineWidth="2" FillColor="1 1 0 0.2"/>
```

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `File` | string | Yes | Path to a `.geojson` file (relative to XML directory) |
| `CRS` | enum | No | Only `#_planet_crs_geodetic_` is currently supported for GeoJSON |
| `HeightAboveTerrain` | double/var | No | Fallback altitude in meters for positions without a third coordinate |
| `LineColor` | color | No | Default line color for features without simplestyle properties |
| `LineWidth` | double/var | No | Default line and polygon outline width in logical display pixels |
| `FillColor` | color | No | Default fill color for polygon features |
| `Enabled` | boolean/var | No | Enables drawing. Defaults to true. |

An optional third coordinate supplies each position's altitude. Positions
without an altitude use `HeightAboveTerrain`, or
`DcPlanetGeojsonStyle.height_above_terrain` when drawn through Logic.
Point markers use a fixed 1000-meter radius; `LineWidth` and simplestyle
`stroke-width` apply only to line and polygon outlines.

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
style.line_width = 2.0f;
style.line_color = (DcVec4){.r = 1, .g = 1, .a = 1};
style.fill_color = (DcVec4){.r = 1, .g = 1, .a = 0.2f};

dc_draw->planet_geojson(draw_ctx, view, features, style);
```

Simplestyle values in the file override flagged fallback values. With no
fallback flags, the XML-compatible defaults apply: points are white spheres
with a 1000-meter radius, lines are white at the default width, and unstyled
polygons are not drawn. GeoJSON resources remain valid until app shutdown.

Planet outline widths use logical display pixels. They remain visually stable
as the camera zooms and are not affected by `<PlanetContainer>` scale. Planet
outlines also do not use the 2D line-width compatibility factor.

Logic APIs use `DcVec3d` for absolute geodetic or cartesian planet positions
and point arrays. This includes cartesian cameras, texture centers, overlays,
and both breadcrumb inputs and returned points. `DcVec3` remains the
single-precision type for attitude and light direction, while `DcVec2` remains
the type for screen-space and planet-local coordinates. `DcVec2d`,
`DcVec3d`, and `DcVec4d` expose the same coordinate, color, UV, and component
aliases as their float equivalents.

## Custom shaders

Planet shaders use Vulkan-style GLSL 450. Index 0 is the built-in diffuse
shader; custom shaders use index 1 or higher.

### Runtime selection

Each `PlanetView` selects from its planet's `PlanetShader` definitions with
`ShaderIndex`. The index may be a variable.

```xml
<Variable Type="#_variable_integer_" InitialValue="0">ActiveShader</Variable>

<Planet Name="Moon">
    <PlanetShader Index="1" FragmentShader="shaders/planet_elevation.frag"/>
    <PlanetShader Index="2" FragmentShader="shaders/planet_slope.frag"/>
</Planet>

<PlanetView Planet="Moon" ... ShaderIndex="@ActiveShader"/>
<PlanetView Planet="Moon" ... ShaderIndex="2"/>
```

### Fragment shader interface

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

The sample shaders are
[`planet_elevation.frag`](../samples/planet/shaders/planet_elevation.frag) and
[`planet_slope.frag`](../samples/planet/shaders/planet_slope.frag).

### Flattened views

Use `Flatten="true"` on `<PlanetView>` to render terrain on the reference sphere while still passing the original terrain position to fragment shaders. This means fragment shaders such as `planet_elevation.frag` can still color by real elevation even though the displayed geometry is flat.

```xml
<PlanetView Planet="Moon" Flatten="true" ShaderIndex="1" .../>
```

You can still provide a custom vertex shader through `VertexShader`, but flattening no longer needs one.

## Running the sample

```bash
./scripts/download-planet-data.sh
./bin/dcapp.sh samples/planet/planet.xml
```
