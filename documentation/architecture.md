# dcapp Architecture

This page describes how dcapp is put together internally. For XML authoring,
start with [primitives.md](primitives.md), [variables.md](variables.md), and
[logic.md](logic.md). For the user-side workflow, start with
[getting-started.md](getting-started.md).

## Big Picture

dcapp is a PilotLight application that turns XML into a live display tree.

At startup, dcapp:

1. Loads the PilotLight and dcapp extensions it needs.
2. Reads the requested XML file and command-line constant overrides.
3. Preprocesses XML authoring helpers such as `Include`, `Constant`, `Default`,
   `Style`, `Dummy`, and static `If`.
4. Parses the cleaned XML into runtime node structs.
5. Creates focused display-model, display-runtime, draw, texture, font,
   pixel-stream, data-link, display-logic, and planet contexts as needed.

Every frame, dcapp:

1. Polls external IO sources.
2. Runs fixed-rate display logic callbacks.
3. Refreshes variables and mouse state.
4. Traverses the runtime node tree.
5. Resolves and draws each node into dcapp draw lists in XML order.
6. Submits those lists through the dcapp draw GPU extension and PilotLight graphics.

## PilotLight Relationship

dcapp uses PilotLight as the host application, platform layer, renderer, memory
tracker, resource manager, and extension registry. The app entry points are the
PilotLight callbacks in `apps/dcapp.c`:

- `pl_app_load`
- `pl_app_shutdown`
- `pl_app_resize`
- `pl_app_update`

`pl_app_load` loads these required extensions:

- `pl_unity_ext`
- `pl_platform_ext`
- `dc_draw_ext`
- `dc_draw_backend_ext`
- `pl_planet_processor_ext`
- `pl_planet_ext`

After extension loading, `dcapp.c` calls each subsystem's `*_init` function.
Each implementation file resolves the PilotLight APIs it uses into file-local
`_ext_*` pointers. Subsystems do not include a root umbrella header or reach
through the application root to use another subsystem's state.

The important boundary is:

- PilotLight owns the platform loop, GPU device, swapchain, render pass, memory
  tracking, extension registry, and common resource APIs.
- dcapp owns XML preprocessing/parsing, the display node tree, dcapp variables,
  display logic integration, external IO mappings, mouse display semantics, and
  the higher-level drawing commands used by XML and logic files.

## Runtime Ownership

The private `_AppData` definition in `apps/dcapp.c` is the composition
root. It contains the window, XML preprocessor, and opaque pointers to the
owning subsystem contexts; it does not contain their arrays, allocators,
callbacks, mouse state, or display-runtime scratch data.

The contexts themselves are separately allocated, so their addresses remain
stable. Movable stretchy buffers stay private inside their owning context:

- Display-model nodes, textures, and pixel-stream sources are referenced by
  append-only integer IDs.
- Planet and planet-view handles are separately allocated opaque objects because
  callers retain those handles.
- The display model owns parsed planet definitions and its PlanetView-node registry.
  The planet context owns only resolved planet, view, breadcrumb, and GeoJSON
  resources.
- Socket, Trick, Edge, GeoJSON, MJPEG, and shared-memory handles refer to
  separately allocated objects. Their addresses do not depend on a
  module-static registry or a movable stretchy buffer.

The `DcAppDisplayBuilderContext` is startup-only. The
`DcAppDisplayRuntimeContext` borrows the focused runtime services it needs and
owns its deferred operations and render scratch. Neither receives `_AppData`.

## XML Pipeline

XML handling is split into three stages.

### Element Names

The canonical `DcAppXmlElementType` enum lives in
`src/app/xml_element_types.h`, with its API in `src/app/xml_element.h` and name
mapping in `src/app/xml_element.c`. If an element is not recognized there, the
builder and validator will not treat it as a first-class dcapp XML element.

### Preprocessing

`src/app/xml_preprocessor.c` and its `DcAppXmlPreprocessorContext` preprocess
authoring-time XML features before validation and runtime model construction.
The major preprocessing features are:

- `Constant` definitions and command-line constant overrides.
- `Include` expansion.
- `Dummy` child splicing.
- `Default` and `Style` expansion.
- Static `If` branch resolution.
- `AlignX` and `AlignY` shorthand expansion.
- `_Directory` propagation so included files can resolve relative resources.

