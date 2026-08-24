# Coordinate frames

This reference covers PilotLight world/camera coordinates and the projection
used by planet terrain. Normal 2D XML layout is covered in
[primitives.md](primitives.md#positioning-and-alignment).

## World and camera space

PilotLight world space is right-handed and Y-up:

| Axis | Direction |
|------|-----------|
| `+Y` | up |
| `+Z` | forward |
| `-X` | right |

The view matrix flips X and Y while preserving handedness. Camera space is
therefore `+X` right, `-Y` up, and `+Z` forward.

## Matrices

Matrices are column-major, translation is stored in `col[3]`, and vectors are
multiplied on the right:

```text
v' = M * v
v_clip = Projection * View * Model * v_local
```

The camera rotation fields use these axes:

| Field | Rotation axis |
|-------|---------------|
| pitch | camera right (X) |
| yaw | camera up (Y) |
| roll | camera forward (Z) |

Their matrix composition is `Y * X * Z` (yaw, pitch, roll).

## Projection and NDC

Perspective and orthographic projections use Vulkan clip coordinates:

| Coordinate | Range |
|------------|-------|
| NDC X, Y | -1 to +1 |
| NDC Z, standard depth | 0 at the near plane, 1 at the far plane |
| NDC Z, reverse depth | 1 at the near plane, 0 at the far plane |

For a standard perspective projection, with vertical field of view `fovy`,
aspect ratio `a`, and near/far distances `n` and `f`:

```text
P.col[0].x = 1 / (a * tan(fovy / 2))
P.col[1].y = 1 / tan(fovy / 2)
P.col[2].z = f / (f - n)
P.col[2].w = 1
P.col[3].z = -n * f / (f - n)
```

Reverse-Z changes the depth terms to:

```text
P.col[2].z = n / (n - f)
P.col[3].z = -n * f / (n - f)
```

## Planet terrain projection

Planet tiles use a polar stereographic projection. For the south-pole form,
the projection parameters are:

```text
phi0 = -PI/2
k0   = 1
lam0 = 0
```

Given latitude `phi`, longitude `lambda`, and body radius `R`, all in
radians/meters:

```text
rho = 2 * R * k0 * tan(PI/4 + phi/2)
x   = rho * sin(lambda - lam0)
y   = -rho * cos(lambda - lam0)
```

The inverse is:

```text
rho = sqrt(x*x + y*y)
c   = 2 * atan(rho / (2 * R * k0))

phi = asin(cos(c) * sin(phi0)
           + (y * sin(c) * cos(phi0)) / rho)
lambda = lam0 + atan2(
    x * sin(c),
    rho * cos(phi0) * cos(c) - y * sin(phi0) * sin(c))
```

At `rho == 0`, the inverse maps directly to the projection origin.

Current `dcapp-planet-chunkgen` metadata stores each tile center as projected
`originX` and `originY` meters. Older `.planet.json` files containing tile
`lat` and `lon` are converted at load time.

### Legacy longitude compatibility

Current terrain metadata includes a `projection` object. Tile centers and
texture overlays use that projection's longitude convention directly.

Metadata without a `projection` object is treated as legacy data. Only for
that path, dcapp preserves the historical mirrored longitude with:

```text
projection_longitude = 180 degrees - user_longitude
```

The compatibility path also accounts for the old projected-Y convention when
placing overlays. The planet extension receives projected meters and does not
apply another longitude transform. Explicit `OriginX`/`OriginY` values are
already in terrain projection space.

## Implementation locations

| File | Relevant code |
|------|---------------|
| `libs/pl_math.h` | matrix storage and transforms |
| `extensions/pl_camera_ext.c` | view and projection matrices |
| `extensions/pl_gizmo_ext.c` | screen/NDC conversion |
| `extensions/pl_planet_ext.c` | projected texture placement |
| `extensions/pl_planet_processor_ext.c` | projected heightmap points to terrain vertices |
| `apps/dcapp_planet_chunkgen.c` | planet metadata and tile origins |
| `src/app/planet.c` | Runtime planet coordinates and legacy metadata conversion |
| `apps/dcapp_planet_snapshot.c` | Snapshot-side metadata conversion |
