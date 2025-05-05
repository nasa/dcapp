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
pilotlight_location = os.path.dirname(os.path.abspath(__file__)) + "/../pilotlight"

# if provided, use pilotlight location in input (absolute)
if len(sys.argv) > 1:
    pilotlight_location = sys.argv[1]
    if not os.path.isabs(pilotlight_location):
        pilotlight_location = os.path.abspath(os.getcwd() + "/" + pilotlight_location)

# append to path to import packages
sys.path.append(pilotlight_location)

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
working_directory = os.path.dirname(os.path.abspath(__file__)) + "/.."

# now, update pilotlight_location to be relative to the output directory
pilotlight_location = os.path.relpath(pilotlight_location, working_directory)

with pl.project("dcapp"):
    
    # used to decide hot reloading
    pl.set_hot_reload_target(pilotlight_location + "/out/pilot_light")

    # project wide settings
    pl.set_output_directory(pilotlight_location + "/out")
    pl.add_link_directories(pilotlight_location + "/out")
    pl.add_include_directories(
        "apps",
        "src",
        pilotlight_location + "/src",
        pilotlight_location + "/libs",
        pilotlight_location + "/extensions",
        pilotlight_location + "/dependencies/stb")

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
    pl.add_profile(compiler_filter=["msvc"],
                    configuration_filter=["release"],
                    compiler_flags=["-O2", "-MD"])


    # linux or gcc only
    pl.add_profile(platform_filter=["Linux"],
                    link_directories=["/usr/lib/x86_64-linux-gnu"],
                    include_directories=["/usr/include/gdal", "/usr/include/libxml2"],
                    linker_flags=["-lstdc++", "-lstdc++fs", "-lxml2", "-lSDL2", "-lpthread", "-lgdal", "-ldl", "-lz"],
                    compiler_flags=[])
    pl.add_profile(compiler_filter=["gcc"],
                    linker_flags=["-ldl", "-lm"],
                    compiler_flags=["-std=c++17", "-fPIC"])
    pl.add_profile(compiler_filter=["gcc"],
                    configuration_filter=["debug"],
                    compiler_flags=["--debug", "-g", "-O0"])
    pl.add_profile(compiler_filter=["gcc"],
                    configuration_filter=["release"],
                    compiler_flags=["-O2"])

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
    pl.add_profile(configuration_filter=["release"], definitions=["NDEBUG", "PL_CONFIG_RELEASE"])

    #-----------------------------------------------------------------------------
    # [SECTION] extensions
    #-----------------------------------------------------------------------------

    # with pl.target("pl_example_ext", pl.TargetType.DYNAMIC_LIBRARY, True):

    #     pl.add_source_files("extensions/pl_example_ext.c")
    #     pl.set_output_binary("pl_example_ext")

    #     # default config
    #     with pl.configuration("debug"):

    #         # win32
    #         with pl.platform("Windows"):

    #             with pl.compiler("msvc"):
    #                 pass

    #         # linux
    #         with pl.platform("Linux"):
    #             with pl.compiler("gcc"):
    #                 pass

    #         # macos
    #         with pl.platform("Darwin"):
    #             with pl.compiler("clang"):
    #                 pass

    #     # release
    #     with pl.configuration("release"):

    #         # win32
    #         with pl.platform("Windows"):

    #             with pl.compiler("msvc"):
    #                 pass

    #         # linux
    #         with pl.platform("Linux"):
    #             with pl.compiler("gcc"):
    #                 pass

    #         # macos
    #         with pl.platform("Darwin"):
    #             with pl.compiler("clang"):
    #                 pass

    #-----------------------------------------------------------------------------
    # [SECTION] app
    #-----------------------------------------------------------------------------

    with pl.target("dcapp", pl.TargetType.DYNAMIC_LIBRARY, True):

        sourceFiles  = list_files_recursive("src", ".cpp")
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
                    pass

            # mac os
            with pl.platform("Darwin"):
                with pl.compiler("clang"):
                    pass

        # release
        with pl.configuration("release"):

            # win32
            with pl.platform("Windows"):
                with pl.compiler("msvc"):
                    pass

            # linux
            with pl.platform("Linux"):
                with pl.compiler("gcc"):
                    pass

            # mac os
            with pl.platform("Darwin"):
                with pl.compiler("clang"):
                    pass

    # with pl.target("dcapp-genheader", pl.TargetType.EXECUTABLE, True):

    #     sourceFiles  = list_files_recursive("src", ".cpp")
    #     pl.add_source_files(*sourceFiles, "apps/dcapp-genheader.cpp")

    #     pl.set_output_binary("dcapp-genheader")

    #     # default config
    #     with pl.configuration("debug"):

    #         # win32
    #         with pl.platform("Windows"):
    #             with pl.compiler("msvc"):
    #                 pass
            
    #         # linux
    #         with pl.platform("Linux"):
    #             with pl.compiler("gcc"):
    #                 pass

    #         # mac os
    #         with pl.platform("Darwin"):
    #             with pl.compiler("clang"):
    #                 pass

    #     # release
    #     with pl.configuration("release"):

    #         # win32
    #         with pl.platform("Windows"):
    #             with pl.compiler("msvc"):
    #                 pass

    #         # linux
    #         with pl.platform("Linux"):
    #             with pl.compiler("gcc"):
    #                 pass

    #         # mac os
    #         with pl.platform("Darwin"):
    #             with pl.compiler("clang"):
    #                 pass

    # with pl.target("dcapp-gendem", pl.TargetType.EXECUTABLE, True):

    #     sourceFiles  = list_files_recursive("src", ".cpp")
    #     pl.add_source_files(*sourceFiles, "apps/dcapp-gendem.cpp")

    #     pl.set_output_binary("dcapp-gendem")

    #     # default config
    #     with pl.configuration("debug"):

    #         # win32
    #         with pl.platform("Windows"):
    #             with pl.compiler("msvc"):
    #                 pass
            
    #         # linux
    #         with pl.platform("Linux"):
    #             with pl.compiler("gcc"):
    #                 pass

    #         # mac os
    #         with pl.platform("Darwin"):
    #             with pl.compiler("clang"):
    #                 pass

    #     # release
    #     with pl.configuration("release"):

    #         # win32
    #         with pl.platform("Windows"):
    #             with pl.compiler("msvc"):
    #                 pass

    #         # linux
    #         with pl.platform("Linux"):
    #             with pl.compiler("gcc"):
    #                 pass

    #         # mac os
    #         with pl.platform("Darwin"):
    #             with pl.compiler("clang"):
    #                 pass
               
#-----------------------------------------------------------------------------
# [SECTION] generate scripts
#-----------------------------------------------------------------------------

if plat.system() == "Windows":
    win32.generate_build(working_directory + '/' + "build.bat")
elif plat.system() == "Darwin":
    apple.generate_build(working_directory + '/' + "build.sh")
elif plat.system() == "Linux":
    outScript = working_directory + '/' + "build.sh"
    linux.generate_build(outScript)
    os.chmod(outScript, 0o755)
    os.system(outScript)
