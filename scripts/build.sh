#!/usr/bin/env bash
set -e

# DCAPP Build Script
# Builds pilotlight (with _experimental suffix) and dcapp apps/samples

DCAPP_HOME="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Default configuration
CONFIG="release"
FORCE=0

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -c)
            CONFIG="$2"
            shift 2
            ;;
        -f)
            FORCE=1
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [-c <config>] [-f]"
            echo "  -c <config>  Build configuration (debug, release)"
            echo "               Default: release"
            echo "  -f           Force all build stages"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Determine platform
if [[ "$OSTYPE" == "darwin"* ]]; then
    PLATFORM="macos"
    PL_BUILD_SCRIPT="build_macos.sh"
else
    PLATFORM="linux"
    PL_BUILD_SCRIPT="build_linux.sh"
fi

PILOTLIGHT_CONFIG="${CONFIG}_experimental"
PILOTLIGHT_OUT="$DCAPP_HOME/pilotlight/out"
PILOTLIGHT_STAMP="$PILOTLIGHT_OUT/.dcapp-pilotlight-${PLATFORM}-${PILOTLIGHT_CONFIG}.stamp"
DCAPP_STAMP="$PILOTLIGHT_OUT/.dcapp-${PLATFORM}-${CONFIG}.stamp"
DCAPP_BUILD_STAMP="$PILOTLIGHT_OUT/.dcapp-build-${PLATFORM}-${CONFIG}.stamp"
DCAPP_HEAD="$(git -C "$DCAPP_HOME" rev-parse HEAD 2>/dev/null || true)"
PILOTLIGHT_HEAD="$(git -C "$DCAPP_HOME/pilotlight" rev-parse HEAD 2>/dev/null || true)"
DCAPP_DIRTY=0
if ! git -C "$DCAPP_HOME" diff --quiet HEAD -- . ':(exclude)pilotlight' 2>/dev/null; then
    DCAPP_DIRTY=1
fi
PILOTLIGHT_DIRTY=0
if ! git -C "$DCAPP_HOME/pilotlight" diff --quiet HEAD -- 2>/dev/null; then
    PILOTLIGHT_DIRTY=1
fi

if [[ "$PLATFORM" == "macos" ]]; then
    DCAPP_APP_OUTPUTS=(
        "$PILOTLIGHT_OUT/libdc_draw_ext.dylib"
        "$PILOTLIGHT_OUT/libdc_draw_backend_ext.dylib"
        "$PILOTLIGHT_OUT/libpl_planet_processor_ext.dylib"
        "$PILOTLIGHT_OUT/libpl_planet_ext.dylib"
        "$PILOTLIGHT_OUT/libdcapp.dylib"
        "$PILOTLIGHT_OUT/dcapp-genheader"
        "$PILOTLIGHT_OUT/dcapp-validate"
        "$PILOTLIGHT_OUT/libdcapp-planet-chunkgen.dylib"
        "$PILOTLIGHT_OUT/libdcapp-planet-snapshot.dylib"
    )
    SAMPLE_LIB_EXT="dylib"
else
    DCAPP_APP_OUTPUTS=(
        "$PILOTLIGHT_OUT/libdc_draw_ext.so"
        "$PILOTLIGHT_OUT/libdc_draw_backend_ext.so"
        "$PILOTLIGHT_OUT/libpl_planet_processor_ext.so"
        "$PILOTLIGHT_OUT/libpl_planet_ext.so"
        "$PILOTLIGHT_OUT/libdcapp.so"
        "$PILOTLIGHT_OUT/dcapp-genheader"
        "$PILOTLIGHT_OUT/dcapp-validate"
        "$PILOTLIGHT_OUT/libdcapp-planet-chunkgen.so"
        "$PILOTLIGHT_OUT/libdcapp-planet-snapshot.so"
    )
    SAMPLE_LIB_EXT="so"
fi
DCAPP_SAMPLE_OUTPUTS=(
    "$DCAPP_HOME/samples/drawfunction1/logic/liblogic.$SAMPLE_LIB_EXT"
    "$DCAPP_HOME/samples/drawfunction2/logic/liblogic.$SAMPLE_LIB_EXT"
    "$DCAPP_HOME/samples/drawfunction3/logic/liblogic.$SAMPLE_LIB_EXT"
    "$DCAPP_HOME/samples/drawfunction4/logic/liblogic.$SAMPLE_LIB_EXT"
    "$DCAPP_HOME/samples/lissajous/logic/liblogic.$SAMPLE_LIB_EXT"
    "$DCAPP_HOME/samples/planet/logic/liblogic.$SAMPLE_LIB_EXT"
    "$DCAPP_HOME/samples/ptz/logic/liblogic.$SAMPLE_LIB_EXT"
)

