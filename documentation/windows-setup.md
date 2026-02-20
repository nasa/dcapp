# Windows Setup

## Prerequisites

- **Git for Windows** — [git-scm.com](https://git-scm.com/download/win)
- **Visual Studio 2022** (Community, Professional, or Enterprise) with the **Desktop development with C++** workload
- **Python 3** — [python.org](https://www.python.org/downloads/) — required to regenerate build scripts via `gen_build_apps.py` / `gen_build_samples.py`
- **vcpkg** — Microsoft's C++ package manager

---

## 1. Clone the Repository

The `pilotlight` submodule uses an SSH remote, so make sure you have an SSH key configured with GitHub before cloning.

```bat
git clone git@github.com:nasa/dcapp.git
cd dcapp
```

---

## 2. Update Submodules

```bat
git submodule update --init --recursive
```

This pulls in the `pilotlight` rendering library that dcapp depends on.

---

## 3. Install vcpkg

If you don't already have vcpkg, clone and bootstrap it somewhere on your machine (e.g. `C:\vcpkg`):

```bat
git clone https://github.com/microsoft/vcpkg C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
```

---

## 4. Install Dependencies

From the root of the dcapp repository, run `vcpkg install`. This reads `vcpkg.json` and installs `libxml2`, `curl`, and `gdal` into the local `vcpkg_installed/` directory:

```bat
C:\vcpkg\vcpkg.exe install
```

> **Note:** GDAL pulls in several transitive dependencies (PROJ, SQLite, etc.) so this may take a few minutes on a fresh machine.

---

## 5. Build

From the root of the repository, run:

```bat
build.bat
```

This builds pilotlight (with the `_experimental` suffix), the dcapp apps, and the dcapp samples. The default configuration is `release`. To build a debug build instead:

```bat
build.bat -c debug
```

Output binaries are placed in `bin/`.

---

## Regenerating Build Scripts

The build scripts in `scripts/` are auto-generated. If you modify `gen_build_apps.py` or `gen_build_samples.py` (e.g. to add a new source file or dependency), regenerate them with:

```bat
cd scripts
python gen_build_apps.py
python gen_build_samples.py
```

Commit the resulting `.bat`, `.sh` files alongside any changes to the gen scripts.
