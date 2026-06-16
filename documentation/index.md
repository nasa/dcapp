# dcapp Documentation Index

Use [Getting Started](getting-started.md) first if you are installing dcapp,
running samples, or trying to understand the display workflow from the user
side. This page is the detailed map.

## XML And Display Authoring

| Topic | File |
|-------|------|
| XML elements and layout primitives | [primitives.md](primitives.md) |
| Runtime variables, text interpolation, and `Set` operators | [variables.md](variables.md) |
| Parse-time constants and built-in constant groups | [constants.md](constants.md) |
| Coordinate system, panel space, and planet/cartesian frames | [coordinate-frame.md](coordinate-frame.md) |
| Button behavior and visual states | [buttons.md](buttons.md) |
| Mouse event children and slider patterns | [mouse-events.md](mouse-events.md) |
| Blink wrapper | [blink.md](blink.md) |
| Stencil clipping | [stencil.md](stencil.md) |
| Runnable sample map | [samples.md](samples.md) |

## Logic And Procedural Drawing

| Topic | File |
|-------|------|
| C/C++ logic libraries, generated `logic/dcapp.h`, lifecycle callbacks | [logic.md](logic.md) |
| `Function` callbacks from XML | [logic.md#function-element](logic.md#function-element) |
| `DrawFunction` procedural drawing API | [logic.md#drawfunction-api](logic.md#drawfunction-api) |

## External Data And Streams

| Topic | File |
|-------|------|
| Trick Variable Server mapping | [trick.md](trick.md) |
| Edge command mapping | [edge.md](edge.md) |
| MJPEG and shared-memory pixel streams | [pixelstream.md](pixelstream.md) |
| Integration overview | [integration.md](integration.md) |

## Planet Rendering

| Topic | File |
|-------|------|
| Planet terrain XML, chunk generation, snapshots, overlays | [planet.md](planet.md) |
| Planet coordinate frames | [coordinate-frame.md](coordinate-frame.md) |

## Project Internals

| Topic | File |
|-------|------|
| Runtime architecture and PilotLight integration | [architecture.md](architecture.md) |
| Coding style and change checklists | [coding-style.md](coding-style.md) |

## Migration And History

| Topic | File |
|-------|------|
| Legacy XML and logic migration | [migration.md](migration.md) |
| Searchable breaking-change notes | [breaking-changes.md](breaking-changes.md) |
| Full project history | [../CHANGELOG.md](../CHANGELOG.md) |

## Current XML Surface

The parser recognizes these XML element names:

`Arc`, `Arg`, `Blink`, `Button`, `ButtonDisabled`, `ButtonEnabled`,
`ButtonIndicatorOff`, `ButtonIndicatorOn`, `ButtonPressed`, `ButtonReleased`,
`ButtonTransition`, `Constant`, `Container`, `DCAPP`, `Default`, `Dummy`,
`DrawFunction`, `EdgeFrom`, `EdgeIO`, `EdgeTo`, `EdgeVariable`, `Ellipse`,
`False`, `Function`, `If`, `Image`, `Include`, `Line`, `Logic`, `MouseActive`,
`MouseHovered`, `MouseInactive`, `MouseMotion`, `MousePressed`,
`MouseReleased`, `Panel`, `PixelStream`, `Planet`, `PlanetBreadcrumbs`,
`PlanetData`, `PlanetEllipse`, `PlanetGeoJSON`, `PlanetLine`, `PlanetPolygon`,
`PlanetShader`, `PlanetSphere`, `PlanetText`, `PlanetTexture`, `PlanetView`,
`Polygon`, `Rectangle`, `Set`, `Sphere`, `Stencil`, `StencilAdd`,
`StencilDraw`, `StencilRemove`, `Style`, `Text`, `TrickFrom`, `TrickIO`,
`TrickTo`, `TrickVariable`, `True`, `Variable`, `Vertex`, and `Window`.

Preprocessing removes or expands several of those before validation/rendering:
`Constant`, `Default`, `Style`, `Include`, and `Dummy` do not remain as runtime
draw nodes. Static `If` branches are also resolved at load time.
