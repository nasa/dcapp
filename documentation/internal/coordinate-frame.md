# Pilotlight Coordinate Frame

## World Space: Y-Up, Right-Handed

Pilotlight uses a **Y-up, right-handed** coordinate system in world space.

From `extensions/pl_camera_ext.c:81-83`:

```c
static const plVec4 tOriginalUpVec      = {0.0f, 1.0f, 0.0f, 0.0f};  // Y = Up
static const plVec4 tOriginalForwardVec = {0.0f, 0.0f, 1.0f, 0.0f};  // Z = Forward
static const plVec4 tOriginalRightVec   = {-1.0f, 0.0f, 0.0f, 0.0f}; // -X = Right
```

- **Y-axis**: Up
- **Z-axis**: Forward
- **X-axis**: Left (negative X = Right)

## Matrix Storage: Column-Major

Matrices are stored in **column-major** format with translation in `col[3]`.

From `libs/pl_math.h:183-205`:

```c
typedef union _plMat4
{
    plVec4 col[4];  // columns stored as vectors
    struct {
        float x11; float x21; float x31; float x41;  // Column 0
        float x12; float x22; float x32; float x42;  // Column 1
        float x13; float x23; float x33; float x43;  // Column 2
        float x14; float x24; float x34; float x44;  // Column 3
    };
    float d[16];
} plMat4;
```

Translation is stored in the 4th column (`libs/pl_math.h:1413-1424`):

```c
static inline plMat4
pl_mat4_translate_xyz(float fX, float fY, float fZ)
{
    plMat4 tResult = pl_create_mat4_diag(1.0f, 1.0f, 1.0f, 1.0f);
    tResult.x14 = fX;  // col[3].x
    tResult.x24 = fY;  // col[3].y
    tResult.x34 = fZ;  // col[3].z
    return tResult;
}
```

Matrix-vector multiplication treats matrices as column-major: `v' = M * v`.

## View Matrix: Flips X & Y

The view matrix applies a special **X and Y flip** to convert from world space to camera space.

From `extensions/pl_camera_ext.c:108-111`:

```c
// flip x & y so camera looks down +z and remains right handed (+x to the right)
const plMat4 tFlipXY = pl_mat4_scale_xyz(-1.0f, -1.0f, 1.0f);
ptCamera->tViewMat = pl_mul_mat4t(&tFlipXY, &ptCamera->tViewMat);
```

After the flip, camera space has:
- **+X**: Right
- **-Y**: Up (flipped)
- **+Z**: Forward

## Projection Matrix: Vulkan-Compatible

The projection matrices output Vulkan-compatible clip coordinates.

### Standard Perspective

From `extensions/pl_camera_ext.c:116-125`:

```c
const float fInvtanHalfFovy = 1.0f / tanf(ptCamera->fFieldOfView / 2.0f);
ptCamera->tProjMat.col[0].x = fInvtanHalfFovy / ptCamera->fAspectRatio;
ptCamera->tProjMat.col[1].y = fInvtanHalfFovy;
ptCamera->tProjMat.col[2].z = ptCamera->fFarZ / (ptCamera->fFarZ - ptCamera->fNearZ);
ptCamera->tProjMat.col[2].w = 1.0f;
ptCamera->tProjMat.col[3].z = -ptCamera->fNearZ * ptCamera->fFarZ / (ptCamera->fFarZ - ptCamera->fNearZ);
ptCamera->tProjMat.col[3].w = 0.0f;
```

### Reverse-Z Perspective

From `extensions/pl_camera_ext.c:128-137`:

```c
ptCamera->tProjMat.col[2].z = ptCamera->fNearZ / (ptCamera->fNearZ - ptCamera->fFarZ);
ptCamera->tProjMat.col[3].z = -ptCamera->fNearZ * ptCamera->fFarZ / (ptCamera->fNearZ - ptCamera->fFarZ);
```

Reverse-Z maps near plane to 1 and far plane to 0, providing better depth precision for distant objects.

### Orthographic

From `extensions/pl_camera_ext.c:140-146`:

```c
ptCamera->tProjMat.col[0].x = 2.0f / ptCamera->fWidth;
ptCamera->tProjMat.col[1].y = 2.0f / ptCamera->fHeight;
ptCamera->tProjMat.col[2].z = 1 / (ptCamera->fFarZ - ptCamera->fNearZ);
ptCamera->tProjMat.col[3].w = 1.0f;
```

