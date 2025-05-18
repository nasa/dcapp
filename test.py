#!/usr/bin/env python3

import os
import sys

if __name__ == "__main__":

    # get dcapp home
    dcappHome = os.path.dirname(os.path.abspath(__file__))

    # run test sample
    binDir = os.path.join(dcappHome, "pilotlight", "out")
    plExe = os.path.join(binDir, "pilot_light")
    os.chdir(binDir)
    os.system(f"{plExe} -a dcapp {sys.argv[1:]}")
    
    # # build dcapp
    # buildScript = os.path.join(dcappHome, "scripts", "gen-build.py")
    # os.system(f"python3 {buildScript}")

    # # build sample
    # # TODO make this cross-platform
    # genHeader = os.path.join(dcappHome, "pilotlight", "out", "dcapp-genheader")
    # testDir = os.path.join(dcappHome, "samples", "test")
    # os.system(f"{genHeader} {os.path.join(testDir, "test.xml")}")
    # gcc -std=c++17 -g -O2 -fPIC -shared logic/logic.cpp -o logic/logic.so
