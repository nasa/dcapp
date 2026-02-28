#!/usr/bin/env python3

import json
import os
import platform
import sys

# ------------------------------------------------------------------------------
# paths
# ------------------------------------------------------------------------------

script_dir = os.path.dirname(os.path.abspath(__file__))
project_root = os.path.abspath(os.path.join(script_dir, ".."))
vscode_dir = os.path.join(project_root, ".vscode")

# include paths relative to project root (shared across all platforms)
common_includes = [
    "src",
    "extensions",
    "shaders",
    "pilotlight/src",
    "pilotlight/libs",
    "pilotlight/extensions",
    "pilotlight/shaders",
    "pilotlight/dependencies/stb",
]

# platform-specific include paths
platform_includes = {
    "Linux":   ["/usr/include/libxml2", "/usr/include/gdal"],
    "Darwin":  ["/opt/homebrew/opt/libxml2/include/libxml2", "/opt/homebrew/opt/gdal/include"],
    "Windows": [],  # vcpkg handles this
}

# vcpkg include path (Windows only)
vcpkg_include = "vcpkg_installed/x64-windows/include"

# ------------------------------------------------------------------------------
# helpers
# ------------------------------------------------------------------------------

def write_json(path, data):
    with open(path, "w") as f:
        json.dump(data, f, indent=4)
        f.write("\n")

def prompt_choice(prompt, options):
    """Prompt user to pick from a list. Returns the chosen option string."""
    for i, opt in enumerate(options):
        print(f"  [{i + 1}] {opt}")
    while True:
        try:
            choice = int(input(prompt + " "))
            if 1 <= choice <= len(options):
                return options[choice - 1]
        except (ValueError, EOFError):
            pass
        print(f"  Please enter a number between 1 and {len(options)}")

# ------------------------------------------------------------------------------
# detect platform
# ------------------------------------------------------------------------------

plat = platform.system()
if plat not in ("Linux", "Darwin", "Windows"):
    print(f"Unsupported platform: {plat}")
    sys.exit(1)

print(f"Platform: {plat}")
print()

# ------------------------------------------------------------------------------
# choose language server
# ------------------------------------------------------------------------------

ls_choice = prompt_choice(
    "Which C/C++ language server?",
    ["clangd", "Microsoft C/C++"]
)
print()

# ------------------------------------------------------------------------------
# ensure .vscode directory exists
# ------------------------------------------------------------------------------

os.makedirs(vscode_dir, exist_ok=True)

# ------------------------------------------------------------------------------
# generate .vscode/settings.json
# ------------------------------------------------------------------------------

settings = {
    "files.associations": {
        "*.inc": "c",
        "*.m": "objective-c",
        "*.mm": "objective-cpp",
    }
}

if ls_choice == "clangd":
    settings["C_Cpp.intelliSenseEngine"] = "disabled"

write_json(os.path.join(vscode_dir, "settings.json"), settings)
print("Wrote .vscode/settings.json")

# ------------------------------------------------------------------------------
# generate .vscode/extensions.json
# ------------------------------------------------------------------------------

recommendations = []
if ls_choice == "clangd":
    recommendations.append("llvm-vs-code-extensions.vscode-clangd")
else:
    recommendations.append("ms-vscode.cpptools")

extensions = {"recommendations": recommendations}
write_json(os.path.join(vscode_dir, "extensions.json"), extensions)
print("Wrote .vscode/extensions.json")

# ------------------------------------------------------------------------------
# generate language server config
# ------------------------------------------------------------------------------

if ls_choice == "clangd":

    # generate .clangd at project root
    lines = []
    lines.append("CompileFlags:")
    lines.append("    Add:")

    # relative include paths
    for inc in common_includes:
        lines.append(f"        - -I{inc}")

    # platform-specific absolute paths
    for inc in platform_includes.get(plat, []):
        lines.append(f"        - -I{inc}")

    # vcpkg on Windows
    if plat == "Windows":
        lines.append(f"        - -I{vcpkg_include}")

    lines.append("InlayHints:")
    lines.append("    Enabled: No")
    lines.append("")

    clangd_path = os.path.join(project_root, ".clangd")
    with open(clangd_path, "w") as f:
        f.write("\n".join(lines))
    print("Wrote .clangd")

    # remove c_cpp_properties.json if it exists (not needed for clangd)
    cpp_props = os.path.join(vscode_dir, "c_cpp_properties.json")
    if os.path.exists(cpp_props):
        os.remove(cpp_props)
        print("Removed .vscode/c_cpp_properties.json (not needed for clangd)")

