# dcapp Runtime Architecture Audit

This report audits the current uncommitted refactor, not the historical
architecture on the main branch. It is intentionally anally thorough. The
purpose is to distinguish dependencies that are normal and useful from
dependencies that point in the wrong direction, hide lifetime requirements, or
make unrelated parts of dcapp change together.

No implementation changes are proposed merely to reduce file size or include
count. The current behavior, XML ordering, callback timing, and single-pass
render semantics are treated as requirements.

## Implementation Status

This document records the pre-fix audit that motivated the 2026-07-24
architecture pass. The actionable ownership and correctness findings described
below have now been resolved:

- Draw/mouse, texture, and planet own the internal runtime contracts;
  `display_logic_api.h` only aggregates them.
- The generated logic header emits a deliberately curated short-name public
  contract plus typed, exported XML callback prototypes.
- The display model owns planet definitions and PlanetView-node registration;
  the display runtime owns their binding policy; the planet context owns runtime
  resources only.
- Draw interaction uses a draw-owned target token rather than scene-node types.
- Variable-registry registration is sealed before generated variable pointers are
  published.
- Retained low-level IO state is heap-owned across app reload, logic capability
  tables are rebound, and libcurl callbacks are refreshed.
- PlanetText teardown, retained shader-path lifetime, and the MJPEG source cap
  are fixed.

The mechanical module and owner-qualified symbol renames described in the
naming section have since been applied. The optional one-symbol
`dc_logic_get_module` design and a two-stage replacement for the existing
explicit XML bootstrap remain possible future changes. App hot reload supports
unchanged retained context layouts; struct-layout migration still requires a
full restart.

