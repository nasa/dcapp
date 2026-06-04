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

## Data Preparation

Before dcapp can render a planet, the raw DEM data must be preprocessed into a chunked tile cache. This is a one-time offline step.

### Source Data

The input is a GeoTIFF or PDS-compatible DEM file. For example, the included planet sample uses LOLA's south polar DEM:

- **LDEM_45S_100M** -- Lunar south pole at 100 meters per pixel, available from the MIT LOLA GDR archive.

The sample's `setup.sh` script downloads both the `.IMG` raster and its `.LBL` label file automatically:

```bash
cd samples/planet
./setup.sh
```

This downloads the DEM into the `cache/` directory and then runs the chunkgen tool.

### The `dcapp-planet-chunkgen` Tool

The `dcapp-planet-chunkgen` command preprocesses a DEM into the chunked tile format that dcapp's planet renderer expects. It reads the raster with GDAL, slices it into square tiles, normalizes elevations to 16-bit PNGs, and then processes each tile into a `.chu` chunk file with a CDLOD quadtree mesh.

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

A convenience wrapper script is provided at `bin/dcapp-planet-chunkgen.sh`, which handles path resolution automatically:

```bash
bin/dcapp-planet-chunkgen.sh /path/to/LDEM_45S_100M.LBL /path/to/output_dir
```

### Output

The tool produces:

1. **`<prefix>.planet.json`** -- A metadata file recording the planet radius, meters per pixel, tile grid dimensions, elevation range, tree depth, and the list of tile files with their projected `originX`/`originY` center positions.
2. **`<prefix>_<col>_<row>.chu`** -- One chunk file per tile, containing the CDLOD quadtree mesh data.

The `.planet.json` file is what you reference from the `<PlanetData>` element in your XML.

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
- `<PlanetView>` inherits from its `<Planet>` unless camera attributes imply a frame or `CRS` is set explicitly.
- `<PlanetView>` children inherit from their containing `<PlanetView>`.
- A child can override inherited CRS with its own `CRS` attribute.

For `<PlanetView>`, `CRS` is inferred from camera attributes when it is omitted: `CameraLatitude`/`CameraLongitude`/`CameraElevation` selects geodetic, while `CameraX`/`CameraY`/`CameraZ` selects cartesian. Child overlays inherit the view CRS unless they set their own `CRS`.

### `<Planet>`

The top-level planet definition. It must be a direct child of `<DCAPP>` and should appear before any `<Window>` element.

```xml
<Planet Name="Moon" CRS="#_planet_crs_geodetic_"
    LightDirectionX="-1" LightDirectionY="-1" LightDirectionZ="-1">
    <PlanetData File="../../cache/LDEM_45S_100M.planet.json"/>
    <PlanetTexture File="../../assets/nasa-worm.png" MetersPerPixel="@TexMpp"
        Latitude="-90" Longitude="180" FireRefresh="@TextureRefresh"/>
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

**Children:** `<PlanetData>`, `<PlanetTexture>`, `<PlanetShader>`

### `<PlanetData>`

Specifies the preprocessed terrain data for a planet. Must be a child of `<Planet>`.

```xml
<PlanetData File="../../cache/LDEM_45S_100M.planet.json"/>
```

**Attributes:**

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `File` | string | Yes | Path to the `.planet.json` metadata file produced by `dcapp-planet-chunkgen`. Resolved relative to the XML file's directory. |

### `<PlanetTexture>`

Overlays an image onto the planet surface at a specific geographic location. Must be a child of `<Planet>`.

```xml
<PlanetTexture File="../../assets/nasa-worm.png" MetersPerPixel="@TexMpp"
    Latitude="-90" Longitude="180" FireRefresh="@TextureRefresh"/>
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
| `FireRefresh` | integer/variable | No | Edge-triggered texture reload. When this value changes (e.g., incremented by a button), the texture path, scale, and position are re-read. Useful for dynamically updating the overlay image at runtime. |