The preprocessed XML can be written with:

```bash
./bin/dcapp-validate.sh path/to/display.xml --preprocessed cache/display.preprocessed.xml
```

### Runtime Parsing

`src/app/display_builder.c` builds the cleaned XML into `DcAppNode` records
from `src/app/node.h`. Its recursive public entry point is
`dc_app_display_builder_process_xml_node`. Most runtime XML elements have a
matching `_process_xml_node_*` helper in `display_builder.c`.

The runtime node tree stores node indexes rather than pointers. Index `0` is
reserved as undefined for nodes and textures. `DcAppDisplayModelContext` owns
both the node buffer and `DcAppVariableRegistryContext`.

## Validation

`apps/dcapp_validate.c` preprocesses XML before validating it. Validation checks
the cleaned XML surface, so authoring helpers that disappear during
preprocessing are not expected to remain as runtime nodes.

When changing XML, keep these in sync:

- `src/app/xml_element_types.h`
- `src/app/xml_element.h`
- `src/app/xml_element.c`
- `src/app/display_builder.c`
- `apps/dcapp_validate.c`
- Relevant docs and samples

## Runtime Frame Loop

The frame loop is in `pl_app_update` in `apps/dcapp.c`.

The high-level order is:

1. `_ext_starter->begin_frame()`
2. `_ext_resource->new_frame()`
3. Data-link context update (Trick and Edge)
4. PixelStream update
5. Fixed-rate display logic update
6. Value refresh
7. Raw mouse input is passed to the draw context
8. `_ext_dc_draw_backend->new_frame()`
9. Draw batch reset
10. Planet definition updates
11. The display runtime traverses the node tree in XML order, resolving and
    drawing each node before moving to the next node
12. Deferred `Set` flush
13. Draw-list submission during the PilotLight main render pass
14. Draw-context interaction state commit

This order matters. Logic and external IO update variables before the XML tree
is drawn. Deferred `Set` operations flush after drawing so updates from the
current traversal apply atomically.

## Drawing Stack

The drawing path is intentionally layered:

- `src/app/display_runtime.c` knows how each XML node behaves. It evaluates node
  values, resolves nested layout, handles mouse state, and immediately calls
  the matching draw helper before continuing traversal. This preserves XML
  ordering for `Function`, `DrawFunction`, stencil, and planet-view nodes.
- `src/app/draw.c` exposes the drawing helpers used by XML nodes and
  `DrawFunction` logic callbacks. It also manages draw batches, transform and
  stencil stacks, and mouse hit registration.
- `src/app/texture.c` owns texture IDs, GPU textures, bind groups, and the
  reusable upload staging buffer.
- `extensions/dc_draw_ext.*` stores immediate-mode 2D and 3D draw lists.
- `extensions/dc_draw_backend_ext.*` uploads and submits those draw lists using
  PilotLight graphics.
- PilotLight owns the GPU device, swapchain, render encoder, and frame
  lifecycle.

XML primitives and logic `DrawFunction` callbacks both end up using the same
dcapp draw API surface. See [logic.md](logic.md) for the generated logic header
and available draw calls.

## Logic Libraries

Displays can load one C/C++ logic library with the `Logic` element. The logic
file is resolved relative to the XML file directory, and dcapp will try platform
library names with and without a `lib` prefix.

`src/app/display_builder.c` recognizes the `Logic` element, while
`src/app/display_logic.c` owns the dynamic library and callback lifecycle:

- `display_pre_init`
- `display_init`
- `display_draw`
- `display_close`

`./bin/dcapp-genheader.sh` generates `logic/dcapp.h` from the same XML after
preprocessing. The generated header exports:

- XML variables as pointers.
- Typed, C-linked declarations for every `Function` and `DrawFunction`
  callback, including the Windows export annotation.
- API tables such as `dc_draw`, `dc_mouse`, `dc_texture`, and `dc_planet`.

Draw/mouse, texture, and planet own their respective public contracts in
`draw_api.h`, `texture_api.h`, and `planet_api.h`. `display_logic_api.h` only
aggregates those capabilities for initialization. The generator emits a
separate, explicitly curated short-name contract for standalone logic builds;
it does not expose the internal headers or `DcApp*` namespace.

See [logic.md](logic.md) for the full logic workflow.

## Variables And Values

