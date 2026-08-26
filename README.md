# dcapp

dcapp is a PilotLight application for XML-defined instrument panels,
simulation controls, terrain views, and other real-time displays. XML defines
the display tree; optional C logic handles procedural drawing, calculations,
state, and I/O.

## Build and run

```bash
git submodule update --init --recursive
./scripts/build.sh

# Run the welcome sample
./bin/dcapp.sh

# Run another display
./bin/dcapp.sh samples/primitives/primitives.xml
```

On Windows:

```bat
scripts\build.bat
bin\dcapp.bat samples\welcome\welcome.xml
```

See [Getting Started](documentation/getting-started.md) for dependencies and
build options. The [documentation index](documentation/index.md) links to the
XML, logic, integration, and project internals references.

## Command-line tools

| Tool | Purpose |
|------|---------|
| `./bin/dcapp.sh` / `bin\dcapp.bat` | Run a display. With no XML path, opens `samples/welcome/welcome.xml`. |
| `./bin/dcapp-validate.sh` / `bin\dcapp-validate.bat` | Preprocess and validate XML. |
| `./bin/dcapp-genheader.sh` / `bin\dcapp-genheader.bat` | Generate `logic/dcapp.h` for displays with C logic. |
| `./bin/dcapp-planet-chunkgen.sh` / `bin\dcapp-planet-chunkgen.bat` | Convert DEM data into planet terrain chunks. |
| `./bin/dcapp-planet-snapshot.sh` / `bin\dcapp-planet-snapshot.bat` | Render a planet PNG from chunk data without XML. |

The [sample index](documentation/samples.md) groups the bundled displays by
topic. `samples/api-test` is the manually run integration test for the
generated logic API.

## Credits

- Mike McFarlane, original creator
- Nathan Reagan, maintainer
- Jonathan Hoffstadt, maintainer and creator of PilotLight

## License

[NASA Open Source Agreement v1.3](LICENSE)
