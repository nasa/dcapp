#!/usr/bin/env bash
set -e

DCAPP_HOME="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RUN_DIR="$DCAPP_HOME/pilotlight/out"

if [ $# -eq 0 ]; then
    CONFIG="$DCAPP_HOME/samples/test/test.xml"
else
    CONFIG="$1"
    shift
fi

CONFIG_REL="$(realpath --relative-to="$RUN_DIR" "$CONFIG")"

cd "$RUN_DIR"
pwd
cmd="./pilot_light -a dcapp $CONFIG_REL $*"
echo "$cmd"
exec $cmd
