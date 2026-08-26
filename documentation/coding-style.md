# Coding Style

This file covers project-specific conventions. Match the surrounding file for
formatting details not mentioned here.

## Names and scope

- Use `snake_case` for functions and local variables.
- Header-visible types use `Dc...` names and tags without a leading
  underscore. File-local types and helpers may use a leading underscore.
- Cross-file identifiers under `src/app` carry the owner name:
  `DcAppDisplayModel...`, `dc_app_display_model_...`,
  `DC_APP_DISPLAY_MODEL_...`.
- Opaque subsystem state types end in `Context`.
- Qualify module-owned scalar and index types, such as
  `DcAppDrawAlignmentType` and `DcAppVariableRegistryVariableIndex`.
- Spell out ownership-bearing words such as `Variable`, `Value`,
  `Element`, and `Alignment`. Keep established initialisms such as `API`,
  `ID`, `XML`, and `CRS`.
- Keep file-local names short. Full owner prefixes are for cross-file
  contracts.
- The generated logic API is a separate public contract. Do not mechanically
  rename its shorter `Dc...` identifiers to match internal `DcApp...` names.
- Comments should explain ownership, ordering, coordinate assumptions, or
  behavior that is not clear from the code.
- Keep files ASCII unless a file already requires another encoding.

## Header boundaries

- A `*_types.h` contains only enums and typedefs for basic scalar,
  index, or ID types.
- Put opaque-type typedefs in the API header that owns them. A header that only
  borrows a pointer should forward-declare the struct or union tag.
- Do not include a full API header solely to obtain an opaque pointer type.
  Implementation files should include every API they call.
- A struct or union used by value requires its complete definition.
- Complete value structs belong to their owning API. Shared foundational values
  may have a dedicated header such as `app/vector.h`.
- Do not use `*_types.h` as an aggregate for configuration, DTOs, model
  structs, or unrelated declarations.

## Memory, APIs, and arrays

Implementation files cache PilotLight and dcapp extension APIs in file-local
`_ext_*` pointers. Refresh those pointers from the module's `*_init`
function.

Use `PL_ALLOC`, `PL_REALLOC`, and `PL_FREE` in runtime code already using
PilotLight's tracked allocator. Plain C allocations must be released through
the matching plain C path.

dcapp uses both `utils/stb_sb.h` arrays and PilotLight stretchy buffers. Match
the local module. Keep movable buffers private to their context and expose
stable IDs or separately allocated handles. Node and texture index `0` is
reserved as undefined.

State kept across an application-library reload belongs in a heap-owned
context, not a file-static registry.

## XML changes

An XML element change usually touches:

1. `src/app/xml_element_types.h`, `src/app/xml_element.h`, and the name mapping
   in `src/app/xml_element.c`;
2. `src/app/node.h`, if the element survives preprocessing;
3. parsing in `src/app/display_builder.c`;
4. resolution in `src/app/display_runtime.c`, if it affects the display;
5. attributes and child rules in `apps/dcapp_validate.c`;
6. the relevant documentation and, for user-facing behavior, a sample;
7. `scripts/convert-legacy-xml.py`, only when legacy conversion changes.

`Constant`, `Default`, `Style`, `Include`, and `Dummy` are authoring
elements and disappear during preprocessing. Document that distinction for
new preprocessing-only elements.

Resource paths resolve relative to the XML file that declares them. Preserve
`_Directory` propagation when adding path attributes so included images,
fonts, logic, shaders, and data do not depend on the process working
directory. Prefer the helpers in `src/utils/file.*`.

## Values and variables

`DcAppValue` is the runtime value representation. Code that reads or writes
XML values should:

- preserve the value type when possible;
- use the existing refresh helpers when a string form must stay synchronized;
- use `src/app/variable_registry.c` instead of open-coding registry access;
- keep `Set` behavior and logic variable pointers consistent.

## Logic API changes

Update the contract and its consumers in one change:

1. edit the owning contract in `src/app/draw_api.h`,
   `src/app/texture_api.h`, `src/app/planet_api.h`, or
   `src/app/display_logic_api.h`;
2. update the owning implementation in `src/app/draw.c`,
   `src/app/texture.c`, `src/app/planet.c`, or the relevant subsystem;
3. update the short-name public declarations in `apps/dcapp_genheader.c`;
4. update `samples/api-test`, any affected examples, and
   [Logic](logic.md).

Public generated structs and function-table fields must stay in the same order
as their internal counterparts. Logic libraries use the generated
`logic/dcapp.h` API and must not depend on private `_AppData` state.

The same boundary applies inside the app: `_AppData` remains private to
`apps/dcapp.c`; subsystems receive opaque contexts, stable IDs, and narrow
APIs.

## Drawing changes

Keep each part in its existing layer:

- XML semantics, layout, and ordered dispatch:
  `src/app/display_runtime.c`
- drawing helpers, batches, transforms, stencils, and hit registration:
  `src/app/draw.c`
- immediate-mode list storage: `extensions/dc_draw_ext.*`
- GPU submission: `extensions/dc_draw_backend_ext.*`

Check 2D/3D batch order, inherited transforms, mouse hit registration, shader
and stencil state, and planet coordinates where applicable.

## External I/O changes

Protocol code stays outside the XML parser:

- Trick: `src/trick.c`
- Edge: `src/edge.c`
- streams: `src/pixelstream/`
- XML mappings and frame updates: app contexts

If an external library keeps a callback into the app library, refresh it before
the library can call it after reload. In XML mappings, `From` means external
source to dcapp variable; `To` means dcapp variable to external target.

## Checks

Validate affected displays after documentation or XML changes. Use
`--preprocessed` when the change affects authoring elements:

```bash
./bin/dcapp-validate.sh samples/welcome/welcome.xml --preprocessed cache/welcome.preprocessed.xml
./bin/dcapp-validate.sh samples/includes/includes.xml --preprocessed cache/includes.preprocessed.xml
```

For generated-header or logic API changes, rebuild `samples/api-test`, run it
when graphics are available, and close it normally so shutdown checks run:

```bash
./bin/dcapp.sh samples/api-test/api-test.xml
```

For planet work, run the sample or tool that covers the changed path:
`dcapp-planet-chunkgen`, `dcapp-planet-snapshot`, or a planet XML display.

Documentation should state the rule once, show one useful example, and link to
the owning reference for details. Keep narrow topics findable by name.
