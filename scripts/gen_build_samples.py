#!/usr/bin/env python3

# Index of this file:
# [SECTION] imports

# -----------------------------------------------------------------------------
# [SECTION] imports
# -----------------------------------------------------------------------------

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
        pl_dir_abs = os.path.abspath(os.getcwd() + "/" + pl_dir_abs)

# append to path to import packages
sys.path.append(pl_dir_abs)

# import build packages
import build.core as pl
import build.backend_win32 as win32
import build.backend_linux as linux
import build.backend_macos as apple

# -----------------------------------------------------------------------------
# [SECTION] project
# -----------------------------------------------------------------------------

# where to output build scripts (absolute)
dcapp_home_abs = os.path.abspath(file_dir_rel + "/..")
build_script_out_dir_abs = os.path.abspath(dcapp_home_abs + "/scripts")
app_bin_dir_abs = os.path.abspath(pl_dir_abs + "/out")

# now, update directories to be relative to the output directory
pl_dir_rel = os.path.relpath(pl_dir_abs, build_script_out_dir_abs)
app_bin_dir_rel = os.path.relpath(app_bin_dir_abs, build_script_out_dir_abs)

with pl.project("samples"):

    # -----------------------------------------------------------------------------
    # [SECTION] profiles
    # -----------------------------------------------------------------------------

    # win32 or msvc only
    pl.add_profile(
        compiler_filter=["msvc"],
        target_type_filter=[pl.TargetType.DYNAMIC_LIBRARY],
        linker_flags=["-noexp", "-nologo", "-noimplib"],
    )

    pl.add_profile(
        compiler_filter=["msvc"],
        linker_flags=["-incremental:no"],
        compiler_flags=[
            "-Zc:preprocessor",
            "-nologo",
            "-W4",
            "-WX",
            "-wd4201",
            "-wd4100",
            "-wd4996",
            "-wd4505",
            "-wd4189",
            "-wd5105",
            "-wd4115",
        ],
    )
    pl.add_profile(
        compiler_filter=["msvc"],
        configuration_filter=["debug"],
        compiler_flags=["-Od", "-MDd", "-Zi"],
    )
    pl.add_profile(
        compiler_filter=["msvc"],
        configuration_filter=["release"],
        compiler_flags=["-O2", "-MD"],
    )

    # linux or gcc only
    pl.add_profile(
        platform_filter=["Linux"], link_directories=["/usr/lib/x86_64-linux-gnu"]
    )
    pl.add_profile(
        compiler_filter=["gcc"], linker_flags=["-ldl", "-lm"], compiler_flags=["-fPIC"]
    )
    pl.add_profile(
        compiler_filter=["gcc"],
        configuration_filter=["debug"],
        compiler_flags=["--debug", "-g", "-O0"],
    )

    # macos or clang only
    pl.add_profile(
        platform_filter=["Darwin"],
        link_frameworks=[
            "Metal",
            "MetalKit",
            "Cocoa",
            "IOKit",
            "CoreVideo",
            "QuartzCore",
        ],
    )
    pl.add_profile(
        compiler_filter=["clang"],
        linker_flags=["-Wl,-rpath,/usr/local/lib"],
        compiler_flags=["-fmodules", "-ObjC", "-fPIC"],
    )
    pl.add_profile(
        compiler_filter=["clang"],
        configuration_filter=["debug"],
        compiler_flags=["--debug", "-g"],
    )

    # -----------------------------------------------------------------------------
    # [SECTION] app
    # -----------------------------------------------------------------------------

    # test sample
    with pl.target("test", pl.TargetType.DYNAMIC_LIBRARY):

        sample_dir_abs = dcapp_home_abs + "/samples/test"

        sample_xml_abs = sample_dir_abs + "/test.xml"
        sample_xml_rel = os.path.relpath(sample_xml_abs, build_script_out_dir_abs)

        logic_dir_abs = os.path.abspath(sample_dir_abs + "/logic")
        logic_dir_rel = os.path.relpath(logic_dir_abs, build_script_out_dir_abs)

        genheader_abs = app_bin_dir_abs + "/dcapp-genheader"
        genheader_rel = os.path.relpath(genheader_abs, build_script_out_dir_abs)

        pl.set_output_directory(logic_dir_rel)
        pl.set_output_binary("logic")

        # add source files
        logic_src_file = os.path.relpath(
            logic_dir_rel + "/logic.cpp", build_script_out_dir_abs
        )
        pl.add_source_files(logic_src_file)

        # release config
        with pl.configuration("release"):

            # win32
            with pl.platform("Windows"):
                with pl.compiler("msvc"):
                    pl.set_pre_target_build_step(
                        f"{genheader_rel}.exe {sample_xml_rel}"
                    )

            # linux
            with pl.platform("Linux"):
                with pl.compiler("gcc"):
                    pl.set_pre_target_build_step(f"{genheader_rel} {sample_xml_rel}")

            # mac os
            with pl.platform("Darwin"):
                with pl.compiler("clang"):
                    pl.set_pre_target_build_step(f"{genheader_rel} {sample_xml_rel}")

        # debug config
        with pl.configuration("debug"):

            # win32
            with pl.platform("Windows"):
                with pl.compiler("msvc"):
                    pl.set_pre_target_build_step(
                        f"{genheader_rel}.exe {sample_xml_rel}"
                    )

            # linux
            with pl.platform("Linux"):
                with pl.compiler("gcc"):
                    pl.set_pre_target_build_step(f"{genheader_rel} {sample_xml_rel}")

            # mac os
            with pl.platform("Darwin"):
                with pl.compiler("clang"):
                    pl.set_pre_target_build_step(f"{genheader_rel} {sample_xml_rel}")

# -----------------------------------------------------------------------------
# [SECTION] generate scripts
# -----------------------------------------------------------------------------

out_script_win32 = build_script_out_dir_abs + "/" + "build_samples_win32.bat"
out_script_macos = build_script_out_dir_abs + "/" + "build_samples_macos.sh"
out_script_linux = build_script_out_dir_abs + "/" + "build_samples_linux.sh"

win32.generate_build(out_script_win32)
apple.generate_build(out_script_macos)
linux.generate_build(out_script_linux)

if plat.system() in ("Darwin", "Linux"):
    os.chmod(out_script_macos, 0o755)
    os.chmod(out_script_linux, 0o755)
