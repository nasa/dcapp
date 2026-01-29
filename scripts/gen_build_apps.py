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

# default pilotlight location (absolute)
file_dir_rel = os.path.dirname(__file__)
if not file_dir_rel:
    file_dir_rel = "."
pl_dir_abs = os.path.abspath(file_dir_rel + "/../pilotlight")

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

# list all files matching extensions
def list_files_recursive(directory, *extensions):

    # prevent globbing root
    if not directory.strip():
        directory = "."

    pattern = []
    for extension in extensions:
        pattern += glob.glob(f"{directory}/**/*{extension}", recursive=True)
    return pattern

#-----------------------------------------------------------------------------
# [SECTION] project
#-----------------------------------------------------------------------------

# where to output build scripts (absolute)
dcapp_home_abs = os.path.abspath(file_dir_rel + "/..")
output_dir_abs = os.path.abspath(dcapp_home_abs + "/scripts")
bin_dir_abs = os.path.abspath(pl_dir_abs + "/out")

# now, update directories to be relative to the output directory
pl_dir_rel = os.path.relpath(pl_dir_abs, output_dir_abs)
bin_dir_rel = os.path.relpath(bin_dir_abs, output_dir_abs)

# set vcpkg (windows only)
vcpkg_abs = ""
vcpkg_rel = ""
vcpkg_copy_cmd = ""
if plat.system() == "Windows":
    vcpkg_abs = os.path.abspath(dcapp_home_abs + "/vcpkg_installed/x64-windows")
    vcpkg_rel = os.path.relpath(vcpkg_abs, output_dir_abs)
    vcpkg_copy_cmd = f'xcopy /Y /I "{vcpkg_rel}\\bin\\*.dll" "{bin_dir_rel}\\"'

with pl.project("apps"):

    # project wide settings
    pl.set_output_directory(bin_dir_rel)
    pl.add_link_directories(bin_dir_rel)
    pl.add_include_directories(
        os.path.relpath(dcapp_home_abs + "/src", output_dir_abs),
        os.path.relpath(dcapp_home_abs + "/extensions", output_dir_abs),
        pl_dir_rel + "/src",
        pl_dir_rel + "/libs",
        pl_dir_rel + "/extensions",
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
                    compiler_flags=["-O2", "-MD"])

    # linux or gcc only
    pl.add_profile(platform_filter=["Linux"],
                    link_directories=["/usr/lib/x86_64-linux-gnu"])
    pl.add_profile(compiler_filter=["gcc"],
                    linker_flags=["-ldl", "-lm"],
                    compiler_flags=["-fPIC"])
    pl.add_profile(compiler_filter=["gcc"],
                    configuration_filter=["debug"],
                    compiler_flags=["--debug", "-g", "-O0"])

    # macos or clang only
    pl.add_profile(platform_filter=["Darwin"],
                    link_frameworks=["Metal", "MetalKit", "Cocoa", "IOKit", "CoreVideo", "QuartzCore"])
    pl.add_profile(compiler_filter=["clang"],
                    linker_flags=["-Wl,-rpath,/usr/local/lib"],
                    compiler_flags=["-fmodules", "-ObjC", "-fPIC"])
    pl.add_profile(compiler_filter=["clang"],
                    configuration_filter=["debug"],
                    compiler_flags=["--debug", "-g"])

    #-----------------------------------------------------------------------------
    # [SECTION] dcapp extensions
    #-----------------------------------------------------------------------------

    dcapp_extensions = [
        "dc_draw_ext",
        "dc_draw_backend_ext",
    ]

    for ext_name in dcapp_extensions:
        with pl.target(ext_name, pl.TargetType.DYNAMIC_LIBRARY):
            pl.set_output_binary(ext_name)
            pl.add_source_files(
                os.path.relpath(dcapp_home_abs + "/extensions/" + ext_name + ".c", output_dir_abs)
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
        src_files_abs = list_files_recursive(dcapp_home_abs + "/src", ".c")
        sources_files_rel = [os.path.relpath(src_file, output_dir_abs) for src_file in src_files_abs]
        pl.add_source_files(
            *sources_files_rel,
            os.path.relpath(dcapp_home_abs + "/apps/dcapp.c", output_dir_abs)
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

        # list source files relative to the output directory
        src_files_abs =  list_files_recursive(dcapp_home_abs + "/src/app", ".c")
        src_files_abs += list_files_recursive(dcapp_home_abs + "/src/utils", ".c")
        src_files_abs += [dcapp_home_abs + "/src/value.c"]
        sources_files_rel = [os.path.relpath(src_file, output_dir_abs) for src_file in src_files_abs]
        pl.add_source_files(
            *sources_files_rel,
            os.path.relpath(dcapp_home_abs + "/apps/dcapp-genheader.c", output_dir_abs)
        )

        # release config
        with pl.configuration("release"):

            # win32
            with pl.platform("Windows"):
                with pl.compiler("msvc"):
                    pl.add_linker_flags("-nologo", "-noimplib", "-noexp")
                    pl.add_static_link_libraries("libxml2")
                    pl.add_include_directories(vcpkg_rel + "/include/libxml2")
                    pl.set_pre_target_build_step(vcpkg_copy_cmd)
            
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

    # dcapp-validate
    with pl.target("dcapp-validate", pl.TargetType.EXECUTABLE):

        pl.set_output_binary("dcapp-validate")

        # list source files relative to the output directory
        src_files_abs =  list_files_recursive(dcapp_home_abs + "/src/app", ".c")
        src_files_abs += list_files_recursive(dcapp_home_abs + "/src/utils", ".c")
        src_files_abs += [dcapp_home_abs + "/src/value.c"]
        sources_files_rel = [os.path.relpath(src_file, output_dir_abs) for src_file in src_files_abs]
        pl.add_source_files(
            *sources_files_rel,
            os.path.relpath(dcapp_home_abs + "/apps/dcapp-validate.c", output_dir_abs)
        )

        # release config
        with pl.configuration("release"):

            # win32
            with pl.platform("Windows"):
                with pl.compiler("msvc"):
                    pl.add_linker_flags("-nologo", "-noimplib", "-noexp")
                    pl.add_static_link_libraries("libxml2")
                    pl.add_include_directories(vcpkg_rel + "/include/libxml2")
                    pl.set_pre_target_build_step(vcpkg_copy_cmd)

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
                    pl.set_pre_target_build_step(vcpkg_copy_cmd)

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

#-----------------------------------------------------------------------------
# [SECTION] generate scripts
#-----------------------------------------------------------------------------

out_script_win32 = output_dir_abs + "/" + "build_apps_win32.bat"
out_script_macos = output_dir_abs + "/" + "build_apps_macos.sh"
out_script_linux = output_dir_abs + "/" + "build_apps_linux.sh"

win32.generate_build(out_script_win32)
apple.generate_build(out_script_macos)
linux.generate_build(out_script_linux)
