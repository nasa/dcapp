# Windows Setup

## Prerequisites

- **Git for Windows** — [git-scm.com](https://git-scm.com/download/win)
- **Visual Studio 2022** (Community, Professional, or Enterprise) with the **Desktop development with C++** workload
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

From the root of the dcapp repository, run `vcpkg install`. This reads `vcpkg.json` and installs `libxml2` and `curl` into the local `vcpkg_installed/` directory:

```bat
C:\vcpkg\vcpkg.exe install
```

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
