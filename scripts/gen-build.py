# gen_build.py

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
pilotlight_location_abs = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "pilotlight"))

# if provided, use pilotlight location in input (absolute)
if len(sys.argv) > 1:
    pilotlight_location_abs = sys.argv[1]
    if not os.path.isabs(pilotlight_location_abs):
        pilotlight_location_abs = os.path.abspath(os.getcwd() + "/" + pilotlight_location)

# append to path to import packages
sys.path.append(pilotlight_location_abs)

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
dcapp_home_abs = os.path.abspath(os.path.dirname(__file__) + "/..")

# now, update pilotlight_location to be relative to the output directory
pilotlight_location_rel = os.path.relpath(pilotlight_location_abs, dcapp_home_abs)

with pl.project("dcapp"):

    #-----------------------------------------------------------------------------
    # [SECTION] profiles
    #-----------------------------------------------------------------------------

    # win32 or msvc only
    pl.add_profile(compiler_filter=["msvc"],
                    target_type_filter=[pl.TargetType.DYNAMIC_LIBRARY],
                    linker_flags=["-noimplib", "-noexp"])
    
    pl.add_profile(compiler_filter=["msvc"],
                    linker_flags=["-incremental:no"],
                    compiler_flags=["-Zc:preprocessor", "-nologo", "-std:c11", "-W4", "-WX", "-wd4201",
                                "-wd4100", "-wd4996", "-wd4505", "-wd4189", "-wd5105", "-wd4115", "-permissive-"])
    pl.add_profile(compiler_filter=["msvc"],
                    configuration_filter=["debug"],
                    compiler_flags=["-Od", "-MDd", "-Zi"])

    # linux or gcc only
    pl.add_profile(platform_filter=["Linux"],
                    link_directories=["/usr/lib/x86_64-linux-gnu"],
                    linker_flags=["-lstdc++"],
                    compiler_flags=[])
    pl.add_profile(compiler_filter=["gcc"],
                    linker_flags=["-ldl", "-lm"],
                    compiler_flags=["-std=c++17", "-fPIC"])
    pl.add_profile(compiler_filter=["gcc"],
                    configuration_filter=["debug"],
                    compiler_flags=["--debug", "-g", "-O0"])

    # macos or clang only
    pl.add_profile(platform_filter=["Darwin"],
                    link_frameworks=["Metal", "MetalKit", "Cocoa", "IOKit", "CoreVideo", "QuartzCore"])
    pl.add_profile(compiler_filter=["clang"],
                    linker_flags=["-Wl,-rpath,/usr/local/lib"],
                    compiler_flags=["-std=c99", "-fmodules", "-ObjC", "-fPIC"])
    pl.add_profile(compiler_filter=["clang"],
                    configuration_filter=["debug"],
                    compiler_flags=["--debug", "-g"])

    # configs
    pl.add_profile(configuration_filter=["debug"], definitions=["_DEBUG", "PL_CONFIG_DEBUG"])

    #-----------------------------------------------------------------------------
    # [SECTION] app
    #-----------------------------------------------------------------------------

    # dcapp
    with pl.target("dcapp", pl.TargetType.DYNAMIC_LIBRARY, True):

        pl.set_output_directory(pilotlight_location_rel + "/out")
        pl.add_link_directories(pilotlight_location_rel + "/out")
        pl.add_include_directories(
            "src",
            pilotlight_location_rel + "/src",
            pilotlight_location_rel + "/libs",
            pilotlight_location_rel + "/extensions",
            pilotlight_location_rel + "/dependencies/stb")

        # used to decide hot reloading
        pl.set_hot_reload_target(pilotlight_location_rel + "/out/pilot_light")

        # list source files relative to the output directory
        os.chdir(dcapp_home_abs)
        sourceFiles = list_files_recursive("src", ".cpp")
        pl.add_source_files(*sourceFiles, "apps/dcapp.cpp")

        pl.set_output_binary("dcapp")

        # default config
        with pl.configuration("debug"):

            # win32
            with pl.platform("Windows"):
                with pl.compiler("msvc"):
                    pass
            
            # linux
            with pl.platform("Linux"):
                with pl.compiler("gcc"):
                    pl.add_include_directories("/usr/include/gdal", "/usr/include/libxml2")
                    pl.add_linker_flags("-lstdc++fs", "-lxml2", "-lpthread", "-lgdal", "-ldl", "-lz")
                    pass

            # mac os
            with pl.platform("Darwin"):
                with pl.compiler("clang"):
                    pass

    # dcapp-genheader
    with pl.target("dcapp-genheader", pl.TargetType.EXECUTABLE):

        pl.set_output_directory(pilotlight_location_rel + "/out")
        pl.add_link_directories(pilotlight_location_rel + "/out")
        pl.add_include_directories("src")

        # list source files relative to the output directory
        os.chdir(dcapp_home_abs)
        sourceFiles  = list_files_recursive("src", ".cpp")
        pl.add_source_files(*sourceFiles, "apps/dcapp-genheader.cpp")

        pl.set_output_binary("dcapp-genheader")

        # default config
        with pl.configuration("debug"):

            # win32
            with pl.platform("Windows"):
                with pl.compiler("msvc"):
                    pass
            
            # linux
            with pl.platform("Linux"):
                with pl.compiler("gcc"):
                    pl.add_include_directories("/usr/include/libxml2")
                    pl.add_linker_flags("-lstdc++fs", "-lxml2")
                    pass

            # mac os
            with pl.platform("Darwin"):
                with pl.compiler("clang"):
                    pass

    # sample "test"
    with pl.target("sample-test", pl.TargetType.DYNAMIC_LIBRARY):

        sample_test_location_abs = dcapp_home_abs + "/samples/test"
        sample_test_location_rel = os.path.relpath(dcapp_home_abs, sample_test_location_abs)

        pl.set_output_directory(sample_test_location_rel + "/logic")
        pl.add_link_directories(pilotlight_location_rel + "/out")
        pl.add_include_directories("src")

        # list source files relative to the output directory
        os.chdir(dcapp_home_abs)
        sourceFiles  = list_files_recursive("src", ".cpp")
        pl.add_source_files(*sourceFiles, "apps/dcapp-genheader.cpp")

        pl.set_output_binary("dcapp-genheader")

        # default config
        with pl.configuration("debug"):

            # win32
            with pl.platform("Windows"):
                with pl.compiler("msvc"):
                    pass
            
            # linux
            with pl.platform("Linux"):
                with pl.compiler("gcc"):
                    pl.add_include_directories("/usr/include/libxml2")
                    pl.add_linker_flags("-lstdc++fs", "-lxml2")
                    pass

            # mac os
            with pl.platform("Darwin"):
                with pl.compiler("clang"):
                    pass

#-----------------------------------------------------------------------------
# [SECTION] generate scripts
#-----------------------------------------------------------------------------

if plat.system() == "Windows":
    win32.generate_build(dcapp_home_abs + '/' + "build.bat")
elif plat.system() == "Darwin":
    outScript = dcapp_home_abs + '/' + "build.sh"
    apple.generate_build(outScript)
    os.chmod(outScript, 0o755)
elif plat.system() == "Linux":
    outScript = dcapp_home_abs + '/' + "build.sh"
    linux.generate_build(outScript)
    os.chmod(outScript, 0o755)
