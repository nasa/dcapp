# Architecture

dcapp is a PilotLight application. It preprocesses an XML display, builds a
retained node model, and resolves that model into draw lists every frame.

## Application lifecycle

PilotLight calls the entry points in `apps/dcapp.c`:

- `pl_app_load`
- `pl_app_shutdown`
- `pl_app_resize`
- `pl_app_update`

`pl_app_load` loads `pl_unity_ext`, `pl_platform_ext`, `dc_draw_ext`,
`dc_draw_backend_ext`, `pl_planet_processor_ext`, and `pl_planet_ext`, then
initializes the dcapp subsystems. Each implementation file keeps the APIs it
uses in file-local `_ext_*` pointers and refreshes them through its `*_init`
function.

PilotLight owns the application loop, platform windows, GPU device, swapchain,
render pass, extension registry, memory tracking, and common resource APIs.
dcapp owns XML processing, runtime nodes and variables, display logic, external
data mappings, mouse behavior, and its drawing commands.

Startup follows this sequence:

1. load the required extensions;
2. read the XML file and command-line constants;
3. preprocess authoring elements;
4. validate and parse the resulting XML;
5. create the display model and any draw, texture, font, stream, data-link,
   logic, or planet contexts it needs.

## Contexts and ownership

The private `_AppData` in `apps/dcapp.c` is the composition root. It keeps
the window, XML preprocessor, and opaque pointers to subsystem contexts. Arrays,
allocators, callbacks, mouse state, and render scratch remain inside their
owning contexts.

Subsystem contexts are allocated separately so their addresses do not change.
Movable stretchy buffers are private to their owner.

- Nodes, textures, and pixel-stream sources use append-only integer IDs.
- Node and texture ID `0` means undefined.
- Planet and planet-view handles are separately allocated because callers keep
  those handles.
- The display model owns parsed planet definitions and its planet-view node
  registry. The planet context owns resolved planets, views, breadcrumbs, and
  GeoJSON resources.
- Socket, Trick, Edge, GeoJSON, MJPEG, and shared-memory handles refer to
  separate allocations, not entries in a movable module registry.

`DcAppDisplayBuilderContext` exists only while the XML is being built.
`DcAppDisplayRuntimeContext` borrows the runtime services it needs and owns
deferred operations and frame scratch. Neither context receives `_AppData`.

## XML pipeline

The element registry is split across:

- `src/app/xml_element_types.h`: `DcAppXmlElementType`
- `src/app/xml_element.h`: registry API
- `src/app/xml_element.c`: XML name mapping

An unregistered name is not a first-class dcapp element to the builder or
validator.

`src/app/xml_preprocessor.c` handles:

- `Constant` declarations and command-line overrides;
- `Include` expansion;
- `Dummy` child splicing;
- `Default` and `Style` expansion;
- static `If` branches;
- `AlignX` and `AlignY` shorthand;
- `_Directory` propagation for relative paths in included files.

The preprocessed document can be inspected directly:

```bash
./bin/dcapp-validate.sh path/to/display.xml --preprocessed cache/display.preprocessed.xml
```

`src/app/display_builder.c` converts that document into the `DcAppNode`
records declared in `src/app/node.h`. Its recursive entry point is
`dc_app_display_builder_process_xml_node`; runtime elements generally have a
matching `_process_xml_node_*` helper. Nodes refer to other nodes by index
rather than pointer. `DcAppDisplayModelContext` owns the node buffer and the
`DcAppVariableRegistryContext`.

