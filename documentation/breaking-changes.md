BREAKING CHANGES
================

Source and ABI changes that may require edits outside XML display files.


[Unreleased]
------------

### Changed
- Logic initialization ABI changed from `display_pre_init(_GetVariableValueAddr)` to `display_pre_init(const DcInit *)`.
- Generated logic headers replaced direct variable lookup through the old `display_pre_init(_GetVariableValueAddr)` callback / generated `get_pointer` storage with `dc_get_variable("VariableName")`.
- Draw, mouse, and texture function-table access now comes from `DcInit.draw`, `DcInit.mouse`, and `DcInit.texture`.
- `DcInit` now includes `user_data`, variable lookup, draw, mouse, and texture API pointers. Host-side initialization currently advertises version 2.
- Generated headers now expose `dc_texture`, `dc_load_image()`, and `dc_get_texture_size()`.
- `dc_mouse->rect`, `dc_mouse->circle`, `dc_mouse->ellipse`, and `dc_mouse->polygon` now use plain geometry inputs without placement. Use `dc_mouse->rect_ex`, `dc_mouse->circle_ex`, `dc_mouse->ellipse_ex`, or `dc_mouse->polygon_ex` for placement-aware hit targets.
- Changed `dc_mouse->down(ctx, id)` to the ID-free `dc_mouse->down(ctx)`. Use `dc_mouse->active(ctx, id)` for captured ID state.

### Migration
- Regenerate `logic/dcapp.h` and rebuild logic shared libraries.
- If user code called the old generated lookup pointer directly, update it to `dc_get_variable("VariableName")`.
- If user code manually implemented `display_pre_init`, update its signature to `void display_pre_init(const DcInit *init)` and copy `init->draw`, `init->mouse`, and `init->texture` into any user globals that need them after checking `init->size`.
- If user code passed a `DcPlacement` to a mouse hit registration call, switch that call to the matching `_ex` function.
- Replace `dc_mouse->down(ctx, id)` with `dc_mouse->active(ctx, id)` for target-owned hold behavior, or `dc_mouse->down(ctx)` for raw button state.
