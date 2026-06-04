# dcapp Samples Index

A catalog of all included sample displays, organized by category and complexity.

---

## Running Samples

All samples are located in the `samples/` directory. To run a sample, use:

```bash
dcapp samples/<sample_name>/<sample_name>.xml
```

For example:

```bash
dcapp samples/colors/colors.xml
```

### Samples That Require Building First

Several samples include C logic files that must be compiled into shared libraries before running. These samples contain a `logic/` directory with source code. Build the logic libraries first, then run the display:

```bash
./scripts/build.sh -c release
dcapp samples/<sample_name>/<sample_name>.xml
```

The following samples require building logic files: **drawfunction1**, **drawfunction2**, **drawfunction3**, **functions**, **lissajous**, **mask**, **planet**, **ptz**, **screensaver**.

### Samples That Require External Services

Some samples connect to external systems that must be running before launching dcapp:

- **trick** and **trick-stress** -- Require a running Trick simulation. Build and start the sim from the `sim/` directory inside each sample before launching dcapp.
- **pixelstream-mjpeg** -- Requires a running MJPEG server. Start the included Python server (`python3 server.py`) before launching dcapp.

---

## Beginner (Pure XML)

These samples demonstrate dcapp features using only XML, with no external dependencies or compiled code.

| Sample | Features | Notes |
|--------|----------|-------|
| alignment | ParentAlign, LocalAlign positioning | Demonstrates how to position elements relative to parent containers and themselves |
| blink | Blink element, frequency/duty cycle | Shows blinking effects with configurable timing |
| button-children | Button child layers, MouseHovered | Demonstrates buttons with layered children and hover state detection |
| buttons | All button types and states | Comprehensive showcase of button configurations including Enabled, Disabled, Transition, and Indicator states |
| colors | RGB sliders, color mixing | Interactive color picker using sliders to mix red, green, and blue channels |
| conditionals | If/True/False, comparison operators | Demonstrates conditional rendering with all comparison operators |
| containers | Container nesting, VirtualWidth/Height | Shows how containers establish local coordinate systems with virtual dimensions |
| dashes | LinePattern on outlines | Demonstrates dashed line patterns on lines, rectangles, ellipses, polygons, and arcs |
| environment | Environment variable display ($USER, etc.) | Reads and displays system environment variables |
| events | MousePressed/Released/Active/Hovered on shapes | Demonstrates all mouse event types on geometric shapes |
| fonts | Custom TTF fonts per Text element | Shows how to use the Font attribute to load different TTF fonts |
| includes | Include element, reusable components | Shows how to split displays across multiple XML files for reuse |
| input | Text input, radio buttons, sliders | Interactive form elements for user data entry |
| primitives | Rectangle, Circle, Ellipse, Arc, Line, Polygon | Reference for all basic drawing primitives and their attributes |
| pushpop | Set push/pop/negate operators | Demonstrates push, pop, and negate operators for variable stack manipulation |
| rotation | Rotation, pivot points | Shows element rotation with configurable pivot points |
| rounded | Rounded rectangles and polygons | Demonstrates the Rounded attribute on Rectangle and Polygon elements for smooth corners |
| shapes | Arc/Ellipse variations, pie wedges | Extended shape demonstrations including arc segments and pie wedge rendering |
| slider | MouseMotion drag, horizontal/vertical sliders | Custom slider controls built with MouseMotion for drag-based interaction |
| static-if | Parse-time conditionals (Static="true") | Demonstrates compile-time conditionals that are resolved during XML parsing |
| stencil | Stencil masking, donut, text mask, nested stencils | Shows how to use stencil masks for clipping, including donut shapes, text-based masks, and nested stencil operations |
| styles | Style and Default elements | Demonstrates reusable style definitions and default attribute values |
| welcome | Complex animation with orbits and variables | A visually rich welcome screen with orbital animation driven by variables |

---

## Intermediate (With C Logic)

