#!/usr/bin/env bash
set -euo pipefail

DCAPP_HOME="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SNAPSHOT="$DCAPP_HOME/bin/dcapp-planet-snapshot.sh"
PLANET_DATA="$DCAPP_HOME/data/LDEM_45S_100M.planet.json"
OUT_DIR="$DCAPP_HOME/data"
EXAMPLE="${1:-}"

if [[ "$EXAMPLE" != "1" && "$EXAMPLE" != "2" && "$EXAMPLE" != "3" ]]; then
    echo "Usage: $0 1|2|3"
    echo "  1  geodetic Clavius"
    echo "  2  geodetic Shackleton with elevation shader"
    echo "  3  cartesian Clavius oblique"
    exit 1
fi

if [[ ! -f "$PLANET_DATA" ]]; then
    echo "Missing planet data: $PLANET_DATA"
    echo "Run ./scripts/download-planet-data.sh first, then rebuild if needed."
    exit 1
fi

mkdir -p "$OUT_DIR"

case "$EXAMPLE" in
    1)
        "$SNAPSHOT" \
            --planet-data "$PLANET_DATA" \
            --crs geodetic \
            --attitude-frame local-ned \
            --lat -58.62 --lon 345.27 --elevation 2000000 \
            --yaw 0 --pitch 0 --roll 0 \
            --width 1280 --height 720 \
            --fov 60 \
            --output "$OUT_DIR/clavius-geodetic.png"
        ;;
    2)
        "$SNAPSHOT" \
            --planet-data "$PLANET_DATA" \
            --crs geodetic \
            --attitude-frame local-ned \
            --lat -89.67 --lon 129.78 --elevation 1400000 \
            --yaw 0 --pitch 0 --roll 0 \
            --width 1024 --height 1024 \
            --fov 55 \
            --fragment-shader "$DCAPP_HOME/samples/planet/shaders/planet_elevation.frag" \
            --output "$OUT_DIR/shackleton-elevation.png"
        ;;
    3)
        "$SNAPSHOT" \
            --planet-data "$PLANET_DATA" \
            --crs cartesian \
            --attitude-frame cartesian-rpy \
            --x -494826 --y -3190740 --z 1882148 \
            --roll 0 --pitch 52 --yaw 158 \
            --width 1280 --height 720 \
            --fov 60 \
            --output "$OUT_DIR/cartesian-oblique.png"
        ;;
esac
