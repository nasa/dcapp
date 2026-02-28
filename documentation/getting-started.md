# Getting Started

This guide covers building and running dcapp on Linux, macOS, and Windows.

---

## 1. Prerequisites

### All Platforms

- **Git** -- [git-scm.com](https://git-scm.com/downloads)

### Linux

- GCC (C compiler)
- [Vulkan SDK](https://vulkan.lunarg.com/sdk/home) -- required by PilotLight for rendering
- X11 and display development libraries (see [Linux Build](#3-linux-build))
- Development packages for libxml2, curl, and GDAL (see [Linux Build](#3-linux-build))

### macOS

- Xcode (`xcode-select --install` for Command Line Tools)
- [Vulkan SDK](https://vulkan.lunarg.com/sdk/home) -- required by PilotLight
- [Homebrew](https://brew.sh/)

### Windows

- [Visual Studio 2022](https://visualstudio.microsoft.com/) (Community, Professional, or Enterprise) with the **Desktop development with C++** and **Game development with C++** workloads
- [Vulkan SDK](https://vulkan.lunarg.com/sdk/home) -- required by PilotLight for rendering
- [vcpkg](https://github.com/microsoft/vcpkg) -- Microsoft's C++ package manager

---

## 2. Clone and Setup

The `pilotlight` submodule uses an SSH remote, so make sure you have an SSH key configured with GitHub before cloning.

```bash
git clone --recursive git@github.com:nasa/dcapp.git
cd dcapp
```

If you already cloned without `--recursive`, pull in the submodules manually:

```bash
git submodule update --init --recursive
```

This pulls in the `pilotlight` rendering library that dcapp depends on.

---

## 3. Linux Build

### Install Dependencies

#### Vulkan SDK

Install the Vulkan SDK from [LunarG](https://vulkan.lunarg.com/sdk/home). On Debian/Ubuntu, you can add the LunarG APT repository:

```bash
wget -qO- https://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo tee /etc/apt/trusted.gpg.d/lunarg.asc
sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-jammy.list https://packages.lunarg.com/vulkan/lunarg-vulkan-jammy.list
sudo apt-get update
sudo apt-get install vulkan-sdk
```

On Fedora, install from the LunarG RPM repository or download the SDK tarball directly.

Make sure `$VULKAN_SDK` is set (the LunarG installer typically does this for you).

#### Libraries

On Debian/Ubuntu:

```bash
sudo apt install build-essential libxml2-dev libcurl4-openssl-dev libgdal-dev \
    libx11-dev libx11-xcb-dev libxkbcommon-x11-dev libxcb-cursor-dev \
    libxcb-xfixes0-dev libxcb-keysyms1-dev libxcursor-dev libxrandr-dev \
    libxinerama-dev libgl-dev libxi-dev
```

On RHEL/Fedora:

```bash
sudo dnf install gcc gcc-c++ libxml2-devel libcurl-devel gdal-devel \
    libX11-devel xcb-util-devel libxkbcommon-x11-devel xcb-util-cursor-devel \
    libxcb-devel xcb-util-keysyms-devel libXcursor-devel libXrandr-devel \
    libXinerama-devel mesa-libGL-devel libXi-devel
```

### Build

From the repository root:

```bash
scripts/build.sh
```

This builds pilotlight, the dcapp apps, and the dcapp samples. The default configuration is `release`. To build a debug build:

```bash
scripts/build.sh -c debug
```

Output binaries are placed in `bin/` (wrapper scripts) and `pilotlight/out/` (compiled binaries).

---

## 4. macOS Build

### Install Dependencies

Install Xcode Command Line Tools if you haven't already:

```bash
xcode-select --install
```

Install the Vulkan SDK from [LunarG](https://vulkan.lunarg.com/sdk/home). Download the macOS DMG installer, run it, and follow the prompts. Make sure `$VULKAN_SDK` is set afterward.

Install the required libraries via Homebrew:

```bash
brew install libxml2 curl gdal
```

### Build

From the repository root:

```bash
scripts/build.sh
```

The build script auto-detects macOS and uses the correct platform backend. The default configuration is `release`. To build a debug build:

```bash
scripts/build.sh -c debug
```

Output binaries are placed in `bin/` (wrapper scripts) and `pilotlight/out/` (compiled binaries).

---

## 5. Windows Build

### Install Vulkan SDK

Download and install the Vulkan SDK from [LunarG](https://vulkan.lunarg.com/sdk/home). The installer sets the `%VULKAN_SDK%` environment variable automatically. You may need to restart your terminal after installation.

### Install vcpkg

If you don't already have vcpkg, clone and bootstrap it somewhere on your machine (e.g. `C:\vcpkg`):

```bat
git clone https://github.com/microsoft/vcpkg C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
```

### Install Dependencies

From the root of the dcapp repository, run `vcpkg install`. This reads `vcpkg.json` and installs `libxml2`, `curl`, and `gdal` into the local `vcpkg_installed/` directory:

```bat
C:\vcpkg\vcpkg.exe install
```

> **Note:** GDAL pulls in several transitive dependencies (PROJ, SQLite, etc.) so this may take a few minutes on a fresh machine.

### Build

From the repository root:

```bat
scripts\build.bat
```

This builds pilotlight, the dcapp apps, and the dcapp samples. The default configuration is `release`. To build a debug build:

```bat
scripts\build.bat -c debug
```

Output binaries are placed in `bin\` (wrapper scripts) and `pilotlight\out\` (compiled binaries).

---

## 6. Running Your First Display

dcapp ships with a built-in welcome screen. After building, run it with no arguments:

**Linux / macOS:**

```bash
bin/dcapp.sh
```

**Windows:**

```bat
bin\dcapp.bat
```

With no arguments, this launches `samples/welcome/welcome.xml` by default.

To run a specific display file, pass the path as the first argument:

**Linux / macOS:**

```bash
bin/dcapp.sh samples/welcome/welcome.xml
```

**Windows:**

```bat
bin\dcapp.bat samples\welcome\welcome.xml
```

Browse the `samples/` directory for additional examples covering primitives, animation, interactivity, stencils, planet rendering, and more.

---

## 7. Command-Line Tools

The build produces several command-line tools in the `bin/` directory:

| Tool | Description |
|------|-------------|
| `dcapp` | Main runtime. Loads and renders a dcapp XML display file. |
| `dcapp-validate` | Validates a dcapp XML file for correctness without running it. |
| `dcapp-genheader` | Generates a C header from a dcapp XML file, used when building samples that include custom logic. |
| `dcapp-planet-chunkgen` | Generates chunked terrain data for the planet rendering primitive. |

Each tool has `.sh` (Linux/macOS) and `.bat` (Windows) wrapper scripts in `bin/`.

---

## 8. IDE Setup

### VSCode

A setup script generates all the necessary VSCode configuration files (settings, extensions, language server config, and launch targets):

```bash
python3 scripts/setup-vscode.py
```

The script will:

1. Prompt you to choose between **clangd** and **Microsoft C/C++** as your language server.
2. Generate `.vscode/settings.json`, `.vscode/extensions.json`, and `.vscode/launch.json`.
3. Generate either a `.clangd` file (if you chose clangd) or `.vscode/c_cpp_properties.json` (if you chose Microsoft C/C++), with the correct include paths for your platform.
4. Create debug launch configurations for every sample in the `samples/` directory.

---

## 9. Building Samples with Logic Files

Some samples include custom C logic (located in a `logic/` subdirectory within the sample folder). The build scripts for these samples are auto-generated.

If you add a new sample with a logic file, or modify the build generation scripts, regenerate the build scripts first:

```bash
cd scripts/internal
python3 gen-build-samples.py
python3 gen-build-apps.py
```

Then rebuild from the repository root:

**Linux / macOS:**

```bash
scripts/build.sh
```

**Windows:**

```bat
scripts\build.bat
```

Commit the resulting `.bat` and `.sh` files in `scripts/internal/` alongside any changes to the generation scripts.

---

## 10. Troubleshooting

### Submodule errors during build

If the build fails looking for pilotlight files, the submodules are likely not initialized:

```bash
git submodule update --init --recursive
```

### Vulkan SDK not found

If the build fails looking for Vulkan headers or libraries, make sure the Vulkan SDK is installed and the `VULKAN_SDK` environment variable is set. On Linux, source the SDK setup script:

```bash
source ~/VulkanSDK/<version>/setup-env.sh
```

On Windows, reinstall the SDK or manually set `%VULKAN_SDK%` to the install directory.

### Missing libraries on Linux

If the build reports missing headers such as `libxml/parser.h` or `curl/curl.h`, install the development packages. See [Linux Build](#3-linux-build) for the full package list including X11 dependencies.

### Homebrew library paths not found on macOS

If the build cannot find Homebrew-installed libraries, make sure Homebrew's prefix is on your path. On Apple Silicon Macs:

```bash
eval "$(/opt/homebrew/bin/brew shellenv)"
```

On Intel Macs:

```bash
eval "$(/usr/local/bin/brew shellenv)"
```

### vcpkg dependencies not found on Windows

Make sure you ran `vcpkg install` from the dcapp repository root so that it picks up the `vcpkg.json` manifest. The installed packages should appear in `vcpkg_installed/x64-windows/`.

### Build configuration issues

Both `scripts/build.sh` and `scripts\build.bat` accept `-c debug` or `-c release`. If you see linker errors related to debug/release mismatches, clean and rebuild with a consistent configuration:

```bash
scripts/build.sh -c release
```

### Display window doesn't appear

Make sure you are running from the repository root so that relative asset paths resolve correctly. The wrapper scripts in `bin/` handle working directory setup automatically.
