#!/usr/bin/env python3

import os
import platform as plat
import sys

if __name__ == "__main__":

    # get dcapp home
    file_dir_rel = os.path.dirname(__file__)
    if not file_dir_rel:
        file_dir_rel = "."
    dcappHome = os.path.abspath(file_dir_rel + "/..")

    # paths
    genHeader = dcappHome + "/pilotlight/out/dcapp-genheader"

    testDirs = [dcappHome + "/samples/test", dcappHome + "/samples/local"]

    # platform specific execution
    if plat.system() == "Windows":
        pass # TODO implement
    elif plat.system() == "Darwin":
        for testDir in testDirs:
            if os.path.exists(testDir):
                os.chdir(testDir)
                os.system(f"{genHeader} test.xml")
                os.system("clang -g -O2 -fPIC -shared logic/logic.cpp -o logic/logic.so")
    elif plat.system() == "Linux":
        for testDir in testDirs:
            if os.path.exists(testDir):
                os.chdir(testDir)
                os.system(f"{genHeader} test.xml")
                os.system("gcc -g -O2 -fPIC -shared logic/logic.cpp -o logic/logic.so")
