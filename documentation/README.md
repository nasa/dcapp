# dcapp Documentation

## Getting Started

1. [Getting Started](getting-started.md) -- Build instructions (Linux, macOS, Windows), first display, command-line tools

## Core Concepts

2. [Variables](variables.md) -- Runtime values, text interpolation, `<Set>` operators, variable types
3. [Constants](constants.md) -- Built-in and user-defined constants, color palette
4. [Primitives](primitives.md) -- All XML elements: Rectangle, Ellipse, Sphere, Text, Image, Container, If, and more

## Interactivity

5. [Buttons](buttons.md) -- Interactive controls with visual states, value/variable inheritance
6. [Mouse Events](mouse-events.md) -- MousePressed, MouseReleased, MouseMotion, slider patterns
7. [Blink](blink.md) -- Flashing elements for alerts and warnings

## Advanced

8. [Logic Files](logic.md) -- Extending displays with C/C++ code
9. [Stencil](stencil.md) -- Clipping masks with StencilAdd/StencilRemove/StencilDraw
10. [Planet](planet.md) -- Chunked terrain rendering, camera modes, custom shaders
11. [Integration](integration.md) -- TrickIO, EdgeIO, PixelStream

## Reference

12. [Samples](samples.md) -- Feature-to-sample index, grouped by difficulty
13. [Migration Guide](migration.md) -- Converting legacy dcapp XML/C++ to current syntax
14. [Breaking Changes](breaking-changes.md) -- Searchable notes for source and ABI changes
15. [DrawFunction Roadmap Notes](drawfunction-roadmap.md) -- Planned draw API coverage and context model

## Quick Element Reference

| Element | Description | Details |
|---------|-------------|---------|
| `<DCAPP>` | Root element | [primitives](primitives.md#dcapp) |
| `<Window>` | Application window | [primitives](primitives.md#window) |
| `<Variable>` | Runtime variable | [variables](variables.md) |
| `<Constant>` | Compile-time constant | [constants](constants.md) |
| `<Container>` | Grouping with coordinate space | [primitives](primitives.md#container) |
| `<Panel>` | Simple virtual-dimension wrapper | [primitives](primitives.md#panel) |
| `<Include>` | Include another XML file | [primitives](primitives.md#include) |
| `<Rectangle>` | Filled/outlined rectangle | [primitives](primitives.md#rectangle) |
| `<Ellipse>` | Filled ellipse or pie wedge | [primitives](primitives.md#ellipse) |
| `<Arc>` | Line-only arc | [primitives](primitives.md#arc) |
| `<Sphere>` | 3D sphere with texture/rotation | [primitives](primitives.md#sphere) |
| `<Line>` | Polyline through vertices | [primitives](primitives.md#line) |
| `<Polygon>` | Filled/outlined polygon | [primitives](primitives.md#polygon) |
| `<Text>` | Text with variable interpolation | [primitives](primitives.md#text) |
| `<Image>` | Image file display | [primitives](primitives.md#image) |
| `<Button>` | Interactive button | [buttons](buttons.md) |
| `<Blink>` | Flashing wrapper | [blink](blink.md) |
| `<Stencil>` | Clipping mask | [stencil](stencil.md) |
| `<If>` | Conditional rendering | [primitives](primitives.md#if) |
| `<Set>` | Variable assignment | [primitives](primitives.md#set) |
| `<Logic>` | C shared library loader | [logic](logic.md) |
| `<Function>` | Call function from logic lib | [primitives](primitives.md#function) |
| `<TrickIO>` | Trick simulation connection | [integration](integration.md#trickio) |
| `<EdgeIO>` | Edge simulation connection | [integration](integration.md#edgeio) |
| `<PixelStream>` | Video stream display | [integration](integration.md#pixelstream) |
| `<Planet>` | Planet terrain definition | [planet](planet.md) |
| `<PlanetView>` | Planet terrain viewport | [planet](planet.md#planetview) |

## Command-Line Tools

| Tool | Description |
|------|-------------|
| `dcapp` | Run a display |
| `dcapp-validate` | Validate XML without rendering |
| `dcapp-genheader` | Generate C header from display |
| `dcapp-planet-chunkgen` | Convert DEM to chunked terrain |