`MetersPerPixel` must be greater than zero. Runtime texture placement uses the same south-pole stereographic projected-meter convention as the generated `.planet.json` tile origins. XML `Latitude`/`Longitude` use the same user-facing longitude convention as cameras and overlays; dcapp converts that to terrain projection longitude before calling the planet extension. New chunk metadata uses `originX`/`originY`; older metadata with per-tile `lat`/`lon` is still accepted and converted at load time.

### `<PlanetShader>`

Defines a custom GLSL shader program that can be applied to the terrain. Must be a child of `<Planet>`. Multiple `<PlanetShader>` elements can be defined, each at a different index.

```xml
<PlanetShader Index="1" FragmentShader="shaders/planet_elevation.frag"/>
<PlanetShader Index="2" FragmentShader="shaders/planet_slope.frag"/>
<PlanetShader Index="3" VertexShader="shaders/planet_flat.vert"
    FragmentShader="shaders/planet_elevation.frag"/>
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
<PlanetView Planet="Moon" X="15" Y="200" Width="450" Height="450"
    CameraLatitude="@Latitude" CameraLongitude="@Longitude"
    CameraElevation="@Elevation" CameraHeading="@Heading"
    CameraOrthographic="@UseOrtho" ShaderIndex="@ActiveShader"/>
```

**Attributes:**

| Attribute | Aliases | Type | Required | Description |
|-----------|---------|------|----------|-------------|
| `Planet` | — | string | Yes | The `Name` of the `<Planet>` element to render |
| `CRS` | — | enum | No | Coordinate reference system for the camera and inherited child overlays. Inferred from camera attributes when omitted. |
| `ShaderIndex` | — | integer/var | No | Index of the active shader from the parent `<Planet>`'s `<PlanetShader>` library. Defaults to 0 (built-in shader). Each view can independently select its shader. |
| `Tau` | — | double/var | No | LOD error threshold controlling chunk resolution. Default 0.3. Lower values load higher-resolution chunks sooner (more aggressive). Can be variable-driven for runtime adjustment. |
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
| `NegateX` | — | boolean | No | Flip the rendered image horizontally |
| `NegateY` | — | boolean | No | Flip the rendered image vertically |
| `CameraOrthographic` | — | integer/var | No | Set to 1 for orthographic projection, 0 for perspective. Can be variable-driven for runtime toggling. |

**LLE Camera Attributes** (geographic positioning):

| Attribute | Type | Description |
|-----------|------|-------------|
| `CameraLatitude` | double/variable | Camera latitude in degrees |
| `CameraLongitude` | double/variable | Camera longitude in degrees |
| `CameraElevation` | double/variable | Camera elevation above the surface in meters |
| `CameraHeading` | double/variable | Camera heading (look direction) in degrees |

**XYZ/RPY Camera Attributes** (Cartesian positioning):

| Attribute | Type | Description |
|-----------|------|-------------|
| `CameraX` | double/variable | Camera X position in meters (body-centered Cartesian) |
| `CameraY` | double/variable | Camera Y position in meters |
| `CameraZ` | double/variable | Camera Z position in meters |
| `CameraRoll` | double/variable | Camera roll angle in degrees |
| `CameraPitch` | double/variable | Camera pitch angle in degrees |
| `CameraYaw` | double/variable | Camera yaw angle in degrees |

Use one camera mode or the other on a given `<PlanetView>` -- do not mix LLE and XYZ/RPY attributes on the same element.

If `CRS` is omitted, a view with `CameraLatitude`/`CameraLongitude`/`CameraElevation` is treated as geodetic, while a view with `CameraX`/`CameraY`/`CameraZ` is treated as cartesian.

---

## Camera Modes

### LLE Mode (Latitude / Longitude / Elevation)

LLE mode positions the camera using geographic coordinates on the planet surface. This is the most intuitive mode for exploring terrain interactively or setting up views at known geographic locations.
Longitude follows the loaded planet data's geodetic convention. For the included lunar DEM, use the east-positive PDS/IAU longitude from the source map data; dcapp converts that into the renderer's native Cartesian frame.

```xml
<PlanetView Planet="Moon" X="15" Y="200" Width="450" Height="450"
    CameraLatitude="@Latitude" CameraLongitude="@Longitude"
    CameraElevation="@Elevation" CameraHeading="@Heading"/>
```

