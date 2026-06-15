# dcapp Logic Files

A guide to extending dcapp displays with custom C/C++ code.

---

## Overview

Logic files allow you to implement complex behavior that goes beyond what's possible in XML alone. A logic file is a shared library (`.so` on Linux, `.dylib` on macOS, `.dll` on Windows) that dcapp loads at runtime, giving your code direct access to all declared variables.

Use cases for logic files:
- Complex mathematical calculations
- State machines and control logic
- Custom algorithms and simulations
- Hardware integration
- File I/O operations
- Network communication beyond TrickIO

---

## Quick Start

### 1. Create Your Display XML

```xml
<DCAPP>
    <!-- Declare your variables -->
    <Variable Type="#_variable_double_" InitialValue="0">altitude</Variable>
    <Variable Type="#_variable_double_" InitialValue="0">velocity</Variable>
    <Variable Type="#_variable_double_" InitialValue="0">acceleration</Variable>
    
    <!-- Reference the logic file -->
    <Logic File="logic/libdisplay_logic.so"/>
    
    <Window Title="Physics Simulation" Width="800" Height="600">
        <Text X="20" Y="100" Size="24" FillColor="0,1,0,1">
            Alt: @altitude(%.1f) m
        </Text>
        <Text X="20" Y="60" Size="24" FillColor="0,1,0,1">
            Vel: @velocity(%.2f) m/s
        </Text>
    </Window>
</DCAPP>
```

### 2. Generate the Header File

Run `dcapp-genheader` on your XML file:

```bash
dcapp-genheader myDisplay.xml
```

This creates `logic/dcapp.h` in your display directory containing variable pointers and function declarations.

### 3. Write Your Logic Code

Create `logic/display_logic.c`:

```c
#include "dcapp.h"

void display_init(DcAppContext *app_ctx, void **user_data) {
    (void)app_ctx;
    (void)user_data;

    // Called once after variables are linked
    *altitude = 1000.0;
    *velocity = 0.0;
    *acceleration = -9.81;
}

void display_draw(DcAppContext *app_ctx, void *user_data) {
    (void)app_ctx;
    (void)user_data;

    // Called each update tick when Window UpdateRate is set
    static double dt = 0.016; // 60 updates per second
    
    *velocity += *acceleration * dt;
    *altitude += *velocity * dt;
    
    // Ground collision
    if (*altitude < 0) {
        *altitude = 0;
        *velocity = 0;
    }
}

void display_close(DcAppContext *app_ctx, void *user_data) {
    (void)app_ctx;
    (void)user_data;

    // Called when display closes
}
```

### 4. Compile the Library

```bash
# Linux
gcc -shared -fPIC -o logic/libdisplay_logic.so logic/display_logic.c

# macOS
gcc -shared -fPIC -o logic/libdisplay_logic.dylib logic/display_logic.c

# Windows (MSVC)
cl /LD logic/display_logic.c /Fe:logic/display_logic.dll
```

### 5. Run Your Display

```bash
dcapp myDisplay.xml
```

---

## The Generated Header File

When you run `dcapp-genheader`, it creates a `dcapp.h` file in the `logic/` subdirectory of your display. This file is auto-generated — **do not edit it manually**.

### Header Contract

The generated header contains the public DrawFunction types first, including
`DcDrawApi`, `DcMouseApi`, `DcDrawContext`, `DcPlacement`, `DcVec2`, `DcVec4`,
and related helper types. After that, it emits the runtime globals and XML
variable pointers. The important ownership/linkage shape is:

