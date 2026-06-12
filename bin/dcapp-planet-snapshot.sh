#!/usr/bin/env bash
set -e

DCAPP_HOME="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
RUN_DIR="$DCAPP_HOME/pilotlight/out"

to_abs_existing() {
    local path="$1"
    if [[ "$path" == /* ]]; then
        echo "$path"
    else
        echo "$(cd "$(dirname "$path")" && pwd)/$(basename "$path")"
    fi
}

to_abs_output() {
    local path="$1"
    local dir
    dir="$(dirname "$path")"
    mkdir -p "$dir"
    if [[ "$path" == /* ]]; then
        echo "$path"
    else
        echo "$(cd "$dir" && pwd)/$(basename "$path")"
    fi
}

ARGS=()
while [ $# -gt 0 ]; do
    case "$1" in
        -h|--help)
            ARGS+=("--snapshot-help")
            ;;
        --planet-data|--vertex-shader|--fragment-shader)
            key="$1"
            shift
            ARGS+=("$key" "$(to_abs_existing "$1")")
            ;;
        --output)
            shift
            ARGS+=("--output" "$(to_abs_output "$1")")
            ;;
        *)
            ARGS+=("$1")
            ;;
    esac
    shift
done

cd "$RUN_DIR"
cmd=("./pilot_light" "-a" "dcapp-planet-snapshot" "${ARGS[@]}")
echo "${cmd[*]}"
exec "${cmd[@]}"
