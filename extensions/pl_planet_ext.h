/*
   pl_planet_ext.h
*/

/*
Index of this file:
// [SECTION] header mess
// [SECTION] apis
// [SECTION] includes
// [SECTION] forward declarations
// [SECTION] public api
// [SECTION] structs
// [SECTION] enums
*/

//-----------------------------------------------------------------------------
// [SECTION] header mess
//-----------------------------------------------------------------------------

#ifndef PL_PLANET_EXT_H
#define PL_PLANET_EXT_H

//-----------------------------------------------------------------------------
// [SECTION] apis
//-----------------------------------------------------------------------------

#define plPlanetI_version {0, 1, 0}

//-----------------------------------------------------------------------------
// [SECTION] includes
//-----------------------------------------------------------------------------

#include <stdbool.h>
#include <stdint.h>
#include "pl_math.h"

//-----------------------------------------------------------------------------
// [SECTION] forward declarations
//-----------------------------------------------------------------------------

// basic types
typedef struct _plPlanetExtInit        plPlanetExtInit;
typedef struct _plPlanetInit           plPlanetInit;
typedef struct _plPlanetRuntimeOptions plPlanetRuntimeOptions;
typedef struct _plPlanet               plPlanet;
typedef struct _plPlanetView           plPlanetView;
typedef struct _plPlanetViewInit       plPlanetViewInit;
typedef struct _plPlanetTexture        plPlanetTexture;

// enums/flags
typedef int plPlanetFlags;
typedef int plPlanetLoadFlags;

// external
typedef struct _plPlanetProcessInfo plPlanetProcessInfo; // pl_planet_processor.h
typedef struct _plDevice            plDevice;            // pl_graphics_ext.h
typedef struct _plRenderEncoder     plRenderEncoder;     // pl_graphics_ext.h
typedef struct _plDynamicDataBlock  plDynamicDataBlock;  // pl_graphics_ext.h
typedef struct _plDrawLayer2D       plDrawLayer2D;       // pl_draw_ext.h
typedef struct _plCamera            plCamera;            // pl_camera_ext.h
typedef struct _plCommandBuffer     plCommandBuffer;     // pl_graphics_ext.h
typedef union  plBindGroupHandle    plBindGroupHandle;   // pl_graphics_ext.h

//-----------------------------------------------------------------------------
// [SECTION] public api
//-----------------------------------------------------------------------------

typedef struct _plPlanetI
{

    // setup/shutdown
    void (*initialize)(plPlanetExtInit);
    void (*cleanup)   (void);

    // terrain setup/finalization/shutdown
    plPlanet* (*create_planet) (plCommandBuffer*, plPlanetInit, plPlanetProcessInfo*);
    void      (*cleanup_planet)(plPlanet*);

    void (*set_texture)(plPlanet*, plPlanetTexture*);

    // per frame
    void (*prepare)(plPlanet*, plCommandBuffer*);

    // views (share terrain data, separate render targets)
    plPlanetView*     (*create_view)     (plPlanet*, plCommandBuffer*, plPlanetViewInit);
    void              (*cleanup_view)    (plPlanetView*);
    void              (*render_view)     (plPlanetView*, plCamera*, plCommandBuffer*);
    plBindGroupHandle (*get_view_texture)(plPlanetView*);

    // drawing
    void (*draw_sphere)(plPlanetView*, float longitude, float latitude, float height, float radius, uint32_t color);

    // debugging helpers mostly
    void                   (*set_runtime_options)(plPlanet*, plPlanetRuntimeOptions);
    plPlanetRuntimeOptions (*get_runtime_options)(plPlanet*);
    void                   (*reload_shaders)     (plPlanet*);
     void                  (*set_shaders)        (plPlanet*, const char* pcVertexShader, const char* pcFragmentShader);

} plPlanetI;

//-----------------------------------------------------------------------------
// [SECTION] structs
//-----------------------------------------------------------------------------

typedef struct _plPlanetExtInit
{
    plDevice* ptDevice;
    uint32_t uStagingBufferSize; // default: 268435456 bytes
} plPlanetExtInit;

typedef struct _plPlanetTexture
{
    const char* pcPath;
    float       fMetersPerPixel;
    float       fLongitude;
    float       fLatitude;
} plPlanetTexture;

typedef struct _plPlanetInit
{
    double dRadius;

    plPlanetLoadFlags tLoadFlags;

    // memory allocations
    uint32_t uVertexBufferSize;  // default: 268435456 bytes
    uint32_t uIndexBufferSize;   // default: 268435456 bytes

    // shaders
    const char* pcVertexShader;   // default: "planet.vert"
    const char* pcFragmentShader; // default: "planet.frag"
} plPlanetInit;

typedef struct _plPlanetViewInit
{
    uint32_t uOutputWidth;
    uint32_t uOutputHeight;
} plPlanetViewInit;

typedef struct _plPlanetRuntimeOptions
{
    plPlanetFlags tFlags;
    float         fTau;
    plVec3        tLightDirection;
} plPlanetRuntimeOptions;

//-----------------------------------------------------------------------------
// [SECTION] enums
//-----------------------------------------------------------------------------

enum _plPlanetFlags
{
    PL_PLANET_FLAGS_NONE        = 0,
    PL_PLANET_FLAGS_WIREFRAME   = 1 << 0,
    PL_PLANET_FLAGS_SHOW_LEVELS = 1 << 1,
    PL_PLANET_FLAGS_SHOW_ORIGIN = 1 << 2,
};

enum _plPlanetLoadFlags
{
    PL_PLANET_LOAD_FLAGS_NONE           = 0,
    PL_PLANET_LOAD_FLAGS_DEBUG          = 1 << 0,
    PL_PLANET_LOAD_FLAGS_CACHE_TEXTURES = 1 << 1,
};

#endif // PL_PLANET_EXT_H