all_outputs_exist() {
    local output
    for output in "$@"; do
        [[ -f "$output" ]] || return 1
    done
    return 0
}

if [[ "$FORCE" -eq 1 ||
      ( -n "$DCAPP_HEAD" &&
        ( ! -f "$DCAPP_STAMP" || "$(cat "$DCAPP_STAMP")" != "$DCAPP_HEAD" ) ) ]]; then
    echo "Cleaning pilotlight outputs..."
    rm -rf "$PILOTLIGHT_OUT" "$DCAPP_HOME/pilotlight/out-temp" "$DCAPP_HOME/pilotlight/out_temp"
    mkdir -p "$PILOTLIGHT_OUT"
    if [[ -n "$DCAPP_HEAD" ]]; then
        echo "$DCAPP_HEAD" > "$DCAPP_STAMP"
    fi
fi

echo "========================================"
echo "DCAPP Build"
echo "========================================"
echo "Configuration: $CONFIG"
echo "Platform: $PLATFORM"
echo "========================================"

# Step 1: Build pilotlight with _experimental suffix
echo ""
BUILD_PILOTLIGHT=0
if [[ "$FORCE" -eq 1 ||
      ! -f "$PILOTLIGHT_OUT/pilot_light" ||
      -z "$PILOTLIGHT_HEAD" ||
      "$PILOTLIGHT_DIRTY" -eq 1 ||
      ! -f "$PILOTLIGHT_STAMP" ||
      "$(cat "$PILOTLIGHT_STAMP")" != "$PILOTLIGHT_HEAD" ]]; then
    BUILD_PILOTLIGHT=1
    echo "[1/3] Building pilotlight..."
    cd "$DCAPP_HOME/pilotlight/src"
    bash "$PL_BUILD_SCRIPT" -c "$PILOTLIGHT_CONFIG"

    if [[ -n "$PILOTLIGHT_HEAD" ]]; then
        mkdir -p "$PILOTLIGHT_OUT"
        echo "$PILOTLIGHT_HEAD" > "$PILOTLIGHT_STAMP"
    fi
else
    echo "[1/3] Skipping pilotlight; cached $PILOTLIGHT_CONFIG build is current."
fi

BUILD_DCAPP=0
if [[ "$FORCE" -eq 1 ||
      "$BUILD_PILOTLIGHT" -eq 1 ||
      -z "$DCAPP_HEAD" ||
      "$DCAPP_DIRTY" -eq 1 ||
      ! -f "$DCAPP_BUILD_STAMP" ||
      "$(cat "$DCAPP_BUILD_STAMP" 2>/dev/null)" != "$DCAPP_HEAD" ]] ||
      ! all_outputs_exist "${DCAPP_APP_OUTPUTS[@]}" "${DCAPP_SAMPLE_OUTPUTS[@]}"; then
    BUILD_DCAPP=1
fi

echo ""
if [[ "$BUILD_DCAPP" -eq 1 ]]; then
    echo "[2/3] Building dcapp apps..."
    bash "$DCAPP_HOME/scripts/internal/build-apps-${PLATFORM}.sh" -c "$CONFIG"
else
    echo "[2/3] Skipping dcapp apps; cached $CONFIG build is current."
fi

echo ""
if [[ "$BUILD_DCAPP" -eq 1 ]]; then
    echo "[3/3] Building dcapp samples..."
    bash "$DCAPP_HOME/scripts/internal/build-samples-${PLATFORM}.sh" -c "$CONFIG"
    if [[ -n "$DCAPP_HEAD" && "$DCAPP_DIRTY" -eq 0 ]]; then
        mkdir -p "$PILOTLIGHT_OUT"
        echo "$DCAPP_HEAD" > "$DCAPP_BUILD_STAMP"
    fi
else
    echo "[3/3] Skipping dcapp samples; cached $CONFIG build is current."
fi

echo ""
echo "========================================"
echo "DCAPP Build Complete"
echo "========================================"