```c
// ********************************************* //
// THIS FILE IS AUTO-GENERATED -- DO NOT EDIT!!! //
// ********************************************* //

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef DCAPP_H
#define DCAPP_H

// DrawFunction API typedefs are generated here.
typedef struct _DcDrawApi DcDrawApi;
typedef struct _DcMouseApi DcMouseApi;
typedef struct _DcTextureApi DcTextureApi;
typedef uint32_t DcTextureId;
typedef struct _DcAppContext DcAppContext;
typedef void *(*DcGetVariableFn)(DcAppContext *app_ctx, const char *name);

typedef struct _DcInit {
    uint32_t size;
    uint32_t version;
    DcAppContext *app_ctx;
    DcGetVariableFn get_variable;
    const DcDrawApi *draw;
    const DcMouseApi *mouse;
    const DcTextureApi *texture;
} DcInit;

#ifndef _DCAPP_LOGIC_EXTERN_

// Runtime globals used by logic files.
DcGetVariableFn dc_get_variable_fn;
const DcDrawApi *dc_draw;
const DcMouseApi *dc_mouse;
const DcTextureApi *dc_texture;

// XML variable pointers resolved during display_pre_init().
double *altitude;
double *velocity;
int    *frameCount;
char   (*statusMessage)[256];
bool   *isRunning;

#ifdef __cplusplus
extern "C" {
#endif

// Function declarations you must implement
void display_init(DcAppContext *app_ctx, void **user_data);
void display_draw(DcAppContext *app_ctx, void *user_data);
void display_close(DcAppContext *app_ctx, void *user_data);

// Auto-generated initialization (do not call directly)
void *dc_get_variable(DcAppContext *app_ctx, const char *name) {
    if (!dc_get_variable_fn) return NULL;
    return dc_get_variable_fn(app_ctx, name);
}

DcTextureId dc_load_image(DcAppContext *app_ctx, const char *path, DcVec2 *out_size);
bool dc_get_texture_size(DcAppContext *app_ctx, DcTextureId texture_id, DcVec2 *out_size);

void display_pre_init(const DcInit *init) {
    if (init && init->version >= 1 && init->size >= sizeof(DcInit)) {
        dc_get_variable_fn = init->get_variable;
        dc_draw = init->draw;
        dc_mouse = init->mouse;
        dc_texture = init->texture;
        altitude = (double *)dc_get_variable(init->app_ctx, "altitude");
        velocity = (double *)dc_get_variable(init->app_ctx, "velocity");
        frameCount = (int *)dc_get_variable(init->app_ctx, "frameCount");
        statusMessage = (char (*)[256])dc_get_variable(init->app_ctx, "statusMessage");
        isRunning = (bool *)dc_get_variable(init->app_ctx, "isRunning");
    }
}

#ifdef __cplusplus
}
#endif

#else

#ifdef __cplusplus
extern "C" {
#endif

void *dc_get_variable(DcAppContext *app_ctx, const char *name);
DcTextureId dc_load_image(DcAppContext *app_ctx, const char *path, DcVec2 *out_size);
bool dc_get_texture_size(DcAppContext *app_ctx, DcTextureId texture_id, DcVec2 *out_size);
extern const DcDrawApi *dc_draw;
extern const DcMouseApi *dc_mouse;
extern const DcTextureApi *dc_texture;

// XML variable pointers shared from the main logic translation unit.
extern double *altitude;
extern double *velocity;
extern int    *frameCount;
extern char   (*statusMessage)[256];
extern bool   *isRunning;

#ifdef __cplusplus
}
#endif

#endif
#endif
```

Define `_DCAPP_LOGIC_EXTERN_` before including `dcapp.h` in additional source
files that are compiled into the same logic library. Do not define it in the
source file that implements `display_init`, `display_draw`, and `display_close`;
that file owns the runtime globals and XML variable pointer definitions.

### Variable Type Mapping

| XML Type | C Type | Pointer Type |
|----------|--------|--------------|
| `Double` | `double` | `double *` |
| `Integer` | `int` | `int *` |
| `Boolean` | `bool` | `bool *` |
| `String` | `char[256]` | `char (*)[256]` |

### Using the Header

In your **main** logic source file (the one that defines the functions):

```c
#include "dcapp.h"
// No #define - gets the actual variable definitions and display_pre_init()
```

In **additional** source files that need access to variables:

