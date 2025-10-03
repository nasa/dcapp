/*
   pl_terrain_ext.h
*/

/*
Index of this file:
// [SECTION] header mess
// [SECTION] apis
// [SECTION] includes
// [SECTION] forward declarations
// [SECTION] public api
// [SECTION] structs
*/

//-----------------------------------------------------------------------------
// [SECTION] header mess
//-----------------------------------------------------------------------------

#ifndef PL_TERRAIN_EXT_H
#define PL_TERRAIN_EXT_H

//-----------------------------------------------------------------------------
// [SECTION] apis
//-----------------------------------------------------------------------------

#define plTerrainI_version {0, 1, 0}

//-----------------------------------------------------------------------------
// [SECTION] includes
//-----------------------------------------------------------------------------

#include <stdbool.h>
#include <stdint.h>

//-----------------------------------------------------------------------------
// [SECTION] forward declarations
//-----------------------------------------------------------------------------

// basic types
typedef struct _plTerrainInit          plTerrainInit;
typedef struct _plHeightMapTile        plHeightMapTile;
typedef struct _plTerrainTilingInfo    plTerrainTilingInfo;
typedef struct _plTerrain              plTerrain;

// enums/flags
typedef int plTerrainFlags;
typedef int plTerrainTileFlags;

// external
typedef struct _plDevice         plDevice;          // pl_graphics_ext.h
typedef struct _plCommandBuffer  plCommandBuffer;   // pl_graphics_ext.h
typedef struct _plCamera         plCamera;          // pl_camera_ext.h
typedef union  plBindGroupHandle plBindGroupHandle; // pl_graphics_ext.h


//-----------------------------------------------------------------------------
// [SECTION] public api
//-----------------------------------------------------------------------------

typedef struct _plTerrainI
{

    // setup/shutdown
    void (*initialize)(plDevice*);
    void (*cleanup)   (void);

    // terrains/scenes
    plTerrain*        (*create_terrain_from_file)(plCommandBuffer*, const char* file);
    plTerrain*        (*create_terrain)          (plCommandBuffer*, plTerrainInit);
    plBindGroupHandle (*get_terrain_texture)     (plTerrain*);
    
    // camera
    void (*set_camera_pos)        (plTerrain*, float fX, float fY, float fZ);
    void (*set_camera_orientation)(plTerrain*, float fPitch, float fYaw, float fRoll);
    void (*set_camera_aspect)     (plTerrain*, float fAspect);

    // loading
    void (*load_mesh)(plCommandBuffer*, const char* file, uint32_t levels, uint32_t meshBaseLodExtentTexels);

    // loading tiles
    void (*tile_height_map)(plTerrain*, plTerrainTilingInfo);

    // per frame
    void (*render)(plTerrain*, plCommandBuffer*);





    plCamera* (*get_terrain_camera) (plTerrain*);
    void (*reload_shaders) (plTerrain*);
} plTerrainI;

//-----------------------------------------------------------------------------
// [SECTION] structs
//-----------------------------------------------------------------------------

typedef struct _plTerrainInit
{
    float    fUnitConversion;
    float    fMetersPerTexel;
    float    fMaxElevation;
    float    fMinElevation;
    plVec2   tMinPosition;
    plVec2   tMaxPosition;
    uint32_t uOutputWidth;
    uint32_t uOutputHeight;
} plTerrainInit;

typedef struct _plTerrainTilingInfo
{
    char   pcFile[64];
    plVec2 tOrigin;    // world coordinates
    float  fMinHeight;
    float  fMaxHeight;
} plTerrainTilingInfo;

#endif // PL_TERRAIN_EXT_H