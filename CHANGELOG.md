CHANGELOG
=========

All notable changes to dcapp are documented in this file.
Format based on Keep a Changelog. Uses Semantic Versioning (2.MINOR.PATCH).


[2.150.0] - 2026-06-04
--------------------

### Changed
- Reduced the public sample set by removing obsolete benchmark/stress fixtures and keeping `bad-sample` as a validation-only sample.
- Removed thin or overlapping samples now covered by stronger XML, stencil, DrawFunction, or documentation examples.
- Reworked `lissajous` into a compact DrawFunction sample instead of generated XML variables.
- Made sample Panel backgrounds explicit so they do not depend on the renderer clear color.
- Reorganized the sample documentation around core XML, C logic, advanced/integration, and showcase samples.
- Removed the standalone `functions` sample after validating that the current Function element is better documented than demonstrated as a public sample.


[2.149.0] - 2026-06-04
--------------------

### Fixed
- Changed `drawfunction3` stars from capped streaks to dots with occasional cross-sparkles for cleaner sample visuals.


[2.148.0] - 2026-06-04
--------------------

### Changed
- Replaced the `drawfunction3` bubble demo with a simpler procedural starfield controlled by XML `+` / `-` buttons.
- Updated the `drawfunction3` sample documentation for the starfield behavior.


[2.147.0] - 2026-06-04
--------------------

### Changed
- Simplified the `drawfunction3` bubble collision code to use equal separation and a velocity swap along the contact normal.


[2.146.0] - 2026-06-04
--------------------

### Changed
- Simplified the `drawfunction3` bubble demo visuals by removing the click ripple, animated background lines, and pulse effect while keeping the mouse push behavior.


[2.145.0] - 2026-06-04
--------------------

### Fixed
- Aligned the `drawfunction3` bubble-count controls and centered the C-rendered annotation text inside its translucent backdrop.


[2.144.0] - 2026-06-04
--------------------

### Added
- Added new sample: `drawfunction4`.
- Moved the original full-logic procedural panel demo into `drawfunction4` as the example where one C DrawFunction owns the entire panel.
- Updated sample documentation for `drawfunction4`.


[2.143.0] - 2026-06-04
--------------------

### Changed
- Reworked the `drawfunction3` sample into a hybrid XML/C bubble simulation.
- XML now owns the bubble-count controls while C generates, simulates, collides, and draws the runtime bubble field.
- Updated the samples documentation for the new `drawfunction3` behavior.


[2.142.0] - 2026-06-04
--------------------

### Removed
- Removed the ellipse-specific cells from the `drawfunction2` sample to keep the reference grid at 36 examples.


[2.141.0] - 2026-06-04
--------------------

### Added
- Added DrawFunction ellipse support to the generated logic API.
- Added `dc_draw->ellipse`, `dc_draw->ellipse_filled`, `dc_mouse->ellipse`, and `dc_mouse->ellipse_ex`.
- Updated the DrawFunction 2 sample to demonstrate ellipse drawing and mouse hit testing.


[2.140.0] - 2026-06-04
--------------------

### Changed
- Cleaned up the DrawFunction mouse API so basic hit targets use plain geometry inputs.
- Moved placement-aware mouse hit registration to `_ex` functions.
- Changed `dc_mouse->down(ctx, id)` to the ID-free `dc_mouse->down(ctx)`.

### Fixed
- Updated DrawFunction samples for the new mouse API.
- Documented the mouse API source break in `documentation/breaking-changes.md`.


[2.139.0] - 2026-06-04
--------------------

### Added
- Added `.gitattributes` for repository line-ending normalization.

### Fixed
- Normalized line endings in planet extension and shader sources.


[2.138.0] - 2026-06-04
--------------------

### Changed
- Updated planet texture placement to use projected `OriginX` / `OriginY` metadata.
- Updated planet chunk generation to write projected tile origins.
- Kept compatibility with older chunk metadata that stores latitude/longitude tile origins.
- Updated planet projection/origin documentation and sample XML.

### Fixed
- Fixed planet texture projection and origin handling.


[2.137.0] - 2026-06-04
--------------------

### Changed
- Updated code and generated build scripts for PilotLight API drift.
- Removed obsolete app/xml calls that no longer matched the updated PilotLight startup flow.

### Fixed
- Fixed Windows build script drift after PilotLight API updates.


[2.136.0] - 2026-06-04
--------------------

### Added
- Added DrawFunction image rendering through `dc_draw->image`.
- Added texture loading helpers exposed to logic code: `dc_load_image()` and `dc_get_texture_size()`.
- Added shared texture helpers used by XML image/sphere rendering and DrawFunction image rendering.
- Updated the DrawFunction 2 sample with image rendering coverage.


[2.135.0] - 2026-06-04
--------------------

### Added
- Added initial DrawFunction support for procedural drawing from logic files.
- Added generated DrawFunction types and APIs to `dcapp-genheader`.
- Added `<DrawFunction>` XML parsing with typed `<Arg>` values.
- Added `draw.h`, `draw_node.c`, and `draw_node.h` to share drawing paths between XML nodes and DrawFunction callbacks.
- Added DrawFunction samples: `drawfunction1`, `drawfunction2`, and `drawfunction3`.
- Added DrawFunction documentation and roadmap notes.

### Changed
- Changed logic initialization from the old variable lookup callback ABI to `display_pre_init(const DcInit *)`.
- Moved node drawing out of `draw.c` into `draw_node.c`.
- Updated sample build scripts to build logic samples with generated headers.


[2.134.0] - 2026-06-03
--------------------

### Changed
- Updated planet CRS handling and chunk processing for projected terrain origins.
- Updated planet extension and processor headers for the CRS update.
- Removed generated tool entries that no longer apply to the updated planet processor flow.


[2.133.0] - 2026-06-02
--------------------

### Fixed
- Fixed static-analysis workflow configuration.


[2.132.0] - 2026-06-02
--------------------

### Changed
- Updated PilotLight to version 0.9.3.
- Updated app, extension, tool, and sample build scripts for the new PilotLight layout and API.


[2.131.0] - 2026-05-29
--------------------

### Added
- Added `FrameRateLimit`, `MaxFPS`, and `MaxFrameRate` attributes to Window elements.
- Added runtime frame pacing support in the window draw loop.
- Updated validation and primitive documentation for frame-rate limits.


[2.130.0] - 2026-05-29
--------------------

### Changed
- Updated build scripts to rebuild PilotLight only when its commit changes.

### Fixed
- Fixed ImGui build issues in the top-level build scripts.


[2.129.0] - 2026-05-29
--------------------

### Added
- Added Panel `BackgroundColor` support.
- Added Text `BackgroundColor` support.
- Added `DCAPP_LINE_WIDTH_FACTOR` compatibility support for line-width scaling.

### Changed
- Updated migration and primitive documentation for legacy background-color behavior.


[2.128.0] - 2026-05-29
--------------------

### Added
- Added planet CRS constants for geodetic and cartesian coordinates.
- Added `CRS` attributes for Planet, PlanetTexture, PlanetView, and planet overlay primitives.
- Added cartesian `X`, `Y`, and `Z` support for PlanetEllipse, PlanetLine, PlanetPolygon, PlanetSphere, and PlanetText.
- Updated planet validation, documentation, and sample XML for CRS selection.


[2.127.0] - 2026-05-20
--------------------

### Added
- Added a thin cross-platform shared library loader in `src/utils/library.c`.

### Fixed
- Fixed logic library loading so XML can specify `.so`, `.dylib`, `.dll`, or an extensionless path across platforms.
- Updated app build scripts to use the new loader.


[2.126.0] - 2026-04-23
--------------------

### Changed
- Updated PilotLight build integration and app build scripts for the newer PilotLight version.


[2.125.0] - 2026-04-23
--------------------

### Changed
- Updated PilotLight submodule version.
- Updated draw backend and terrain tool code for PilotLight API changes.


[2.124.0] - 2026-04-20
--------------------

### Changed
- Changed Window `Fullscreen` storage from integer to boolean.
- Updated XML parsing so `Fullscreen="true"` enables fullscreen mode.


[2.123.0] - 2026-04-17
--------------------

### Added
- Added `Fullscreen` support for Window elements.


[2.122.0] - 2026-04-16
--------------------

### Changed
- Updated PilotLight for upcoming fullscreen support.
- Adjusted draw code for the updated PilotLight APIs.


[2.121.0] - 2026-04-12
--------------------

### Fixed
- Fixed GitHub Actions build workflow issues.
- Expanded CI build coverage and corrected workflow command behavior.


[2.120.0] - 2026-04-12
--------------------

### Added
- Added GitHub Actions build workflow.
- Added GitHub Actions static-analysis workflow.

### Changed
- Updated clang-format settings for CI.


[2.119.0] - 2026-04-12
--------------------

### Added
- Added `.clang-tidy` static-analysis configuration.

### Fixed
- Fixed static-analysis warnings in app, draw, XML, and draw extension code.


[2.118.0] - 2026-04-10
--------------------

### Fixed
- Fixed the double vector struct name after the PilotLight update.


[2.117.0] - 2026-04-10
--------------------

### Changed
- Updated PilotLight submodule commit for macOS build fixes.


[2.116.0] - 2026-04-10
--------------------

### Fixed
- Fixed the double vector struct name after the PilotLight update.


[2.115.0] - 2026-04-10
--------------------

### Changed
- Updated PilotLight submodule commit for macOS build fixes.


[2.114.0] - 2026-04-09
--------------------

### Added
- Added double-precision support to planet rendering and terrain preprocessing paths.
- Updated planet shaders, processor extension, terrain tool, and chunk generator for double support.


[2.113.0] - 2026-04-09
--------------------

### Changed
- Updated PilotLight submodule commit in preparation for double support.
- Updated draw code for the matching PilotLight API.


[2.112.0] - 2026-04-07
--------------------

### Added
- Added Text `LineColor` outline rendering.
- Added Text `Bold` and `Italic` styling support.
- Added SDF bold and outline shaders.
- Expanded the fonts sample with bold, italic, outline, and combined style examples.


[2.111.0] - 2026-04-03
--------------------

### Added
- Added `LinePattern` support for dashed outlines on Line, Rectangle, Ellipse, Polygon, and Arc elements.
- Added dash-pattern support to the 2D draw extension and stencil shader path.
- Added new sample: `dashes`.


[2.110.0] - 2026-04-02
--------------------

### Added
- Added separate SDF render-size tiers per custom font.

### Fixed
- Fixed custom font rendering quality across different requested text sizes.


[2.109.0] - 2026-04-02
--------------------

### Fixed
- Fixed custom font SDF packing and glyph rect indexing.
- Fixed custom font atlas offsets for multiple SDF font configs.


[2.108.0] - 2026-04-01
--------------------

### Fixed
- Fixed text size calculation so trailing whitespace contributes to measured width.
- Fixed Text parsing so leading and trailing whitespace in node content is preserved.
- Fixed Constant preprocessing so significant leading and trailing whitespace is preserved.


[2.107.0] - 2026-03-22
--------------------

### Fixed
- Hid PilotLight screen debug logging in release builds.
- Added `NDEBUG` to generated release build profiles for apps and samples.


