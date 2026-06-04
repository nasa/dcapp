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

void display_init(void) {
    // Called once after variables are linked
    *altitude = 1000.0;
    *velocity = 0.0;
    *acceleration = -9.81;
}

void display_draw(void) {
    // Called every frame
    static double dt = 0.016; // ~60fps
    
    *velocity += *acceleration * dt;
    *altitude += *velocity * dt;
    
    // Ground collision
    if (*altitude < 0) {
        *altitude = 0;
        *velocity = 0;
    }
}

void display_close(void) {
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
typedef void *(*DcGetVariableFn)(void *user_data, const char *name);

typedef struct _DcInit {
    uint32_t size;
    uint32_t version;
    void *user_data;
    DcGetVariableFn get_variable;
    const DcDrawApi *draw;
    const DcMouseApi *mouse;
} DcInit;

#ifndef _DCAPP_LOGIC_EXTERN_

// Runtime globals used by logic files.
void *dc_user_data;
DcGetVariableFn dc_get_variable_fn;
const DcDrawApi *dc_draw;
const DcMouseApi *dc_mouse;

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
void display_init(void);
void display_draw(void);
void display_close(void);

// Auto-generated initialization (do not call directly)
void *dc_get_variable(const char *name) {
    if (!dc_get_variable_fn) return NULL;
    return dc_get_variable_fn(dc_user_data, name);
}

void display_pre_init(const DcInit *init) {
    if (init && init->version >= 1 && init->size >= sizeof(DcInit)) {
        dc_user_data = init->user_data;
        dc_get_variable_fn = init->get_variable;
        dc_draw = init->draw;
        dc_mouse = init->mouse;
        altitude = (double *)dc_get_variable("altitude");
        velocity = (double *)dc_get_variable("velocity");
        frameCount = (int *)dc_get_variable("frameCount");
        statusMessage = (char (*)[256])dc_get_variable("statusMessage");
        isRunning = (bool *)dc_get_variable("isRunning");
    }
}

#ifdef __cplusplus
}
#endif

#else

#ifdef __cplusplus
extern "C" {
#endif

void *dc_get_variable(const char *name);
extern const DcDrawApi *dc_draw;
extern const DcMouseApi *dc_mouse;

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

Your logic file must implement these three functions:

### `display_init()`

Called once after all variables have been linked. Use for:
- Setting initial variable values
- Allocating resources
- Opening files or connections

```c
void display_init(void) {
    *altitude = 10000.0;
    *velocity = 0.0;
    *isRunning = true;
    strcpy(*statusMessage, "Initialized");
}
```

### `display_draw()`

Called every frame (typically 60fps). Use for:
- Updating variable values
- Calculations and simulations
- State machine logic

```c
void display_draw(void) {
    static int frame = 0;
    frame++;
    
    // Update physics
    *velocity += *acceleration * 0.016;
    *altitude += *velocity * 0.016;
    
    // Update status every second
    if (frame % 60 == 0) {
        snprintf(*statusMessage, 256, "Running: %d seconds", frame / 60);
    }
}
```

### `display_close()`

Called when the display closes. Use for:
- Freeing allocated resources
- Closing files or connections
- Final cleanup

```c
void display_close(void) {
    // Cleanup code here
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

## C++ Support

The generated header includes C++ compatibility guards. For C++ code:

```cpp
#include "dcapp.h"

#include <cmath>
#include <string>

void display_init(void) {
    *altitude = 1000.0;
}

void display_draw(void) {
    // Use C++ features
    *altitude = std::max(0.0, *altitude + *velocity * 0.016);
    
    // String handling
    std::string status = "Alt: " + std::to_string(*altitude);
    strncpy(*statusMessage, status.c_str(), 255);
}

void display_close(void) {
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

void display_init(void) {
    physics_init();
}

void display_draw(void) {
    physics_update(0.016);
    update_status();
}

void display_close(void) {
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

void display_draw(void) {
    static int frame = 0;
    if (frame++ % 60 == 0) {  // Print once per second
        printf("Frame %d: alt=%.2f vel=%.2f\n", 
               frame, *altitude, *velocity);
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

void display_init(void) {
    // Initial conditions set in XML, but we can override:
    // *altitude = 10000.0;
    // *velocity = 0.0;
    strcpy(*status, "READY");
}

void display_draw(void) {
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

void display_close(void) {
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

When this element is rendered, it calls the function `my_function()` from your loaded logic library every frame.

### Edge-Triggered Calls with `FireCall`

By default, `<Function>` fires every frame. To make it fire only when a value changes, use the `FireCall` attribute:

```xml
<Function Name="my_function" FireCall="@triggerVar"/>
```

The function will be called once each time the value of `triggerVar` changes. If `FireCall` is omitted, the function fires every frame (original behavior).

### Function Signature

Functions called via `<Function>` must have this signature:

```c
void my_function(void);
```

No parameters, no return value.

### Example: Button Click Handlers

```xml
<Variable Type="#_variable_integer_" InitialValue="0">buttonState</Variable>

<Logic File="logic/logic.so"/>

<Window Title="Function Demo" Width="400" Height="300">
    <Button X="100" Y="100" Width="100" Height="50"
            Variable="buttonState" On="1" Off="0" Type="#_button_momentary_">
        <ButtonIndicatorOn>
            <Rectangle FillColor="0.2,0.6,0.2,1" Width="100" Height="50"/>
            <Text X="50" Y="25" LocalAlignX="#_align_center_" LocalAlignY="#_align_middle_">Clicked!</Text>
            <Function Name="on_button_click"/>
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

void display_init(void) {}
void display_draw(void) {}
void display_close(void) {}

void on_button_click() {
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