Runtime values are represented by `DcAppValue` in `src/app/value.c`. Named XML
variables and values are tracked by `DcAppVariableRegistryContext` in
`src/app/variable_registry.c`.

Common value users include:

- XML attributes and text interpolation.
- `Set` operators.
- Logic variable pointers.
- Trick and Edge transmit/receive mappings.
- Mouse and button state nodes.

See [variables.md](variables.md) for authoring details.

## External IO

External IO is implemented outside the XML parser, then wired into XML nodes by
runtime contexts:

- Trick Variable Server support lives in `src/trick.c` and the `TrickIO`,
  `TrickVariable`, `TrickFrom`, and `TrickTo` XML nodes.
- Edge support lives in `src/edge.c` and the `EdgeIO`, `EdgeVariable`,
  `EdgeFrom`, and `EdgeTo` XML nodes.
- Pixel streams live in `src/pixelstream/mjpeg.c` and
  `src/pixelstream/shmem.c`; `src/app/pixelstream.c` owns their manager
  contexts, source IDs, frame storage, and texture uploads before
  `PixelStream` nodes draw them.
- `src/app/data_link.c` owns Trick and Edge connections and variable
  bindings.

See [trick.md](trick.md), [edge.md](edge.md), and [pixelstream.md](pixelstream.md).

## Planet Path

Planet rendering has two major halves:

- Data processing and chunk generation use `extensions/pl_planet_processor_ext.c`
  and the `./bin/dcapp-planet-chunkgen.*` wrappers.
- Runtime rendering and overlays use `extensions/pl_planet_ext.c`,
  `src/app/planet.c`, and the planet XML nodes parsed in
  `src/app/display_builder.c`. The display model owns the parsed definitions,
  the display runtime resolves and updates those definitions, and `planet.c`
  owns the resulting runtime resources.

Geo helpers live in `src/geo.c` and `src/geojson.c`. XML planet views draw
through the same frame loop as the rest of dcapp, but use PilotLight planet
resources and dcapp's planet draw helpers.

See [planet.md](planet.md) and [coordinate-frame.md](coordinate-frame.md).

## App Hot Reload

When PilotLight reloads the dcapp application library, `pl_app_load` keeps the
heap-owned subsystem contexts, reacquires every extension API, and reruns logic
pre-initialization so the logic library receives the new capability-table
addresses. MJPEG also refreshes libcurl callback pointers before libcurl may
invoke them.

This supports code-only reloads whose retained context layouts are unchanged.
Changing the layout or meaning of a retained private struct still requires a
full application restart; dcapp does not attempt live state migration.

## Source Map

| Area | Main Files |
|------|------------|
| App lifecycle and frame loop | `apps/dcapp.c` |
| XML preprocessing | `src/app/xml_preprocessor.c` |
| XML element names | `src/app/xml_element_types.h`, `src/app/xml_element.h`, `src/app/xml_element.c` |
| XML runtime parsing | `src/app/display_builder.c` |
| Display-model and variable-registry ownership | `src/app/display_model.c`, `src/app/variable_registry.c` |
| Runtime node structs | `src/app/node.h` |
| XML node resolution and ordered drawing | `src/app/display_runtime.c` |
| Draw API and batches | `src/app/draw.c`, `src/app/draw.h`, `src/app/draw_api.h` |
| Fonts and textures | `src/app/font.c`, `src/app/texture.c` |
| Logic library lifecycle | `src/app/display_logic.c` |
| Trick and Edge bindings | `src/app/data_link.c` |
| Pixel-stream runtime | `src/app/pixelstream.c` |
| dcapp draw extensions | `extensions/dc_draw_ext.*`, `extensions/dc_draw_backend_ext.*` |
| Generated logic header | `apps/dcapp_genheader.c` |
| XML validator | `apps/dcapp_validate.c` |
| Planet resource runtime | `src/app/planet.c`, `extensions/pl_planet_ext.c` |
| Planet processing | `extensions/pl_planet_processor_ext.c`, `apps/dcapp_planet_chunkgen.c` |
| Trick and Edge | `src/trick.c`, `src/edge.c` |
| Pixel streams | `src/pixelstream/mjpeg.c`, `src/pixelstream/shmem.c` |
| Values and variable registry | `src/app/value.c`, `src/app/variable_registry.c` |