[2.106.0] - 2026-03-22
--------------------

### Added
- Custom font support: Text elements accept a Font attribute with a path to a TTF file
- Bundled Hack (MIT) and Liberation Sans (SIL OFL) fonts in assets/fonts/
- New sample: fonts

### Changed
- Removed Font from convert-legacy-xml.py "not yet implemented" list


[2.105.0] - 2026-03-22
--------------------

### Fixed
- Updated README.md script names to include .sh extensions and added dcapp-planet-chunkgen
- Updated documentation for new features and corrected script references
- Added missing planet overlay elements to dcapp-validate
- Fixed dcapp-validate to allow MouseMotion and Function inside True/False blocks
- Added MeshCacheSize to Planet validator


[2.104.0] - 2026-03-22
--------------------

### Added
- Rounded corners for Rectangle and Polygon elements via Rounded attribute
- Rounded polygon draw functions in dc_draw extension (add_polygon_rounded, add_convex_polygon_rounded_filled)
- New sample: rounded

### Changed
- Updated dc_draw extension version to 1.5.0


[2.103.0] - 2026-03-22
--------------------

### Changed
- Moved node type definitions from dcapp.h to node.h
- Added _ValIndex typedef (replacing _ValIndex1) for consistency with _ValIndex2/3/4
- Replaced DcAppValIndex with _ValIndex throughout node.h for readability


[2.102.0] - 2026-03-22
--------------------

### Added
- PlanetGeoJSON element for rendering GeoJSON features on terrain
- PlanetLine element for rendering line strips on terrain with geographic coordinates
- PlanetPolygon element for rendering polygons on terrain
- Planet overlay documentation (PlanetEllipse, PlanetSphere, PlanetText, PlanetLine, PlanetPolygon, PlanetGeoJSON)

### Fixed
- pl_json_member_by_name prefix matching bug (strncmp used strlen of stored name, causing false matches for keys like "stroke" vs "stroke-opacity")
- Updated draw_2d shader references to use pl_ prefix (pl_draw_2d.vert, pl_draw_2d.frag, pl_draw_2d_sdf.frag)


[2.101.0] - 2026-03-20
--------------------

### Fixed
- Conversion script stencil handling


[2.100.0] - 2026-03-19
--------------------

### Added
- Terrain sandbox sample
- Version checking for convert-legacy-xml script

### Fixed
- Terrain sandbox for Windows
- Conversion script calling deprecated code


[2.99.1] - 2026-03-16
--------------------

### Added
- CHANGELOG.md documenting all changes from the dcapp-pl branch


[2.99.0] - 2026-03-10
--------------------

### Changed
- Updated pilotlight submodule commit to fix window resizing issue


[2.98.0] - 2026-03-10
--------------------

### Added
- Pixelstream source deduplication for connections sharing the same connection key
- New struct _PixelstreamSource for managing unique pixelstream connections
- New stretchy buffers sb_ps_sources, sb_ps_source_keys, sb_ps_source_key_offsets for pixelstream management
- New function _update_pixelstream_sources() to update unique sources once per frame

### Changed
- Moved pixelstream cleanup from per-node to per-source to avoid double-free


[2.97.0] - 2026-03-10
--------------------

### Added
- Preprocessed XML generation flag (--preprocessed <output.xml>) for all executables: dcapp, dcapp-genheader, dcapp-validate
- New function dc_app_config_save_preprocessed() in config.c for serializing preprocessed XML tree to file
- New declaration in config.h: void dc_app_config_save_preprocessed(DcAppConfig *config, const char *output_path)


[2.96.0] - 2026-03-10
--------------------

### Added
- Pixelstream shared memory reconnection support
- New constant _STALE_THRESHOLD = 300 frames for detecting stale pixelstream connections
- New stale_frames counter field in shmem context
- New function _detach_shm() for clean shared memory detachment

### Changed
- Pixelstream shmem now auto-reconnects when stream is stale (no new data for 300 frames)


[2.95.0] - 2026-03-09
--------------------

### Changed
- Updated pilotlight submodule to latest commit


[2.94.0] - 2026-03-09
-----------------------------

### Changed
- Renamed all forked PilotLight draw structs from plDraw* prefix to dcDraw* prefix
- plDrawList -> dcDrawList, plDrawLayer -> dcDrawLayer, plDrawList3D -> dcDrawList3D, etc.
- Updated all references throughout dcapp code and draw backend extension


[2.93.0] - 2026-03-09
--------------------

### Added
- PlanetText XML element for rendering text at geographic coordinates on planet surfaces
- New enum values: NODE_TYPE_PLANET_TEXT, DC_APP_ELEM_TYPE_PLANET_TEXT
- New struct _NodePlanetText with fields: lat, lon, height_above_terrain, size, fill_color, config_flags, planet_def_index, sb_vals, sb_fillers, sb_filler_indices, sb_formats, sb_format_indices, sb_format_types
- New function _draw_node_planet_text() in draw.c: resolves lat/lon/height, expands text with variable interpolation, converts to 3D position, calls draw_text
- New function _process_xml_node_planet_text() in xml.c with full text parsing including @var and @{var} syntax and format specifiers
- PlanetText XML attributes: Latitude, Longitude, HeightAboveTerrain, Size, FillColor
- New function pl_draw_text() added to plPlanetI vtable: renders 3D text at world position with ray-sphere occlusion test, world-to-pixel size conversion, billboard centering, and size clamping (1-500 pixels)


[2.92.0] - 2026-03-09
--------------------

### Added
- PlanetEllipse XML element for drawing ellipses/circles on planet surfaces
- New enum values: NODE_TYPE_PLANET_ELLIPSE, DC_APP_ELEM_TYPE_PLANET_ELLIPSE
- New struct _NodePlanetEllipse with fields: lat, lon, radius_x, radius_y, rotation, height_above_terrain, line_color, line_width, fill_color, segments, config_flags, planet_def_index
- New function _draw_node_planet_ellipse() in draw.c: generates ellipse points on planet surface using ENU tangent plane, supports fill (triangle fan) and outline modes
- New function _process_xml_node_planet_ellipse() in xml.c: parses Latitude, Longitude, Radius (shorthand), RadiusX, RadiusY, Rotation, HeightAboveTerrain, Segments, LineWidth, FillColor, LineColor
- New _NodeIndex child field added to _NodePlanetView (PlanetView now supports children)
- New functions pl_draw_polygon() and pl_draw_polygon_filled() added to plPlanetI vtable


[2.91.1] - 2026-03-08
--------------------

### Fixed
- Fixed flat planet bounding box bug: flat position bounds were being accumulated into sphere bounding box instead of tMinBoundingFlat/tMaxBoundingFlat


[2.91.0] - 2026-03-08
--------------------

### Added
- MeshCacheSize attribute for `<Planet>` element to configure GPU mesh cache size in MB
- New uint32_t mesh_cache_size field in _PlanetDef struct
- New _load_apis() helper function extracted from pl_app_load() to deduplicate API loading code

### Changed
- Planet mesh cache size now configurable via XML attribute, converted to bytes (* 1048576) and split evenly between vertex and index buffers


[2.90.1] - 2026-03-07
------------------------------

### Fixed
- Fixed VRAM allocation issues: resizes staging buffer to max pixelstream size (4K RGBA) after planet init
- Fixed raw_jpeg_size calculation (was incorrectly multiplied by sizeof(float))
- Added use_dedicated_allocator parameter to _create_texture(); pixelstream textures now use dedicated allocator
- Changed buddy allocator threshold to 4MB fixed
- Fixed planet extension file reading: changed fseek skip from sizeof(plVec3)*2 to sizeof(plVec3)*4 for updated chunk file format
- Fixed light direction in planet.frag: changed -normalize(lightDir) to normalize(lightDir)


[2.90.0] - 2026-03-05
--------------------

### Added
- Flatten attribute for PlanetView element to support oblate spheroid rendering
- New DcAppValIndex flatten field in _NodePlanetView struct
- PL_PLANET_FLAGS_FLATTEN flag for plPlanetViewRuntimeOptions

### Changed
- Planet extension fully rewritten with flatten support


[2.89.0] - 2026-03-01
--------------------

### Added
- LightDirectionX, LightDirectionY, LightDirectionZ attributes for `<Planet>` element
- New _ValIndex3 light_direction field in _PlanetDef struct
- Tau attribute for `<PlanetView>` element (LOD error threshold)
- New DcAppValIndex tau field in _NodePlanetView struct
- Light direction update via plPlanetRuntimeOptions get/set API

### Changed
- Moved per-view shader swap from _update_planet_views() into _draw_node_planet_view() (inline during draw)
- Removed _update_planet_views() function entirely

### Removed
- _update_planet_views() function


[2.88.1] - 2026-03-01
--------------------

### Changed
- Updated pilotlight submodule commit


[2.88.0] - 2026-03-01
------------------------------

### Added
- ShaderIndex attribute moved from `<Planet>` to `<PlanetView>` for per-view shader selection
- New DcAppValIndex shader_index and int active_shader_index fields in _NodePlanetView struct
- NegateX, NegateY added to PlanetView valid attributes
- Full PlanetView attribute list added to validator including positioning, alignment, rotation, pivot attributes
- South-pole stereographic projection documentation in coordinate-frame.md

### Changed
- Changed set_texture() signature to accept slot parameter: set_texture(planet, texture, slot)
- Removed latitude negation in _build_planet_texture
- Rewrote _compute_tile_latlon() in chunkgen with proper south-pole stereographic inverse projection
- Moved coordinate-frame.md from internal/ to top-level documentation/

### Removed
- shader_index and active_shader_index from _PlanetDef struct
- documentation/internal/PLANET_TODO.txt (all items marked DONE)


[2.87.1] - 2026-02-25
--------------------

### Fixed
- Fixed shutdown errors by reordering cleanup sequence: staging buffer destruction now before planet cleanup
- Moved _ext_resource->cleanup() outside planet conditional (was only called if planets existed)
- Added _ext_draw->cleanup() call to shutdown
- Fixed cleanup for 3D draw commands, textured vertex/index buffers in dc_draw_ext.c


[2.87.0] - 2026-02-25
--------------------

### Changed
- Blink: renamed Variable attribute to FireBlink with edge detection via dc_value_is_equal() with last_fire_blink_value
- Function: added FireCall attribute with edge detection via last_fire_call_value
- PlanetTexture: renamed Refresh attribute to FireRefresh
- In dcapp.h _NodeBlink: DcAppVarIndex var -> DcAppValIndex fire_blink, int last_trigger_value -> DcValue last_fire_blink_value
- In dcapp.h _NodeFunction: added fire_call and last_fire_call_value fields
- In dcapp.h _PlanetTextureEntry: refresh -> fire_refresh, last_refresh_value -> last_fire_refresh_value


[2.86.1] - 2026-02-26
------------------------------

### Fixed
- Updated validation code to support polygon alignment attributes
- Negated latitude for planet primitive to match expected behavior


[2.86.0] - 2026-02-25
--------------------

