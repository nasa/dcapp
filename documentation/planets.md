# Planet Processor Coordinate System Reference

This documents the coordinate conventions used by the planet processor
(`pl_planet_processor_ext.c`). The chunkgen tool must produce data
consistent with these conventions.

## Heightmap Grid (iX, iZ)

- The processor creates a grid of size `iSize x iSize` where
  `iSize = (1 << iLogSize) + 1` (e.g. 4097 for tile_size=4096).
- Indices `0..tile_size-1` hold the tile's own pixel data.
- Index `tile_size` (= `iSize-1`) is the halo row/column from
  the adjacent tile.

## Stereographic Mapping (`pl__get_cartesian`)

```
fX = iX * mpp + fMinExtent + tCenter.x
fZ = iZ * mpp + fMinExtent + tCenter.z

where mpp = fMetersPerPixel, fMinExtent = -iSize * mpp / 2
```

- `fX` increases with `iX` (eastward in stereographic space).
- `fZ` increases with `iZ`.
- `tCenter` is the tile center in stereographic coordinates.

## Image Loading Order

- Image row `j` maps directly to grid `iZ = j` (NO vertical flip
  inside the processor itself).
- Image row 0 goes to `iZ=0`, which maps to the **minimum fZ**
  position in the tile (`fMinExtent + tCenter.z`).
- GDAL rasters are stored north-up: row 0 = max stereographic Y.
  The processor maps iZ=0 = min fZ. These are **opposite**, so the
  chunkgen must **flip each tile PNG vertically** before the
  processor consumes it (row 0 in the PNG = min Y within that tile).
- Tile centers are computed from the **lower-left** of the DEM
  (minimum stereographic Y) and increase with row. Tile row 0 has
  the most-negative center (near the pole for south-pole stereo).
  To match data to centers, the chunkgen extracts tiles from the DEM
  in **reverse row order**: `srcwin_y = (rows-1-row) * tile_size`.
  This ensures tile row 0 gets the bottom of the DEM (min Y area,
  matching its min-Y center), and tile row N gets the top (max Y).
- The processor's neighbor convention (line 472/484 in the processor)
  expects `row+1 = +iZ direction = increasing fZ`. The lower-left
  origin + addition formula satisfies this: `center_{R+1} - center_R
  = tile_size * mpp` (positive).

## Halo Layout

Main heightmap data (`auHeightMapData`):
- `[iSize-1, *]` = east neighbor's west column (column 0)
- `[*, iSize-1]` = neighbor's row in the +iZ direction
- `[iSize-1, iSize-1]` = diagonal neighbor's corner pixel

Halo data for normals (`auHaloHeightMapData`):

| Index Range | Direction | Description |
|---|---|---|
| `0 .. iSize-2` | -iZ ("north") | One row beyond iZ=0 edge |
| `(iSize-1)*1 .. (iSize-1)*2-1` | +iX ("east") | One column beyond iX=iSize-1 |
| `(iSize-1)*2 .. (iSize-1)*3-1` | +iZ ("south") | One row beyond iZ=iSize-1 |
| `(iSize-1)*3 .. (iSize-1)*4-1` | -iX ("west") | One column beyond iX=0 |
| `(iSize-1)*4` | NE corner | Beyond (iSize-1, 0) |
| `(iSize-1)*4 + 1` | SW corner | Beyond (0, iSize-1) |

## Inverse Stereographic (south-pole)

```c
longitude = atan2(fX, fZ);
r         = hypot(fX, fZ);
latitude  = -PI/2 + 2 * atan(r / (2 * radius));
```

- At `r=0`: `latitude = -90 deg` (south pole).
- As `r` increases, latitude increases toward the equator.

## Forward Stereographic (chunkgen lat/lon to center)

```c
fR       = 2 * radius * tan(PI/4 - latitude / 2);
center.x = fR * sin(longitude);
center.z = fR * cos(longitude);
```

The chunkgen uses a different latitude convention
(`lat_chunkgen = PI/2 - 2*atan(r/(2*R))`), but the round-trip
through the forward projection recovers the exact stereographic
coordinates regardless of sign convention.

## WGS to Pilotlight Coordinate Frame

```
WGS -> PL:
  x -> -x
  y ->  z
  z ->  y
```

## 3D Cartesian (Pilotlight frame)

```c
X = -radius * cos(lat) * cos(lon)   // negated (WGS x -> PL -x)
Y =  radius * sin(lat)              // Y-up; south pole at Y = -radius
Z =  radius * cos(lat) * sin(lon)
```

Final vertex position = `surface_point + normal * elevation`.

## Normal Computation (`pl__get_normal`)

- Uses central differences: `eL/eR` (`iX-1`/`iX+1`), `eD/eU` (`iZ-1`/`iZ+1`).
- `tX = cartesian(eR) - cartesian(eL)` (increasing iX direction)
- `tZ = cartesian(eU) - cartesian(eD)` (increasing iZ direction)
- `normal = cross(tZ, tX)`
- Halo elements provide neighbor data for edge normals.
