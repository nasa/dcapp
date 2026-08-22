# dcapp Coding Style

This page records the style and maintenance rules that keep dcapp changes
consistent. It is not a full C style guide. It focuses on the habits that
matter for this codebase.

## General Style

- Use C-style `snake_case` for functions and local variables.
- Types declared in headers use `Dc...` names and tags without a leading
  underscore. Reserve leading underscores for file-local types and helpers in
  implementation files, or for private struct members.
- Internal helper functions commonly use a leading underscore, such as
  `_process_xml_node_logic`.
- Keep comments useful and short. Prefer comments that explain ordering,
  ownership, coordinate assumptions, or non-obvious behavior.
- Keep files ASCII unless the existing file already needs another encoding.

## Public Naming And Ownership

Cross-file names in `src/app` are deliberately verbose. C has no native
namespace, so a public name should identify the subsystem that owns its
contract without requiring the reader to find its declaration first.

- Derive the owner prefix from the module name: `display_model` uses
  `DcAppDisplayModel...`, `dc_app_display_model_...`, and
  `DC_APP_DISPLAY_MODEL_...`; `variable_registry` follows the same pattern.
- End opaque subsystem-state types in `Context`, such as
  `DcAppDisplayRuntimeContext` and `DcAppVariableRegistryContext`.
- Qualify module-owned scalar and index types too. For example, use
  `DcAppDrawAlignmentType`, `DcAppVariableRegistryVariableIndex`, and
  `DcAppVariableRegistryValueIndex`.
- Spell out ownership-bearing words such as `Variable`, `Value`, `Element`,
  and `Alignment`. Retain established technical initialisms such as `API`,
  `ID`, `XML`, and `CRS` where the surrounding API already uses them.
- Keep file-local types, static helpers, parameters, and local variables
  concise; the full owner prefix is for cross-file interfaces.
- Do not rename the generated logic API mechanically. Its curated `Dc...`
  names are a separate public contract even when the internal `DcApp...`
  counterpart is more explicit.

## Header Boundaries

- Use a focused `*_types.h` only for enums and typedefs of basic scalar,
  index, or ID types. Do not collect unrelated declarations in an aggregate
  type header.
- Do not put struct or union forward declarations, opaque pointer aliases,
  complete structs, configuration, DTOs, or model types in `*_types.h`.
- Put an opaque type's typedef in its owning API header. Headers that only
  borrow a pointer should forward-declare its struct or union tag locally.
- Do not include a module's full API header from another header just to obtain
  its types. Implementation files should directly include the APIs they call.
- If a header uses a non-basic struct or union by value, include the header
  that owns its complete definition; a forward declaration is not sufficient.
- Complete value structs belong in their owning API header. Foundational values
  that are intentionally shared by value may own a real focused header such as
  `app/vector.h`.

## PilotLight APIs

Each implementation file stores the PilotLight and dcapp extension APIs it uses
in file-local `_ext_*` pointers and refreshes them through its `*_init`
function. Follow that pattern when adding integration with an extension.

Use PilotLight memory helpers in app/runtime code that already depends on
PilotLight memory tracking:

- `PL_ALLOC`
- `PL_REALLOC`
- `PL_FREE`

Do not mix ownership casually. If a subsystem allocates with ordinary C
allocation, free it the same way. If it allocates through PilotLight tracked
memory, release it through the matching PilotLight path.

## Dynamic Arrays

dcapp uses stretchy-buffer arrays in several places. Existing app code often
uses the `sb*` macros from `utils/stb_sb.h`, such as `sbpush`, `sbcount`, and
`sbfree`. Some extension code uses PilotLight stretchy-buffer helpers. Match
the local file.

Index `0` is commonly reserved as undefined for runtime node and texture
handles. Preserve that convention when adding indexed runtime arrays.

## XML Changes

When adding or changing an XML element, update the full surface area in the same
change:

1. Add or change the element enum in `src/app/xml_element_types.h` and its name
   mapping in `src/app/xml_element.c`.
2. Add or change the runtime node data in `src/app/node.h` if the element
   survives preprocessing.
3. Parse the element in `src/app/display_builder.c`.
4. Resolve it in `src/app/display_runtime.c` if it affects runtime display.
5. Validate allowed attributes/children in `apps/dcapp_validate.c`.
6. Update documentation in `documentation/`.
7. Add or update a sample when the behavior is user-facing.
8. Update `scripts/convert-legacy-xml.py` only if the change affects legacy XML
   migration.