### Added
- pl_planet_set_texture() function for runtime texture updates
- pl__chlod_update_chunk_file() for updating chunk file texture data
- tInfo, tTopLeftGlobal, tBottomRightGlobal, uTileCount, atTiles fields added to plPlanet struct
- uTextureIndex field added to plChunkFileData
- UV offset/scale per chunk for texture mapping
- set_texture function added to plPlanetI vtable
- tUVInfo (vec4) added to plGpuDynPlanetData in shader interop

### Changed
- Changed pl_chlod_load_chunk_file() signature: removed texture param
- Updated planet.frag: added UV transformation using tUVInfo.xy (scale) and tUVInfo.zw (offset)

### Removed
- Removed pak file dependency for textures
- Removed atTextures from plPlanetInit


[2.85.0] - 2026-02-25
--------------------

### Added
- Heading attribute for Planet primitive
- New DcAppValIndex heading field in __NodePlanet struct
- Heading applied as camera.fRoll = pl_radiansf(heading_deg) in draw.c
- Heading documentation in primitives.md


[2.84.0] - 2026-02-25
--------------------

### Added
- PlanetShader as valid child of Planet element in validation
- DC_APP_ELEM_TYPE_PLANET_SHADER validation with required Index attribute
- New _valid_attrs_planet_shader[] = {"Index", "VertexSource", "FragmentSource"}
- Full documentation for `<Planet>`, `<PlanetData>`, `<PlanetShader>`, `<PlanetTexture>` elements with attribute tables


[2.83.0] - 2026-02-25
--------------------

### Changed
- Renamed XML attribute ShaderVariable to ShaderIndex throughout
- Renamed XML attribute Operation to Operator throughout
- Updated all documentation and sample files accordingly
- Updated fix_split_if.py to use Operator attribute


[2.82.1] - 2026-02-25
--------------------

### Changed
- Added PL_BUILD_STATUS tracking variable to build scripts
- Added exit 1 on individual build failure and exit ${PL_BUILD_STATUS} at end of scripts
- Build scripts now propagate error codes properly


[2.82.0] - 2026-02-24
--------------------

### Added
- pl__free_chunk_until() for dual-pool eviction (vertex + index) in planet extension
- pl__lru_unlink() and pl__lru_push_front() for proper LRU management
- bInReplacementList bool field added to plPlanetChunk
- Hysteresis refinement: tauMerge = tauSubdivide * 0.5f
- gptStats API integration for planet draw call counter
- dcapp now creates a PlanetView per planet automatically

### Changed
- Removed pl_render_planet() and pl_get_planet_texture() from plPlanetI vtable; moved all rendering to PlanetView path
- Changed pl_render_to_planet_view() signature: removed plPlanet* param, renamed to render_view
- Changed pl_draw_sphere() to take plPlanetView* instead of plPlanet*
- Simplified UPS projection in processor to single formula (removed bUpsNorth branch)
- Removed --radius requirement from chunkgen (auto-detect from DEM)
- Fixed FOV calculation: 2.0f * atanf(tanf(0.5f * fFieldOfView) * fAspectRatio)

### Removed
- apps/dcapp-terrain.c (648 lines, standalone terrain sandbox)
- bUpsNorth from plPlanetProcessInfo and plPlanetHeightMap
- uOutputWidth/uOutputHeight from plPlanetInit (now on plPlanetViewInit)


[2.81.1] - 2026-02-26
--------------------

### Fixed
- Fixed preprocessor bug with spliced children: _splice_children_into_parent_and_free_wrapper() now propagates _Directory attribute to children that don't already have one
- Without this fix, included files lost track of their source directory after being spliced into the parent tree


[2.81.0] - 2026-02-27
--------------------

### Added
- Sample demonstrating flat planet shader for maps


[2.80.1] - 2026-02-27
--------------------

### Fixed
- edge.c: Added NULL check after malloc for temp_buffer
- shmem.c: Added NULL check after strdup for filepath; added NULL check after realloc for pixel buffer
- sock.c: Changed strcpy to strncpy with INET6_ADDRSTRLEN bounds (2 locations) to prevent buffer overflow
- trick.c: Added NULL check after malloc for temp_buffer
- trick.c: Changed _dc_trick_append_to_tx_buffer, _dc_trick_send, _dc_trick_receive, _dc_trick_connect, _dc_trick_close from extern to static linkage
- math.c: Added DC_DEG_TO_RAD and DC_RAD_TO_DEG defines for conversion functions
- value.c: Fixed incorrect error messages in dc_value_get_addr() and dc_value_is_equal() (both were printing "dc_value_set_from_string()")


[2.80.0] - 2026-02-27
--------------------

### Changed
- Renamed PlanetShader attributes: VertexSource -> VertexShader, FragmentSource -> FragmentShader
- Updated _valid_attrs_planet_shader[] in validation to use new names
- Updated all documentation and sample XML


[2.79.1] - 2026-02-27
--------------------

### Changed
- Updated .clang-format: AllowShortIfStatementsOnASingleLine: false -> WithoutElse
- Applied clang-format to all source files


[2.79.0] - 2026-02-27
--------------------

### Changed
- Cleaned up include paths: changed ../../pilotlight/... to just "pl.h", "pl_math.h", etc.
- Changed ../../src/... to "utils/env.h", "app/elem.h", etc.
- Updated setup-vscode.py to use absolute include paths for clangd resolution


[2.78.1] - 2026-02-27
--------------------

### Fixed
- Fixed missing static definitions after utils refactor


[2.78.0] - 2026-02-27
--------------------

### Changed
- Eliminated apps/dcapp/utils.c entirely by moving contents into dcapp.c and dcapp.h
- Moved _build_planet_texture(), _init_planets(), _update_planet_defs() into dcapp.c as static functions
- Moved _get_node() inline into dcapp.h
- Moved _node_type_to_string(), _register_node(), _load_color_from_string(), _init_stencil_pipelines() into their respective call sites

### Removed
- apps/dcapp/utils.c (934 lines)


[2.77.0] - 2026-02-27
--------------------

### Changed
- Moved main dcapp app code into apps/dcapp/ subdirectory
- apps/dcapp.c -> apps/dcapp/dcapp.c
- apps/dcapp.h -> apps/dcapp/dcapp.h
- apps/_dcapp_draw.c -> apps/dcapp/draw.c
- apps/_dcapp_process_xml.c -> apps/dcapp/xml.c
- apps/_dcapp_utils.c -> apps/dcapp/utils.c
- Updated all include paths and build scripts to reference apps/dcapp/dcapp.c


[2.76.0] - 2026-02-27
--------------------

### Changed
- Reversed hyphen convention for app source files (C files use underscores, not hyphens)
- apps/dcapp-genheader.c -> apps/dcapp_genheader.c
- apps/dcapp-planet-chunkgen.c -> apps/dcapp_planet_chunkgen.c
- apps/dcapp-validate.c -> apps/dcapp_validate.c


[2.75.0] - 2026-02-27
--------------------

### Changed
- Standardized all filenames to use hyphens instead of underscores
- Assets renamed: button_off.png -> button-off.png, button_on.png -> button-on.png, nasa_worm.png -> nasa-worm.png
- Samples renamed: pixelstream_mjpeg/ -> pixelstream-mjpeg/, trick_stress/ -> trick-stress/
- Scripts renamed: convert_legacy_xml.py -> convert-legacy-xml.py, setup_vscode.py -> setup-vscode.py
- Internal scripts renamed: build_apps_*.sh -> build-apps-*.sh, build_samples_*.sh -> build-samples-*.sh, gen_build_apps.py -> gen-build-apps.py, gen_build_samples.py -> gen-build-samples.py
- Updated all references in XML files, documentation, and build scripts


[2.74.0] - 2026-02-27
--------------------

### Changed
- Moved build/clean scripts from repo root to scripts/ directory
- Updated DCAPP_HOME paths in scripts to use parent directory

### Removed
- scripts/fix_split_if.py (275 lines) legacy converter script


[2.73.0] - 2026-02-27
--------------------

### Added
- 60+ new named color constants across all color groups:
  - Reds: _color_scarlet_, _color_tomato_, _color_wine_, _color_raspberry_, _color_dark_red_
  - Oranges: _color_dark_orange_, _color_mango_, _color_persimmon_
  - Yellows: _color_cream_, _color_ivory_, _color_saffron_, _color_golden_rod_, _color_canary_
  - Greens: _color_dark_green_, _color_sage_, _color_spring_green_, _color_hunter_green_, _color_kelly_green_, _color_pine_, _color_fern_, _color_neon_green_
  - Blues: _color_royal_blue_, _color_midnight_blue_, _color_cobalt_, _color_cornflower_blue_, _color_turquoise_, _color_cyan_, _color_aquamarine_, _color_electric_blue_, _color_periwinkle_
  - Purples: _color_magenta_, _color_mauve_, _color_lilac_, _color_grape_, _color_royal_purple_
  - Browns: _color_sienna_, _color_cinnamon_, _color_sandy_brown_
  - Neutrals: _color_snow_, _color_pearl_, _color_smoke_, _color_bone_, _color_graphite_, _color_iron_, _color_steel_
- Both documentation entries and _add_const() calls in config.c


[2.72.0] - 2026-02-27
--------------------

### Added
- Major documentation overhaul:
  - documentation/README.md with full documentation index, quick element reference table, and command-line tools table
  - documentation/blink.md (171 lines) with full Blink reference, FireBlink edge-trigger explanation, examples
  - documentation/mouse-events.md
  - documentation/stencil.md
  - documentation/integration.md
  - documentation/planet.md
  - documentation/samples.md
  - documentation/getting-started.md
  - documentation/migration.md

### Changed
- Updated documentation/buttons.md: renamed child elements from `<Enabled>`/`<On>`/`<Off>`/`<Disabled>`/`<Pressed>`/`<Released>`/`<Transition>` to `<ButtonEnabled>`/`<ButtonIndicatorOn>`/`<ButtonIndicatorOff>`/`<ButtonDisabled>`/`<ButtonPressed>`/`<ButtonReleased>`/`<ButtonTransition>`


[2.71.1] - 2026-02-27
--------------------

### Changed
- Added gitignore entries for AI tools: .claude/, .cursor/, .windsurf/, .aider*, .codeium/, .continue/, .codex/, .copilot/, .github/copilot/, .tabnine/


[2.71.0] - 2026-02-27
--------------------

### Added
- New Python script scripts/setup_vscode.py (268 lines) for generating VS Code config, .clangd, and compile_flags.txt based on platform and language server choice
- Supports clangd and Microsoft C/C++ extension (ccpptools)
- Detects platform (Linux/macOS/Windows) and sets appropriate include paths
- Generates launch.json with debug configs for dcapp, dcapp-validate, dcapp-planet-chunkgen

### Changed
- Added .vscode/, .clangd, compile_flags.txt to .gitignore as generated files

### Removed
- .clangd, .vscode/c_cpp_properties.json, .vscode/extensions.json, .vscode/launch.json, .vscode/settings.json, compile_flags.txt, recent-renames.txt


[2.70.1] - 2026-02-27
--------------------

### Changed
- Removed log_dir_path creation and cleanup from config

### Removed
- char *log_dir_path from DcAppConfig struct


[2.70.0] - 2026-02-27
--------------------

