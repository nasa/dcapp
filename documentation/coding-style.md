# dcapp Coding Style

This page records the style and maintenance rules that keep dcapp changes
consistent. It is not a full C style guide. It focuses on the habits that
matter for this codebase.

## General Style

- Use C-style `snake_case` for functions and local variables.
- Public or shared dcapp types usually use `Dc...` names.
- Internal app/runtime structs commonly use leading underscores, such as
  `_AppData`, `_Node`, and `_DrawBatch`.
- Internal helper functions commonly use a leading underscore, such as
  `_process_xml_node_logic`.
- Keep comments useful and short. Prefer comments that explain ordering,
  ownership, coordinate assumptions, or non-obvious behavior.
- Keep files ASCII unless the existing file already needs another encoding.

## PilotLight APIs

PilotLight and dcapp extension API pointers are stored as global `_ext_*`
pointers in `apps/dcapp/dcapp.h`. Follow that pattern when adding integration
with a PilotLight extension.

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

1. Add or change the element enum/name mapping in `src/app/elem.h` and
   `src/app/elem.c`.
2. Add or change the runtime node data in `apps/dcapp/node.h` if the element
   survives preprocessing.
3. Parse the element in `apps/dcapp/xml.c`.
4. Draw or update it in `apps/dcapp/draw_node.c` if it affects runtime display.
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

`DcValue` is the central runtime value representation. When adding behavior that
reads or writes XML values:

- Preserve the value type when possible.
- Call the existing refresh/update helpers when a string representation needs to
  stay in sync.
- Use lookup helpers from `src/app/lookup.c` instead of open-coding variable
  access.
- Keep `Set` behavior and logic variable pointers consistent.

## Logic API Changes

Logic API changes touch generated code, runtime code, docs, and samples. Update
them together:

1. Runtime API structs in `src/logic_api.h`.
2. Implementations in `apps/dcapp/draw.c`, `apps/dcapp/texture.c`,
   `apps/dcapp/planet.c`, or another owning runtime file.
3. Generated header output in `apps/dcapp_genheader.c`.
4. Relevant samples under `samples/`.
5. [logic.md](logic.md).

Logic libraries should not depend on private `_AppData` internals. Expose
needed behavior through the generated `logic/dcapp.h` API surface instead.

## Drawing Changes

Use the existing split:

- XML node semantics belong in `apps/dcapp/draw_node.c`.
- Reusable draw API helpers and draw batching belong in `apps/dcapp/draw.c`.
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

When adding an IO mapping, document the direction clearly: `From` means external
source to dcapp variable, and `To` means dcapp variable to the external target.

## Tests And Checks

For documentation or XML behavior changes, at minimum run validation on affected
samples:

```bash
./bin/dcapp-validate.sh samples/welcome/welcome.xml --preprocessed cache/welcome.preprocessed.xml
./bin/dcapp-validate.sh samples/includes/includes.xml --preprocessed cache/includes.preprocessed.xml
```

For logic API or generated-header changes, also run the header generator against
an affected logic sample.

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