## NDC Coordinates

- **X, Y**: -1 to +1
- **Z**: 0 to +1 (standard) or 1 to 0 (reverse-Z)

Evidence from `extensions/pl_gizmo_ext.c` (screen-to-NDC conversion):

```c
plVec4 tNDC = {
    -1.0f + 2.0f * tMousePos.x / gptIOI->get_io()->tMainViewportSize.x,
    -1.0f + 2.0f * tMousePos.y / gptIOI->get_io()->tMainViewportSize.y,
    1.0f, 1.0f
};
```

## Camera Rotation Convention

Rotations use **Pitch/Yaw/Roll** mapped to axes:

```c
float fPitch;  // rotation about right vector (X-axis)
float fYaw;    // rotation about up vector (Y-axis)
float fRoll;   // rotation about forward vector (Z-axis)
```

Composition order is **Y * X * Z** (Yaw, then Pitch, then Roll):

```c
plMat4 tRotations = pl_mul_mat4t(&tXRotMat, &tZRotMat);
tRotations = pl_mul_mat4t(&tYRotMat, &tRotations);
```

## MVP Composition

Model-View-Projection is composed as `Projection * View * Model`:

```c
const plMat4 tMVP = pl_mul_mat4(&ptCamera->tProjMat, &ptCamera->tViewMat);
```

Vertices are transformed as: `v_clip = P * V * M * v_local`

## Summary

| Aspect | Convention |
|--------|-----------|
| World Space | Y-up, right-handed (Y=up, Z=forward, -X=right) |
| Matrix Storage | Column-major (translation in col[3]) |
| Vector Transform | `matrix * vector` (column-major multiplication) |
| NDC Range | X,Y: [-1, +1]; Z: [0, 1] (standard) or [1, 0] (reverse-Z) |
| Camera Space | Flipped X,Y from world (-X right, -Y up, +Z forward before flip) |
| Rotation Order | Y * X * Z (Yaw * Pitch * Roll) |
| MVP Order | Projection * View * Model |

## Planet Terrain: South-Pole Stereographic Projection

The planet terrain system uses a **south-pole stereographic projection** to map geographic coordinates (latitude, longitude) to a 2D plane for tile indexing and texture placement.

### Forward Projection (geographic → plane)

```
phi0 = -PI/2     (projection center: south pole)
k0   = 1.0       (scale factor)
lam0 = 0.0       (central meridian)

rho = 2 * R * k0 * tan(PI/4 + phi/2)
x   = rho * sin(lambda - lam0)
y   = -rho * cos(lambda - lam0)
```

Where `phi` is latitude in radians and `lambda` is longitude in radians. This maps the south pole to the origin, with `rho` increasing toward the north pole.

### Inverse Projection (plane → geographic)

```
rho = sqrt(x^2 + y^2)
c   = 2 * atan(rho / (2 * R * k0))

phi = asin(cos(c) * sin(phi0) + (y * sin(c) * cos(phi0)) / rho)
lam = lam0 + atan2(x * sin(c), rho * cos(phi0) * cos(c) - y * sin(phi0) * sin(c))
```

This inverse is used by `dcapp-planet-chunkgen` to compute the latitude/longitude of each tile center from its position on the stereographic plane.

### Texture Longitude Convention

When placing textures via `set_texture()`, the extension internally transforms longitude as `-longitude + 180` before the forward projection. This means that `Longitude="180"` in the XML maps to `lambda = 0` (the central meridian) in stereographic space.

### Source Files

| File | Projection Usage |
|------|-----------------|
| `extensions/pl_planet_ext.c` | Forward projection in `pl_planet_set_texture()` |
| `extensions/pl_planet_processor_ext.c` | Forward projection in `pl_planet_process()`, inverse in `pl__get_cartesian()` |
| `apps/dcapp_planet_chunkgen.c` | Inverse projection in `_compute_tile_latlon()` |

---

## Key Source Files

| File | Purpose |
|------|---------|
| `libs/pl_math.h` | Matrix/vector types, transforms, math utilities |
| `extensions/pl_camera_ext.c` | View and projection matrix construction |
| `extensions/pl_gizmo_ext.c` | NDC conversion, screen-space picking |
| `shaders/draw_3d.vert` | MVP application in shaders |