### Added
- clean.bat: Windows clean script removing pilotlight/out, out-temp, shader-temp, cache, sample DLLs
- clean.sh: Unix clean script doing same


[2.69.0] - 2026-02-25
--------------------

### Changed
- Renamed _draw_node_planet to _draw_node_planet_view
- Changed NODE_TYPE_PLANET to NODE_TYPE_PLANET_VIEW
- All node->planet.* references changed to node->planet_view.*
- Added DC_APP_ELEM_TYPE_PLANET_VIEW enum value in elem.h


[2.68.0] - 2026-02-18
--------------------

### Added
- Terrain sandbox application: apps/dcapp-terrain.c (648 lines) using PilotLight + Dear ImGui + ImPlot
- Struct plAppData with ptWindow, camera controls, planet pointers (ptPlanet0, ptPlanet1)
- Creates two planets with radius 1737400.0 (Moon), 8x8 tile grid, tree depth 6
- Planet processing: plPlanetProcessInfo with fRadius, fMetersPerPixel, tile params
- Camera: perspective reverse-Z, WASD + mouse look, +/- for speed
- Debug UI: wireframe toggle, level display, tau slider, light direction controls
- VFS mounts for shaders, assets, tiles, cache


[2.67.0] - 2026-02-16
--------------------

### Added
- Placeholder code for terrain primitive in main application


[2.66.0] - 2026-02-16
--------------------

### Changed
- Massive cleanup: removed hundreds of explicit "else { field = DC_APP_VAL_INDEX_UNDEFINED; }" blocks across ALL node types
- Relies on C zero-initialization ({} / {0}) since DC_APP_VAL_INDEX_UNDEFINED equals 0
- Affected node types: Arc, Ellipse, Blink, Button, Container, Image, Line, PixelStream, Polygon, Rectangle, Set, Text, Sphere, Terrain
- Removed explicit NODE_INDEX_UNDEFINED assignments for next/child fields
- Changed _Node initialization from uninitialized to = {} or = {0}
- Net reduction of ~300+ lines of boilerplate


[2.65.0] - 2026-02-09
--------------------

### Added
- Log attribute on `<Text>` element for runtime logging
- New struct field: _NodeText.log (DcAppValIndex)
- Parsed as DC_VALUE_TYPE_STRING
- At draw time, if log is defined, prints [label] text_content to stdout


[2.64.0] - 2026-02-09
--------------------

### Added
- New built-in constant _stencil_color_ = "0 0 0 1" (black, fully opaque)
- New conversion script function force_stencil_mask_colors(root) that sets FillColor/LineColor to #_stencil_color_ on all elements inside StencilAdd/StencilRemove

### Changed
- All stencil samples updated: stencil FillColor changed from "1 1 1" to "#_stencil_color_"


[2.63.0] - 2026-02-09
------------------------------

### Added
- Trick variable reset on reconnect: new _TrickContext.was_connected (bool) field for init-on-connect tracking
- Edge variable reset on reconnect: new _EdgeContext.was_connected (bool) field for disconnect detection

### Changed
- On Trick or Edge (re)connect, zeros out prev_values so all TX vars get sent to initialize the sim


[2.62.1] - 2026-02-09
--------------------

### Fixed
- Updated conversion script to account for pivot positioning


[2.62.0] - 2026-02-08
--------------------

### Added
- Multilevel stencil support (nested stencils)
- Extended stencil system depth handling


[2.61.1] - 2026-02-08
---------------------------------------------------------------------------------------

### Fixed
- Fixed button on_press timing
- Fixed spherical positioning on macOS
- Fixed ADI sample
- Fixed `<Set>` missing operand cases
- Fixed set default operand type
- Fixed button event queuing events for indicator
- Fixed ellipse/arc starting angles
- Added "Queue" to Set valid params in validation


[2.61.0] - 2026-02-08
------------------------------

### Added
- Queue attribute on `<Set>` element for deferred execution
- New struct _QueuedSetOp with fields: var_index (DcAppVarIndex), operation (DcAppSetType), value (DcValue)
- New struct field: _NodeSet.queued (bool, later changed to DcAppValIndex)
- New field in _AppData: sb_queued_sets (stretchy buffer of _QueuedSetOp)
- New function _flush_queued_sets() to iterate queued ops, apply via _apply_set_operation(), and clear buffer
- New function _apply_set_operation() helper for applying set operations

### Changed
- Queue attribute changed from presence-based bool to DcAppValIndex (DC_VALUE_TYPE_BOOLEAN value)


[2.60.0] - 2026-02-18
--------------------

### Added
- PivotParentAlignX and PivotParentAlignY attributes added to ALL positionable primitives: Button, Arc, Ellipse, Container, Image, Line, PixelStream, Polygon, Rectangle, Text
- New struct field: pivot_parent_align (_ValIndex2) added to all node structs

### Changed
- When rotation + pivot_parent_align are both set, rotation pivots around parent-relative alignment point
- Pivot position calculated from parent dimensions using alignment type (Left/Center/Right, Bottom/Middle/Top)


[2.59.0] - 2026-02-19
--------------------

### Added
- ParentAlignX and ParentAlignY attributes for `<Polygon>` element
- New struct field: _NodePolygon.parent_align (_ValIndex2)

### Changed
- Polygon position calculation now uses parent alignment anchor: position = parent_position + anchor + offset


[2.58.1] - 2026-02-19
------------------------------

### Added
- documentation/windows-setup.md (67 lines) with Windows prerequisites, clone, submodule, vcpkg, build instructions

### Fixed
- Logic library loading: added dc_utils_file_exists() check before attempting to load .so/.dylib/.dll
- PixelStream: replaced realpath() with cross-platform dc_utils_canonicalize_path()
- dcapp-genheader.c: fixed size_t vs int comparison warning
- dcapp.c: fixed is_connected variable redeclaration in edge update loop
- screensaver logic: replaced gettimeofday() (POSIX) with time() (C standard) for Windows compat
- Windows batch scripts: removed PowerShell relative path computation, simplified to direct absolute paths
- Fixed path separators: reverted accidental backslash introduction in Linux/macOS scripts
- Sorted source file lists alphabetically across all platform build scripts
- Windows build scripts: updated include/lib paths to ../vcpkg_installed/x64-windows/
- Added xcopy commands to copy vcpkg DLLs to output directory


[2.58.0] - 2026-02-20
--------------------

### Added
- More Set operation variants


[2.57.0] - 2026-02-18
--------------------

### Added
- 3D stenciling support (3D solid + 3D textured shader pipelines)
- New shader handles in _AppData: stencil_create_3d_solid_shader, stencil_remove_3d_solid_shader, stencil_draw_3d_solid_shader[DC_STENCIL_MAX_DEPTH], stencil_cleanup_3d_solid_shader
- New shader handles: stencil_create_3d_textured_shader, stencil_remove_3d_textured_shader, stencil_draw_3d_textured_shader[DC_STENCIL_MAX_DEPTH], stencil_cleanup_3d_textured_shader
- New shader override pointers: active_2d_shader_override, active_sdf_shader_override, active_3d_solid_shader_override, active_3d_textured_shader_override
- New flags: stencil_2d_dirty, stencil_3d_dirty (bool flags for lazy shader injection)
- New extension function pl_set_3d_shader() for setting shader overrides on 3D draw lists
- 3D vertex buffer layouts: vertex_layout_3d_solid (pos3+color), vertex_layout_3d_textured (pos3+uv2+color)
- New draw backend fields: pt3dSolidShaderOverride, pt3dTexturedShaderOverride, bCustom3DShaderActive

### Changed
- Stencil phase changes now set override pointers + dirty flags instead of calling set_shader directly
- _draw_batch_get_2d() and _draw_batch_get_3d() inject stencil overrides on phase change
- Refactored pl_submit_3d_drawlist() with Phase 1 (upload) / Phase 2 (draw) separation


[2.56.0] - 2026-02-18
--------------------

### Added
- dcappHome environment variable set to config->dcapp_dir_path in all three executables: dcapp, dcapp-genheader, dcapp-validate

### Changed
- Removed empty Set warning (DC_LOG_WARN for empty set content silenced)


[2.55.0] - 2026-02-17
--------------------

### Changed
- Renamed button attributes: EnabledOn -> EnableOn, EnabledVariable -> EnableVariable
- Updated in: XML parsing, validation, conversion script, documentation, samples
- Legacy conversion updated: ActiveVariable -> EnableVariable, ActiveOn -> EnableOn


[2.54.0] - 2026-02-17
--------------------

### Changed
- Renamed Queue -> Defer throughout:
  - XML attribute: "Queue" -> "Defer"
  - Struct field: _NodeSet.queued -> _NodeSet.deferred
  - Struct: _QueuedSetOp -> _DeferredSetOp
  - AppData field: sb_queued_sets -> sb_deferred_sets
  - Function: _flush_queued_sets() -> _flush_deferred_sets()
  - Validation: _valid_attrs_set[] updated from "Queue" to "Defer"
  - Conversion script: all occurrences of Queue="true" -> Defer="true"


[2.53.0] - 2026-02-06
--------------------

### Added
- Text shadow rendering (ShadowOffset attribute on `<Text>` element)
- New struct field: _NodeText.shadow_offset (DcAppValIndex)
- Shadow color hardcoded to black: PL_COLOR_32_RGBA(0, 0, 0, 1)
- Shadow drawn before main text with offset in both X and Y


[2.52.0] - 2026-02-06
--------------------

### Added
- Validation check to detect accidental variable dereferences in XML


[2.51.0] - 2026-02-06
--------------------

### Added
- Timestamp prefixes added to log output messages


[2.50.1] - 2026-02-06
--------------------

### Changed
- Updated StaticIf to be automatically inferred rather than requiring explicit declaration


[2.50.0] - 2026-02-05
--------------------

### Added
- Structured logging system with DC_LOG_ERROR, DC_LOG_WARN, DC_LOG_INFO macros replacing all fprintf/printf/perror calls
- New #include "utils/log.h" added to: _dcapp_draw.c, _dcapp_process_xml.c, elem.c, lookup.c, config.c, mjpeg.c, shmem.c, sock.c, trick.c, file.c
- Log categories used: "Draw", "Button", "Image", "Line", "Logic", "Polygon", "Rectangle", "PixelStream", "Config", "Elem", "Lookup", "MJPEG", "Shmem", "Sock", "Trick", "File", "Set", "Text", "Container", "Ellipse", "Arc"

### Changed
- Replaced all fprintf(stderr, ...) calls with structured DC_LOG macros throughout entire codebase
- Replaced printf() for MJPEG connect message with DC_LOG_INFO
- Replaced perror() calls in sock.c with DC_LOG_ERROR
- Consistent log message formatting: removed "DCAPP" prefix, removed "\n" suffixes


[2.49.1] - 2026-02-05
--------------------

### Changed
- Changed GPU memory allocations to use PilotLight's allocator extension API


[2.49.0] - 2026-02-04
------------------------------

### Changed
- Updated pilotlight submodule commit reference

### Fixed
- Fixed draw extension API mismatches after PilotLight update


[2.48.1] - 2026-02-04
--------------------

### Fixed
- Fixed texture deduplication to avoid loading duplicate textures


