#!/usr/bin/env python3

# Index of this file:
# [SECTION] imports

#-----------------------------------------------------------------------------
# [SECTION] imports
#-----------------------------------------------------------------------------

import glob
import os
import sys
import platform as plat

def fwd(path):
    return path.replace("\\", "/")

# default pilotlight location (absolute)
file_dir_rel = os.path.dirname(__file__)
if not file_dir_rel:
    file_dir_rel = "."
pl_dir_abs = os.path.abspath(file_dir_rel + "/../../pilotlight")

# if provided, use pilotlight location in input (absolute)
if len(sys.argv) > 1:
    pl_dir_abs = sys.argv[1]
    if not os.path.isabs(pl_dir_abs):
        pl_dir_abs = os.path.abspath(os.getcwd() + "/" + pl_location)

# append to path to import packages
sys.path.append(pl_dir_abs)

import build.core as pl
import build.backend_win32 as win32
import build.backend_linux as linux
import build.backend_macos as apple

#-----------------------------------------------------------------------------
# [SECTION] utilities
#-----------------------------------------------------------------------------

# List matching files directly in a directory. Use this for source groups whose
# directory boundary is part of the target definition.
def list_files(directory, *extensions):
    if not directory.strip():
        directory = "."
    files = []
    for extension in extensions:
        files.extend(glob.glob(f"{directory}/*{extension}"))
    return sorted(path for path in files if os.path.isfile(path))


# List matching files below a directory when a target intentionally owns the
# entire subtree.
def list_files_recursive(directory, *extensions):
    if not directory.strip():
        directory = "."
    files = []
    for extension in extensions:
        files.extend(glob.glob(f"{directory}/**/*{extension}", recursive=True))
    return sorted(path for path in files if os.path.isfile(path))

#-----------------------------------------------------------------------------
# [SECTION] project
#-----------------------------------------------------------------------------

# where to output build scripts (absolute)
dcapp_home_abs = os.path.abspath(file_dir_rel + "/../..")
output_dir_abs = os.path.abspath(file_dir_rel)
bin_dir_abs = os.path.abspath(pl_dir_abs + "/out")

# now, update directories to be relative to the output directory
pl_dir_rel  = fwd(os.path.relpath(pl_dir_abs, output_dir_abs))
bin_dir_rel = fwd(os.path.relpath(bin_dir_abs, output_dir_abs))

def source(path):
    """Resolve a repository-relative source path."""
    return os.path.join(dcapp_home_abs, path)


def relative_sources(*source_groups):
    """Flatten absolute source groups into deterministic build-script paths."""
    files = []
    for group in source_groups:
        if isinstance(group, (list, tuple)):
            files.extend(group)
        else:
            files.append(group)
    return [fwd(os.path.relpath(path, output_dir_abs)) for path in files]


def normalize_windows_script(path):
    """Keep the committed batch file deterministic on every host platform."""
    with open(path, "r", newline=None) as script_file:
        lines = script_file.read().splitlines()
    with open(path, "w", newline="") as script_file:
        script_file.write("\r\n".join(line.rstrip() for line in lines) + "\r\n")


# Source groups make target ownership explicit. Adding a new directory under
# src does not silently add it to dcapp or either command-line tool.
app_runtime_sources = list_files(source("src/app"), ".c")
pixelstream_sources = list_files(source("src/pixelstream"), ".c")
utility_sources = list_files(source("src/utils"), ".c")
core_runtime_sources = [
    source("src/edge.c"),
    source("src/geo.c"),
    source("src/geojson.c"),
    source("src/sock.c"),
    source("src/trick.c"),
]
config_utility_sources = [
    source("src/utils/env.c"),
    source("src/utils/file.c"),
    source("src/utils/log.c"),
    source("src/utils/math.c"),
    source("src/utils/string.c"),
]

