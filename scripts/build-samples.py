#!/usr/bin/env python3

import os
import platform as plat
import sys

if __name__ == "__main__":

    # get dcapp home
    dcappHome = os.path.abspath(os.path.dirname(__file__) + "/..")

    # paths
    genHeader = dcappHome + "/pilotlight/out/dcapp-genheader"
    testDir = dcappHome + "/samples/test"

    # platform specific execution
    if plat.system() == "Windows":
        pass # TODO implement
    elif plat.system() == "Darwin":
        os.chdir(testDir)
        os.system(f"{genHeader} test.xml")
        os.system("clang -std=c++17 -g -O2 -fPIC -shared logic/logic.cpp -o logic/logic.so")
    elif plat.system() == "Linux":
        os.chdir(testDir)
        os.system(f"{genHeader} test.xml")
        os.system("gcc -std=c++17 -g -O2 -fPIC -shared logic/logic.cpp -o logic/logic.so")