- **Latitude/Longitude** place the camera above a specific point on the surface.
- **Elevation** controls the height above the surface in meters. Higher values zoom out; lower values bring the camera closer to the terrain.
- **Heading** rotates the camera's look direction around the surface normal at that location.

Use LLE mode when:
- Building interactive terrain browsers with sliders for lat/lon/elevation
- Positioning cameras at known geographic features (craters, landing sites)
- Displaying overhead or oblique views of a specific region

### XYZ/RPY Mode (Cartesian Position / Roll-Pitch-Yaw)

XYZ/RPY mode positions the camera using body-centered Cartesian coordinates and Euler angles. This mode is typically driven by an external simulation or a logic file that computes camera state.

```xml
<PlanetView Planet="Moon" X="535" Y="200" Width="450" Height="450"
    CameraX="@CamX" CameraY="@CamY" CameraZ="@CamZ"
    CameraRoll="@CamRoll" CameraPitch="@CamPitch" CameraYaw="@CamYaw"/>
```

- **X, Y, Z** specify the camera position in the planet's body-centered coordinate frame (meters).
- **Roll, Pitch, Yaw** specify the camera orientation as Euler angles (degrees).

Use XYZ/RPY mode when:
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

### `<Vertex>`

Defines a point inside `<PlanetLine>` or `<PlanetPolygon>`. The containing line or polygon determines the CRS unless the primitive explicitly overrides it.

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
    <PlanetShader Index="3" VertexShader="shaders/planet_flat.vert"
        FragmentShader="shaders/planet_elevation.frag"/>
</Planet>

<!-- Each view can use a different shader -->
<PlanetView Planet="Moon" ... ShaderIndex="@ActiveShader"/>
<PlanetView Planet="Moon" ... ShaderIndex="2"/>
```

Setting `ShaderIndex` to 0 uses the built-in shader. Setting it to 1, 2, or 3 activates the corresponding custom shader. Different views can use different variables or literal values.

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
- `tDynamicData.tData.tUVInfo` -- UV scale and offset for texture atlas lookup
- `tDynamicData.tData.uTextureIndex` -- Bindless texture index for the overlay texture
- `tDynamicData.tData.tFlags` -- Flags for wireframe, LOD level, and chunk visualization
- `tDynamicData.tData.iChunkID` -- Chunk identifier (for debug coloring)
- `tDynamicData.tData.fHazardMapStrength` -- Hazard map overlay intensity (default 0.3)

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

### Custom Vertex Shaders

You can also provide a custom vertex shader. The included `planet_flat.vert` demonstrates flattening terrain to a reference sphere while preserving the original normals for shading:

```glsl
vec3 flatPos = normalize(inPos) * 1737400.0;
gl_Position = tDynamicData.tData.tMvp * vec4(flatPos, 1.0);
tShaderOut.tWorldPosition = flatPos;
tShaderOut.tWorldNormal = Decode(inNormal);
```

This can be paired with any fragment shader. For example, index 3 in the sample combines the flat vertex shader with the elevation fragment shader.

---

## Complete Example

The `samples/planet/planet.xml` sample demonstrates the full planet rendering pipeline. Here is a walkthrough of its key parts.

### Variables

The sample declares slider-driven variables for interactive camera control, plus state variables for shader selection and projection mode:

```xml
<!-- Slider raw values -->
<Variable Type="#_variable_double_" InitialValue="45">SliderLat</Variable>
<Variable Type="#_variable_double_" InitialValue="180">SliderLon</Variable>
<Variable Type="#_variable_double_" InitialValue="200">SliderEle</Variable>

<!-- Computed LLE camera values -->
<Variable Type="#_variable_double_">Latitude</Variable>
<Variable Type="#_variable_double_">Longitude</Variable>
<Variable Type="#_variable_double_">Elevation</Variable>
<Variable Type="#_variable_double_" InitialValue="0">Heading</Variable>

<!-- Computed XYZ/RPY camera values (written by logic file) -->
<Variable Type="#_variable_double_">CamX</Variable>
<Variable Type="#_variable_double_">CamY</Variable>
<Variable Type="#_variable_double_">CamZ</Variable>
<Variable Type="#_variable_double_">CamRoll</Variable>
<Variable Type="#_variable_double_">CamPitch</Variable>
<Variable Type="#_variable_double_">CamYaw</Variable>

