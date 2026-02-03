#!/usr/bin/env bash
set -e

DCAPP_HOME="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
RUN_DIR="$DCAPP_HOME/pilotlight/out"

if [ $# -eq 0 ]; then
    echo "Usage: dcapp-genheader <config.xml> [output.h]"
    exit 1
fi

CONFIG="$1"
shift

# Get absolute path of config (works on both Linux and macOS)
CONFIG_ABS="$(cd "$(dirname "$CONFIG")" && pwd)/$(basename "$CONFIG")"

# Get relative path using only bash
get_relative_path() {
    local source="$1"
    local target="$2"

    local common_part="$source"
    local result=""

    while [[ "${target#"$common_part"}" == "${target}" ]]; do
        common_part="$(dirname "$common_part")"
        result="../$result"
    done

    if [[ "$common_part" == "/" ]]; then
        result="$result${target:1}"
    else
        result="$result${target#"$common_part"/}"
    fi

    echo "$result"
}

CONFIG_REL="$(get_relative_path "$RUN_DIR" "$CONFIG_ABS")"

cd "$RUN_DIR"
cmd="./dcapp-genheader $CONFIG_REL $*"
echo "$cmd"
exec $cmd
