# dcapp

A lightweight XML-based framework for building real-time display applications and cockpit interfaces.

---

## Overview

dcapp allows you to create interactive graphical displays using a declarative XML syntax. Developed for NASA simulation environments, it provides a simple yet powerful way to build instrument panels, control interfaces, and data visualization displays.

**Key Features:**
- Declarative XML syntax for rapid UI development
- Real-time variable binding and updates
- Interactive buttons with multiple states
- Drawing primitives (rectangles, circles, polygons, lines, text, images)
- Conditional rendering based on variable values
- Integration with Trick simulation framework
- Extensible via C/C++ logic files
- Cross-platform (Linux, Windows, macOS)

Rendering powered by [PilotLight](https://github.com/PilotLightTech/pilotlight).

---

## Quick Start

```bash
# Run a display (defaults to samples/welcome/welcome.xml)
bin/dcapp.sh

# Run a specific display
bin/dcapp.sh path/to/display.xml

# Run with constants
bin/dcapp.sh path/to/display.xml DEBUG_MODE=1

# Validate a display file
bin/dcapp-validate path/to/display.xml

# Generate a C header from a display
bin/dcapp-genheader path/to/display.xml output.h
```

On Windows, use the `.bat` equivalents (`bin/dcapp.bat`, `bin/dcapp-validate.bat`, `bin/dcapp-genheader.bat`).

---

## Documentation

| Document | Description |
|----------|-------------|
| [Variables](documentation/variables.md) | Runtime values, text interpolation, and the `<Set>` element |
| [Constants](documentation/constants.md) | Built-in and user-defined constants, color palette |
| [Buttons](documentation/buttons.md) | Interactive controls with visual states |
| [Primitives](documentation/primitives.md) | Drawing elements: rectangles, circles, text, images, etc. |
| [Logic Files](documentation/logic.md) | Extending displays with C/C++ code |

---

## Credits

- **Mike McFarlane** — Original creator
- **Nathan Reagan** — Maintainer
- **Jonathan Hoffstadt** — Maintainer, creator of [PilotLight](https://github.com/PilotLightTech/pilotlight)

---

## License

[NASA Open Source Agreement v1.3](LICENSE)
