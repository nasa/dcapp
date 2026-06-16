# dcapp Samples

The public samples are intentionally small enough to read. They are grouped by
what they teach, not by how flashy they are.

Run a sample with:

```bash
./bin/dcapp.sh samples/<sample_name>/<sample_name>.xml
```

For example:

```bash
./bin/dcapp.sh samples/primitives/primitives.xml
```

## Build Requirements

Most samples are pure XML. The following samples include C logic and must be
built first:

```bash
./scripts/build.sh -c release
```

Logic samples: **drawfunction1**, **drawfunction2**, **drawfunction3**,
**drawfunction4**, **lissajous**, **planet**, and **ptz**.

External-service samples:

- **trick** requires a running Trick simulation from `samples/trick/sim`.
- **pixelstream-mjpeg** requires the included Python MJPEG server.

## Core XML

These samples demonstrate XML features directly.

| Sample | Focus |
|--------|-------|
| primitives | Basic drawing primitives, including rectangles, circles, ellipses, arcs, lines, polygons, images, and containers |
| alignment | Parent/local alignment behavior |
| containers | Nested coordinate systems and virtual dimensions |
| rotation | Rotation and pivot points |
| styles | Style, Default, and Constant reuse |
| includes | Splitting displays into reusable XML files |
| conditionals | Runtime If/True/False logic |
| static-if | Parse-time conditionals |
| blink | Blink timing and triggered flashing |
| buttons | Button types, states, indicators, and child content |
| input | Form-like controls and simple state binding |
| slider | MouseMotion-based drag controls |
| events | MousePressed, MouseReleased, MouseActive, and MouseHovered |
| colors | RGB control using simple interactive state |
| fonts | Text rendering and custom fonts |
| stencil | Stencil clipping, text masks, image masks, and add/remove masks |

## C Logic

These samples demonstrate where C logic is a better fit than static XML.

| Sample | Focus |
|--------|-------|
| drawfunction1 | Small hybrid XML/C DrawFunction examples |
| drawfunction2 | DrawFunction C API reference grid |
| drawfunction3 | XML controls changing a C-generated starfield |
| drawfunction4 | One DrawFunction owning an entire procedural panel |
| lissajous | C-generated polyline points drawn with a DrawFunction |
| ptz | XML controls with C-computed pan/tilt/zoom image placement |

## Advanced And Integration

These samples cover larger systems or external inputs.

| Sample | Focus |
|--------|-------|
| adi | Attitude Direction Indicator using sphere rendering and stencil clipping |
| planet | Moon terrain rendering with shaders, texture overlays, and dual camera paths |
| pixelstream-mjpeg | Live MJPEG PixelStream input from the included Python server |
| trick | Trick Variable Server integration with a cannonball trajectory display |

## Showcase

| Sample | Focus |
|--------|-------|
| welcome | A larger composition that combines animation, variables, stencil, buttons, and primitives |

## Validation-Only

| Sample | Focus |
|--------|-------|
| bad-sample | Intentionally invalid XML used to check validator failures |
