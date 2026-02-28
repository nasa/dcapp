#!/usr/bin/env bash
set -e

# DCAPP Build Script
# Builds pilotlight (with _experimental suffix) and dcapp apps/samples

DCAPP_HOME="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Default configuration
CONFIG="release"

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -c)
            CONFIG="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [-c <config>]"
            echo "  -c <config>  Build configuration (debug, release)"
            echo "               Default: debug"
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

echo "========================================"
echo "DCAPP Build"
echo "========================================"
echo "Configuration: $CONFIG"
echo "Platform: $PLATFORM"
echo "========================================"

# Step 1: Build pilotlight with _experimental suffix
echo ""
echo "[1/3] Building pilotlight..."
cd "$DCAPP_HOME/pilotlight/src"
bash "$PL_BUILD_SCRIPT" -c "${CONFIG}_experimental"

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