The body below preserves the former filenames and symbols when describing the
audited snapshot. See [Naming Recommendations](#naming-recommendations) for the
former-to-adopted mapping used by the current tree.

## Executive Verdict

The current refactor is a materially better foundation than the old unity-build
application. It is not fundamentally poorly designed.

The strongest parts are:

- `apps/dcapp.c` is now a real composition root rather than a public bag of
  mutable application state.
- Subsystems own separately allocated opaque contexts.
- Movable arrays are normally referenced through stable integer IDs, while
  retained public planet handles are separately allocated.
- Every app source is compiled as an ordinary translation unit. There are no
  remaining `.c` includes or hidden unity-build dependencies.
- The renderer performs one recursive, XML-ordered pass. That is the correct
  behavior for nested layout, functions, draw callbacks, stencils, buttons,
  immediate sets, and state nodes.
- Most modules with many dependencies are high-level orchestrators, or depend
  on lower-level mechanisms in the same way Pilot Light extensions do.

The central problem is not that dcapp has many dependencies. It has a few
dependencies whose ownership or direction is wrong:

1. `logic_api.h` currently owns draw, mouse, texture, and planet contracts, so
   the subsystems implementing those contracts depend back on the user-logic
   facade.
2. The planet resource context owns display-model definitions and scene-node
   indexes.
3. The draw interaction layer knows that one kind of hit target is a scene node.
4. The application root contains a large amount of planet binding behavior that
   is legal at that layer, but not cohesive with lifecycle composition.

There are also correctness and lifetime findings that should be fixed ahead of
pure architectural cleanup:

| Priority | Finding | Consequence |
|---|---|---|
| Critical if hot reload is supported | App-dylib reload preserves heap contexts but loses several file-static registries and leaves logic API table pointers stale | Retained handles can refer to vanished state or unloaded code |
| High on Windows/C++ | XML `Function` and `DrawFunction` symbols are neither generated with typed/exported declarations nor included in the Windows export list | `GetProcAddress` failure, C++ name mangling, or mismatched callback calls |
| High, small fix | `DcAppNodePlanetText` buffers are not freed by scene teardown | Definite shutdown leak |
| Medium | Planet view shader paths are stack buffers retained by `plPlanetView` | Dangling pointers after `dc_app_planet_set_view_shaders` returns |

The recommended outcome is not a larger framework. It is a cleaner dependency
direction with roughly the same set of runtime objects.

## The Architectural Test

A useful dependency test for this repository is:

```text
authoring and configuration
        ↓
retained display model
        ↓
display behavior and policy
        ↓
reusable drawing and resource services
        ↓
GPU, filesystem, network, and platform mechanisms
```

Dependencies pointing down this graph are expected. Examples include:

- The display builder knowing how to create a planet node.
- The display runtime knowing how a button mutates state.
- Draw code using a texture service.
- A texture service using Pilot Light graphics and image APIs.

Dependencies pointing up the graph deserve scrutiny. Examples include:

- A planet resource service storing scene-node indexes.
- A draw service accepting `DcAppNodeIndex` as part of its interaction model.
- A GPU extension knowing about XML or command-line parsing.

Control can travel upward through an opaque callback without violating this
rule. The important question is which layer has concrete knowledge of the
other, not which function happens to call which function at runtime.

This is also the relevant lesson from Pilot Light. `pl_renderer_ext` and
`pl_planet_ext` acquire many APIs, but those APIs are allocation, graphics,
shader, resource, filesystem, image, collision, profiling, and other rendering
mechanisms. A high dependency count is reasonable for a high-level renderer.
It would be a different problem if that renderer knew the application's XML
schema or command-line policy.

## Current Runtime Map

The current graph is acyclic at the subsystem level:

```text
apps/dcapp.c
├── config ──────────────> elem
├── xml ─────────────────> elem, scene, lookup, node, font, logic,
│                          texture, pixelstream, planet, data_link
├── renderer ────────────> scene, lookup, node, draw, font, logic,
│                          texture, pixelstream, planet
├── draw ────────────────> texture, planet, DC draw extensions
├── pixelstream ─────────> texture, MJPEG, shared memory
├── scene ───────────────> lookup, node
├── data_link ───────────> lookup, value, Trick, Edge
├── planet ──────────────> Pilot Light planet, geo, GeoJSON, VFS
├── lookup ──────────────> value
├── texture ─────────────> graphics, image, GPU allocation
├── font ────────────────> draw and GPU mechanisms
└── logic_runtime ───────> dynamic-library mechanisms and user logic
```

That graph is broadly healthy. The following table describes what each module
really is, rather than judging it by its current filename.

| Current module | Actual responsibility | Coupling verdict |
|---|---|---|
| `apps/dcapp.c` | Composition, lifecycle, frame ordering, public logic adapters | Correct owner of context assembly; planet behavior is too detailed |
| `config` | XML authoring preprocessor for includes, constants, defaults, styles, and static conditions | Cohesive; consuming lower domain enums is legitimate |
| `elem` | Canonical XML element vocabulary and name mapping | Small foundational authoring module |
| `xml` | Startup display-model builder, not a generic XML library | Broad dependencies are appropriate for an upper-layer builder |
| `scene` | Retained display model: lookup, node storage, and root window | Cohesive; `display_model` would be a more exact name |
| `renderer` | Stateful display interpreter: layout, interaction, callbacks, mutation, and drawing | Broad dependencies are appropriate; `display_runtime` is more exact |
| `draw` | Reusable immediate-mode semantic drawing, scopes, batching, and interaction | Mostly clean; aggregate logic and node-identity dependencies should go |
| `font` | Font registration, size variants, and atlas build | Clean resource service |
| `texture` | Texture IDs, GPU textures, bind groups, upload staging, and image loading | Clean resource/mechanism service |
| `pixelstream` | MJPEG/shared-memory sources and texture updates | Legitimately depends on texture and protocol mechanisms |
| `planet` | Planet, view, breadcrumbs, and GeoJSON resource handles | Resource core is good; display-model ownership is misplaced |
| `data_link` | Trick/Edge-to-display-variable binding and frame synchronization | Cohesive and accurately named |
| `lookup` | Named variable/value arena and temporary variable stacks | Cohesive foundational runtime service |
| `logic_runtime` | User-library loading, callback storage, fixed-rate updates, and teardown | Cohesive; it should not need the aggregate draw/planet API definitions |
| `logic_api` | Host-to-user capability aggregate and ABI declarations | Currently owns too much and is declared twice |
| `dc_draw_ext` | CPU-side draw-list commands and font atlas | Correct low-level extension boundary |
| `dc_draw_backend_ext` | GPU upload and submission of dcapp draw lists | Correct low-level extension boundary |

### Header-level dependency review

The headers are substantially cleaner than the source-level fan-in suggests.
`config.h`, `font.h`, `logic_runtime.h`, and `renderer.h` use standard types
and local forward declarations only. The remaining direct header edges are:

| Header | Direct project-header dependencies | Verdict |
|---|---|---|
| `draw.h` | `logic_api.h`, Pilot Light, DC draw-list API | Mixed; aggregate logic dependency is inverted and mechanism declarations should move internal |
| `draw_internal.h` | `node_types.h` | Real but shallow scene-identity leak |
| `elem.h` | `elem_types.h` | Correct focused enum dependency |
| `logic_api.h` | draw/planet/texture/value scalar types and vectors | Reasonable for an aggregate, but it should consume subsystem-owned complete contracts |
| `lookup.h` | lookup and value scalar types | Correct |
| `node.h` | focused IDs/enums plus complete geo, math, and value definitions used by value | Mostly necessary for the complete model; the two misplaced records should move |
| `pixelstream.h` | pixel-stream and texture IDs | Correct |
| `planet.h` | GeoJSON ID and all of `logic_api.h` | Aggregate logic dependency is inverted |
| `scene.h` | node index type | Correct |
| `texture.h` | texture ID/index types | Correct |
| `data_link.h` | lookup index types | Correct |
| `xml.h` | element and node index types | Correct focused startup-builder contract |

All current `*_types.h` files obey the intended convention: enums and
typedefs of basic scalar/index/ID types only. They do not contain complete
model structs or shared forward-declaration buckets. That rule should remain.

The fact that `node.h` includes complete definitions is not automatically a
failure. A struct field stored by value requires a complete type. The useful
question is whether the node model truly owns that value, not whether an
include can be mechanically eliminated.

## What Is Already Correct

### The application context is not a god object

`DcAppContext` at `apps/dcapp.c:52-65` contains the window/configuration and
opaque pointers to owning subsystem contexts. It does not contain every
stretchy buffer, staging allocation, renderer stack, callback, and stencil
field.

That is normal application composition. Replacing it with global file-static
buffers would make ownership, multiple instances, teardown, tests, and hot
reload harder. Passing the entire context into every subsystem would also be a
regression. The current explicit constructor dependencies are better.

The thin public-logic wrappers in `apps/dcapp.c` are also legitimate. A call
that accepts an opaque `DcAppContext *` has to be translated somewhere into
the private texture or planet context. The composition root is the appropriate
place for that translation.

### The handle choices are mostly right

- Scene nodes, lookup values, textures, and pixel-stream sources live in
  movable buffers and are referenced by append-only integer IDs.
- Planet, view, breadcrumbs, and GeoJSON handles are separately allocated
  objects because callers can retain them.
- Planet definitions are separately allocated, so pointers to definitions are
  not invalidated when the definition-pointer buffer grows.

No generation counter is currently necessary for app-lifetime objects with no
individual destruction/reuse operation. A generational handle system would add
complexity without solving a current problem.

### Separate translation units now work

The build generator enumerates every source under `src` and adds
`apps/dcapp.c` separately at `scripts/internal/gen-build-apps.py:198-204`.
There are no `.c` includes under `apps`, `src`, or `extensions`.

Every `src/app/*.h` header was syntax-checked in isolation, and all 13 app
implementation files plus `apps/dcapp.c` were syntax-checked independently.
This is evidence that the old unity build is no longer hiding declarations or
file-order dependencies.

Per-translation-unit `*_init(plApiRegistryI *)` functions and file-local
Pilot Light API pointers match Pilot Light's non-unity extension pattern. They
should not be replaced with `extern` globals or fields in `DcAppContext`.

### The single-pass renderer should remain single-pass

The traversal at `src/app/renderer.c:159-304` processes one node and performs
its effect before moving to the next node. Recursive child rendering preserves
the parent draw scope and XML order.

That ordering is semantically observable:

- `Function` and `DrawFunction` execute at their XML positions.
- Immediate and deferred `Set` operations differ.
- Buttons and state children read and mutate runtime state.
- Stencil begin/add/remove/draw/end operations are ordered.
- Mouse targets and nested containers depend on current draw scopes.
- Planet children depend on the current planet view/container.

A global "resolve every node, then draw every resolved shape" pass would not
match the data model or existing behavior. The current pattern is effectively:

```c
for each node in XML order:
    inspect and resolve this node
    execute this node's behavior or draw call
    recurse while its scopes are active
```

That is the right design for this display language.

### The frame order is deliberate

The order at `apps/dcapp.c:283-312` should remain:

1. External data-link update.
2. Pixel-stream update.
3. Fixed-rate user logic update.
4. Value normalization/refresh.
5. Draw input and draw-frame begin.
6. Planet binding update.
7. Recursive display runtime.
8. Deferred-set flush.
9. Variable-stack reset.
10. GPU submission.
11. Interaction-state commit.

It gives logic same-frame external data, keeps display values coherent before
rendering, preserves deferred-set semantics, and intentionally publishes the
completed interaction target set after the frame.

## Actual Boundary Problems

### 1. `logic_api.h` owns contracts that should be owned below it

`src/app/logic_api.h` defines:

- Draw PODs and the complete draw function table.
- Mouse state and the mouse function table.
- Texture API declarations.
- Planet handles, options, values, and the planet function table.
- The app API and initialization aggregate.

`draw.h` includes that entire file at `src/app/draw.h:4`, and `planet.h`
includes it at `src/app/planet.h:5`. This means the lower implementations
depend on an upper user-logic facade in order to implement their own concepts.
It also contradicts the focused-header rule in `coding-style.md`.

This is primarily an ownership and compile-boundary problem, not runtime
coupling. It should be inverted:

```text
draw owns draw and mouse contracts ──┐
planet owns planet contracts ───────┼──> logic host aggregate
texture owns texture contracts ─────┘
```

The app root can still assemble tables whose functions need to translate
`DcAppContext *` into private subsystem contexts.

`draw.h` also exposes `pl.h`, `pl_math.h`, and `dc_draw_ext.h` because it
mixes reusable dcapp drawing with renderer-only and mechanism-facing helpers.
Moving only the mechanism-facing declarations to `draw_internal.h` would let
the draw-owned contract remain focused without creating a file per primitive.

### 2. Planet resource ownership and display-model ownership are mixed

`DcAppPlanetContext` contains resource storage:

```c
plPlanet **sb_planets;
plPlanetView **sb_views;
DcAppPlanetHandle *sb_planet_handles;
DcAppPlanetViewHandle *sb_view_handles;
```

Those belong there. It also contains:

```c
DcAppPlanetDefinition **sb_definitions;
DcAppNodeIndex *sb_view_node_indices;
```

at `src/app/planet.c:43-44`.

The latter two are display-model concepts. `DcAppPlanetDefinition` contains
lookup indexes, XML texture enable/refresh edge state, shader selection data,
and runtime links at `src/app/node.h:477-531`. View-node indexes are explicitly
scene identity. Their presence forces `planet.c` to include all of `node.h` and
causes a lower resource service to understand the retained display model.

The pragmatic correction is:

- The scene, preferably renamed `display_model`, owns planet definitions and
  the list of planet-view node indexes.
- The planet service owns only resolved planet resources and public
  app-lifetime handles.
- The display runtime resolves definitions against lookup values, creates the
  resources, writes resulting handles into the model, and updates texture/light
  bindings each frame.

This does not require moving buffers back into `DcAppContext`. It also does not
require one source file per planet node. The behavior can live in the existing
high-level runtime if avoiding another `planet_binding.c` is preferred.

### 3. The application root contains planet behavior, not just composition

`_build_planet_texture`, `_initialize_planets`, and `_update_planets` occupy
`apps/dcapp.c:430-690`. They interpret display lookup values, perform CRS
conversion, track enable/refresh edges, create views, select shaders, and
mutate retained nodes.

This is not an upward dependency violation: the application root is allowed to
know every subsystem. It is still weak cohesion. The root should ideally:

```c
create services;
wire services together;
call initialize/update/render/shutdown in the required order;
```

It should not contain the implementation of planet-display bindings. Once the
planet definitions belong to the display model, these functions fit naturally
in `display_runtime` because it already owns model interpretation and borrows
scene, lookup, and planet services.

### 4. Draw interaction knows scene-node identity

`draw_internal.h` imports `node_types.h` and exposes
`dc_app_draw_mouse_*_node(..., DcAppNodeIndex)` at
`src/app/draw_internal.h:4,18-23`. The draw context stores targets as either
node indexes or hashed string IDs.

Hit testing does need a stable target token. It does not need to know what a
scene node is. A draw-owned token such as `DcAppDrawTargetId` can represent both
cases. The display runtime can namespace/convert node indexes, while public
logic mouse IDs can be hashed into another namespace.

This is a small but real policy leak. It is less important than the logic and
planet ownership problems.

### 5. XML construction has a lifecycle knot

`xml.c` is not merely parsing XML. It is a startup builder that creates nodes,
registers resources, binds external data, loads logic, and populates the
retained display model. Its broad source dependencies are therefore
appropriate.

The awkward part is the mid-parse bootstrap:

- `DcAppXmlContext` stores the opaque app context and a bootstrap callback at
  `src/app/xml.c:32-45`.
- Parsing the `Window` invokes the callback at `src/app/xml.c:5794-5795`.
- The app creates the window/GPU-dependent services and injects fonts,
  textures, and pixel streams back into the builder.

The callback is a valid opaque upcall and does not itself invert concrete
dependencies. It does, however, create temporal coupling: the builder is
partially initialized, invokes the root, and then receives additional services
before it may continue.

A cleaner behavior-preserving lifecycle would be:

1. Build or return the window/bootstrap descriptor.
2. Let the root initialize the window and GPU-dependent services.
3. Complete display-model construction with those services already supplied.

This is lower priority than the logic and planet boundaries because the current
contract is explicit and works.

### 6. Data-link and planet builders rely on "the most recent object"

`dc_app_data_link_add_edge_rx/tx` and `dc_app_data_link_add_trick_rx/tx` bind
to the last connection in their buffers at `src/app/data_link.c:125-178`.
Planet XML children similarly mutate the most recently registered definition.

XML nesting currently guarantees the required order, so this is not a current
behavior bug. It is an implicit API contract. Returning a stable connection or
definition ID from `add` and passing it to child-add operations would make the
relationship explicit.

This is a modest cleanup, not justification for a general object framework.

### 7. `node.h` contains two records owned elsewhere

- `DcAppDeferredSetOp` at `src/app/node.h:381-385` is renderer-only scratch and
  should be file-local to the display runtime.
- Planet definition/shader/texture records at `src/app/node.h:477-531` belong
  to the display model but are not nodes themselves.

The rest of the large node union is cohesive. Splitting every node type into a
separate header/source pair would increase navigation and dependency overhead
without improving direction. File size alone is not an architectural defect.

## Logic API and Generated Header Audit

### Current control flow

The current path is:

1. The display builder encounters `<Logic>` and calls
   `dc_app_logic_load`.
2. `logic_runtime.c` opens the dynamic library and resolves the four lifecycle
   symbols independently.
3. XML `Function` and `DrawFunction` nodes resolve arbitrary named symbols with
   their own locally repeated casts.
4. After parsing and planet initialization, `apps/dcapp.c` builds
   `DcAppInit`, passes it to generated `display_pre_init`, and calls
   `display_init`.
5. Generated `display_pre_init` caches the capability-table pointers and
   resolves every generated XML variable pointer.
6. Each frame, `display_draw` is invoked as a fixed-rate update before the
   display runtime renders.
7. XML `DrawFunction` callbacks execute inside a guarded draw scope.
8. Shutdown calls `display_close` and unloads the logic library before
   resource teardown.

That overall capability-table model is good. The opaque app context, explicit
user data, and scoped draw callback are all worth keeping.

### Two manually synchronized ABI universes

The host ABI is declared across `src/app/draw_api.h`,
`src/app/texture_api.h`, `src/app/planet_api.h`, and
`src/app/logic_api.h` using `DcApp*` types. The generator manually prints the
curated public ABI in `apps/dcapp_genheader.c` using `Dc*` types.

The current Mac/Clang layouts happen to match:

| ABI object | Current size |
|---|---:|
| Draw table | 560 bytes / 70 function pointers |
| Mouse table | 120 bytes |
| Texture table | 16 bytes |
| Planet table | 152 bytes |
| Init aggregate | 48 bytes |
| Placement | 36 bytes |
| Draw-function argument | 40 bytes |
| Vec2 / Vec3 / Vec4 | 8 / 12 / 16 bytes |

This costs essentially no meaningful VRAM or runtime memory. The issue is that
the declarations can drift silently. The host and user library use different
nominal C types and rely on identical binary layout and calling conventions,
including for structs passed and returned by value. A change to only one side
can compile successfully and still corrupt calls.

The chosen arrangement keeps the internal `DcApp*` contracts private and
manually curates the short-name public contract in `dcapp_genheader.c`. This
duplicates ABI declarations intentionally at the application boundary. Every
logic API change therefore needs a generated-header compile test plus a direct
comparison of shared sizes, alignment, offsets, constants, and function-table
field order.

### Resolved: `get_variable` was duplicated

Before the fix, `DcAppInit` contained both:

```c
DcAppGetVariableFn get_variable;
const DcAppApi *app;
```

and `DcAppApi` itself contains `get_variable`. The app fills both at
`apps/dcapp.c:235-245`, but generated setup uses only
`init->app->get_variable`.

The standalone field and typedef were removed. `DcAppInit` now carries only
the `app` capability table.

### Resolved: `size` and `version` implied unsupported compatibility

Before the fix, the host wrote `.version = 7`; generated code accepted `version >= 1` and
`size >= sizeof(DcInit)`. That is an append-only prefix compatibility scheme.
It cannot protect against reordered fields, a changed callback signature, or a
stale pre-init symbol. The project has explicitly rejected preserving table
offsets and assumes regenerated headers.

The clean choices are:

- Remove `size` and `version` and require lockstep generated headers, or
- Keep one exact ABI identifier solely to report a stale logic library and
  reject anything unequal.

The fields and greater-than/at-least check were removed. dcapp now requires
logic to be rebuilt from the latest generated header.

### Resolved: XML callbacks were not one typed/exported contract

Before the fix, the generator emitted lifecycle declarations but not typed
declarations for every XML `Function` and `DrawFunction` symbol.
`xml.c`, `node.h`, and `renderer.c` repeat compatible-looking callback
prototypes instead.

Consequences:

- A user can compile a callback with the wrong signature.
- The runtime later invokes the `dlsym` result through a cast.
- A symbol used once as `Function` and once as `DrawFunction` is ambiguous.
- In C++, a callback without an existing `extern "C"` declaration is mangled.
- Windows sample builds export only the four lifecycle symbols at
  `scripts/internal/gen-build-samples.py:183-220`, so arbitrary XML callbacks
  need not be visible to `GetProcAddress`.

The implemented fix makes the generator collect callback names and emit:

- One canonical callback typedef for each callback kind.
- An exact prototype for every callback symbol.
- Portable C linkage and export decoration.
- A generation-time error for one name used with conflicting callback kinds.

The cleaner optional design is one exported entry point:

```c
const DcLogicModuleApi *dc_logic_get_module(void);
```

The returned descriptor would contain lifecycle callbacks and typed named
function/draw-function registrations. The host would resolve one known symbol,
validate the descriptor, and resolve XML callback names from it after parsing.
That removes arbitrary `dlsym` casts, Windows export-list generation, C++
mangling, and the requirement that `<Logic>` appear before callback nodes.

The one-symbol descriptor is worthwhile if Windows, C++, and many custom
callbacks are first-class requirements. Typed generated exports are the
smaller change if they are not.

### Generated variable pointers rely on a frozen lookup

`DcAppLookup` stores values inline in a stretchy buffer.
`dc_app_lookup_get_value` explicitly returns a pointer that callers should not
store because later registration can reallocate that buffer.

Generated logic intentionally stores pointers into those values for the entire
logic-library lifetime. This is safe today because all values are registered
during startup and `display_pre_init` runs only after parsing is complete.
There is no runtime registration API.

That phase invariant should be made explicit. The minimal solution is to seal
the lookup after model construction and reject/assert any later registration.
Converting all values to individually allocated objects or generational handles
is unnecessary while registration is startup-only.

### Borrowed API lifetimes need explicit contracts

- `DcAppDrawPlanetViewHandle` is allocated for a draw scope and freed when that
  scope flushes. It must not escape the current draw callback/scope.
- `DcAppPlanetBreadcrumbsPoints.points` is a borrowed stretchy-buffer pointer
  invalidated by the next breadcrumbs mutation or destruction.
- `DcAppDrawFuncArgs.values` borrows renderer scratch for the callback.
- `dc_app_draw_get_area` returns mutable current-scope state, not an owned
  snapshot.

These designs are acceptable. They need comments in the public/generated
contract because their pointer syntax otherwise looks retainable.

### Smaller logic-surface cleanup

- `clear_texture` takes an app context even though the handle owns everything
  needed and the adapter only checks the context. It can be handle-only,
  consistent with light and breadcrumb mutations.
- `dc_app_logic_draw` and `dc_app_logic_has_draw` have no consumers.
- The callback named `display_draw` is semantically a fixed-rate update. A
  public rename is optional and breaking, but the internal field should at
  least be named `update`.
- `dc_app_draw_triangles_filled_ex` is labeled as public DrawFunction API in
  `draw.h` but is absent from the function table and used by the renderer only.
  It belongs in the internal draw declarations unless deliberately exposed.
- Texture-slot and ellipse limits are spelled independently in the host,
  extension, and generator. They should have one source even though their
  values currently match.
- Generated `_Dc...` tags and `_DCAPP...` macros use implementation-reserved
  identifier forms and do not match the repository's public naming rule.

## Correctness and Lifetime Findings

### App hot reload is not currently state-safe

The reload branch at `apps/dcapp.c:148-152` preserves `DcAppContext`, reloads
API pointers, and returns. Under actual dylib replacement, several contexts
retain handles into file-static registries that reset with the old app image:

- MJPEG `_multi_handle` and `_sb_contexts`.
- Shared-memory pixel-stream `_sb_contexts`.
- Trick `_contexts`.
- Edge `_contexts`.
- Socket `_contexts`.
- GeoJSON `_sb_datas`.

The generated logic library also caches pointers to static host API tables in
`display_pre_init`. The reload branch does not call pre-init again, so those
pointers can address the unloaded app image.

There are two honest policies:

1. If app hot reload is not a requirement, disable or document it and perform a
   full application restart rather than preserving state.
2. If it is a requirement, move retained low-level state into explicit heap
   contexts owned by the appropriate app subsystem, or into non-reloadable
   extensions/Pilot Light's data registry, and re-run logic pre-init after the
   host tables are reloaded.

The data registry is useful for state that must survive module replacement. It
is not a reason to register every ordinary app subsystem globally.

Live code reload also needs an explicit policy for context layout changes;
preserving a heap object compiled with an old struct layout cannot be made safe
by merely reacquiring function pointers.

### Planet text leaks on scene destruction

`DcAppNodePlanetText` owns six stretchy buffers at
`src/app/node.h:646-664`. The destruction switch at
`src/app/scene.c:45-85` frees ordinary text and other variable-size node
storage, but has no `NODE_TYPE_PLANET_TEXT` case.

The six planet-text buffers should be freed. This is a definite leak and does
not require an architectural redesign.

### Planet view shader paths are retained stack addresses

`dc_app_planet_set_view_shaders` creates VFS paths in local stack arrays at
`src/app/planet.c:408-425` and passes them to
`pl_planet_set_shaders`. That function stores the pointers in the long-lived
`plPlanetView` before loading the shaders at
`extensions/pl_planet_ext.c:1255-1259`.

The immediate load may make the current call appear to work, but the retained
fields are dangling after return. Either:

- `DcAppPlanetView` should own copied shader paths and pass those stable
  strings, or
- `pl_planet_ext` should explicitly own/copy its shader paths.

The latter is the stronger general contract if all extension callers expect
the view to retain the configuration.

### MJPEG's pointer-stability cap is not enforced

The MJPEG backend pre-grows its context buffer to ten entries because libcurl
retains `_Context *` pointers. The add path does not enforce the corresponding
maximum, so an eleventh source can grow the buffer and invalidate pointers held
by libcurl.

This is pre-existing and independent of the app-module architecture. Enforcing
the existing cap or separately allocating contexts would close the hole.

### Teardown is mostly correct

The current teardown keeps draw/resource dependencies alive until their
borrowers are destroyed. Logic closes while resource contexts are still alive,
which is useful if `display_close` releases app-owned resources.

For strict reverse dependency order, the renderer can be destroyed before
logic closes, because renderer teardown only frees scratch and no callback is
invoked afterward. This is low priority; the current order is safe with the
current destructor behavior.

## Recommended Target Architecture

```text
apps/dcapp.c
  composition, window/GPU lifecycle, frame order, logic-facing adapters
        │
        ├── config
        │     authoring preprocessing
        │
        ├── display_builder          (currently xml)
        │     startup translation into retained model
        │             ↓
        ├── display_model            (currently scene + node + lookup ownership)
        │     nodes, values, planet definitions, view-node registry
        │             ↓
        └── display_runtime          (currently renderer)
              layout, state, callbacks, planet bindings, ordered drawing
                    ↓
              draw | font | texture | pixelstream | planet | data_bridge
                    ↓
              dc_draw_* | pl_planet | Trick/Edge | image/VFS/GPU/platform
```

The graph shows knowledge direction, not strict frame-call nesting.
`data_bridge`, pixel streams, and logic remain frame services called by the
root before the display runtime.

The logic boundary sits beside the root:

```text
draw-owned contract ──────┐
mouse-owned contract ─────┤
texture-owned contract ───┼──> small logic host aggregate
planet-owned contract ────┤            ↓
app/root-owned contract ──┘      generated display bindings
```

Only the root and logic host need to see the complete aggregate. Draw, planet,
texture, the display model, and the display builder should not include it.

## Naming Recommendations

The audit's recommendations were adopted as the following behavior-free
module renames after the ownership work settled:

| Former | Adopted | Rationale |
|---|---|---|
| `config.c/.h` | `xml_preprocessor.c/.h` | It preprocesses the XML authoring language rather than storing general configuration |
| `elem.c/.h`, `elem_types.h` | `xml_element.c/.h`, `xml_element_types.h` | The vocabulary is specifically the source XML element vocabulary |
| `xml.c/.h` | `display_builder.c/.h` | It builds the retained display model rather than providing generic XML utilities |
| `scene.c/.h` | `display_model.c/.h` | It owns the retained model, variable registry, and parsed planet definitions |
| `renderer.c/.h` | `display_runtime.c/.h` | It interprets callbacks, interaction, state mutation, and drawing rather than merely rendering |
| `logic_runtime.c/.h` | `display_logic.c/.h` | It owns the loaded display-logic library and lifecycle |
| `logic_api.h`, `logic_callbacks.h` | `display_logic_api.h`, `display_logic_callbacks.h` | The contracts belong to loaded display logic |
| `lookup.c/.h`, `lookup_types.h` | `variable_registry.c/.h`, `variable_registry_types.h` | It owns named variables, value slots, stacks, and write tracking rather than merely performing lookups |
| `data_link.c/.h` | Unchanged | It already names the Trick/Edge data-link boundary accurately |

The adopted public C names also carry their owner explicitly, for example
`DcAppDisplayModelContext`, `dc_app_display_model_*`,
`DcAppVariableRegistryValueIndex`, and `DcAppDrawAlignmentType`. The generated
logic header retains its deliberately shorter `Dc...` public contract.

## Minimal, Behavior-Preserving Work Plan

### Phase 0: correctness and explicit policy

1. Free `NODE_TYPE_PLANET_TEXT` buffers.
2. Give retained planet shader paths stable ownership.
3. Decide whether app hot reload is supported:
   - If no, make restart behavior explicit.
   - If yes, context-own the file-static registries and rebind logic tables.
4. Generate/export typed XML callback declarations for Windows and C++.
5. Enforce the existing MJPEG connection/pointer-stability rule.

### Phase 1: repair the logic boundary

1. Move draw/mouse values and table declarations under draw ownership.
2. Move planet values/handles and table declarations under planet ownership.
3. Keep texture IDs/table declarations under texture ownership.
4. Reduce `logic_api.h` to the host aggregate and shared callback typedefs.
5. Let only the app composition root assemble cross-subsystem adapters.
6. Remove duplicate `get_variable`.
7. Remove prefix-offset compatibility checks; optionally keep one exact ABI
   identifier for stale-library diagnostics.
8. Make generated and host ABI declarations come from one source. Use a
   complete layout test only as an intermediate guard.
9. Seal lookup registration before generated variable pointers are published.
10. Document borrowed handle/pointer lifetimes.

### Phase 2: repair planet/display ownership

1. Move planet definitions and planet-view node registration to the display
   model.
2. Move planet definition initialization/update behavior from the app root to
   the display runtime, or to one focused planet-binding module if that logic
   grows independently.
3. Remove `node.h` from `planet.c`.
4. Leave runtime planet resources and stable public handles in the planet
   context.

### Phase 3: remove shallow policy and temporal leaks

1. Replace draw's `DcAppNodeIndex` interaction API with a draw-owned target ID.
2. Move `DcAppDeferredSetOp` into the display runtime implementation.
3. Make data-link/planet child-building APIs accept explicit parent IDs rather
   than "last added."
4. Replace the mid-parse XML bootstrap with a two-stage build if the lifecycle
   remains hard to change safely.
5. Apply the module renames mechanically.

### Optional ideal logic-module design

Replace multiple arbitrary symbol lookups with one exported
`dc_logic_get_module` descriptor. This is not required to fix the dependency
direction, but it is the cleanest final design for typed callbacks, Windows,
C++, ordering independence, validation, and reload rebinding.

## Things Not to Do

- Do not move subsystem arrays back into a giant `DcAppContext`.
- Do not use file-static stretchy buffers for ordinary per-app state.
- Do not turn every app module into a Pilot Light registry extension.
- Do not pursue zero header includes as a goal; complete by-value types require
  their owning definitions.
- Do not put complete structs or forward declarations into `*_types.h`.
- Do not split `renderer.c` or `xml.c` into one file per node merely because
  they are large.
- Do not reintroduce a global two-pass flattened renderer.
- Do not preserve obsolete function-table offsets or append-only ABI padding
  when all logic is rebuilt from the latest generated header.
- Do not add broad defensive frameworks where a stable ownership contract or
  one explicit phase assertion is enough.

## Verification Performed

The following checks were run against the current worktree:

- All 24 `src/app/*.h` headers compile independently as C11 headers.
- All 13 `src/app/*.c` files and `apps/dcapp.c` pass independent Clang syntax
  checks with implicit function declarations treated as errors.
- No source under `apps`, `src`, or `extensions` includes another `.c` file.
- The generated build scripts list the app sources as separate translation
  units.
- All 27 top-level valid sample displays validate with zero errors and zero
  warnings; the intentionally invalid `samples/bad-sample` was excluded.
- Representative logic/planet/stencil samples and the generated ABI layouts
  were inspected separately.

These checks establish compile-boundary and display-validation health. They do
not constitute GPU runtime, Windows export, C++ logic, or live-reload tests.

## Final Assessment

The refactor should be kept. Its core decisions—opaque subsystem contexts,
explicit dependency injection, stable IDs for movable storage, separate
translation units, and the recursive single-pass display runtime—are sound.

The next pass should not be another broad rewrite. It should make three precise
ownership corrections:

1. Subsystems own their contracts; the logic host aggregates them.
2. The display model/runtime own XML-bound planet policy; the planet service
   owns planet resources.
3. Draw owns generic drawing/interaction concepts; the display runtime maps
   scene identity into them.

Alongside those changes, the four concrete correctness items—reload policy,
typed/exported callbacks, planet-text cleanup, and shader-path ownership—should
be resolved explicitly. After that, the module graph will be not only
separately compilable, but directionally clean.
