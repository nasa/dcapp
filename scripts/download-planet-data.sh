#!/usr/bin/env bash
set -euo pipefail

DCAPP_HOME="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

DATA_DIR="${DCAPP_PLANET_DATA_DIR:-$DCAPP_HOME/data}"
SOURCE_DIR="$DATA_DIR"
CHUNK_DIR="$DATA_DIR"

IMG_URL="https://imbrium.mit.edu/DATA/LOLA_GDR/POLAR/IMG/LDEM_45S_100M.IMG"
LBL_URL="https://imbrium.mit.edu/DATA/LOLA_GDR/POLAR/IMG/LDEM_45S_100M.LBL"
IMG_FILE="$SOURCE_DIR/LDEM_45S_100M.IMG"
LBL_FILE="$SOURCE_DIR/LDEM_45S_100M.LBL"
PLANET_JSON="$CHUNK_DIR/LDEM_45S_100M.planet.json"

FORCE=false
EXTRA_ARGS=()

while [ $# -gt 0 ]; do
    case "$1" in
        -h|--help)
            echo "Usage: ./scripts/download-planet-data.sh [--force] [chunkgen options]"
            echo ""
            echo "Downloads the LOLA LDEM_45S_100M lunar DEM and generates planet chunks."
            echo ""
            echo "Environment:"
            echo "  DCAPP_PLANET_DATA_DIR  Override output directory"
            echo "                         default: data"
            echo ""
            echo "Options:"
            echo "  --force                Regenerate chunks even if the .planet.json exists"
            echo "  -h, --help             Show this help"
            echo ""
            echo "Any other options are passed through to dcapp-planet-chunkgen."
            exit 0
            ;;
        --force)
            FORCE=true
            ;;
        *)
            EXTRA_ARGS+=("$1")
            ;;
    esac
    shift
done

echo "========================================"
echo "Planet Data Download"
echo "========================================"
echo "Data directory: $DATA_DIR"

mkdir -p "$DATA_DIR"

if [ ! -f "$IMG_FILE" ]; then
    echo "Downloading LDEM_45S_100M.IMG..."
    curl -L -o "$IMG_FILE" "$IMG_URL"
else
    echo "LDEM_45S_100M.IMG already downloaded, skipping."
fi

if [ ! -f "$LBL_FILE" ]; then
    echo "Downloading LDEM_45S_100M.LBL..."
    curl -L -o "$LBL_FILE" "$LBL_URL"
else
    echo "LDEM_45S_100M.LBL already downloaded, skipping."
fi

if [ "$FORCE" = true ] || [ ! -f "$PLANET_JSON" ]; then
    echo ""
    echo "Running chunkgen..."
    if [ ${#EXTRA_ARGS[@]} -gt 0 ]; then
        "$DCAPP_HOME/bin/dcapp-planet-chunkgen.sh" "$LBL_FILE" "$CHUNK_DIR" --radius 1737400 "${EXTRA_ARGS[@]}"
    else
        "$DCAPP_HOME/bin/dcapp-planet-chunkgen.sh" "$LBL_FILE" "$CHUNK_DIR" --radius 1737400
    fi
else
    echo "LDEM_45S_100M.planet.json already exists, skipping chunkgen. Use --force to regenerate."
fi

echo ""
echo "Planet data ready:"
echo "  $PLANET_JSON"