# set vcpkg paths (always computed so the windows bat is correct regardless of host platform)
vcpkg_abs = os.path.abspath(dcapp_home_abs + "/vcpkg_installed/x64-windows")
vcpkg_rel = fwd(os.path.relpath(vcpkg_abs, output_dir_abs))
vcpkg_copy_cmd = f'xcopy /Y /I "{vcpkg_rel}\\bin\\*.dll" "{bin_dir_rel}\\"'

with pl.project("apps"):

    # project wide settings
    pl.set_output_directory(bin_dir_rel)
    pl.add_link_directories(bin_dir_rel)
    pl.add_include_directories(
        fwd(os.path.relpath(dcapp_home_abs + "/src", output_dir_abs)),
        fwd(os.path.relpath(dcapp_home_abs + "/extensions", output_dir_abs)),
        fwd(os.path.relpath(dcapp_home_abs + "/shaders", output_dir_abs)),
        pl_dir_rel + "/src",
        pl_dir_rel + "/libs",
        pl_dir_rel + "/extensions",
        pl_dir_rel + "/shaders",
        pl_dir_rel + "/dependencies/stb")

    #-----------------------------------------------------------------------------
    # [SECTION] profiles
    #-----------------------------------------------------------------------------

    # win32 or msvc only
    pl.add_profile(compiler_filter=["msvc"],
                    target_type_filter=[pl.TargetType.DYNAMIC_LIBRARY],
                    linker_flags=["-noimplib", "-noexp"])
    
    pl.add_profile(compiler_filter=["msvc"],
                    linker_flags=["-incremental:no"],
                    compiler_flags=["-Zc:preprocessor", "-nologo", "-W4", "-WX", "-wd4201", "-wd4100", "-wd4996", "-wd4505", "-wd4189", "-wd5105", "-wd4115"],
                    link_directories=[vcpkg_rel + "/lib"],
                    include_directories=[vcpkg_rel + "/include"])
    pl.add_profile(compiler_filter=["msvc"],
                    configuration_filter=["debug"],
                    compiler_flags=["-Od", "-MDd", "-Zi"])
    pl.add_profile(compiler_filter=["msvc"],
                    configuration_filter=["release"],
                    compiler_flags=["-O2", "-MD", "-DNDEBUG"])

    # linux or gcc only
    pl.add_profile(platform_filter=["Linux"],
                    link_directories=["/usr/lib/x86_64-linux-gnu"])
    pl.add_profile(compiler_filter=["gcc"],
                    linker_flags=["-ldl", "-lm"],
                    compiler_flags=["-fPIC"])
    pl.add_profile(compiler_filter=["gcc"],
                    configuration_filter=["debug"],
                    compiler_flags=["--debug", "-g", "-O0"])
    pl.add_profile(compiler_filter=["gcc"],
                    configuration_filter=["release"],
                    compiler_flags=["-DNDEBUG"])

    # macos or clang only
    pl.add_profile(platform_filter=["Darwin"],
                    link_frameworks=["Metal", "MetalKit", "Cocoa", "IOKit", "CoreVideo", "QuartzCore"],
                    link_directories=["$(brew --prefix)/lib"])
    pl.add_profile(compiler_filter=["clang"],
                    linker_flags=["-Wl,-rpath,/usr/local/lib"],
                    compiler_flags=["-fmodules", "-ObjC", "-fPIC"])
    pl.add_profile(compiler_filter=["clang"],
                    configuration_filter=["debug"],
                    compiler_flags=["--debug", "-g"])
    pl.add_profile(compiler_filter=["clang"],
                    configuration_filter=["release"],
                    compiler_flags=["-DNDEBUG"])

    #-----------------------------------------------------------------------------
    # [SECTION] dcapp extensions
    #-----------------------------------------------------------------------------

    dcapp_extensions = [
        "dc_draw_ext",
        "dc_draw_backend_ext",
        "pl_planet_processor_ext",
        "pl_planet_ext",
    ]

    for ext_name in dcapp_extensions:
        with pl.target(ext_name, pl.TargetType.DYNAMIC_LIBRARY):
            pl.set_output_binary(ext_name)
            pl.add_source_files(
                fwd(os.path.relpath(dcapp_home_abs + "/extensions/" + ext_name + ".c", output_dir_abs))
            )

            # release config
            with pl.configuration("release"):

                # win32
                with pl.platform("Windows"):
                    with pl.compiler("msvc"):
                        pl.add_linker_flags("-nologo", "-noimplib", "-noexp")

                # linux
                with pl.platform("Linux"):
                    with pl.compiler("gcc"):
                        pass

                # mac os
                with pl.platform("Darwin"):
                    with pl.compiler("clang"):
                        pass

            # debug config
            with pl.configuration("debug"):

                # win32
                with pl.platform("Windows"):
                    with pl.compiler("msvc"):
                        pl.add_linker_flags("-nologo", "-noimplib", "-noexp")

                # linux
                with pl.platform("Linux"):
                    with pl.compiler("gcc"):
                        pl.add_compiler_flags("--debug", "-g")

                # mac os
                with pl.platform("Darwin"):
                    with pl.compiler("clang"):
                        pass

    #-----------------------------------------------------------------------------
    # [SECTION] app
    #-----------------------------------------------------------------------------

    # dcapp
    with pl.target("dcapp", pl.TargetType.DYNAMIC_LIBRARY):

        pl.set_output_binary("dcapp")

        # list source files relative to the output directory
        pl.add_source_files(
            *relative_sources(
                app_runtime_sources,
                core_runtime_sources,
                pixelstream_sources,
                utility_sources,
                source("apps/dcapp.c"),
            )
        )

        # release config
        with pl.configuration("release"):

            # win32
            with pl.platform("Windows"):
                with pl.compiler("msvc"):
                    pl.add_linker_flags("-nologo", "-noimplib", "-noexp")
                    pl.add_static_link_libraries("libxml2", "libcurl")
                    pl.add_include_directories(vcpkg_rel + "/include/libxml2")
                    pl.set_pre_target_build_step(vcpkg_copy_cmd)

            # linux
            with pl.platform("Linux"):
                with pl.compiler("gcc"):
                    pl.add_include_directories("/usr/include/libxml2")
                    pl.add_linker_flags("-lxml2", "-lcurl")

            # mac os
            with pl.platform("Darwin"):
                with pl.compiler("clang"):
                    pl.add_include_directories("/opt/homebrew/opt/libxml2/include/libxml2")
                    pl.add_linker_flags("-lxml2", "-lcurl")
                    
        # debug config
        with pl.configuration("debug"):

            pl.add_static_link_libraries("dearimguid")

            # win32
            with pl.platform("Windows"):
                with pl.compiler("msvc"):
                    pl.add_linker_flags("-nologo", "-noimplib", "-noexp")
                    pl.add_static_link_libraries("libxml2", "libcurl")
                    pl.add_include_directories(vcpkg_rel + "/include/libxml2")
                    pl.set_pre_target_build_step(vcpkg_copy_cmd)

            # linux
            with pl.platform("Linux"):
                with pl.compiler("gcc"):
                    pl.add_include_directories("/usr/include/libxml2")
                    pl.add_compiler_flags("--debug", "-g")
                    pl.add_linker_flags("-lxml2", "-lcurl")

            # mac os
            with pl.platform("Darwin"):
                with pl.compiler("clang"):
                    pl.add_include_directories("/opt/homebrew/opt/libxml2/include/libxml2")
                    pl.add_linker_flags("-lxml2", "-lcurl")

    # dcapp-genheader
    with pl.target("dcapp-genheader", pl.TargetType.EXECUTABLE):

        pl.set_output_binary("dcapp-genheader")

        pl.add_source_files(
            *relative_sources(
                source("src/app/config.c"),
                source("src/app/elem.c"),
                source("src/app/lookup.c"),
                source("src/app/value.c"),
                config_utility_sources,
                source("apps/dcapp_genheader.c"),
            )
        )

        # release config
        with pl.configuration("release"):

            # win32
            with pl.platform("Windows"):
                with pl.compiler("msvc"):
                    pl.add_linker_flags("-nologo", "-noimplib", "-noexp")
                    pl.add_static_link_libraries("libxml2")
                    pl.add_include_directories(vcpkg_rel + "/include/libxml2")
            
            # linux
            with pl.platform("Linux"):
                with pl.compiler("gcc"):
                    pl.add_include_directories("/usr/include/libxml2")
                    pl.add_linker_flags("-lxml2")

            # mac os
            with pl.platform("Darwin"):
                with pl.compiler("clang"):
                    pl.add_include_directories("/opt/homebrew/opt/libxml2/include/libxml2")
                    pl.add_linker_flags("-lxml2")
                    
        # debug config
        with pl.configuration("debug"):

            # win32
            with pl.platform("Windows"):
                with pl.compiler("msvc"):
                    pl.add_linker_flags("-nologo", "-noimplib", "-noexp")
                    pl.add_static_link_libraries("libxml2")
                    pl.add_include_directories(vcpkg_rel + "/include/libxml2")
            
            # linux
            with pl.platform("Linux"):
                with pl.compiler("gcc"):
                    pl.add_include_directories("/usr/include/libxml2")
                    pl.add_compiler_flags("--debug", "-g")
                    pl.add_linker_flags("-lxml2", "-lcurl")

            # mac os
            with pl.platform("Darwin"):
                with pl.compiler("clang"):
                    pl.add_include_directories("/opt/homebrew/opt/libxml2/include/libxml2")
                    pl.add_linker_flags("-lxml2", "-lcurl")

    # dcapp-validate
    with pl.target("dcapp-validate", pl.TargetType.EXECUTABLE):

        pl.set_output_binary("dcapp-validate")

        pl.add_source_files(
            *relative_sources(
                source("src/app/config.c"),
                source("src/app/elem.c"),
                config_utility_sources,
                source("apps/dcapp_validate.c"),
            )
        )

        # release config
        with pl.configuration("release"):

            # win32
            with pl.platform("Windows"):
                with pl.compiler("msvc"):
                    pl.add_linker_flags("-nologo", "-noimplib", "-noexp")
                    pl.add_static_link_libraries("libxml2")
                    pl.add_include_directories(vcpkg_rel + "/include/libxml2")

            # linux
            with pl.platform("Linux"):
                with pl.compiler("gcc"):
                    pl.add_include_directories("/usr/include/libxml2")
                    pl.add_linker_flags("-lxml2")

            # mac os
            with pl.platform("Darwin"):
                with pl.compiler("clang"):
                    pl.add_include_directories("/opt/homebrew/opt/libxml2/include/libxml2")
                    pl.add_linker_flags("-lxml2")

        # debug config
        with pl.configuration("debug"):

            # win32
            with pl.platform("Windows"):
                with pl.compiler("msvc"):
                    pl.add_linker_flags("-nologo", "-noimplib", "-noexp")
                    pl.add_static_link_libraries("libxml2")
                    pl.add_include_directories(vcpkg_rel + "/include/libxml2")

            # linux
            with pl.platform("Linux"):
                with pl.compiler("gcc"):
                    pl.add_include_directories("/usr/include/libxml2")
                    pl.add_compiler_flags("--debug", "-g")
                    pl.add_linker_flags("-lxml2")

            # mac os
            with pl.platform("Darwin"):
                with pl.compiler("clang"):
                    pl.add_include_directories("/opt/homebrew/opt/libxml2/include/libxml2")
                    pl.add_linker_flags("-lxml2")

    # dcapp-planet-chunkgen
    with pl.target("dcapp-planet-chunkgen", pl.TargetType.DYNAMIC_LIBRARY):

        pl.set_output_binary("dcapp-planet-chunkgen")

        pl.add_source_files(
            fwd(os.path.relpath(dcapp_home_abs + "/apps/dcapp_planet_chunkgen.c", output_dir_abs)),
            fwd(os.path.relpath(dcapp_home_abs + "/src/utils/file.c", output_dir_abs)),
            fwd(os.path.relpath(dcapp_home_abs + "/src/utils/log.c", output_dir_abs)),
        )

        # release config
        with pl.configuration("release"):

            # win32
            with pl.platform("Windows"):
                with pl.compiler("msvc"):
                    pl.add_linker_flags("-nologo", "-noimplib", "-noexp")
                    pl.add_static_link_libraries("gdal")

            # linux
            with pl.platform("Linux"):
                with pl.compiler("gcc"):
                    pl.add_include_directories("/usr/include/gdal")
                    pl.add_linker_flags("-lgdal")

            # mac os
            with pl.platform("Darwin"):
                with pl.compiler("clang"):
                    pl.add_include_directories("/opt/homebrew/opt/gdal/include")
                    pl.add_linker_flags("-lgdal")

        # debug config
        with pl.configuration("debug"):

            # win32
            with pl.platform("Windows"):
                with pl.compiler("msvc"):
                    pl.add_linker_flags("-nologo", "-noimplib", "-noexp")
                    pl.add_static_link_libraries("gdal")

            # linux
            with pl.platform("Linux"):
                with pl.compiler("gcc"):
                    pl.add_include_directories("/usr/include/gdal")
                    pl.add_linker_flags("-lgdal")
                    pl.add_compiler_flags("--debug", "-g")

            # mac os
            with pl.platform("Darwin"):
                with pl.compiler("clang"):
                    pl.add_include_directories("/opt/homebrew/opt/gdal/include")
                    pl.add_linker_flags("-lgdal")

    # dcapp-planet-snapshot
    with pl.target("dcapp-planet-snapshot", pl.TargetType.DYNAMIC_LIBRARY):

        pl.set_output_binary("dcapp-planet-snapshot")

        pl.add_source_files(
            fwd(os.path.relpath(dcapp_home_abs + "/apps/dcapp_planet_snapshot.c", output_dir_abs)),
            fwd(os.path.relpath(dcapp_home_abs + "/src/geo.c", output_dir_abs)),
            fwd(os.path.relpath(dcapp_home_abs + "/src/utils/file.c", output_dir_abs)),
            fwd(os.path.relpath(dcapp_home_abs + "/src/utils/log.c", output_dir_abs)),
            fwd(os.path.relpath(dcapp_home_abs + "/src/utils/math.c", output_dir_abs)),
            fwd(os.path.relpath(dcapp_home_abs + "/src/utils/string.c", output_dir_abs)),
        )

        # release config
        with pl.configuration("release"):

            # win32
            with pl.platform("Windows"):
                with pl.compiler("msvc"):
                    pl.add_linker_flags("-nologo", "-noimplib", "-noexp")

            # linux
            with pl.platform("Linux"):
                with pl.compiler("gcc"):
                    pass

            # mac os
            with pl.platform("Darwin"):
                with pl.compiler("clang"):
                    pass

        # debug config
        with pl.configuration("debug"):

            # win32
            with pl.platform("Windows"):
                with pl.compiler("msvc"):
                    pl.add_linker_flags("-nologo", "-noimplib", "-noexp")

            # linux
            with pl.platform("Linux"):
                with pl.compiler("gcc"):
                    pl.add_compiler_flags("--debug", "-g")

            # mac os
            with pl.platform("Darwin"):
                with pl.compiler("clang"):
                    pass

#-----------------------------------------------------------------------------
# [SECTION] generate scripts
#-----------------------------------------------------------------------------

out_script_win32 = output_dir_abs + "/" + "build-apps-win32.bat"
out_script_macos = output_dir_abs + "/" + "build-apps-macos.sh"
out_script_linux = output_dir_abs + "/" + "build-apps-linux.sh"

win32.generate_build(out_script_win32)
normalize_windows_script(out_script_win32)
apple.generate_build(out_script_macos)
linux.generate_build(out_script_linux)
