#!/usr/bin/env python3

import os
import sys

# format:
# ./start-dcapp.py <xml config> <optional args>
# will use the test XML by default if no args supplied

# TODO implement windows support

if __name__ == "__main__":

    # get dcapp home
    dcappHome = os.path.dirname(os.path.abspath(__file__))

    # get config file
    dcappArgs = os.path.abspath(dcappHome + "/samples/test/test.xml")
    if len(sys.argv) > 1:
        dcappArgs = sys.argv[1:]

    # run test sample
    os.chdir(f"{dcappHome}/pilotlight/out")
    cmd = f"./pilot_light -a dcapp {dcappArgs}"
    print(cmd)
    os.system(cmd)
