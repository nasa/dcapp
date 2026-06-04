BREAKING CHANGES
================

Source and ABI changes that may require edits outside XML display files.


[Unreleased]
------------

### Changed
- Logic initialization ABI changed from `display_pre_init(_GetVariableValueAddr)` to `display_pre_init(const DcInit *)`.
- Generated logic headers replaced direct variable lookup through the old `display_pre_init(_GetVariableValueAddr)` callback / generated `get_pointer` storage with `dc_get_variable("VariableName")`.
- Draw and mouse function-table access now comes from `DcInit.draw` and `DcInit.mouse`.
- `dc_mouse->rect`, `dc_mouse->circle`, and `dc_mouse->polygon` now match the plain `dc_draw` primitive signatures. Use `dc_mouse->rect_ex`, `dc_mouse->circle_ex`, or `dc_mouse->polygon_ex` for placement-aware hit targets.
- Changed `dc_mouse->down(ctx, id)` to the ID-free `dc_mouse->down(ctx)`. Use `dc_mouse->active(ctx, id)` for captured ID state.

### Migration
- Regenerate `logic/dcapp.h` and rebuild logic shared libraries.
- If user code called the old generated lookup pointer directly, update it to `dc_get_variable("VariableName")`.
- If user code manually implemented `display_pre_init`, update its signature to `void display_pre_init(const DcInit *init)` and copy `init->draw` / `init->mouse` into any user globals that need them.
- If user code passed a `DcPlacement` to a mouse hit registration call, switch that call to the matching `_ex` function.
- Replace `dc_mouse->down(ctx, id)` with `dc_mouse->active(ctx, id)` for target-owned hold behavior, or `dc_mouse->down(ctx)` for raw button state.
