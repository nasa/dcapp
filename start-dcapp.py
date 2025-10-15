#!/usr/bin/env python3

import os
import sys

# format:
# ./start-dcapp.py <xml config> <optional args>
# will use the test XML by default if no args supplied

# TODO implement windows support

if __name__ == "__main__":

    # get dcapp home
    dcapp_home = os.path.dirname(os.path.abspath(__file__))

    # get config file
    args = [os.path.abspath(dcapp_home + "/samples/test/test.xml")]
    if len(sys.argv) > 1:
        args = sys.argv[1:]
    dcapp_config = args[0]
    dcapp_args   = args[1:]

    # run test sample
    run_dir = f"{dcapp_home}/pilotlight/out"
    dcapp_config_path = os.path.relpath(dcapp_config, start=run_dir)

    os.chdir(run_dir)
    cmd = f"./pilot_light -a dcapp {dcapp_config_path} {', '.join(map(str, dcapp_args))}"
    print(cmd)
    os.system(cmd)