```c
#define _DCAPP_LOGIC_EXTERN_
#include "dcapp.h"
// With #define - gets extern declarations only
```

---

## Required Functions

Your logic file must implement these three functions. `app_ctx` is the dcapp
runtime context for app-owned APIs. `user_data` is optional user-owned state:
set `*user_data` in `display_init()`, then cast and use it in later callbacks.

### `display_init()`

Called once after all variables have been linked. Use for:
- Setting initial variable values
- Allocating resources
- Opening files or connections

```c
void display_init(DcAppContext *app_ctx, void **user_data) {
    (void)app_ctx;
    (void)user_data;

    *altitude = 10000.0;
    *velocity = 0.0;
    *isRunning = true;
    strcpy(*statusMessage, "Initialized");
}
```

### `display_draw()`

Called once per render by default, or at the fixed `<Window UpdateRate="...">` cadence when `UpdateRate` is set. Use for:
- Updating variable values
- Calculations and simulations
- State machine logic

```c
void display_draw(DcAppContext *app_ctx, void *user_data) {
    (void)app_ctx;
    (void)user_data;

    static int update_count = 0;
    update_count++;
    
    // Update physics
    *velocity += *acceleration * 0.016;
    *altitude += *velocity * 0.016;
    
    // Update status every second
    if (update_count % 60 == 0) {
        snprintf(*statusMessage, 256, "Running: %d seconds", update_count / 60);
    }
}
```

### `display_close()`

Called when the display closes. Use for:
- Freeing allocated resources
- Closing files or connections
- Final cleanup

```c
void display_close(DcAppContext *app_ctx, void *user_data) {
    (void)app_ctx;
    (void)user_data;

    // Cleanup code here
}
```

### Optional User Data

Use `user_data` when your logic needs persistent state without file-scope
globals:

```c
#include "dcapp.h"
#include <stdlib.h>

typedef struct DisplayState {
    double elapsed;
    int event_count;
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
}

void display_close(DcAppContext *app_ctx, void *user_data) {
    (void)app_ctx;
    free(user_data);
}
```

---

## Working with Variables

### Numeric Variables (Double, Integer, Boolean)

These are simple pointers — dereference to read or write:

```c
// Reading
double current_alt = *altitude;
int count = *frameCount;
bool running = *isRunning;

// Writing
*altitude = 5000.0;
*frameCount = 0;
*isRunning = false;
```

### String Variables

Strings are fixed-size character arrays (256 bytes). Use standard C string functions:

```c
// Reading
printf("Status: %s\n", *statusMessage);

// Writing
strcpy(*statusMessage, "New status");
snprintf(*statusMessage, 256, "Altitude: %.1f", *altitude);

// Comparing
if (strcmp(*statusMessage, "ARMED") == 0) {
    // ...
}
```

---

## DrawFunction API

Logic libraries can also export draw callbacks and call them from XML with
`<DrawFunction Name="...">`. A DrawFunction receives the current draw context,
an optional typed argument list, and optional user-owned state:

```c
void draw_widget(DcDrawContext *ctx, const DcDrawFuncArgs *args, void *user_data) {
    (void)user_data;
    (void)args;

    DcStroke stroke = {
        .color = { .r = 0.2f, .g = 0.8f, .b = 1.0f, .a = 1.0f },
        .width = 2.0f,
        .pattern = 0xFF
    };

    dc_draw->rect_filled(ctx, (DcVec2){10, 10}, (DcVec2){160, 80},
                         (DcVec4){ .r = 0.05f, .g = 0.07f, .b = 0.09f, .a = 1.0f });
    dc_draw->ellipse(ctx, (DcVec2){90, 50}, (DcVec2){58, 28}, stroke);
}
```

```xml
<DrawFunction Name="draw_widget">
    <Arg Type="#_variable_string_" Value="demo"/>
</DrawFunction>
```

### Drawing

Use the global `dc_draw` table for immediate-mode drawing in the current XML
coordinate space. The API includes:

