# Logic Files

Logic files are optional shared libraries loaded by `<Logic File="..."/>`.
Use XML for stable layout and simple interaction. Use C/C++ logic for
procedural drawing, state machines, calculations, IO, or behavior that would be
awkward to express in XML.

Use logic when the display needs behavior rather than just structure. Good fits
include algorithms, filtered values, reusable procedural drawing, custom IO, and
state that would make the XML hard to understand.

Keep simple presentation rules in XML. A color change, a button state, a
conditional label, or a direct variable assignment usually belongs in XML.

## XML Hook

```xml
<DCAPP>
    <Variable Type="#_variable_double_" InitialValue="0">PHASE</Variable>
    <Logic File="logic/logic.so"/>

    <Window Title="Logic Demo" Width="900" Height="600" UpdateRate="60">
        <Panel VirtualWidth="900" VirtualHeight="600">
            <DrawFunction Name="draw_panel">
                <Arg Type="#_variable_double_" Value="@PHASE"/>
            </DrawFunction>
        </Panel>
    </Window>
</DCAPP>
```

`File` is resolved relative to the XML file that contains the `<Logic>` element.
The loader accepts either a base path or a platform-specific filename. If you
write `logic/logic.so`, dcapp strips the extension and tries both `logic.so`
and `liblogic.so`, plus `.dylib` and `.dll` variants as appropriate.

Only one `<Logic>` library may be loaded per display.
`<Logic>` must be a direct child of `<DCAPP>`, but its position among the
root children is irrelevant. dcapp loads the declaration before it resolves
callbacks in the window render tree.

## Generated Header

Generate the display-specific logic header:

```bash
./bin/dcapp-genheader.sh path/to/display.xml
```

This writes `path/to/logic/dcapp.h`. The header contains:

- pointers for every XML variable
- lifecycle callback prototypes
- typed callback prototypes for every XML `Function` and `DrawFunction`
- `display_pre_init()`, called internally by dcapp
- the curated drawing, mouse, texture, planet, and lifecycle API contract

Do not edit `logic/dcapp.h`; regenerate it when XML variables change.

The generated file deliberately contains only the short logic-facing names,
such as `DcVec2`, `DcStroke`, and `DcDrawApi`. It does not expose dcapp's
internal `DcApp*` headers or aliases. The public declarations are maintained
explicitly in `apps/dcapp_genheader.c`, so changing an internal API requires
updating the generated contract in the same change.

The generated header exists so the logic library and XML stay coupled by the
display definition, not by hand-written declarations. If an XML variable is
renamed or its type changes, regenerating the header updates the C pointer
declarations and catches stale code at compile time. The same applies to
`Function` and `DrawFunction` names and signatures.

The generated declarations use C linkage in C++ and carry the platform export
annotation needed by dynamically loaded callbacks. Include `dcapp.h` before
defining callbacks; no separate Windows export list is required.

dcapp and its logic library use the exact current generated interface. There is
no size/version or field-offset compatibility layer: after updating dcapp,
regenerate `dcapp.h` and rebuild the logic library before loading it.

## Lifecycle

Your main logic source includes `dcapp.h` and implements:

```c
#include "dcapp.h"

void display_init(DcAppContext *app_ctx, void **user_data) {
    (void)app_ctx;
    (void)user_data;
}

void display_draw(DcAppContext *app_ctx, void *user_data) {
    (void)app_ctx;
    (void)user_data;
}

void display_close(DcAppContext *app_ctx, void *user_data) {
    (void)app_ctx;
    (void)user_data;
}
```

`display_init` runs once after XML variables are linked. `display_draw` runs
once per frame by default, or at the fixed `Window UpdateRate` cadence when
`UpdateRate` is set. `display_close` runs during cleanup.

## Why `user_data` Exists

`user_data` is the logic library's private per-display state pointer. dcapp does
not know what type it points to; it only stores the pointer and passes it back to
your callbacks.

Use it when logic needs state that should live longer than one callback, but
does not belong in XML as a display variable. Common examples are:

