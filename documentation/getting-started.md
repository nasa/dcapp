# Getting Started

This page is the short user-side path: install what dcapp needs, build it, run
samples, and understand what happens when a display starts.

For detailed XML and API references, use [index.md](index.md).

## How dcapp Works

A dcapp display is an XML file. At startup dcapp:

1. loads the XML file,
2. applies parse-time constants and command-line overrides,
3. expands `<Include>` files,
4. applies `<Default>` and `<Style>` templates,
5. resolves static `<If>` branches,
6. validates the resulting XML,
7. builds the runtime display tree,
8. loads optional C logic from `<Logic File="..."/>`,
9. opens one or more windows and draws each frame.

Runtime variables are declared with `<Variable>` and referenced with `@NAME`.
Parse-time constants are declared with `<Constant>` and referenced with `#Name`.

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

## Requirements

All platforms need Git and the Vulkan SDK.

Linux also needs a compiler plus libxml2, curl, GDAL, and X11/XCB development
packages. On Debian/Ubuntu:

```bash
sudo apt install build-essential libxml2-dev libcurl4-openssl-dev libgdal-dev \
    libx11-dev libx11-xcb-dev libxkbcommon-x11-dev libxcb-cursor-dev \
    libxcb-xfixes0-dev libxcb-keysyms1-dev libxcursor-dev libxrandr-dev \
    libxinerama-dev libgl-dev libxi-dev
```

macOS needs Xcode Command Line Tools and Homebrew packages:

```bash
xcode-select --install
brew install libxml2 curl gdal
```

Windows needs Visual Studio 2022 with C++ workloads, Vulkan SDK, and vcpkg:

```bat
C:\vcpkg\vcpkg.exe install
```

## Clone And Build

```bash
git clone --recursive https://github.com/nasa/dcapp.git
cd dcapp
./scripts/build.sh
```

If the repo was cloned without submodules:

```bash
git submodule update --init --recursive
```

Windows:

```bat
scripts\build.bat
```

The default configuration is `release`. Use `-c debug` for debug builds and
`-f` to force all build stages:

```bash
./scripts/build.sh -c debug
./scripts/build.sh -f
```

Build outputs live in `pilotlight/out/`; wrapper scripts live in `bin/`.

## Run Displays

Run the welcome display:

```bash
./bin/dcapp.sh
```

Run a specific display:

```bash
./bin/dcapp.sh samples/primitives/primitives.xml
```

Pass parse-time constants after the XML path:

```bash
./bin/dcapp.sh samples/static-if/static-if.xml DEBUG_MODE=0 PRODUCTION_MODE=1
```

Windows equivalents:

```bat
bin\dcapp.bat
bin\dcapp.bat samples\primitives\primitives.xml
```

## Validate XML

Use validation before chasing runtime behavior:

```bash
./bin/dcapp-validate.sh samples/includes/includes.xml
```

Dump the preprocessed XML when debugging constants, styles, includes, or static
conditionals:

```bash
./bin/dcapp-validate.sh samples/includes/includes.xml --preprocessed cache/includes.preprocessed.xml
```

## Generate Logic Headers

Displays with C logic need a generated `logic/dcapp.h`:

```bash
./bin/dcapp-genheader.sh samples/drawfunction1/drawfunction1.xml
```

The normal build scripts do this automatically for bundled samples.

## Sample Tour

Good first samples:

| Sample | Shows |
|--------|-------|
| `welcome` | Feature overview |
| `primitives` | Basic shapes, text, images, arcs, sphere |
| `styles` | `Default`, `Style`, constants |
| `includes` | Reusable XML fragments |
| `conditionals` / `static-if` | Runtime and parse-time `If` |
| `buttons`, `events`, `slider` | Interaction |
| `stencil`, `blink` | Display effects |
| `drawfunction1` through `drawfunction4` | C procedural drawing |
| `trick` | Trick Variable Server integration |
| `pixelstream-mjpeg` | MJPEG video stream |
| `planet` | Planet terrain rendering |

More detail lives in [samples.md](samples.md).
