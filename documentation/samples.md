# Samples

Run a display with:

```bash
./bin/dcapp.sh samples/primitives/primitives.xml
```

Start with `welcome`, then use the focused samples as working references. The
integrations at the end need additional data or server processes.

## XML

| Sample | Covers |
|---|---|
| `primitives` | Shapes, lines, images, and text rendering |
| `layout` | Nested coordinates, alignment, rotation, and pivots |
| `includes` | Reusable fragments, defaults, styles, and constants |
| `buttons` | Button state machines and pointer-event states |
| `static-if` | Parse-time configuration and constant overrides |
| `stencil` | Add/remove masks and clipped draw passes |

## C logic

Run `./scripts/build.sh` before opening these samples.

| Sample | Covers |
|---|---|
| `lissajous` | Minimal `DrawFunction` and XML/C variable binding |
| `starfield` | XML controls around C-owned procedural state |
| `procedural-panel` | A panel drawn and operated entirely from C |

`api-test` is the developer-facing check for the generated Logic API. It is
kept with the samples because it must build and run through the same path as a
normal display.

## Larger displays and integrations

`planet` also includes C logic, so build it with `./scripts/build.sh` first.

| Sample | Notes |
|---|---|
| `welcome` | Interactive overview of drawing, layout, live state, and pointer input |
| `adi` | Attitude display with sphere rendering and stencil clipping |
| `planet` | Terrain data, shaders, overlays, and two camera frames |
| `pixelstream-mjpeg` | MJPEG input from the included Python server |
| `trick` | Trick Variable Server connection and cannonball simulation |

The planet sample needs prepared terrain data. Trick and PixelStream also need
their accompanying server processes; their documentation includes the setup
commands.
