BREAKING CHANGES
================

Source and ABI changes that may require edits outside XML display files.


[Unreleased]
------------

### 2026-06-04 - Logic Header Initialization ABI

This section compares the current logic API against the public logic API that
existed before the DrawFunction logic API work. It intentionally does not list
short-lived intermediate header or bootstrap shapes from that implementation
work.

#### Affected Code
- Logic shared libraries compiled against an older generated `logic/dcapp.h`.
- User code that manually declared or called generated logic internals such as
  `display_pre_init`, `_GetVariableValueAddr`, or `get_pointer`.

#### Changed
- The generated logic initialization hook changed from
  `display_pre_init(_GetVariableValueAddr)` to
  `display_pre_init(const DcInit *)`.
- Generated logic headers replaced the old `_GetVariableValueAddr` /
  `get_pointer` variable lookup path with `dc_get_variable("VariableName")`.
- The generated header no longer declares `get_pointer` as the public/manual
  variable lookup escape hatch. Manual lookups should use `dc_get_variable()`.

#### Migration
- Regenerate `logic/dcapp.h` and rebuild logic shared libraries.
- If user code called the old generated lookup pointer directly, update it to `dc_get_variable("VariableName")`.
- Remove user-maintained declarations of `_GetVariableValueAddr` and
  `get_pointer`; those names are no longer part of the generated header
  contract.
- Do not implement `display_pre_init` in user logic code unless you are
  deliberately replacing generated-header initialization. The generated
  `logic/dcapp.h` owns that hook.
- If custom user code really does implement `display_pre_init`, update its
  signature to `void display_pre_init(const DcInit *init)`.

#### Excluded Intermediate Changes
- Earlier same-day DrawFunction mouse and texture helper changes are not listed
  here because they were intermediate implementation states, not migration steps
  from the previous public logic API.