- Geometry: `line`, `polyline`, `polygon`, `polygon_filled`, `quad`, `quad_filled`, and rounded polygon/quad variants.
- Shapes: `rect`, `rect_filled`, rounded rectangle variants, `circle`, `circle_filled`, `ellipse`, and `ellipse_filled`.
- Text and images: `text_size`, `text`, and `image`.
- Layout/stencil helpers: `container_push`, `container_push_ex`, `container_push_area`, `container_pop`, `stencil_begin`, `stencil_add`, `stencil_remove`, `stencil_draw`, and `stencil_end`.

Most draw functions also have an `_ex` variant that accepts a `DcPlacement` and
returns a `DcDrawResult` containing the resolved draw area.

### Mouse

Use the global `dc_mouse` table to register hit targets and query events:

```c
dc_mouse->rect(ctx, "button", (DcVec2){10, 10}, (DcVec2){120, 40});
if (dc_mouse->clicked(ctx, "button")) {
    *counter += 1;
}
```

Basic hit registration is available for `rect`, `circle`, `ellipse`, and
`polygon`. Use the matching `_ex` function when the hit target needs placement,
rotation, or pivot handling. Event queries include `hovered`, `pressed`,
`released`, `active`, `clicked`, `down`, and `get_state`.

### Textures

Load images during `display_init()` or another setup path, then draw them from
DrawFunction callbacks:

```c
static DcTextureId logo;
static DcVec2 logo_size;

void display_init(DcAppContext *app_ctx, void **user_data) {
    (void)user_data;
    logo = dc_load_image(app_ctx, "assets/logo.png", &logo_size);
}

void draw_logo(DcDrawContext *ctx, const DcDrawFuncArgs *args, void *user_data) {
    (void)user_data;
    (void)args;

    if (logo) {
        dc_draw->image(ctx, logo, (DcVec2){20, 20}, logo_size,
                       (DcVec4){ .r = 1, .g = 1, .b = 1, .a = 1 });
    }
}
```

`dc_get_texture_size(texture_id, &size)` can be used later if you need to query
the dimensions again.

---

## C++ Support

The generated header includes C++ compatibility guards. For C++ code:

```cpp
#include "dcapp.h"

#include <cmath>
#include <string>

void display_init(DcAppContext *app_ctx, void **user_data) {
    (void)app_ctx;
    (void)user_data;
    *altitude = 1000.0;
}

void display_draw(DcAppContext *app_ctx, void *user_data) {
    (void)app_ctx;
    (void)user_data;

    // Use C++ features
    *altitude = std::max(0.0, *altitude + *velocity * 0.016);
    
    // String handling
    std::string status = "Alt: " + std::to_string(*altitude);
    strncpy(*statusMessage, status.c_str(), 255);
}

void display_close(DcAppContext *app_ctx, void *user_data) {
    (void)app_ctx;
    (void)user_data;
}
```

Compile with:
```bash
g++ -shared -fPIC -o logic/libdisplay_logic.so logic/display_logic.cpp
```

---

## Multiple Source Files

For larger projects, split your code across multiple files:

**logic/main.c** (defines functions — do NOT define _DCAPP_LOGIC_EXTERN_):
```c
#include "dcapp.h"
#include "physics.h"
#include "utils.h"

void display_init(DcAppContext *app_ctx, void **user_data) {
    (void)app_ctx;
    (void)user_data;
    physics_init();
}

void display_draw(DcAppContext *app_ctx, void *user_data) {
    (void)app_ctx;
    (void)user_data;
    physics_update(0.016);
    update_status();
}

void display_close(DcAppContext *app_ctx, void *user_data) {
    (void)app_ctx;
    (void)user_data;
    physics_cleanup();
}
```

**logic/physics.c** (additional file — MUST define _DCAPP_LOGIC_EXTERN_):
```c
#define _DCAPP_LOGIC_EXTERN_
#include "dcapp.h"
#include "physics.h"

void physics_init() {
    *altitude = 10000.0;
    *velocity = 0.0;
}

void physics_update(double dt) {
    *velocity += *acceleration * dt;
    *altitude += *velocity * dt;
}

void physics_cleanup() {
    // ...
}
```