[2.48.0] - 2026-02-04
--------------------

### Fixed
- Fixed legacy conversion of Circle to Arc/Ellipse


[2.47.1] - 2026-02-04
-------------------------------------------------

### Fixed
- Added defensive checks for missing/undefined attributes across primitives
- Fixed default values for Window VirtualWidth/VirtualHeight
- Added handling for various invalid XML patterns that could cause crashes
- Fixed mouse event processing to skip disabled mouse events


[2.47.0] - 2026-02-03
---------------------------------------

### Changed
- Merged Circle implementation into Ellipse (Circle becomes Ellipse with equal RadiusX/RadiusY)
- Circle element still exists in XML but internally maps to the ellipse node type

### Fixed
- Corrected Arc and Ellipse rendering behavior
- Fixed local alignment calculation for arc primitives


[2.46.1] - 2026-02-04
--------------------

### Changed
- Updated button variable conversion for legacy script


[2.46.0] - 2026-02-02
--------------------

### Changed
- Removed legacy .tga to .png image conversion code


[2.45.1] - 2026-02-02
--------------------

### Fixed
- Fixed uninitialized memory causing garbage in image loading output


[2.45.0] - 2026-02-03
--------------------

### Changed
- Renamed dcapp-genheader and dcapp-validate script files


[2.44.1] - 2026-02-02
-------------------------------------------------

### Fixed
- Added warning message and error handling when a referenced logic (.so/.dll) file cannot be found or loaded
- Added null check for texture file paths to prevent segfault
- Improved handling of undefined value indices to prevent crashes
- Fixed documentation and warning messages for TrickFrom/EdgeFrom


[2.44.0] - 2026-02-02
--------------------

### Changed
- Moved binary/executable scripts to /bin directory


[2.43.0] - 2026-02-02
--------------------

### Changed
- Streamlined naming conventions for Trick and Edge IO elements
- Updated conversion script to match


[2.42.1] - 2026-02-02
--------------------

### Fixed
- Fixed bug where `<If>` block processing would incorrectly remove the root XML element when If was at top level


[2.42.0] - 2026-02-02
--------------------

### Changed
- Updated build scripts


[2.41.1] - 2026-02-02
--------------------

### Changed
- Added Trick/Edge IO elements to validation

### Fixed
- Fixed legacy primitive handling


[2.41.0] - 2026-02-02
--------------------

### Fixed
- Fixed bug where constants passed on command line would not properly override default values during preprocessing


[2.40.1] - 2026-02-02
--------------------

### Changed
- Added TestPattern to PixelStream's valid attribute list in validation


[2.40.0] - 2026-02-02
----------------------------------------

### Fixed
- Fixed broken pixelstream conversion in legacy script
- Fixed `<If>` element conversion in legacy script
- Fixed Panel validation rules


[2.39.0] - 2026-02-02
--------------------

### Added
- DisplayIndex attribute for `<Panel>` element, allowing panels to target specific display monitors in multi-monitor setups


[2.38.0] - 2026-02-02
--------------------

### Added
- EDGE IO protocol support with new XML elements:
  - `<EdgeIO>` with Host, Port, DataRate attributes
  - `<FromEdge>` and `<ToEdge>` child elements
  - `<EdgeVariable>` with Name attribute
- Similar architecture to TrickIO but for EDGE data protocol


[2.37.0] - 2026-02-02
--------------------

### Changed
- Refactored Trick IO socket handle interface for cleaner API


[2.36.1] - 2026-02-02
----------------------------------------

### Changed
- Updated pixelstream handling in legacy conversion
- Updated arc conversion from legacy format
- Marked ZeroTrim as hidden legacy attribute in conversion


[2.36.0] - 2026-02-02
--------------------

### Changed
- Internal renaming of conditional-related constant names


[2.35.0] - 2026-02-02
--------------------

### Changed
- Consolidated If and StaticIf: unified the conditional evaluation logic shared between `<If>` and `<StaticIf>`


[2.34.0] - 2026-02-02
--------------------

### Added
- TestPattern attribute for `<PixelStream>` element
- When set, displays a built-in test pattern instead of connecting to a stream source; useful for layout testing


[2.33.0] - 2026-02-02
--------------------

### Added
- Shared memory (shmem) transport for PixelStream primitive as alternative to MJPEG HTTP streaming
- Uses POSIX shared memory for zero-copy frame delivery


[2.32.0] - 2026-01-31
--------------------

### Added
- NegateX and NegateY boolean attributes for primitives
- When set, they negate (flip sign) the X or Y position value, enabling mirroring without math


[2.31.0] - 2026-01-31
--------------------

### Added
- ParentAlignX/Y support for `<Vertex>` elements within `<Polygon>`
- Polygon vertices can now be positioned relative to parent alignment anchors


[2.30.0] - 2026-01-31
--------------------

### Added
- Push/pop stack operations for `<Set>` primitive
- New Set operators: #_set_push_, #_set_pop_

### Changed
- Updated DcValue to use an array internally to support stack operations


[2.29.0] - 2026-01-31
--------------------

### Added
- AlignX and AlignY shorthand attributes that set BOTH ParentAlign and LocalAlign simultaneously
- AlignX="#_align_center_" is equivalent to ParentAlignX="#_align_center_" LocalAlignX="#_align_center_"


[2.28.1] - 2026-01-31
------------------------------

### Fixed
- Added OriginX/OriginY conversion for legacy code, fixed alignment conversion

### Changed
- Updated legacy conversion to use new AlignX/Y shorthand


[2.28.0] - 2026-01-31
--------------------

### Added
- ConnectedVariable attribute for `<TrickIO>` element, allowing a variable to reflect the connection status of the Trick socket


[2.27.1] - 2026-01-31
-------------------------------------------------

### Changed
- Added `<Set>` element validation rules to dcapp-validate
- Updated validation and conversion script refinements
- More validation rule updates
- Updated validation rules in dcapp-validate


[2.27.0] - 2026-01-31
--------------------

### Fixed
- Config file saving now preserves original whitespace formatting


[2.26.0] - 2026-01-31
------------------------------

### Changed
- Updated conversion script with new conversion rules
- More conversion script updates and validation fixes


[2.25.0] - 2026-01-31
--------------------

### Changed
- Cleanup of style application code


[2.24.0] - 2026-01-30
--------------------

### Changed
- Changed `<If>` conditional processing to use an event-based model


[2.23.0] - 2026-01-30
--------------------

### Changed
- Changed mouse event handling (active, hovered, pressed, released, inactive) to use child XML nodes instead of attributes
- Affects all shapes with mouse events


[2.22.0] - 2026-01-30
--------------------

### Changed
- Restructured button event handling so press/release events are separate child nodes


[2.21.0] - 2026-01-30
--------------------

### Added
- Alias so "MousePressed" works as equivalent to "Pressed" for button events


[2.20.1] - 2026-01-30
--------------------

### Fixed
- Fixed button rendering to properly use layered draw order


[2.20.0] - 2026-01-29
--------------------

### Fixed
- Added null check to prevent segfault when a referenced style doesn't exist


[2.19.1] - 2026-01-29
--------------------

### Fixed
- Fixed validation of Window's allowed child elements


[2.19.0] - 2026-01-29
------------------------------

### Fixed
- Removed `<Style>` elements from preprocessed XML tree before validation

### Changed
- Changed the output filename for the preprocessed XML dump


[2.18.1] - 2026-01-29
--------------------

### Fixed
- Added error checking for missing or invalid style definitions


[2.18.0] - 2026-01-29
--------------------

### Added
- Ellipse primitive documentation


[2.17.0] - 2026-01-28
--------------------

### Changed
- Switched legacy conversion script from xml.etree.ElementTree to lxml.etree for XML processing
- Updated type hints from ET.Element to etree._Element throughout
- Functions updated: convert_alignment(), convert_variable_type(), convert_button_type()


[2.16.0] - 2026-01-28
--------------------

### Added
- Ellipse XML primitive with independent X/Y radii
- New enum values: DC_APP_ELEM_TYPE_ELLIPSE, NODE_TYPE_ELLIPSE
- New struct _NodeEllipse with fields: _ValIndex2 position, pivot_local_align, pivot_position, local_align, parent_align; DcAppValIndex rotation, radius_x, radius_y, num_segments, line_width; _ValIndex4 fill_color, line_color; _MouseEventChildren mouse_events; bool fill_enabled, line_enabled
- New constant _NODE_ELLIPSE_MAX_SEGMENTS = 1000
- XML attributes for Ellipse: RadiusX, RadiusY, Segments (default 40), PositionX/Y, ParentAlignX/Y, LocalAlignX/Y, Rotation, PivotX/Y, PivotLocalAlignX/Y, FillColor, LineColor, LineWidth
- New function _draw_node_ellipse(): generates ellipse points using parametric equation (radius_x * cos(angle), radius_y * sin(angle))
- Mouse hit testing uses ellipse equation: (dx/rx)^2 + (dy/ry)^2 <= 1
- New function _process_xml_node_ellipse()
- Updated validation tool: added Ellipse to all parent-child valid lists, added _valid_attrs_ellipse


[2.15.0] - 2026-01-28
--------------------

### Added
- dcapp-validate executable: apps/dcapp-validate.c (1134 lines)
  - Usage: dcapp-validate <config.xml> [CONSTANT=value ...]
  - Creates config, sets environment, preprocesses XML, then validates
  - Dumps preprocessed XML to xml.log for debugging
  - Reports error/warning counts, returns 1 if errors found
- New struct: ValidationContext { error_count, warning_count }
- New validation functions:
  - _validate_node(): validates a single node (parent-child, attributes, values)
  - _validate_children(): recurses into children
  - _is_valid_child(): comprehensive parent-child relationship validation for ALL element types
  - _validate_required_attributes(): checks for required attrs per element type
  - _validate_attribute_names(): checks all attrs are valid for element type
  - _validate_attribute_values(): validates attribute value formats
- Valid attribute lists defined as static arrays for: _valid_attrs_position, _valid_attrs_align, _valid_attrs_pivot, _valid_attrs_rotation, _valid_attrs_color, _valid_attrs_line, _valid_attrs_arc, _valid_attrs_blink, _valid_attrs_button, _valid_attrs_circle, _valid_attrs_constant, _valid_attrs_function, _valid_attrs_if, and many more

### Changed
- Renamed dc_app_config_clean_xml -> dc_app_config_preprocess_xml (API rename)


[2.14.0] - 2026-01-28
--------------------

### Changed
- Major simplification of StaticIf processing in config.c:
  - Replaced 4-case handling with single unified approach
  - New algorithm: (1) remove non-matching branch without processing, (2) process surviving children, (3) unwrap True/False wrappers, (4) splice remaining children into parent
  - Added error message for invalid StaticIf children (only `<True>` and `<False>` allowed)
- Updated static-if sample to use explicit `<True>` wrappers (required by strict mode)


[2.13.0] - 2026-01-14
--------------------