- cached calculations or filter history
- loaded resources owned by the logic library
- protocol/client handles
- state machines with internal fields
- small structs that organize several related runtime values

The usual flow is:

1. Allocate or assign the state in `display_init` through `void **user_data`.
2. Read it in `display_draw`, `Function`, or `DrawFunction` callbacks through
   `void *user_data`.
3. Release it in `display_close`.

This avoids forcing private implementation details into XML variables, and it
also avoids relying on file-scope globals when a display can keep the state
attached to the dcapp runtime that loaded the logic library.

Use XML variables for values the display should bind, render, set, transmit, or
inspect. Use `user_data` for private C/C++ state that supports that behavior.

Example:

```c
typedef struct DisplayState {
    double elapsed;
    double filtered_phase;
} DisplayState;

void display_init(DcAppContext *app_ctx, void **user_data) {
    (void)app_ctx;
    *user_data = calloc(1, sizeof(DisplayState));
}

void display_draw(DcAppContext *app_ctx, void *user_data) {
    (void)app_ctx;
    DisplayState *state = (DisplayState *)user_data;
    if (!state) return;

    state->elapsed += 1.0 / 60.0;
    state->filtered_phase = 0.9 * state->filtered_phase + 0.1 * (*PHASE);
}

void display_close(DcAppContext *app_ctx, void *user_data) {
    (void)app_ctx;
    free(user_data);
}
```

`app_ctx` and `user_data` serve different purposes. `app_ctx` is dcapp's runtime
context; pass it back to dcapp APIs when they require it. `user_data` is your
logic library's own state.

For additional source files compiled into the same logic library:

```c
#define DCAPP_LOGIC_EXTERN
#include "dcapp.h"
```

Do not define `DCAPP_LOGIC_EXTERN` in the source file that owns
`display_init`, `display_draw`, and `display_close`.

## Variables

XML variables become typed pointers in `logic/dcapp.h`.

| XML type | C pointer |
|----------|-----------|
| `#_variable_double_` | `double *` |
| `#_variable_integer_` | `int *` |
| `#_variable_boolean_` | `bool *` |
| `#_variable_string_` | `char (*)[256]` |

```c
void display_draw(DcAppContext *app_ctx, void *user_data) {
    (void)app_ctx;
    (void)user_data;

    *PHASE += 0.02;
}
```

Strings are fixed-size 256-byte arrays:

```c
snprintf(*STATUS_TEXT, 256, "phase %.2f", *PHASE);
```

Variables can also be looked up by name:

```c
double *phase = (double *)dc_app->get_variable(app_ctx, "PHASE");
```

## `Function` Element

`<Function Name="...">` calls a symbol in the loaded logic library. Without
`FireCall`, it runs whenever the node is drawn. With `FireCall`, it runs when
that value changes.

`Function` is executable and must appear somewhere inside the `<Window>` render
tree. It may be nested in a panel, container, conditional branch, or event
element. A root-level `Function` is invalid because root declarations are not
rendered.

Use `Function` for event-like work that is triggered from XML but easier to
write in C, such as resetting several variables, sending a command, or advancing
a state machine. Use `display_draw` for regular per-update logic. Use
`DrawFunction` when the C code needs to emit drawing commands.

```xml
<MousePressed>
    <Function Name="reset_phase"/>
</MousePressed>
```

```c
void reset_phase(DcAppContext *app_ctx, void *user_data) {
    (void)app_ctx;
    (void)user_data;
    *PHASE = 0.0;
}
```

## DrawFunction API

`<DrawFunction Name="...">` calls a C drawing callback during XML drawing. It
receives the current XML coordinate context, an optional typed argument list,
and `user_data`.

Like `Function`, `DrawFunction` must appear somewhere inside the `<Window>`
render tree.

Use `DrawFunction` when XML should own placement and surrounding layout, but C
should generate the visual content. This is useful for plots, custom gauges,
procedural shapes, dense repeated geometry, or drawing that depends on
calculated intermediate state.

