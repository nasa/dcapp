# Getting Started

A dcapp display is an XML file. Loading one runs the following pipeline:

1. read the file and command-line constant overrides;
2. apply constants and expand includes, defaults, styles, and static
   conditionals;
3. validate the preprocessed XML;
4. build the runtime display tree;
5. load the optional C logic library;
6. open the windows and begin drawing.

Runtime variables use `@NAME`. Parse-time constants use `#Name`.

```xml
<DCAPP>
    <Constant Name="PanelWidth">900</Constant>
    <Variable Type="#_variable_double_" InitialValue="0">COUNTER</Variable>

    <Window Title="Example" Width="#PanelWidth" Height="600">
        <Panel VirtualWidth="#PanelWidth" VirtualHeight="600">
            <Set Variable="COUNTER" Operator="#_set_add_">1</Set>
            <Text X="20" Y="560">counter: @COUNTER(%.0f)</Text>
        </Panel>
    </Window>
</DCAPP>
```

The [documentation index](index.md) links to the full XML and logic references.

## Requirements

All platforms need Git and the Vulkan SDK.

On Debian or Ubuntu, install a compiler and the libxml2, curl, GDAL, and
X11/XCB development packages:

```bash
sudo apt install build-essential libxml2-dev libcurl4-openssl-dev libgdal-dev \
    libx11-dev libx11-xcb-dev libxkbcommon-x11-dev libxcb-cursor-dev \
    libxcb-xfixes0-dev libxcb-keysyms1-dev libxcursor-dev libxrandr-dev \
    libxinerama-dev libgl-dev libxi-dev
```

On macOS, install the Xcode Command Line Tools and the Homebrew dependencies:

```bash
xcode-select --install
brew install libxml2 curl gdal
```

On Windows, install Visual Studio 2022 with the C++ workloads, the Vulkan SDK,
and vcpkg, then install the vcpkg dependencies:

```bat
C:\vcpkg\vcpkg.exe install
```

## Build

```bash
git clone --recursive https://github.com/nasa/dcapp.git
cd dcapp
./scripts/build.sh
```

For an existing clone without submodules:

```bash
git submodule update --init --recursive
```

The default configuration is `release`. `-c debug` selects a debug build and
`-f` reruns every build stage.

```bash
./scripts/build.sh -c debug
./scripts/build.sh -f
```

On Windows:

```bat
scripts\build.bat
```

Build output goes to `pilotlight/out/`. User-facing wrappers are in `bin/`.

## Run a display

With no arguments, dcapp opens the welcome sample:

```bash
./bin/dcapp.sh
```

Pass an XML path to open another display:

```bash
./bin/dcapp.sh samples/primitives/primitives.xml
```

Parse-time constant overrides follow the path:

```bash
./bin/dcapp.sh samples/static-if/static-if.xml DEBUG_MODE=0 PRODUCTION_MODE=1
```

Windows uses the same argument order:

```bat
bin\dcapp.bat
bin\dcapp.bat samples\primitives\primitives.xml
```

## Validate XML

```bash
./bin/dcapp-validate.sh samples/includes/includes.xml
```

Use `--preprocessed` to inspect the XML after constants, styles, includes, and
static conditionals have been resolved:

```bash
./bin/dcapp-validate.sh samples/includes/includes.xml --preprocessed cache/includes.preprocessed.xml
```

## Displays with C logic

These displays need a generated `logic/dcapp.h`:

```bash
./bin/dcapp-genheader.sh samples/starfield/starfield.xml
```

The build scripts generate headers automatically for bundled samples. The
[logic reference](logic.md) covers callbacks and the procedural drawing API.

## Samples

- `samples/primitives` covers the basic drawing elements.
- `samples/layout` covers coordinate spaces, alignment, and pivots.
- `samples/includes` covers reusable XML, defaults, and named styles.
- `samples/buttons` covers button behavior and pointer events; `samples/welcome`
  includes a working slider.
- `samples/starfield` and `samples/procedural-panel` use C drawing callbacks.
- `samples/trick`, `samples/pixelstream-mjpeg`, and `samples/planet` cover the
  larger integrations.

See [Samples](samples.md) for the complete list. `samples/api-test` is a manual
integration test for generated logic headers and callbacks.