### Changed
- MAJOR POSITIONING CHANGE: X/Y coordinates now treated as OFFSETS from ParentAlign anchor point, not absolute positions
- Formula: position = parent_position + anchor + offset (where anchor comes from ParentAlign, offset from X/Y)
- Refactored position calculation for ALL drawable node types: Button, Arc, Circle, Container, Image, PixelStream, Rectangle, Sphere, Terrain, Text
- ParentAlignX="#_align_center_" X="10" now means "10 pixels right of center"


[2.12.1] - 2026-01-14
--------------------

### Fixed
- Added dc_utils_trim_whitespace_inplace() calls to trim leading/trailing whitespace from XML node content in: `<Set>` operand, `<Text>` content, `<TrickVariable>` variable name, `<Variable>` name


[2.12.0] - 2026-01-14
--------------------

### Added
- Complete Trick simulation sample with cannonball physics:
  - CANNON struct with pos[2], vel[2], vel0[2], time, impact, impactTime
  - cannon_init(): sets initial speed (50 m/s), angle (30 degrees), velocity components
  - cannon_update(): analytical ballistic trajectory with TIME_SCALE (1/12), impact detection when pos[1] < 0
  - S_define with CannonSimObject, init and scheduled (0.01s) jobs
  - input.py: variable server on port 7000, real-time mode, 0.1s software frame
- Trick display (trick.xml) with: metric + imperial variants, two TrickIO connections (fast 10Hz, slow 1Hz), trajectory visualization, data panels
- New XML attributes demonstrated: TrickVariable Units (for unit conversion), TrickIO DataRate


[2.11.1] - 2026-01-14
--------------------

### Fixed
- Fixed TrickIO data rate attribute: was reading "Rotation" instead of "DataRate"
- Fixed TrickVariable units attribute: was reading "Name" instead of "Units"


[2.11.0] - 2026-01-14
------------------------------

### Added
- `<StaticIf>` XML primitive for parse-time conditional evaluation
  - Evaluates ONCE during XML parsing, not at runtime
  - Attributes: Operation (integer), Value/Value1 (string/var, required), Value2 (string/var)
  - Operation constants: #_conditional_true_ (0), #_conditional_false_ (1), #_conditional_eq_ (2), #_conditional_ne_ (3), #_conditional_lt_ (4), #_conditional_gt_ (5), #_conditional_lte_ (6), #_conditional_gte_ (7)
  - Children: `<True>`, `<False>` (content included when condition matches)
  - Matching branch spliced directly into parent; no runtime overhead
- Static-if sample demonstrating constants, conditional variable creation, and side-by-side comparison with `<If>`
- New static function _splice_children_into_parent_and_free_wrapper() in config.c for XML tree manipulation
- Include node handling now adds _Directory attribute to children for correct relative path resolution
- _process_xml_node() now handles _Directory attribute for directory override

### Changed
- Refactored DC_APP_ELEM_TYPE_DUMMY and DC_APP_ELEM_TYPE_INCLUDE cases in _clean_xml_node() to use new splice helper


[2.10.1] - 2026-01-14
---------------------------------------

### Fixed
- Removed erroneous `<ElseIf>`, `<Else>`, `<Switch>`, `<Case>`, `<Default>` from conditionals sample (these elements do not exist in dcapp)
- Changed #_conditional_neq_ to #_conditional_ne_ (correct constant name) in samples
- Fixed build sample compilation order in scripts
- Fixed source paths in build scripts across all three platforms (linux, macos, win32)


[2.10.0] - 2026-01-15
---------------------------------------

### Changed
- Updated alignment system documentation to explain new anchor+offset behavior
- Removed _process_xml_node_include() function entirely (Include nodes now fully handled during preprocessing)
- Removed DC_APP_ELEM_TYPE_INCLUDE case from _process_xml_node() switch
- Replaced all plain-string variable types in documentation with constant syntax: Type="Double" -> Type="#_variable_double_", etc.

### Fixed
- Improved Include file path resolution: added empty filepath check, separated absolute path resolution from canonicalization, added file existence check before canonicalization


[2.9.0] - 2026-01-07
--------------------

