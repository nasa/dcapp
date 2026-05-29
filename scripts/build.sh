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
DCAPP_HEAD="$(git -C "$DCAPP_HOME" rev-parse HEAD 2>/dev/null || true)"
PILOTLIGHT_HEAD="$(git -C "$DCAPP_HOME/pilotlight" rev-parse HEAD 2>/dev/null || true)"
PILOTLIGHT_DIRTY=0
if ! git -C "$DCAPP_HOME/pilotlight" diff --quiet HEAD -- 2>/dev/null; then
    PILOTLIGHT_DIRTY=1
fi

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
if [[ "$FORCE" -eq 1 ||
      ! -f "$PILOTLIGHT_OUT/pilot_light" ||
      -z "$PILOTLIGHT_HEAD" ||
      "$PILOTLIGHT_DIRTY" -eq 1 ||
      ! -f "$PILOTLIGHT_STAMP" ||
      "$(cat "$PILOTLIGHT_STAMP")" != "$PILOTLIGHT_HEAD" ]]; then
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

# Step 2: Build dcapp apps
echo ""
echo "[2/3] Building dcapp apps..."
bash "$DCAPP_HOME/scripts/internal/build-apps-${PLATFORM}.sh" -c "$CONFIG"

# Step 3: Build dcapp samples
echo ""
echo "[3/3] Building dcapp samples..."
bash "$DCAPP_HOME/scripts/internal/build-samples-${PLATFORM}.sh" -c "$CONFIG"

echo ""
echo "========================================"
echo "DCAPP Build Complete"
echo "========================================"