```xml
<DrawFunction Name="draw_wave">
    <Arg Type="#_variable_double_" Value="@PHASE"/>
</DrawFunction>
```

```c
void draw_wave(DcDrawContext *ctx, const DcDrawFuncArgs *args, void *user_data) {
    (void)user_data;

    double phase = 0.0;
    if (args && args->count > 0) {
        phase = args->values[0].value_double;
    }

    dc_draw->line(ctx,
        (DcVec2){20.0f, 60.0f},
        (DcVec2){180.0f, 60.0f + (float)(20.0 * sin(phase))},
        (DcStroke){.color = {.r = 0.2f, .g = 0.8f, .b = 1.0f, .a = 1.0f}, .width = 2.0f});
}
```

The generated header exposes these global API tables:

| API | Use |
|-----|-----|
| `dc_app` | App-level helpers such as explicit-context variable lookup |
| `dc_draw` | Draw lines, polygons, rectangles, rounded rectangles, circles, ellipses, text, images, containers, and stencils |
| `dc_mouse` | Register hit targets and query hover/press/release/click/active state |
| `dc_texture` | Load and query app-owned textures |
| `dc_planet` | Create planet resources and load/configure textures, lighting, views, breadcrumbs, and GeoJSON |

For a registered `dc_mouse` target, `pressed()` reports the initial button-down
edge and `active()` remains true while that captured press is held.
`released()` reports the end of the captured press even if the pointer has
left the target. `clicked()` reports a completed activation only when that
release occurs while the pointer is over the target.

Target registration and ID-based event queries are pipelined by one frame.
Calls to `rect()`, `circle()`, `ellipse()`, `polygon()`, and their `_ex`
variants contribute to the current frame's hit test. During that same draw
callback, `hovered()`, `pressed()`, `released()`, `active()`, and `clicked()`
still report the target state committed after the preceding frame. The current
registrations become visible to those queries on the next frame, so register
interactive targets every frame with stable IDs.

`down()` and `get_state()` are not delayed with the target queries. They expose
the current frame's mouse input; `get_state()` reports its position in the
current draw context's local space.

See the DrawFunction samples for concrete API usage.

Planet overlays are drawn through `dc_draw`, including geodetic/cartesian
spheres, lines, polygons, images, text, ellipses, and loaded GeoJSON. See
[Planet Rendering](planet.md) for coordinate and style semantics.

## Building Logic

The top-level build scripts regenerate headers and build all bundled sample
logic libraries:

```bash
./scripts/build.sh
```

For a hand-built Linux/macOS library:

```bash
./bin/dcapp-genheader.sh my_display.xml
cc -shared -fPIC -o logic/liblogic.so logic/logic.c
```

For a hand-built Windows library:

```bat
cl /LD logic\logic.c /Fe:logic\logic.dll
```

The generated `dcapp.h` declarations export the lifecycle callbacks and all
callbacks named by XML `Function` and `DrawFunction` elements.

## Samples

| Sample | Pattern |
|--------|---------|
| `samples/drawfunction1` | XML owns cards and labels; C draws focused pieces |
| `samples/drawfunction2` | C API reference gallery |
| `samples/drawfunction3` | Hybrid XML/C procedural panel |
| `samples/drawfunction4` | Full procedural panel with C mouse hit regions |
| `samples/lissajous` | C-updated variables and drawing |
| `samples/ptz` | Logic-backed controls |
| `samples/planet` | Logic-created planet/view interop |

## Troubleshooting

- If variables are missing, regenerate `logic/dcapp.h`.
- If a symbol is missing, check that the function name in XML exactly matches
  the generated declaration and C function definition, then regenerate
  `logic/dcapp.h`.
- A callback name cannot be used by both `Function` and `DrawFunction`, because
  those elements require different C signatures.
- If the library fails to load, check the `Logic File` path and platform
  filename (`liblogic.so`, `liblogic.dylib`, or `logic.dll`).
- In C++, include the generated header before callback definitions so they
  inherit its C linkage and export annotation.