else:

    # generate .vscode/c_cpp_properties.json
    include_paths = [f"${{workspaceFolder}}/{inc}" for inc in common_includes]

    if plat == "Windows":
        include_paths.append(f"${{workspaceFolder}}/{vcpkg_include}")
        include_paths.append(f"${{workspaceFolder}}/{vcpkg_include}/libxml2")

    for inc in platform_includes.get(plat, []):
        include_paths.append(inc)

    config = {"name": "", "includePath": include_paths, "cStandard": "c17"}

    if plat == "Windows":
        config["name"] = "Win32"
        config["defines"] = ["_WIN32"]
        config["intelliSenseMode"] = "windows-msvc-x64"
    elif plat == "Darwin":
        config["name"] = "Mac"
        config["intelliSenseMode"] = "macos-clang-arm64"
    elif plat == "Linux":
        config["name"] = "Linux"
        config["intelliSenseMode"] = "linux-gcc-x64"

    cpp_properties = {"version": 4, "configurations": [config]}
    write_json(os.path.join(vscode_dir, "c_cpp_properties.json"), cpp_properties)
    print("Wrote .vscode/c_cpp_properties.json")

    # remove .clangd if it exists (not needed for Microsoft C/C++)
    clangd_path = os.path.join(project_root, ".clangd")
    if os.path.exists(clangd_path):
        os.remove(clangd_path)
        print("Removed .clangd (not needed for Microsoft C/C++)")

# ------------------------------------------------------------------------------
# generate .vscode/launch.json
# ------------------------------------------------------------------------------

# discover samples (directories containing <name>/<name>.xml)
samples_dir = os.path.join(project_root, "samples")
sample_names = []
if os.path.isdir(samples_dir):
    for entry in sorted(os.listdir(samples_dir)):
        if entry.startswith("."):
            continue
        sample_xml = os.path.join(samples_dir, entry, entry + ".xml")
        if os.path.isfile(sample_xml):
            sample_names.append(entry)

# build platform-specific debug config
configs = []

if plat == "Linux":
    configs.append({
        "name": "dcapp (linux)",
        "type": "cppdbg",
        "request": "launch",
        "program": "${workspaceFolder}/pilotlight/out/pilot_light",
        "args": ["-a", "dcapp", "${workspaceFolder}/samples/${input:sampleName}/${input:sampleName}.xml"],
        "cwd": "${workspaceFolder}/pilotlight/out",
        "MIMode": "gdb",
        "miDebuggerPath": "/usr/bin/gdb",
    })
elif plat == "Darwin":
    configs.append({
        "name": "dcapp (macos)",
        "type": "lldb",
        "request": "launch",
        "program": "${workspaceFolder}/pilotlight/out/pilot_light",
        "args": ["-a", "dcapp", "${workspaceFolder}/samples/${input:sampleName}/${input:sampleName}.xml"],
        "cwd": "${workspaceFolder}/pilotlight/out",
    })
elif plat == "Windows":
    configs.append({
        "name": "dcapp (windows)",
        "type": "cppvsdbg",
        "request": "launch",
        "program": "${workspaceFolder}\\pilotlight\\out\\pilot_light.exe",
        "args": ["-a", "dcapp", "${workspaceFolder}/samples/${input:sampleName}/${input:sampleName}.xml"],
        "cwd": "${workspaceFolder}\\pilotlight\\out",
    })

launch = {
    "version": "0.2.0",
    "configurations": configs,
    "inputs": [
        {
            "id": "sampleName",
            "type": "pickString",
            "description": "Select a sample to run",
            "options": sample_names,
            "default": sample_names[0] if sample_names else "functions",
        }
    ],
}

write_json(os.path.join(vscode_dir, "launch.json"), launch)
print("Wrote .vscode/launch.json")

# ------------------------------------------------------------------------------
# cleanup stale files
# ------------------------------------------------------------------------------

compile_flags = os.path.join(project_root, "compile_flags.txt")
if os.path.exists(compile_flags):
    os.remove(compile_flags)
    print("Removed compile_flags.txt (superseded by setup)")

print()
print("Done!")