If an element is only an authoring helper, make that clear in the docs. Examples
include `Constant`, `Default`, `Style`, `Include`, and `Dummy`; they are
preprocessed away before runtime drawing.

## XML Paths

Resource paths in XML should resolve relative to the display file or included
file that declared them. Preserve `_Directory` handling when adding new path
attributes. Included XML must be able to carry local image, font, logic, shader,
and data paths without depending on the process working directory.

Use existing file/path helpers from `src/utils/file.*` where possible.

## Values And Variables

`DcAppValue` is the central runtime value representation. When adding behavior that
reads or writes XML values:

- Preserve the value type when possible.
- Call the existing refresh/update helpers when a string representation needs to
  stay in sync.
- Use variable-registry helpers from `src/app/variable_registry.c` instead of
  open-coding variable access.
- Keep `Set` behavior and logic variable pointers consistent.

## Logic API Changes

Logic API changes touch generated code, runtime code, docs, and samples. Update
them together:

1. The contract owned by `src/app/draw_api.h`, `src/app/texture_api.h`,
   `src/app/planet_api.h`, or the small aggregate in
   `src/app/display_logic_api.h`.
2. Implementations in `src/app/draw.c`, `src/app/texture.c`,
   `src/app/planet.c`, or another owning runtime file.
3. The explicitly curated short-name public contract and display-specific
   declarations in `apps/dcapp_genheader.c`. Keep every public struct and
   function-table field in the same order as its internal counterpart.
4. Relevant samples under `samples/`.
5. [logic.md](logic.md).

Logic libraries should not depend on private `_AppData` internals. Expose
needed behavior through the generated `logic/dcapp.h` API surface instead.

Runtime subsystems follow the same boundary internally: the private `_AppData`
composition root exists only in `dcapp.c`. Give each subsystem an opaque
context, keep its movable buffers private, and expose focused functions or
stable IDs to other implementation files.

## Drawing Changes

Use the existing split:

- XML node semantics, layout resolution, and XML-ordered dispatch to the draw
  API belong in `src/app/display_runtime.c`.
- Reusable draw API helpers and draw batching belong in `src/app/draw.c`.
- Raw draw list storage belongs in `extensions/dc_draw_ext.*`.
- GPU submission belongs in `extensions/dc_draw_backend_ext.*`.

When adding draw behavior, think about:

- 2D vs 3D draw batch ordering.
- Transform inheritance from parent nodes.
- Mouse hit registration if the new primitive is interactive.
- Shader overrides and stencil state.
- Planet-view drawing if the primitive can appear in planet coordinates.

## External IO Changes

Keep IO protocol code separate from XML parsing:

- Trick protocol behavior belongs in `src/trick.c`.
- Edge protocol behavior belongs in `src/edge.c`.
- PixelStream protocol behavior belongs under `src/pixelstream/`.
- XML mapping and runtime contexts belong in the app parser/frame loop.

State retained across app-library reloads must live in a heap-owned context,
not a file-static registry. If an external library retains a callback into the
app library, refresh that callback before it can run after reload.

When adding an IO mapping, document the direction clearly: `From` means external
source to dcapp variable, and `To` means dcapp variable to the external target.

## Tests And Checks

For documentation or XML behavior changes, at minimum run validation on affected
samples:

```bash
./bin/dcapp-validate.sh samples/welcome/welcome.xml --preprocessed cache/welcome.preprocessed.xml
./bin/dcapp-validate.sh samples/includes/includes.xml --preprocessed cache/includes.preprocessed.xml
```

For logic API or generated-header changes, regenerate an affected logic sample,
build its logic library, and compare the public and internal function-table
field order. Compile the generated header as C++ as well when changing shared
types or inline helpers. `scripts/check-logic-api.sh` performs these checks and
is run by the top-level build.

For planet changes, run the relevant planet tool or sample that exercises the
changed path, such as `dcapp-planet-chunkgen`, `dcapp-planet-snapshot`, or a
planet XML sample.

## Documentation Rule

Documentation should stay explicit, but not padded. Prefer:

- A short getting-started path for users.
- An index that names every major topic.
- Focused topic pages for details that someone would search for directly.

Do not hide narrow topics inside broad names. Trick, Edge, PixelStream, logic,
coordinate frames, planet rendering, migration, and architecture should remain
findable by name.
