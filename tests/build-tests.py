#!/usr/bin/env python3

import glob
import os
import platform as plat
import sys

# list all files matching extensions
def list_files(directory, *extensions, recursive=False):

    # prevent globbing root
    if not directory.strip():
        directory = "."

    pattern = []
    for extension in extensions:
        pattern += glob.glob(f"{directory}/**/*{extension}", recursive=recursive)
    return pattern

if __name__ == "__main__":

    # get dcapp home
    file_dir_rel = os.path.dirname(__file__)
    if not file_dir_rel:
        file_dir_rel = "."
    dcapp_home = os.path.abspath(file_dir_rel + "/..")

    # paths
    bin_dir   = dcapp_home + "/bin"
    tests_dir = dcapp_home + "/tests"
    src_dir   = dcapp_home + "/src"

    # platform specific execution
    if plat.system() == "Windows":
        print("windows not implemented", file=sys.stderr)
    elif plat.system() == "Darwin":
        os.chdir(tests_dir)
        os.system(f"clang -std=c++17 -g trick.cpp {src_dir}/trick.cpp {src_dir}/sock.cpp -o {bin_dir}/test-trick")
    elif plat.system() == "Linux":
        print("linux not implemented", file=sys.stderr)
