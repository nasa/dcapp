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
5. Initializes draw, texture, pixel stream, Trick, Edge, logic, and planet
   systems as needed.

Every frame, dcapp:

1. Polls external IO sources.
2. Runs fixed-rate display logic callbacks.
3. Refreshes variables and mouse state.
4. Traverses the runtime node tree.
5. Emits draw commands into dcapp draw lists.
6. Submits those lists through the dcapp draw backend and PilotLight graphics.

## PilotLight Relationship

dcapp uses PilotLight as the host application, platform layer, renderer, memory
tracker, resource manager, and extension registry. The app entry points are the
PilotLight callbacks in `apps/dcapp/dcapp.c`:

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

After extension loading, dcapp resolves PilotLight and dcapp APIs into the
global `_ext_*` pointers declared in `apps/dcapp/dcapp.h`, such as
`_ext_starter`, `_ext_gfx`, `_ext_resource`, `_ext_dc_draw`,
`_ext_dc_draw_backend`, `_ext_planet`, and `_ext_planet_processor`.

The important boundary is:

- PilotLight owns the platform loop, GPU device, swapchain, render pass, memory
  tracking, extension registry, and common resource APIs.
- dcapp owns XML preprocessing/parsing, the display node tree, dcapp variables,
  display logic integration, external IO mappings, mouse display semantics, and
  the higher-level drawing commands used by XML and logic files.

## XML Pipeline

XML handling is split into three stages.

### Element Names

The canonical list of XML element names lives in `src/app/elem.h` and
`src/app/elem.c`. If an element is not recognized there, the parser and
validator will not treat it as a first-class dcapp XML element.

### Preprocessing

`src/app/config.c` preprocesses authoring-time XML features before validation
and runtime parsing. The major preprocessing features are:

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

`apps/dcapp/xml.c` parses the cleaned XML into `_Node` structs from
`apps/dcapp/node.h`. The main parser entry point is
`dc_app_process_xml_node`. Most runtime XML elements have a matching
`_process_xml_node_*` helper in `xml.c`.

The runtime node tree stores node indexes rather than pointers. Index `0` is
reserved as undefined for nodes and textures.

## Validation

`apps/dcapp_validate.c` preprocesses XML before validating it. Validation checks
the cleaned XML surface, so authoring helpers that disappear during
preprocessing are not expected to remain as runtime nodes.

When changing XML, keep these in sync:

- `src/app/elem.h`
- `src/app/elem.c`
- `apps/dcapp/xml.c`
- `apps/dcapp_validate.c`
- Relevant docs and samples

## Runtime Frame Loop

The frame loop is in `pl_app_update` in `apps/dcapp/dcapp.c`.

The high-level order is:

1. `_ext_starter->begin_frame()`
2. `_ext_resource->new_frame()`
3. Trick transmit/receive update
4. Edge transmit/receive update
5. PixelStream update
6. Fixed-rate display logic update
7. Value refresh
8. Mouse state update
9. `_ext_dc_draw_backend->new_frame()`
10. Draw batch reset
11. Planet definition updates
12. XML node traversal through `dc_app_draw_node`
13. Deferred `Set` flush
14. Draw-list submission during the PilotLight main render pass

This order matters. Logic and external IO update variables before the XML tree
is drawn. Deferred `Set` operations flush after drawing so updates from the
current traversal apply atomically.

## Drawing Stack

The drawing path is intentionally layered:

- `apps/dcapp/draw_node.c` knows how each XML node behaves. It evaluates node
  values, manages layout, handles mouse state, and decides what to draw.
- `apps/dcapp/draw.c` exposes the drawing helpers used by XML nodes and
  `DrawFunction` logic callbacks. It also manages draw batches, textures, and
  mouse hit registration.
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

Logic callback symbols are loaded dynamically from `apps/dcapp/xml.c`:

- `display_pre_init`
- `display_init`
- `display_draw`
- `display_close`

`./bin/dcapp-genheader.sh` generates `logic/dcapp.h` from the same XML after
preprocessing. The generated header exports:

- XML variables as pointers.
- `Function` callback registration.
- `DrawFunction` callback registration.
- API tables such as `dc_draw`, `dc_mouse`, `dc_texture`, and `dc_planet`.

See [logic.md](logic.md) for the full logic workflow.

## Variables And Values

Runtime values are represented by `DcValue` in `src/value.c`. Named XML
variables and values are tracked by the lookup system in `src/app/lookup.c`.

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
  `src/pixelstream/shmem.c`, then draw through `PixelStream` XML nodes.

See [trick.md](trick.md), [edge.md](edge.md), and [pixelstream.md](pixelstream.md).

## Planet Path

Planet rendering has two major halves:

- Data processing and chunk generation use `extensions/pl_planet_processor_ext.c`
  and the `./bin/dcapp-planet-chunkgen.*` wrappers.
- Runtime rendering and overlays use `extensions/pl_planet_ext.c`,
  `apps/dcapp/planet.c`, and the planet XML nodes parsed in
  `apps/dcapp/xml.c`.

Geo helpers live in `src/geo.c` and `src/geojson.c`. XML planet views draw
through the same frame loop as the rest of dcapp, but use PilotLight planet
resources and dcapp's planet draw helpers.

See [planet.md](planet.md) and [coordinate-frame.md](coordinate-frame.md).

## Source Map

| Area | Main Files |
|------|------------|
| App lifecycle and frame loop | `apps/dcapp/dcapp.c`, `apps/dcapp/dcapp.h` |
| XML preprocessing | `src/app/config.c` |
| XML element names | `src/app/elem.h`, `src/app/elem.c` |
| XML runtime parsing | `apps/dcapp/xml.c` |
| Runtime node structs | `apps/dcapp/node.h` |
| XML node drawing | `apps/dcapp/draw_node.c` |
| Draw API and batches | `apps/dcapp/draw.c`, `apps/dcapp/draw.h` |
| dcapp draw extensions | `extensions/dc_draw_ext.*`, `extensions/dc_draw_backend_ext.*` |
| Generated logic header | `apps/dcapp_genheader.c` |
| XML validator | `apps/dcapp_validate.c` |
| Planet runtime | `apps/dcapp/planet.c`, `extensions/pl_planet_ext.c` |
| Planet processing | `extensions/pl_planet_processor_ext.c`, `apps/dcapp_planet_chunkgen.c` |
| Trick and Edge | `src/trick.c`, `src/edge.c` |
| Pixel streams | `src/pixelstream/mjpeg.c`, `src/pixelstream/shmem.c` |
| Values and lookup | `src/value.c`, `src/app/lookup.c` |