`apps/dcapp_validate.c` validates after preprocessing, so `Constant`,
`Default`, `Style`, `Include`, `Dummy`, and static branches are not part
of the runtime tree. The files that must move together when the XML vocabulary
changes are listed in [Coding style](coding-style.md#xml-changes).

## Frame order

`pl_app_update` performs each frame in this order:

1. `_ext_starter->begin_frame()`
2. `_ext_resource->new_frame()`
3. update Trick and Edge data links
4. update pixel streams
5. run fixed-rate display logic
6. refresh values
7. pass raw mouse input to the draw context
8. call `_ext_dc_draw_backend->new_frame()`
9. reset draw batches
10. update planet definitions
11. traverse and draw the XML node tree
12. flush deferred `Set` operations
13. submit draw lists in the PilotLight render pass
14. commit draw-context interaction state

External input and logic update variables before traversal. Ordinary `Set`
operations run when their nodes are visited. A `Set` with `Defer="true"` is
queued and flushed after traversal in encounter order.

## Drawing

`src/app/display_runtime.c` evaluates node values, resolves layout and mouse
state, and calls the corresponding draw function before visiting the next
node. Drawing during traversal preserves XML order for ordinary elements as
well as `Function`, `DrawFunction`, stencil, and planet-view nodes.

The lower layers are:

- `src/app/draw.c`: public drawing helpers, batches, transform and stencil
  stacks, and mouse hit registration;
- `src/app/texture.c`: texture IDs, GPU textures, bind groups, and reusable
  upload staging;
- `extensions/dc_draw_ext.*`: immediate-mode 2D and 3D draw lists;
- `extensions/dc_draw_backend_ext.*`: GPU upload and submission;
- PilotLight: GPU and frame lifecycle.

XML primitives and C `DrawFunction` callbacks share the same drawing API.

## Logic libraries

A display may load one C or C++ library through `Logic`. Paths are relative
to the declaring XML file; dcapp tries platform library names both with and
without a `lib` prefix.

`src/app/display_logic.c` owns the library and these callbacks:

- `display_pre_init`
- `display_init`
- `display_draw`
- `display_close`

`dcapp-genheader` preprocesses the display and generates `logic/dcapp.h`.
The header exposes XML variables as pointers, typed C-linkage declarations for
`Function` and `DrawFunction` callbacks, the Windows export annotation, and
the `dc_draw`, `dc_mouse`, `dc_texture`, and `dc_planet` API tables.

`draw_api.h`, `texture_api.h`, and `planet_api.h` own those contracts.
`display_logic_api.h` aggregates them for initialization. The generator
exports a separate short-name API for standalone logic builds rather than the
internal `DcApp*` names and headers. See [Logic](logic.md) for the build and
callback details.

## Values and external data

`DcAppValue` in `src/app/value.c` represents runtime values.
`DcAppVariableRegistryContext` in `src/app/variable_registry.c` owns named
variables and their values. XML attributes, text interpolation, `Set`, logic
variable pointers, I/O mappings, buttons, and mouse state all use this layer.

External protocols remain outside the XML parser:

- `src/trick.c` implements Trick Variable Server;
- `src/edge.c` implements Edge;
- `src/pixelstream/mjpeg.c` and `src/pixelstream/shmem.c` implement the two
  pixel-stream transports;
- `src/app/data_link.c` owns Trick and Edge connections and variable
  bindings;
- `src/app/pixelstream.c` owns stream IDs, frames, and texture uploads.

The XML nodes connect these contexts to the display model. Protocol updates run
before the model is traversed.

## Planet rendering

Terrain processing and chunk generation use
`extensions/pl_planet_processor_ext.c` and the
`dcapp-planet-chunkgen` wrappers. Runtime terrain and overlays use
`extensions/pl_planet_ext.c`, `src/app/planet.c`, and the planet nodes built
by `display_builder.c`.

The display model owns parsed definitions. The display runtime resolves and
updates them, and the planet context owns the resulting resources. Geographic
helpers live in `src/geo.c` and `src/geojson.c`. Planet views run in the
normal XML frame traversal.

## Application reload

During a PilotLight application-library reload, `pl_app_load` retains the
heap-owned contexts, reacquires extension APIs, and reruns logic
pre-initialization with the new API-table addresses. MJPEG refreshes its
libcurl callbacks before libcurl can invoke them.

This works for code-only reloads while retained context layouts stay
compatible. A change to the layout or meaning of retained private state
requires a full restart; dcapp does not migrate that state.

## File map

| Area | Files |
|------|-------|
| App lifecycle | `apps/dcapp.c` |
| Preprocessing and element names | `src/app/xml_preprocessor.c`, `src/app/xml_element*` |
| Model construction | `src/app/display_builder.c`, `src/app/display_model.c`, `src/app/node.h` |
| Frame traversal | `src/app/display_runtime.c` |
| Drawing and textures | `src/app/draw.c`, `src/app/texture.c`, `extensions/dc_draw_*` |
| Logic loading and header generation | `src/app/display_logic.c`, `apps/dcapp_genheader.c` |
| Validation | `apps/dcapp_validate.c` |
| Data links and streams | `src/app/data_link.c`, `src/app/pixelstream.c`, `src/trick.c`, `src/edge.c` |
| Planet runtime and tools | `src/app/planet.c`, `extensions/pl_planet_*`, `apps/dcapp_planet_chunkgen.c` |
