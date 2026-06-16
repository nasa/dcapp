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

## Generated Header

Generate the display-specific logic header:

```bash
./bin/dcapp-genheader.sh path/to/display.xml
```

This writes `path/to/logic/dcapp.h`. The header contains:

- pointers for every XML variable
- lifecycle callback prototypes
- `display_pre_init()`, called internally by dcapp
- API tables for drawing, mouse hit targets, textures, and planet helpers

Do not edit `logic/dcapp.h`; regenerate it when XML variables change.

The generated header exists so the logic library and XML stay coupled by the
display definition, not by hand-written declarations. If an XML variable is
renamed or its type changes, regenerating the header updates the C pointer
declarations and catches stale code at compile time.

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
#define _DCAPP_LOGIC_EXTERN_
#include "dcapp.h"
```

Do not define `_DCAPP_LOGIC_EXTERN_` in the source file that owns
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
double *phase = (double *)dc_get_variable(app_ctx, "PHASE");
```

## `Function` Element

`<Function Name="...">` calls a symbol in the loaded logic library. Without
`FireCall`, it runs whenever the node is drawn. With `FireCall`, it runs when
that value changes.

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
| `dc_draw` | Draw lines, polygons, rectangles, rounded rectangles, circles, ellipses, text, images, containers, and stencils |
| `dc_mouse` | Register hit targets and query hover/press/release/click/active state |
| `dc_texture` | Load and query app-owned textures |
| `dc_planet` | Create planet resources and draw planet overlays from logic |

See the DrawFunction samples for concrete API usage.

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

On Windows, export the lifecycle symbols:

```bat
cl /LD logic\logic.c /Fe:logic\logic.dll ^
  /link -EXPORT:display_pre_init -EXPORT:display_init -EXPORT:display_draw -EXPORT:display_close
```

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
  the exported C function.
- If the library fails to load, check the `Logic File` path and platform
  filename (`liblogic.so`, `liblogic.dylib`, or `logic.dll`).
- On Windows, make sure the four lifecycle symbols are exported.
