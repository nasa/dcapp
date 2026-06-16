# dcapp

dcapp is a PilotLight-based runtime for XML-defined real-time displays:
instrument panels, simulation controls, cockpit-style interfaces, terrain
views, and small custom display tools.

XML describes the display tree. Optional C logic handles the pieces that are
better as code: procedural drawing, state machines, calculations, IO, and
advanced planet/terrain behavior.

## Start Here

- [Getting Started](documentation/getting-started.md): install/build, run a
  display, run samples, and understand the basic user-side workflow.
- [Documentation Index](documentation/index.md): detailed topic map for XML,
  values, primitives, buttons, mouse events, logic, Trick, Edge, PixelStream,
  planet rendering, coordinate frames, architecture, coding style, migration,
  and samples.

## Quick Run

```bash
git submodule update --init --recursive
./scripts/build.sh

# Opens samples/welcome/welcome.xml
./bin/dcapp.sh

# Or open a specific sample
./bin/dcapp.sh samples/primitives/primitives.xml
```

Windows wrappers use `.bat`:

```bat
scripts\build.bat
bin\dcapp.bat samples\welcome\welcome.xml
```

## Common Tools

| Tool | Purpose |
|------|---------|
| `./bin/dcapp.sh` / `bin\dcapp.bat` | Run a display. With no XML path, opens `samples/welcome/welcome.xml`. |
| `./bin/dcapp-validate.sh` / `bin\dcapp-validate.bat` | Preprocess and validate XML. |
| `./bin/dcapp-genheader.sh` / `bin\dcapp-genheader.bat` | Generate `logic/dcapp.h` for displays with C logic. |
| `./bin/dcapp-planet-chunkgen.sh` / `bin\dcapp-planet-chunkgen.bat` | Convert DEM data into planet terrain chunks. |
| `./bin/dcapp-planet-snapshot.sh` / `bin\dcapp-planet-snapshot.bat` | Render a planet PNG from chunk data without XML. |

## Samples

Start with `samples/welcome/welcome.xml`, then browse focused samples such as
`primitives`, `styles`, `includes`, `conditionals`, `buttons`, `events`,
`slider`, `stencil`, `drawfunction1` through `drawfunction4`, `trick`,
`pixelstream-mjpeg`, and `planet`.

## Credits

- Mike McFarlane, original creator
- Nathan Reagan, maintainer
- Jonathan Hoffstadt, maintainer and creator of PilotLight

## License

[NASA Open Source Agreement v1.3](LICENSE)