**logic/physics.h**:
```c
#ifndef PHYSICS_H
#define PHYSICS_H

void physics_init(void);
void physics_update(double dt);
void physics_cleanup(void);

#endif
```

Compile all together:
```bash
gcc -shared -fPIC -o logic/libdisplay_logic.so \
    logic/main.c logic/physics.c logic/utils.c
```

---

## XML Element Reference

### `<Logic>`

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `File` | string | **Yes** | Path to shared library (.so/.dll/.dylib) |

The path can be:
- **Relative:** Resolved from the XML file's directory
- **Absolute:** Used as-is

```xml
<!-- Relative path (recommended) -->
<Logic File="logic/libdisplay_logic.so"/>

<!-- Absolute path -->
<Logic File="/opt/displays/myapp/liblogic.so"/>
```

### Placement in XML

Place `<Logic>` after your `<Variable>` declarations but before `<Window>`:

```xml
<DCAPP>
    <Variable Type="#_variable_double_" InitialValue="0">altitude</Variable>
    <Variable Type="#_variable_double_" InitialValue="0">velocity</Variable>
    
    <Logic File="logic/libdisplay_logic.so"/>
    
    <Window ...>
        ...
    </Window>
</DCAPP>
```

**Note:** dcapp will warn if `<Logic>` appears after variables are defined, but it will still work. The warning exists because the header generation must happen before compilation.

---

## Build System Integration

### Makefile Example

```makefile
CC = gcc
CFLAGS = -fPIC -Wall -O2
LDFLAGS = -shared

DISPLAY = myDisplay.xml
LOGIC_DIR = logic
LOGIC_SRC = $(wildcard $(LOGIC_DIR)/*.c)
LOGIC_LIB = $(LOGIC_DIR)/libdisplay_logic.so

.PHONY: all clean header

all: header $(LOGIC_LIB)

header:
	dcapp-genheader $(DISPLAY)

$(LOGIC_LIB): $(LOGIC_SRC) $(LOGIC_DIR)/dcapp.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(LOGIC_SRC)

clean:
	rm -f $(LOGIC_LIB)
```

### CMake Example

```cmake
cmake_minimum_required(VERSION 3.10)
project(display_logic)

# Generate header (run manually or as custom command)
# dcapp-genheader ${CMAKE_SOURCE_DIR}/myDisplay.xml

add_library(display_logic SHARED
    logic/main.c
    logic/physics.c
)

target_include_directories(display_logic PRIVATE
    ${CMAKE_SOURCE_DIR}/logic
)

set_target_properties(display_logic PROPERTIES
    PREFIX ""  # Remove 'lib' prefix on Linux if desired
)
```

---

## Debugging Tips

### Print Debugging

```c
#include <stdio.h>

void display_draw(DcAppContext *app_ctx, void *user_data) {
    (void)app_ctx;
    (void)user_data;

    static int update_count = 0;
    if (update_count++ % 60 == 0) {  // Print once per second with UpdateRate="60"
        printf("Update %d: alt=%.2f vel=%.2f\n",
               update_count, *altitude, *velocity);
    }
}
```

### GDB Debugging

1. Compile with debug symbols:
   ```bash
   gcc -shared -fPIC -g -o logic/libdisplay_logic.so logic/*.c
   ```

2. Run dcapp under GDB:
   ```bash
   gdb --args dcapp myDisplay.xml
   ```

3. Set breakpoints in your logic:
   ```
   (gdb) break display_draw
   (gdb) run
   ```

### Common Issues

**Library not found:**
- Check the path in `<Logic File="..."/>`
- Ensure the file exists and has correct permissions
- On Linux, you may need to set `LD_LIBRARY_PATH`

