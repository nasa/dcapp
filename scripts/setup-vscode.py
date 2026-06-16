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
    "apps",
    "apps/dcapp",
    "src",
    "extensions",
    "shaders",
    "assets",
    "pilotlight/src",
    "pilotlight/libs",
    "pilotlight/extensions",
    "pilotlight/shaders",
    "pilotlight/dependencies/stb",
    "pilotlight/dependencies/cgltf",
    "pilotlight/dependencies/imgui",
    "pilotlight/dependencies/glfw/include",
]

# platform-specific include paths
platform_includes = {
    "Linux":   ["/usr/include/libxml2", "/usr/include/gdal"],
    "Darwin":  [
        "/opt/homebrew/opt/libxml2/include/libxml2",
        "/opt/homebrew/opt/gdal/include",
        "/usr/local/opt/libxml2/include/libxml2",
        "/usr/local/opt/gdal/include",
    ],
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

def existing_relative_paths(paths):
    """Return only paths that exist relative to the project root."""
    result = []
    seen = set()
    for path in paths:
        normalized = path.replace("\\", "/").rstrip("/")
        if normalized in seen:
            continue
        if os.path.exists(os.path.join(project_root, normalized)):
            result.append(normalized)
            seen.add(normalized)
    return result

def discover_sample_logic_includes():
    """Find sample logic directories, even before generated headers exist."""
    result = []
    samples_dir = os.path.join(project_root, "samples")
    if not os.path.isdir(samples_dir):
        return result
    for entry in sorted(os.listdir(samples_dir)):
        logic_dir = os.path.join(samples_dir, entry, "logic")
        if os.path.isdir(logic_dir):
            result.append(f"samples/{entry}/logic")
    return result

def discover_samples():
    """Discover samples that follow samples/<name>/<name>.xml."""
    result = []
    samples_dir = os.path.join(project_root, "samples")
    if not os.path.isdir(samples_dir):
        return result
    for entry in sorted(os.listdir(samples_dir)):
        if entry.startswith("."):
            continue
        sample_xml = os.path.join(samples_dir, entry, entry + ".xml")
        if os.path.isfile(sample_xml):
            result.append(entry)
    return result

def default_sample(sample_names):
    if "welcome" in sample_names:
        return "welcome"
    if "primitives" in sample_names:
        return "primitives"
    return sample_names[0] if sample_names else ""

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

project_includes = existing_relative_paths(common_includes + discover_sample_logic_includes())
sample_names = discover_samples()
sample_default = default_sample(sample_names)

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

if plat == "Darwin":
    recommendations.append("vadimcn.vscode-lldb")

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

    # include paths (absolute so clangd resolves them for files in any subdirectory)
    for inc in project_includes:
        lines.append(f"        - -I{project_root}/{inc}")

    # platform-specific absolute paths
    for inc in platform_includes.get(plat, []):
        lines.append(f"        - -I{inc}")

    # vcpkg on Windows
    if plat == "Windows":
        lines.append(f"        - -I{project_root}/{vcpkg_include}")

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
    include_paths = [f"${{workspaceFolder}}/{inc}" for inc in project_includes]

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
        "preLaunchTask": "build debug",
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
        "preLaunchTask": "build debug",
    })
elif plat == "Windows":
    configs.append({
        "name": "dcapp (windows)",
        "type": "cppvsdbg",
        "request": "launch",
        "program": "${workspaceFolder}\\pilotlight\\out\\pilot_light.exe",
        "args": ["-a", "dcapp", "${workspaceFolder}/samples/${input:sampleName}/${input:sampleName}.xml"],
        "cwd": "${workspaceFolder}\\pilotlight\\out",
        "preLaunchTask": "build debug",
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
            "default": sample_default,
        }
    ],
}

write_json(os.path.join(vscode_dir, "launch.json"), launch)
print("Wrote .vscode/launch.json")

# ------------------------------------------------------------------------------
# generate .vscode/tasks.json
# ------------------------------------------------------------------------------

if plat == "Windows":
    build_command = "${workspaceFolder}\\scripts\\build.bat"
    validate_command = "${workspaceFolder}\\bin\\dcapp-validate.bat"
    genheader_command = "${workspaceFolder}\\bin\\dcapp-genheader.bat"
else:
    build_command = "${workspaceFolder}/scripts/build.sh"
    validate_command = "${workspaceFolder}/bin/dcapp-validate.sh"
    genheader_command = "${workspaceFolder}/bin/dcapp-genheader.sh"

tasks = {
    "version": "2.0.0",
    "tasks": [
        {
            "label": "build",
            "type": "shell",
            "command": build_command,
            "group": {"kind": "build", "isDefault": True},
            "problemMatcher": "$gcc",
        },
        {
            "label": "build debug",
            "type": "shell",
            "command": build_command,
            "args": ["-c", "debug"],
            "group": "build",
            "problemMatcher": "$gcc",
        },
        {
            "label": "validate selected sample",
            "type": "shell",
            "command": validate_command,
            "args": ["${workspaceFolder}/samples/${input:sampleName}/${input:sampleName}.xml"],
            "group": "test",
            "problemMatcher": [],
        },
        {
            "label": "generate selected sample logic header",
            "type": "shell",
            "command": genheader_command,
            "args": ["${workspaceFolder}/samples/${input:sampleName}/${input:sampleName}.xml"],
            "problemMatcher": [],
        },
    ],
    "inputs": [
        {
            "id": "sampleName",
            "type": "pickString",
            "description": "Select a sample",
            "options": sample_names,
            "default": sample_default,
        }
    ],
}

write_json(os.path.join(vscode_dir, "tasks.json"), tasks)
print("Wrote .vscode/tasks.json")

# ------------------------------------------------------------------------------
# cleanup stale files
# ------------------------------------------------------------------------------

compile_flags = os.path.join(project_root, "compile_flags.txt")
if os.path.exists(compile_flags):
    os.remove(compile_flags)
    print("Removed compile_flags.txt (superseded by setup)")

print()
print("Done!")