<!-- Active shader index: 0=default, 1=elevation, 2=slope -->
<Variable Type="#_variable_integer_" InitialValue="0">ActiveShader</Variable>

<!-- Orthographic projection toggle -->
<Variable Type="#_variable_integer_" InitialValue="0">UseOrtho</Variable>
```

### Logic File

A logic file converts the LLE camera position into XYZ/RPY coordinates so both views show the same scene:

```xml
<Logic File="logic/logic.so"/>
```

### Planet Definition

A single `<Planet>` is defined with one data source, one texture overlay, and three custom shaders. The shader definitions act as a shared library -- each view selects its own active shader via `ShaderIndex`:

```xml
<Planet Name="Moon" CRS="#_planet_crs_geodetic_"
    LightDirectionX="-1" LightDirectionY="-1" LightDirectionZ="-1">
    <PlanetData File="../../cache/LDEM_45S_100M.planet.json"/>
    <PlanetTexture File="../../assets/nasa-worm.png" MetersPerPixel="@TexMpp"
        Latitude="-90" Longitude="180" FireRefresh="@TextureRefresh"/>
    <PlanetShader Index="1" FragmentShader="shaders/planet_elevation.frag"/>
    <PlanetShader Index="2" FragmentShader="shaders/planet_slope.frag"/>
    <PlanetShader Index="3" VertexShader="shaders/planet_flat.vert"
        FragmentShader="shaders/planet_elevation.frag"/>
</Planet>
```

### Two PlanetViews

The sample renders two side-by-side viewports of the same planet -- one using LLE camera mode and one using XYZ/RPY. Both use `ShaderIndex="@ActiveShader"` so the shader buttons affect both views, but each view could use a different variable for independent control:

```xml
<!-- Geodetic camera and overlays (left) -->
<PlanetView Planet="Moon" CRS="#_planet_crs_geodetic_"
    X="15" Y="200" Width="450" Height="450"
    CameraLatitude="@Latitude" CameraLongitude="@Longitude"
    CameraElevation="@Elevation" CameraHeading="@Heading"
    CameraOrthographic="@UseOrtho" ShaderIndex="@ActiveShader">
    <PlanetEllipse Latitude="-58.62" Longitude="-14.73"
        Radius="115385" FillColor="0.0 1.0 1.0 0.10"/>
</PlanetView>

<!-- Cartesian camera and overlays (right) -->
<PlanetView Planet="Moon" CRS="#_planet_crs_cartesian_"
    X="535" Y="200" Width="450" Height="450"
    CameraX="@CamX" CameraY="@CamY" CameraZ="@CamZ"
    CameraRoll="@CamRoll" CameraPitch="@CamPitch" CameraYaw="@CamYaw"
    CameraOrthographic="@UseOrtho" ShaderIndex="@ActiveShader">
    <PlanetEllipse X="-230161.415" Y="-1484128.773" Z="875455.350"
        Radius="115385" FillColor="0.0 1.0 1.0 0.10"/>
</PlanetView>
```

Because the logic file converts LLE to XYZ/RPY, both views display the same camera angle. The two viewports demonstrate that either camera mode can be used depending on your application's needs. The sample also marks known lunar craters in both CRS forms, which is useful for verifying that geodetic and cartesian overlays land in the same place.

### Interactive Controls

The sample provides sliders for latitude, longitude, elevation, and heading, along with toggle buttons for orthographic projection, texture refresh, and shader selection (Default, Elevation, Slope, Flat Map).

### Running the Sample

```bash
# 1. Prepare the terrain data (downloads DEM, runs chunkgen)
cd samples/planet
./setup.sh

# 2. Run the display
dcapp planet.xml
```

---

## See Also

- [logic.md](logic.md) -- Writing logic files for custom C/C++ behavior
- [variables.md](variables.md) -- Declaring and using variables in dcapp XML
- [Coordinate frame reference](coordinate-frame.md) -- Pilotlight coordinate system and planet terrain projection