**Undefined symbols:**
- Make sure all three functions are implemented
- Make sure only ONE source file includes `dcapp.h` without `#define _DCAPP_LOGIC_EXTERN_`
- All other source files must `#define _DCAPP_LOGIC_EXTERN_` before including `dcapp.h`

**Variables are NULL:**
- Ensure `<Logic>` appears after `<Variable>` declarations in XML
- Regenerate `dcapp.h` after adding new variables

**Crash on string access:**
- Remember strings are `char (*)[256]`, not `char *`
- Always use `*statusMessage` not `statusMessage`
- Don't write more than 255 characters (leave room for null terminator)

---

## Complete Example

**myDisplay.xml:**
```xml
<DCAPP>
    <Variable Type="#_variable_double_" InitialValue="10000">altitude</Variable>
    <Variable Type="#_variable_double_" InitialValue="0">velocity</Variable>
    <Variable Type="#_variable_double_" InitialValue="-9.81">gravity</Variable>
    <Variable Type="#_variable_string_" InitialValue="FALLING">status</Variable>
    <Variable Type="#_variable_boolean_" InitialValue="true">simRunning</Variable>
    
    <Logic File="logic/libphysics.so"/>
    
    <Window Title="Freefall Simulation" Width="400" Height="300">
        <Rectangle FillColor="0.1,0.1,0.2,1" Width="400" Height="300"/>
        
        <Text X="20" Y="250" Size="16" FillColor="0.6,0.6,0.6,1">ALTITUDE</Text>
        <Text X="20" Y="220" Size="28" FillColor="0,1,0,1">@altitude(%.1f) m</Text>
        
        <Text X="20" Y="170" Size="16" FillColor="0.6,0.6,0.6,1">VELOCITY</Text>
        <Text X="20" Y="140" Size="28" FillColor="0,1,0,1">@velocity(%.2f) m/s</Text>
        
        <Text X="20" Y="80" Size="20" FillColor="1,1,0,1">@status</Text>
        
        <Button X="20" Y="20" Width="100" Height="40" 
                Variable="simRunning" On="true" Off="false" Type="#_button_toggle_">
            <Enabled>
                <On>
                    <Rectangle FillColor="0.2,0.6,0.2,1" Width="100" Height="40"/>
                    <Text X="50" Y="20" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
                          FillColor="1,1,1,1">RUNNING</Text>
                </On>
                <Off>
                    <Rectangle FillColor="0.6,0.2,0.2,1" Width="100" Height="40"/>
                    <Text X="50" Y="20" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_"
                          FillColor="1,1,1,1">PAUSED</Text>
                </Off>
            </Enabled>
        </Button>
    </Window>
</DCAPP>
```

**logic/physics.c:**
```c
#include "dcapp.h"

#include <string.h>
#include <stdio.h>

static const double DT = 0.016;  // ~60fps

void display_init(DcAppContext *app_ctx, void **user_data) {
    (void)app_ctx;
    (void)user_data;

    // Initial conditions set in XML, but we can override:
    // *altitude = 10000.0;
    // *velocity = 0.0;
    strcpy(*status, "READY");
}

void display_draw(DcAppContext *app_ctx, void *user_data) {
    (void)app_ctx;
    (void)user_data;

    // Only simulate if running
    if (!*simRunning) {
        return;
    }
    
    // Physics integration
    *velocity += *gravity * DT;
    *altitude += *velocity * DT;
    
    // Ground collision
    if (*altitude <= 0) {
        *altitude = 0;
        *velocity = 0;
        *simRunning = false;
        strcpy(*status, "LANDED");
    } else if (*velocity < -100) {
        strcpy(*status, "TERMINAL VELOCITY");
    } else {
        strcpy(*status, "FALLING");
    }
}

void display_close(DcAppContext *app_ctx, void *user_data) {
    (void)app_ctx;
    (void)user_data;

    // Nothing to clean up
}
```

**Build and run:**
```bash
dcapp-genheader myDisplay.xml
gcc -shared -fPIC -o logic/libphysics.so logic/physics.c
dcapp myDisplay.xml
```

