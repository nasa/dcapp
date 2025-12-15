# dcapp Logic Files

A guide to extending dcapp displays with custom C/C++ code.

---

## Overview

Logic files allow you to implement complex behavior that goes beyond what's possible in XML alone. A logic file is a shared library (`.so` on Linux, `.dll` on Windows) that dcapp loads at runtime, giving your code direct access to all declared variables.

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
<dcapp>
    <!-- Declare your variables -->
    <Variable Type="Double" InitialValue="0">altitude</Variable>
    <Variable Type="Double" InitialValue="0">velocity</Variable>
    <Variable Type="Double" InitialValue="0">acceleration</Variable>
    
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
</dcapp>
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

void display_init() {
    // Called once after variables are linked
    *altitude = 1000.0;
    *velocity = 0.0;
    *acceleration = -9.81;
}

void display_draw() {
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

void display_close() {
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

### Header Structure

```c
// ********************************************* //
// THIS FILE IS AUTO-GENERATED -- DO NOT EDIT!!! //
// ********************************************* //

#include <stdbool.h>

#ifndef _DCAPP_LOGIC_EXTERN_
#define _DCAPP_LOGIC_EXTERN_

// Variable pointer declarations
double *altitude;
double *velocity;
int    *frameCount;
char   (*statusMessage)[256];
bool   *isRunning;

#ifdef __cplusplus
extern "C" {
#endif

// Function declarations you must implement
void display_init();
void display_draw();
void display_close();

// Auto-generated initialization (do not call directly)
typedef void *(*_GetVariableValueAddr)(const char *name);
void display_pre_init(_GetVariableValueAddr get_variable_value_addr) {
    if (get_variable_value_addr) {
        altitude      = (double *)get_variable_value_addr("altitude");
        velocity      = (double *)get_variable_value_addr("velocity");
        frameCount    = (int *)get_variable_value_addr("frameCount");
        statusMessage = (char (*)[256])get_variable_value_addr("statusMessage");
        isRunning     = (bool *)get_variable_value_addr("isRunning");
    }
}

#ifdef __cplusplus
}
#endif

#else

// Extern declarations for additional source files
extern double *altitude;
extern double *velocity;
extern int    *frameCount;
extern char   (*statusMessage)[256];
extern bool   *isRunning;

#endif
```

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
void display_init() {
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
void display_draw() {
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
void display_close() {
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

void display_init() {
    *altitude = 1000.0;
}

void display_draw() {
    // Use C++ features
    *altitude = std::max(0.0, *altitude + *velocity * 0.016);
    
    // String handling
    std::string status = "Alt: " + std::to_string(*altitude);
    strncpy(*statusMessage, status.c_str(), 255);
}

void display_close() {
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

void display_init() {
    physics_init();
}

void display_draw() {
    physics_update(0.016);
    update_status();
}

void display_close() {
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
<dcapp>
    <Variable Type="Double" InitialValue="0">altitude</Variable>
    <Variable Type="Double" InitialValue="0">velocity</Variable>
    
    <Logic File="logic/libdisplay_logic.so"/>
    
    <Window ...>
        ...
    </Window>
</dcapp>
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

void display_draw() {
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
<dcapp>
    <Variable Type="Double" InitialValue="10000">altitude</Variable>
    <Variable Type="Double" InitialValue="0">velocity</Variable>
    <Variable Type="Double" InitialValue="-9.81">gravity</Variable>
    <Variable Type="String" InitialValue="FALLING">status</Variable>
    <Variable Type="Boolean" InitialValue="true">simRunning</Variable>
    
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
</dcapp>
```

**logic/physics.c:**
```c
#include "dcapp.h"

#include <string.h>
#include <stdio.h>

static const double DT = 0.016;  // ~60fps

void display_init() {
    // Initial conditions set in XML, but we can override:
    // *altitude = 10000.0;
    // *velocity = 0.0;
    strcpy(*status, "READY");
}

void display_draw() {
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

void display_close() {
    // Nothing to clean up
}
```

**Build and run:**
```bash
dcapp-genheader myDisplay.xml
gcc -shared -fPIC -o logic/libphysics.so logic/physics.c
dcapp myDisplay.xml
```