These samples include C logic files that must be compiled before running. They demonstrate how to extend dcapp with custom computation.

| Sample | Features | Notes |
|--------|----------|-------|
| drawfunction1 | DrawFunction overview | Shows XML layout with small C draw functions injected for procedural details; requires building `logic/logic.c` |
| drawfunction2 | DrawFunction C API reference | Generates a numbered C reference grid covering primitive drawing, placement, containers, stencils, text, mouse targets, and args; requires building `logic/logic.c` |
| drawfunction3 | Procedural DrawFunction rendering | Generates a square field, signal analyzer, and mouse-click ripples from C loops/math; requires building `logic/logic.c` |
| functions | Function element, C callbacks | Shows the Function element for invoking named C callbacks from XML; requires building `logic/logic.c` |
| lissajous | Parametric curves via C logic | Renders Lissajous curves computed in C and displayed via dcapp variables; requires building `logic/logic.c` |
| mask | Animated stencil mask via C logic | Combines stencil masking with C-driven animation for dynamic mask effects; requires building `logic/logic.c` |
| ptz | Pan/tilt/zoom image control | Implements pan, tilt, and zoom controls over an image using C logic for coordinate math; requires building `logic/logic.c` |
| screensaver | Bouncing animation with time display | A screensaver-style bouncing animation with a live time display, driven by C logic; requires building `logic/logic.c` |

---

## Advanced

These samples demonstrate complex rendering techniques, 3D graphics, or large-scale geometry.

| Sample | Features | Notes |
|--------|----------|-------|
| adi | Attitude Direction Indicator (Sphere + Stencil) | A flight instrument combining a 3D sphere primitive with stencil masking to create a realistic ADI |
| metrics | Large geometry performance test | Stress test rendering large numbers of geometric primitives; includes a Makefile for optional build steps |
| mona-lisa | 10K triangle rendering | Renders an approximation of the Mona Lisa using roughly 10,000 colored triangles |
| planet | 3D moon terrain, custom shaders, dual cameras | A 3D moon surface with terrain rendering, custom shader programs, and dual camera viewpoints; requires building `logic/logic.c` |

---

## Integration

These samples demonstrate connecting dcapp to external systems.

| Sample | Features | Notes |
|--------|----------|-------|
| trick | Trick Variable Server connection (cannonball trajectory) | Connects to a Trick simulation of a cannonball trajectory; shows dual DataRate connections with unit conversion. Requires a running Trick sim -- build and start from `sim/` directory first |
| trick-stress | Trick stress test with trajectory visualization | Stress test with many Trick variables mapped simultaneously; requires a running Trick sim from `sim/` directory |
| pixelstream-mjpeg | MJPEG video streaming | Embeds a live MJPEG video stream from an HTTP endpoint; requires starting the included Python server (`python3 server.py`) first |

---

## Utility / Testing

These samples serve special purposes such as validation testing or complex visual compositions.

| Sample | Features | Notes |
|--------|----------|-------|
| bad-sample | Intentional validation errors for testing | Contains deliberate XML errors used to test the dcapp validator; not intended to run as a display |
| scene | Cherry blossom animation (complex composition) | A visually complex scene combining many dcapp features into a single animated composition |

---

## Quick Reference: Build Requirements

| Requirement | Samples |
|-------------|---------|
| None (pure XML) | alignment, blink, button-children, buttons, colors, conditionals, containers, dashes, environment, events, fonts, includes, input, primitives, pushpop, rotation, rounded, shapes, slider, static-if, stencil, styles, welcome, adi, mona-lisa, bad-sample, scene |
| Build C logic (`./scripts/build.sh -c release`) | drawfunction1, drawfunction2, drawfunction3, functions, lissajous, mask, ptz, screensaver, planet |
| Build + Makefile (`make` in sample dir) | metrics |
| Running Trick simulation | trick, trick-stress |
| Running Python MJPEG server | pixelstream-mjpeg |
