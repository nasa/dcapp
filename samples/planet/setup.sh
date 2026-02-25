#!/usr/bin/env bash
set -e

SAMPLE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DCAPP_HOME="$(cd "$SAMPLE_DIR/../.." && pwd)"

CACHE_DIR="$DCAPP_HOME/cache"
IMG_URL="https://imbrium.mit.edu/DATA/LOLA_GDR/POLAR/IMG/LDEM_45S_100M.IMG"
LBL_URL="https://imbrium.mit.edu/DATA/LOLA_GDR/POLAR/IMG/LDEM_45S_100M.LBL"
IMG_FILE="$CACHE_DIR/LDEM_45S_100M.IMG"
LBL_FILE="$CACHE_DIR/LDEM_45S_100M.LBL"

echo "========================================"
echo "Planet Sample Setup"
echo "========================================"

mkdir -p "$CACHE_DIR"

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

echo ""
echo "Running chunkgen..."
"$DCAPP_HOME/bin/dcapp-planet-chunkgen.sh" "$LBL_FILE" "$CACHE_DIR"