### Added
- Legacy-to-modern XML conversion script: scripts/convert_legacy_xml.py (616 lines)
  - Element renaming (DisplayLogic -> Logic, Mask -> Stencil, etc.)
  - Attribute renaming (global and per-element)
  - Button child element conversion
  - Variable prefix stripping (@ removal)
  - Constant prefix conversion (% -> #)
  - Unsupported attribute removal (ShadowOffset, ForceMono, ForceUpdate, etc.)
  - Mask/Stencil structure conversion
  - If/True/False structure conversion to Value1/Operation pattern
  - Recursive tree processing
  - XML formatting with indentation (-f flag) and in-place editing (-i flag)
  - Key functions: process_element(), process_mask_element(), process_true_false_elements(), process_tree(), convert_xml(), format_xml(), main()
- Documentation sections 11-19 in notes.txt covering conversion rules for: Set Min/Max, Blink, removed attributes, Line, DisplayLogic, Mask/Stencil, unimplemented elements, quick reference constants, complete conversion example


[2.8.0] - 2026-01-07
----------------------------

### Added
- Mona Lisa polygon rendering sample: 10,396 colored Polygon primitives (~52000 lines XML)
- Optional="true" attribute support on `<Include>` XML element
  - When set, missing include files produce a warning instead of an error


[2.7.1] - 2026-01-07
--------------------

### Fixed
- Comprehensive memory leak cleanup across the entire application:
  - apps/dcapp.c: Added comprehensive pl_app_shutdown() cleanup with per-node resource freeing, trick/mjpeg context cleanup, texture/shader resource cleanup, draw batch system cleanup, staging buffer cleanup, lookup/config table cleanup
  - extensions/dc_draw_ext.c: Fixed missing sbfree() calls in pl_return_2d_drawlist() and pl_return_3d_drawlist()
  - src/app/config.c: Added style XML node freeing, const value freeing, free(config) call
  - src/app/lookup.c: Added free(lookup) call
  - src/pixelstream/mjpeg.c: Added per-context cleanup in dc_ps_mjpeg_cleanup()
  - src/trick.c: Added missing sbfree() for rx_oad_var_values and offsets arrays


[2.7.0] - 2026-01-07
------------------------------------------------

### Added
- `<Function>` XML element for calling C functions from XML
  - New enum value: NODE_TYPE_FUNCTION, DC_APP_ELEM_TYPE_FUNCTION
  - New struct _NodeFunction with field: void (*callback)(void)
  - New function _draw_node_function(): calls node->function.callback() if non-NULL
  - New function _process_xml_node_function(): parses Name attribute, loads function from logic library
  - XML element: <Function Name="function_name"/>, function signature: void function_name(void)
  - samples/functions/ demo with button click handlers calling C functions from XML
- Cross-platform DLL/SO/DYLIB logic library loading: strips existing extension, tries .so, .dylib, .dll in order
- Documentation updates for `<Function>` element and cross-platform library loading

### Changed
- Updated start and build scripts with improved path handling


[2.6.1] - 2026-01-07
-----------------------------

### Changed
- Permanently removed deprecated sample directories that were previously hidden with '.' prefix
- Updated VS Code launch.json: added sample picker input for selecting which sample to run, removed preLaunchTasks
- Added recommendation for clangd extension in extensions.json


[2.6.0] - 2026-01-06
---------------------------------------------------------

### Added
- Alignment demo sample demonstrating ParentAlign, LocalAlign, and PivotAlign positioning system
- Rotation demo sample demonstrating rotation transforms with pivot points
- Lissajous curve sample demonstrating parametric equations with Set operations and trigonometric functions
- Additional demonstration samples showcasing various dcapp primitives
- Scene demo sample demonstrating a composed scene with multiple primitives


[2.5.1] - 2026-01-06
------------------------------------------------------------------

### Changed
- Prefixed deprecated/unimplemented sample directories with '.' to hide them
- Rewrote blink sample with more comprehensive demonstration
- Renamed ADI sample files for consistency
- Rewrote buttons sample with comprehensive button type demonstrations
- Rewrote colors sample with comprehensive color demonstrations
- Rewrote specs sample with comprehensive specification demonstrations


[2.5.0] - 2026-01-05
--------------------------------------

### Added
- ADI (Attitude Direction Indicator) sample with textured sphere for attitude ball, rate indicator bars, sliders for controlling roll/pitch/yaw, and auto-animate toggles
- Auto-increment functionality with bounce logic for ADI sample

### Fixed
- Fixed positioning of rate indicator bars in ADI sample
- Fixed missing `break` statement in GTE conditional case in _dcapp_draw.c


[2.4.1] - 2026-01-06
--------------------

### Fixed
- Fixed percentage escape bug: added '%' to escape character list in text content parsing (changed "@#$" to "@#$%")


[2.4.0] - 2026-01-05
--------------------

### Added
- `<Arc>` XML primitive for drawing circular arcs and pie shapes
- New enum values: DC_APP_ELEM_TYPE_ARC, NODE_TYPE_ARC
- New struct _NodeArc: position, alignment, pivot, rotation, radius, angle, segments, line_width, fill_color, line_color, pie flag
- New constant _NODE_ARC_MAX_SEGMENTS = 200
- New function _draw_node_arc(): draws arc shapes with pie mode (closed wedge), arc line drawing, arc fill drawing, full transform pipeline
- New function _process_xml_node_arc(): parses PositionX/Y, ParentAlignX/Y, LocalAlignX/Y, Rotation, PivotPositionX/Y, PivotLocalAlignX/Y, Radius, Angle, Segments, LineWidth, FillColor, LineColor, Pie
- samples/arc/arc.xml demonstration


[2.3.0] - 2026-01-05
--------------------

### Added
- `<MouseMotion>` XML primitive for mouse drag tracking
- New enum values: DC_APP_ELEM_TYPE_MOUSE_MOTION, NODE_TYPE_MOUSE_MOTION
- New struct _NodeMouseMotion with fields: var_x (DcAppVarIndex), var_y (DcAppVarIndex)
- New function _draw_node_mouse_motion(): transforms mouse position from screen to parent virtual coordinates, sets VariableX and VariableY
- New function _process_xml_node_mouse_motion(): parses VariableX, VariableY attributes
- New Set operations: DC_APP_SET_TYPE_MIN and DC_APP_SET_TYPE_MAX for clamping values
- New constant strings "_set_min_" and "_set_max_"
- samples/slider/slider.xml demonstrating slider with MouseMotion and Set clamping


[2.2.1] - 2026-01-05
--------------------

### Fixed
- Fixed UV mapping for sphere: increased GPU staging buffer from 1MB to 10MB for larger texture uploads
- Fixed orthographic projection matrix: Y axis no longer flipped, uses bottom-left origin coordinate system


[2.2.0] - 2026-01-04
--------------------

### Added
- `<Sphere>` XML primitive with texture mapping and full transform support
- New enum values: NODE_TYPE_SPHERE, DC_APP_ELEM_TYPE_SPHERE
- New struct _NodeSphere: position, alignment, pivot, rotation, radius, color, RPY (roll/pitch/yaw), image
- Draw batch system for ordering 2D and 3D draw calls:
  - New struct _DrawBatch for batch tracking
  - New struct _DrawList2D for 2D draw list wrapper
  - New functions: _draw_batch_reset(), _draw_batch_get_2d(), _draw_batch_get_3d()
- New function _draw_node_sphere(): full sphere rendering with position, rotation, pivot, alignment, roll/pitch/yaw transforms, texture mapping
- New function _process_xml_node_sphere(): parses PositionX/Y, ParentAlignX/Y, LocalAlignX/Y, PivotLocalAlignX/Y, PivotPositionX/Y, Rotation, Radius, FillColor, Roll, Pitch, Yaw, Image
- Refactored all existing draw calls from app_data->pl_layer to _draw_batch_get_2d() for batched rendering


[2.1.0] - 2026-01-04
-----------------------------

### Added
- Forked pilotlight draw extension into dcapp's own dc_draw_ext with full stencil masking support
- New file extensions/dc_draw_ext.c with full 2D/3D drawing API implementation:
  - Functions: pl__add_3d_circle_xz_filled(), pl__add_3d_band_xz_filled(), pl__add_3d_band_xy_filled(), pl__add_3d_band_yz_filled(), pl__add_3d_sphere_filled(), pl__add_3d_cylinder_filled(), pl__add_3d_cone_filled(), pl__add_3d_line(), pl__add_3d_text(), pl__add_3d_cross(), pl__add_3d_transform(), pl__add_3d_frustum(), pl__add_3d_sphere_ex(), pl__add_3d_capsule_ex(), pl__add_3d_cylinder(), pl__add_3d_cone_ex(), pl__add_3d_circle_xz(), pl__add_3d_centered_box(), pl__add_3d_aabb(), pl__add_3d_bezier_quad(), pl__add_3d_bezier_cubic()
- New file extensions/dc_draw_ext.h with structs: plDrawI (version 1.4.0), plDrawInit, plDrawFrustumDesc, plDrawLineOptions, plDrawSolidOptions, plDrawTextOptions, plFontRange, plFontConfig, plFontGlyph, plFont, plDrawVertex, plDrawVertex3DSolid, plDrawVertex3DLine, plDraw3DText, plDrawList3D, plDrawCommand, plDrawList2D, plFontAtlas
- New enums: _plDrawFlags, _plDrawRectFlags
- New shaders: dc_draw_2d.frag/vert, dc_draw_2d_sdf.frag, dc_draw_2d_sdf_stencil.frag, dc_draw_2d_stencil.frag, dc_draw_3d.frag/vert, dc_draw_3d_line.vert
- Stencil masking support with SDF text compatibility
- samples/stencil/stencil.xml with 8 test cases demonstrating StencilAdd/StencilRemove/StencilDraw phases
- `<Mask>` XML element for stencil-based masking/clipping


[2.0.11] - 2025-12-17
---------------------------------------

### Added
- NASA Open Source License file

### Removed
- Deprecated files from legacy codebase


[2.0.10] - 2025-12-16
--------------------

### Added
- `<Blink>` XML primitive for toggling visibility of children at a specified rate
- New enum values: DC_APP_ELEM_TYPE_BLINK, NODE_TYPE_BLINK
- Blink attributes: Rate (seconds between toggles)


[2.0.9] - 2025-12-16
--------------------

### Added
- Bitstream Vera font files bundled: Vera.ttf, VeraMono.ttf, and others
- Consistent text rendering across platforms with bundled fonts


[2.0.8] - 2025-12-10
-----------------------------

### Added
- `<Button>` XML element with press/release event handling
- Button is essentially a container with mouse event children
- Supports OnPress, OnRelease events that trigger child node execution
- Button-related struct definitions

### Fixed
- Button initialization value error


[2.0.7] - 2025-11-16
---------------------------------------

### Added
- Command-line argument parsing for dcapp executable
- Supports passing constants via command line (e.g., CONSTANT=VALUE)
- Windows platform support: Windows-specific socket code (WSAStartup), Windows build script generation, platform-specific file path handling

### Changed
- Generated dcapp.h variables changed from direct values to pointers for bidirectional sync


[2.0.6] - 2025-11-15
--------------------

### Added
- Immutable attribute for `<Constant>` XML element
- New struct _Constant with val and is_immutable fields
- Immutable constants cannot be overwritten by later `<Constant>` definitions
- All built-in constants (alignment types, conditional types, set types) marked as immutable
- Color constants marked as mutable (can be overridden)

### Changed
- Renamed _set_const_by_name -> _register_const_by_name with is_immutable parameter
- Renamed dc_app_config_set_const_by_name -> dc_app_config_register_const_by_name


[2.0.5] - 2025-11-15
--------------------

### Added
- Multiline string support: \n escape sequence in `<Text>` content for multiline text rendering
- New utility function dc_utils_char_in


[2.0.4] - 2025-11-06
------------------------------------------------

### Added
- `<PixelStream>` XML element for MJPEG video streaming display
- New enum values: DC_APP_ELEM_TYPE_PIXELSTREAM, NODE_TYPE_PIXELSTREAM
- PixelStream types: DC_APP_PIXELSTREAM_TYPE_DYNAMIC_FILE, DC_APP_PIXELSTREAM_TYPE_MJPEG
- New files: src/pixelstream/mjpeg.c, src/pixelstream/mjpeg.h
- New struct _NodePixelstream: position, dimension, pivot_local_align, pivot_position, local_align, parent_align, rotation, texture_index, mouse_events, type, frame, frame_width, frame_height, union { _PixelstreamMjpegData mjpeg }
- New struct _PixelstreamMjpegData: handle (DcPsMjpegHandle), raw_jpeg (unsigned char*), raw_jpeg_size (size_t)
- New struct DcPsMjpegHandle with internal MJPEG streaming handle
- Functions: dc_ps_mjpeg_init, dc_ps_mjpeg_update, dc_ps_mjpeg_cleanup, dc_ps_mjpeg_add_server, dc_ps_mjpeg_remove_server, dc_ps_mjpeg_server_is_connected, dc_ps_mjpeg_server_has_new_data, dc_ps_mjpeg_get_server_data
- MJPEG parsing with boundary detection (_memmem, _memrmem helper functions)
- PixelStream attributes: URL, Type, Width, Height
- Default constants: _pixelstream_mjpeg_, _pixelstream_dynamic_file_

### Fixed
- Fixed graphics initialization to use updated PilotLight API
- Fixed uninitialized position bug in image/pixelstream rendering
- Added proper quad rendering with UV coordinates for images/pixelstreams
- Added PL-specific Y-axis flip transforms for correct coordinate system
- Fixed resource cleanup on shutdown (destroy staging buffer)
- Removed deprecated draw backend calls and libcurl case


[2.0.3] - 2025-10-28
------------------------------------------------

### Changed
- Split monolithic dcapp.c into separate files:
  - dcapp.h: Shared header with all type definitions, global declarations
  - dcapp.c: App lifecycle (load/shutdown/resize/update)
  - _dcapp_draw.c: Node drawing functions
  - _dcapp_process_xml.c: XML processing and node creation
  - _dcapp_utils.c: Utility functions, initialization
- Cleaned up draw and process_xml functions
- Merged PilotLight app data and dcapp data into single _AppData struct, removed separate plAppData struct
- Moved PL declarations to main .c file


[2.0.2] - 2025-10-28
-----------------------------

### Added
- `<Image>` XML element for displaying raster images
- New enum values: DC_APP_ELEM_TYPE_IMAGE, NODE_TYPE_IMAGE
- New struct DcAppNodeImage: position, origin, dimensions, alignment, parent_align, rotation, file_path (char*), texture_id
- Image attributes: File, X, Y, Width, Height, OriginX, OriginY, AlignX, AlignY, ParentAlignX, ParentAlignY, Rotate
- Image loading using stb_image via PilotLight API
- Texture management: deduplication of loaded textures, GPU upload via staging buffer
- Moved PilotLight graphics initialization into dedicated function

### Changed
- Renamed global dc_app_data to _dc_data (static/internal to reduce global scope)


[2.0.1] - 2025-10-25
-----------------------------

### Added
- `<Circle>` XML element with rendering support
- New enum values: DC_APP_ELEM_TYPE_CIRCLE, NODE_TYPE_CIRCLE
- New struct DcAppNodeCircle: position, origin, alignment, parent_align, radius, fill_color, line_color, line_width, fill_enabled, line_enabled
- Circle attributes: X, Y, Radius, FillColor, LineColor, LineWidth, OriginX, OriginY, AlignX, AlignY, ParentAlignX, ParentAlignY
- `<Rectangle>` XML element with rendering support
- New enum values: DC_APP_ELEM_TYPE_RECTANGLE, NODE_TYPE_RECTANGLE
- New struct DcAppNodeRectangle: position, origin, alignment, parent_align, dimensions, fill_color, line_color, line_width, corner_radius, fill_enabled, line_enabled
- Rectangle attributes: X, Y, Width, Height, FillColor, LineColor, LineWidth, CornerRadius, OriginX, OriginY, AlignX, AlignY, ParentAlignX, ParentAlignY
- `<Line>` XML element for drawing line segments
- New enum values: DC_APP_ELEM_TYPE_LINE, NODE_TYPE_LINE
- New struct DcAppNodeLine: position, origin, alignment, parent_align, points (DcAppValueIndex2*), num_points, color (DcAppValueIndex4), width (DcAppValueIndex)
- Line attributes: LineColor, LineWidth, uses Vertex children for points

### Changed
- Removed parent_align from polygon (lines replace polygon line-only use case)


[2.0.0] - 2025-05-04
---------------------

### Added (Initial Project)
- Complete dcapp project built on PilotLight rendering framework
- Core architecture: XML-driven display configuration parsed into a node tree, rendered via Vulkan/Metal
- Apps: dcapp.cpp (main app), dcapp-genheader.cpp (header generator), dcapp-gendem.cpp (DEM placeholder)
- Structs: plAppData, DcValue (type, valueString/Integer/Float/Boolean, isDynamic), DcValue2/3/4, DcValueIndex/2/3/4, DcNode (union of all node types), DcNodeConditional, DcNodeContainer, DcNodeMap, DcNodePanel, DcNodePolygon, DcNodeWindow, DcLogic, DcVariable, DcappData
- Enums: DcValueType (UNDEFINED/STRING/INTEGER/FLOAT/BOOLEAN), DcElemType (18 values), DcAlignType (7 values), DcConditionalType (9 values), DcNodeType (7 values)
- Value system functions: valueTypeFromString, createTypedValueFromString, createValueString/Integer/Float/Boolean, refreshValue
- Data system functions: elemToString, stringToElem, xmlNodeToElementType, setConstant, getConstant, dereferenceConstants, indexToDcValue, registerDcValue, createAndRegisterTypedDcValueFromString, setVariable, dcNodeTypeToString, indexToDcNode, registerDcNode, cleanXmlData, initData
- String utils: stringToFloat, stringToInteger, stringToBoolean, stringToHash, trimWhitespace, splitStringByDelimiters
- File utils: filepathToCanonical, getExeFilepath
- XML utils: getAttributeString, getNodeContentString
- Math utils: doubleEquals, floatEquals
- XML elements: DCAPP, Window, Panel, Container, Polygon, Vertex, Variable, Constant, Include, Logic, If, True, False, Dummy
- XML attributes: Title, X, Y, Width, Height, VirtualWidth, VirtualHeight, HorizontalAlign, VerticalAlign, OriginX, OriginY, Rotate, FillColor, LineColor, LineWidth, Name, Type, InitialValue, Operation, Value, Value1, Value2, File, Directory
- Variable referencing syntax: @ prefix for variables, # prefix for constants, $ for env vars, {} for braced expansion
- Build system: Python-based gen-build.py using PilotLight build system (Linux/macOS/Windows)
- Default constants: alignment types, conditional types, 8 basic colors (black, blue, green, cyan, red, magenta, yellow, white)

### Added (Trick Integration)
- Trick variable server client: src/sb.h, src/trick.c, src/trick.h
- Stretchy buffer library with macros: sbheader, sbcount, sbcapacity, sbgrow, sbpush, sbcat, sbfree, sbclear, sbpushn, sbpop, sbpopn, sbshift, sbshiftn
- TrickContext struct: host, port, isAlive, dataRate, hasNewData, socket fields, rx/tx buffers
- Trick functions: trickInitContext, trickCleanupContext, trickAddTxVariable, trickSetVariable, trickAddRxVariable, trickAddRxOadVariable, trickConnectToServer, trickReceive
- Platform-independent socket abstraction: src/sock.c, src/sock.h
- DcSock struct (sock_fd, flags), DcSockResult enum (SUCCESS/FAIL/CONN_WOULD_BLOCK/CONN_INTERRUPTED/CONN_CLOSED), DcSockFlags (NONE/NON_BLOCKING/NON_NAGLE), DcSockState (UNKNOWN/DISCONNECTED/CONNECTING/CONNECTED)
- Socket functions: dc_sock_create, dc_sock_host_to_ip, dc_sock_set_non_nagle, dc_sock_set_non_blocking, dc_sock_connect, dc_sock_close, dc_sock_connection_status, dc_sock_send, dc_sock_receive
- Trick integration in main loop with XML elements: `<TrickIO>`, `<FromTrick>`, `<ToTrick>`, `<TrickVariable>`
- DcAppTrickContext struct with DcTrick*, rx_var_contexts, tx_var_contexts vectors
- DcAppTrickTxVarContext (dcapp_var_index, trick_var_index, prev_value), DcAppTrickRxVarContext (dcapp_var_index, trick_var_index)
- TrickIO attributes: Host, Port, Type (data rate); TrickVariable attributes: Name, Units
- Functions: dc_app_get_var_index(), dc_app_set_var_to_string(), dc_app_refresh_var_from_extern(), dc_app_refresh_var_from_value()
- dc_value_copy(), dc_value_is_equal() functions
- is_connected field in DcTrick struct
- DcAppVariableIndex typedef (uint32_t) for indexed variable storage

### Added (Set Primitive)
- `<Set>` XML element for modifying variables at draw time
- DcAppNodeSet struct with var_index, operation, operand fields
- DcAppSetType enum: EQUAL, ADD, SUBTRACT, MULTIPLY, DIVIDE
- Default constants: _set_equal_, _set_add_, _set_subtract_, _set_multiply_, _set_divide_

### Added (If Conditional)
- `<If>` conditional rendering in draw loop with TRUE/FALSE/EQ/NE/LT/GT/LTE/GTE evaluation
- Value comparison functions: dc_value_is_equal, dc_value_is_not_equal, dc_value_is_greater, dc_value_is_greater_or_equal, dc_value_is_less, dc_value_is_less_or_equal
- `<True>` and `<False>` XML child element parsing: DC_APP_ELEM_TYPE_TRUE, DC_APP_ELEM_TYPE_FALSE

### Added (Text Primitive)
- `<Text>` XML element with variable interpolation (@varname, @{varname}) and format specifiers
- DcAppNodeText struct: position, origin, alignment, size, sb_fillers, sb_filler_indices, sb_formats, sb_format_indices, vals
- Format specifier validation: dc_utils_format_specifier_length_bool/int/float/string
- Text rendering using PilotLight SDF font rendering
- Text attributes: X, Y, Size, Rotate, Color, Font
- MSAA (Multi-Sample Anti-Aliasing) support via PL_STARTER_FLAGS_MSAA

### Added (Terrain)
- `<Terrain>` XML element infrastructure: DC_APP_ELEM_TYPE_TERRAIN, DC_APP_NODE_TYPE_TERRAIN
- DcAppNodeTerrain struct: position, origin, dimensions, virtual_dimensions, alignment, rotation, child
- Terrain rendering source files with DEM support
- Terrain shader infrastructure and extension setup code

### Added (Mouse Events)
- Mouse event system with hit testing for polygons
- Frame data: pressed_node, hovered_node, is_mouse_pressed, is_mouse_released, is_mouse_down, mouse_position
- _MouseEventChildren struct: active, hovered, inactive, pressed, released node indices and enabled flag
- XML elements: `<MouseActive>`, `<MouseHovered>`, `<MouseInactive>`, `<MousePressed>`, `<MouseReleased>`
- New enum values: DC_APP_ELEM_TYPE_MOUSE_ACTIVE/HOVERED/INACTIVE/PRESSED/RELEASED

### Added (Defaults and Styles)
- `<Style>` XML element for reusable attribute sets
- `<Default>` element for default attribute values per element type
- Style system: define named styles applied to elements via Style="name" attribute
- dc_app_config_save_to_file, dc_app_config_clean_xml functions

### Added (ParentAlign)
- ParentAlignX and ParentAlignY attributes for parent-relative alignment
- ParentAlign added to Container, Polygon, Text, Terrain, Window, and all remaining node types
- Unified alignment+position transform logic across all node types

### Changed (C++ to C Conversion)
- Converted entire codebase from C++ to C:
  - Replaced std::map with stretchy buffer-based lookup tables
  - Replaced std::vector with stretchy buffers
  - Replaced std::string with char arrays and fixed-size buffers (DC_VALUE_STRING_BUFFER_SIZE = 512)
  - Files renamed from .cpp/.hpp to .c/.h
  - New architecture: src/app/config.c/h, src/app/lookup.c/h, src/app/elem.c/h
  - New structs: DcAppConfig (xml_doc, config_file_path, etc.), DcAppLookup (_index handle)
  - DcValue uses char value_string[DC_VALUE_STRING_BUFFER_SIZE]
  - All C++ exceptions replaced with fprintf(stderr) error messages

### Changed (snake_case Refactor)
- Massive renaming from camelCase/PascalCase to snake_case across entire codebase
- Removed C++ namespace "dc" entirely
- All functions renamed with dc_ prefix: filepathToCanonical -> dc_utils_filepath_to_canonical, getExeFilepath -> dc_utils_get_exe_filepath, doubleEquals -> dc_utils_double_equals, stringToFloat -> dc_utils_string_to_float, stringToBoolean -> dc_utils_string_to_boolean, stringToHash -> dc_utils_string_to_hash, trimWhitespace -> dc_utils_trim_whitespace, splitStringByDelimiters -> dc_utils_split_string_by_delimiters, getAttributeString -> dc_utils_get_attribute_string, getNodeContentString -> dc_utils_get_node_content_string, valueTypeFromString -> dc_value_type_from_string, createTypedValueFromString -> dc_value_create_typed_value_from_string, etc.
- Renamed value enum types DC_VALUE_TYPE_* -> DC_APP_VALUE_TYPE_* (then back to DC_VALUE_TYPE_*)
- Renamed util files: file-utils.hpp -> file.hpp, string-utils -> string, xml-utils -> xml, math-utils -> math
- All struct members renamed to snake_case: fillEnabled -> fill_enabled, lineEnabled -> line_enabled, numPoints -> num_points, childTrue -> child_true, virtualDimensions -> virtual_dimensions, lineColor -> line_color, lineWidth -> line_width, preInit -> pre_init, valueIndex -> value_index, configDirPath -> config_dir_path, valueString -> value_string, valueInteger -> value_integer, valueFloat -> value_float, isDynamic -> is_dynamic
- DcAppData -> DcAppData with dc_app_data global instance
- DcNode -> DcAppNode, DcVariable -> DcAppVariable, etc.
- dc_app_index_to_dc_value -> dc_app_get_value, dc_app_register_dc_value -> dc_app_register_value
- dc_app_set_variable -> dc_app_register_variable
- Variable storage changed from map<string, DcAppVariable> to vector + index map
- dc_app_register_variable -> dc_app_register_var, DcAppVariable -> DcAppVar, variable_indices -> var_indices, variables -> vars
- dc_app_index_to_node -> dc_app_get_node

### Changed (Float to Double)
- Changed DcValue.value_float from float to double throughout
- Changed dc_value_create_value_float parameter from float to double
- Updated genheader to output "double" instead of "float" for DC_VALUE_TYPE_FLOAT variables

### Changed (Other)
- Applied consistent clang-format across all source files
- Standardized positional parameter naming across all node types
- Text parsing rewritten from C++ std::string/vector to C stretchy buffers

### Fixed
- Fixed dc_sock_connection_status() with null/invalid fd check, error fd_set, getpeername() verification
- Fixed Trick receive: removed off-by-2 offset error, added null terminator, fixed variable index check
- Fixed missing return DC_TRICK_RESULT_FAIL in default case of _dc_trick_receive
- Fixed TrickTo initialization bug: set dcapp_var_index before using it to access dc_value
- Fixed initial value handling: after loading logic library, calls dc_app_refresh_var_from_value
- Fixed C++ struct initialization GCC issue: changed from designated initializer list to explicit member assignment
- Fixed text rotation offset
- Fixed window default values (0,0 position, 800x600 dimensions) and error checks
- Fixed Trick units parsing when units are not provided (null check)
- Fixed libxml2 include path for C
- Fixed broken includes after C++ to C conversion
- Fixed DC_TRICK_RESULT_FAILss -> DC_TRICK_RESULT_FAIL (bad Unicode character removed)
- Fixed build for macOS
- Fixed general bugs found during sample integration
- Fixed pixelstream and image draw ordering

### Removed
- apps/dcapp-gendem.cpp placeholder
- shaders/Makefile
- src/utils/math.cpp and math.hpp (unused)
- GDAL and ZIP library dependencies from build system
- Deprecated C++ string utility file

### Chores
- VSCode launch configurations for Linux (gdb), macOS (lldb), Windows (gdb)
- VSCode build tasks for Linux and macOS debug
- Updated .clang-format from Mozilla-based to LLVM-based style
- Updated pilotlight submodule
- Added shebang to gen-build.py
- Updated windows build jobs
- Renamed .c files to .cpp: sock.c->sock.cpp, trick.c->trick.cpp, sb.h->sb.hpp, sock.h->sock.hpp, trick.h->trick.hpp
- Added template function sbshift_fn`<T>` to replace GCC typeof extension in sb.hpp
- Added tests/build-tests.py and tests/trick.cpp test file
- Changed sprintf -> snprintf throughout trick.c for safety
- Added DC_TRICK_TEMP_BUFFER_SIZE constant (16384)
- Added execution order documentation to README
- Added legacy sample XML files for testing compatibility
- Updated start scripts
- Updated sample build scripts
- Updated documentation
- Variables now use constant expansion for types
- Minor refactor of deprecated xml code