---

## The `<Function>` Element

The `<Function>` element allows you to call functions from your logic file directly from XML. This is useful for triggering actions in response to button clicks, conditionals, or other events without having to poll variables in `display_draw()`.

### Basic Usage

```xml
<Function Name="my_function"/>
```

When this element is rendered, it calls the function `my_function(app_ctx, user_data)` from your loaded logic library every frame.

### Edge-Triggered Calls with `FireCall`

By default, `<Function>` fires every frame. To make it fire only when a value changes, use the `FireCall` attribute:

```xml
<Function Name="my_function" FireCall="@triggerVar"/>
```

The function will be called once each time the value of `triggerVar` changes. If `FireCall` is omitted, the function fires every frame (original behavior).

### Function Signature

Functions called via `<Function>` must have this signature:

```c
void my_function(DcAppContext *app_ctx, void *user_data);
```

`app_ctx` is the dcapp runtime context. `user_data` is the pointer set through
`display_init()`, or `NULL` if you did not set one.

### Example: Button Click Handlers

```xml
<Variable Type="#_variable_integer_" InitialValue="0">buttonState</Variable>

<Logic File="logic/logic.so"/>

<Variable Type="#_variable_integer_" InitialValue="0">buttonState</Variable>
<Variable Type="#_variable_integer_" InitialValue="0">buttonTrigger</Variable>

<Function Name="on_button_click" FireCall="@buttonTrigger"/>

<Window Title="Function Demo" Width="400" Height="300">
    <Button X="100" Y="100" Width="100" Height="50"
            Variable="buttonState" On="1" Off="0" Type="#_button_momentary_">
        <MousePressed>
            <Set Variable="buttonTrigger" Operator="#_set_add_">1</Set>
        </MousePressed>
        <ButtonIndicatorOn>
            <Rectangle FillColor="0.2,0.6,0.2,1" Width="100" Height="50"/>
            <Text X="50" Y="25" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_">Clicked!</Text>
        </ButtonIndicatorOn>
        <ButtonIndicatorOff>
            <Rectangle FillColor="0.3,0.3,0.3,1" Width="100" Height="50"/>
            <Text X="50" Y="25" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_">Click Me</Text>
        </ButtonIndicatorOff>
    </Button>
</Window>
```

**logic/logic.c:**
```c
#include "dcapp.h"
#include <stdio.h>

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

void on_button_click(DcAppContext *app_ctx, void *user_data) {
    (void)app_ctx;
    (void)user_data;

    printf("Button was clicked!\n");
    // Do something interesting here
}
```

### Attributes

| Attribute | Type | Required | Description |
|-----------|------|----------|-------------|
| `Name` | string | Yes | Name of the C function to call |
| `FireCall` | integer/var | No | Edge-triggered: function fires once each time this value changes. If omitted, fires every frame. |

### Use Cases

- **Button handlers:** Execute code when buttons are pressed/released
- **Conditional actions:** Run functions when conditions become true
- **Event-driven logic:** Use `FireCall` to trigger actions on value change instead of polling in `display_draw()`

### Notes

- The function must exist in the loaded logic library
- If the function doesn't exist, dcapp will print a warning but continue running
- Functions are called during the render phase, so keep them fast to avoid frame drops

---

## Cross-Platform Library Loading

dcapp automatically handles different shared library extensions across platforms. You can specify any extension in your XML and dcapp will try all platform-appropriate extensions:

```xml
<!-- Any of these work on any platform -->
<Logic File="logic/mylib.so"/>
<Logic File="logic/mylib.dylib"/>
<Logic File="logic/mylib.dll"/>
<Logic File="logic/mylib"/>  <!-- Extension optional -->
```

dcapp tries extensions in this order: `.so`, `.dylib`, `.dll`

This means you can:
- Use the same XML file across Linux, macOS, and Windows
- Just compile your logic library with the appropriate extension for each platform
- Not worry about changing XML when switching platforms
