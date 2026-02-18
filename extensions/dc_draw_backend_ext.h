/*
   dc_draw_backend_ext.h
     * forked from pilotlight (https://github.com/PilotLightTech/pilotlight)
*/

/*
Index of this file:
// [SECTION] header mess
// [SECTION] apis
// [SECTION] includes
// [SECTION] public api struct
*/

//-----------------------------------------------------------------------------
// [SECTION] header mess
//-----------------------------------------------------------------------------

#ifndef PL_DRAW_BACKEND_EXT_H
#define PL_DRAW_BACKEND_EXT_H

//-----------------------------------------------------------------------------
// [SECTION] apis
//-----------------------------------------------------------------------------

#define dcDrawBackendI_version {1, 1, 0}

//-----------------------------------------------------------------------------
// [SECTION] includes
//-----------------------------------------------------------------------------

#include <stdint.h>
#include "pl_math.h"

//-----------------------------------------------------------------------------
// [SECTION] forward declarations & basic types
//-----------------------------------------------------------------------------

// external
typedef struct _plFontAtlas     plFontAtlas;       // pl_draw_ext.h
typedef struct _plDrawList2D    plDrawList2D;      // pl_draw_ext.h
typedef struct _plDrawList3D    plDrawList3D;      // pl_draw_ext.h
typedef struct _plDrawLayer2D   plDrawLayer2D;     // pl_draw_ext.h
typedef int    plDrawFlags;                        // pl_draw_ext.h
typedef struct _plDevice        plDevice;          // pl_graphics_ext.h
typedef struct _plRenderEncoder plRenderEncoder;   // pl_graphics_ext.h
typedef struct _plCommandBuffer plCommandBuffer;   // pl_graphics_ext.h
typedef struct _plBindGroupPool plBindGroupPool;   // pl_graphics_ext.h
typedef union plBindGroupHandle plBindGroupHandle; // pl_graphics_ext.h
typedef union plTextureHandle   plTextureHandle;   // pl_graphics_ext.h
typedef union plShaderHandle    plShaderHandle;    // pl_graphics_ext.h

// shader override data (passed via callback userData)
typedef struct _plShaderOverride
{
    plShaderHandle* pt2dShader;  // for regular draws (NULL = use default)
    plShaderHandle* ptSdfShader; // for SDF draws (NULL = use default)
} plShaderOverride;

// 3D shader override data (passed via callback userData)
typedef struct _plShaderOverride3D
{
    plShaderHandle* ptSolidShader;    // for solid draws (NULL = use default)
    plShaderHandle* ptTexturedShader; // for textured draws (NULL = use default)
} plShaderOverride3D;

//-----------------------------------------------------------------------------
// [SECTION] public api struct
//-----------------------------------------------------------------------------

typedef struct _plDrawBackendI
{
    // init/cleanup
    void (*initialize)(plDevice*);
    void (*cleanup)   (void);

    void (*new_frame)(void);

    bool (*build_font_atlas)  (plCommandBuffer*, plFontAtlas*);
    void (*cleanup_font_atlas)(plFontAtlas*);

    plBindGroupHandle (*create_bind_group_for_texture)(plTextureHandle);
    plBindGroupPool*  (*get_bind_group_pool)(void);

    void (*submit_2d_drawlist)(plDrawList2D*, plRenderEncoder*, float fWidth, float fHeight, uint32_t sampleCount);
    void (*submit_3d_drawlist)(plDrawList3D*, plRenderEncoder*, float fWidth, float fHeight, const plMat4* ptMVP, plDrawFlags, uint32_t sampleCount);

    // misc.
    void (*use_nearest_sampler)(plDrawLayer2D*);
    void (*use_linear_sampler) (plDrawLayer2D*);

    // shader overrides (inserts callback into command stream)
    // pass NULL for both shaders to reset to default
    void (*set_shader)   (plDrawLayer2D*, plShaderHandle* pt2dShader, plShaderHandle* ptSdfShader);
    void (*set_3d_shader)(plDrawList3D*,  plShaderHandle* ptSolidShader, plShaderHandle* ptTexturedShader);
} plDrawBackendI;

// Alias for API registration (separate from pilotlight's plDrawBackendI)
typedef plDrawBackendI dcDrawBackendI;

#endif // PL_DRAW_BACKEND_EXT_H