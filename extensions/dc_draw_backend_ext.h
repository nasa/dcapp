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

#ifndef DC_DRAW_BACKEND_EXT_H
#define DC_DRAW_BACKEND_EXT_H

//-----------------------------------------------------------------------------
// [SECTION] apis
//-----------------------------------------------------------------------------

#define dcDrawBackendI_version {1, 4, 0}

//-----------------------------------------------------------------------------
// [SECTION] includes
//-----------------------------------------------------------------------------

#include <stdint.h>
#include "pl_math.h"

//-----------------------------------------------------------------------------
// [SECTION] forward declarations & basic types
//-----------------------------------------------------------------------------

// external
typedef struct _dcFontAtlas     dcFontAtlas;       // pl_draw_ext.h
typedef struct _dcDrawList2D    dcDrawList2D;      // pl_draw_ext.h
typedef struct _dcDrawList3D    dcDrawList3D;      // pl_draw_ext.h
typedef struct _dcDrawLayer2D   dcDrawLayer2D;     // pl_draw_ext.h
typedef int    dcDrawFlags;                        // pl_draw_ext.h
typedef struct _plDevice        plDevice;          // pl_graphics_ext.h
typedef struct _plRenderEncoder plRenderEncoder;   // pl_graphics_ext.h
typedef struct _plCommandBuffer plCommandBuffer;   // pl_graphics_ext.h
typedef struct _plBindGroupPool plBindGroupPool;   // pl_graphics_ext.h
typedef union plBindGroupHandle plBindGroupHandle; // pl_graphics_ext.h
typedef union plTextureHandle   plTextureHandle;   // pl_graphics_ext.h
typedef union plShaderHandle    plShaderHandle;    // pl_graphics_ext.h

// shader override data (passed via callback userData)
typedef struct _dcShaderOverride
{
    plShaderHandle* pt2dShader;  // for regular draws (NULL = use default)
    plShaderHandle* ptSdfShader; // for SDF draws (NULL = use default)
} dcShaderOverride;

// 3D shader override data (passed via callback userData)
typedef struct _dcShaderOverride3D
{
    plShaderHandle* ptSolidShader;    // for solid draws (NULL = use default)
    plShaderHandle* ptTexturedShader; // for textured draws (NULL = use default)
} dcShaderOverride3D;

//-----------------------------------------------------------------------------
// [SECTION] public api struct
//-----------------------------------------------------------------------------

typedef struct _dcDrawBackendI
{
    // init/cleanup
    void (*initialize)(plDevice*);
    void (*cleanup)   (void);

    void (*new_frame)(void);

    bool (*build_font_atlas)  (plCommandBuffer*, dcFontAtlas*);
    void (*cleanup_font_atlas)(dcFontAtlas*);

    plBindGroupHandle (*create_bind_group_for_texture)(plTextureHandle);
    plBindGroupPool*  (*get_bind_group_pool)(void);

    void (*submit_2d_drawlist)(dcDrawList2D*, plRenderEncoder*, float fWidth, float fHeight, uint32_t sampleCount);
    void (*submit_3d_drawlist)(dcDrawList3D*, plRenderEncoder*, float fWidth, float fHeight, const plMat4* ptMVP, dcDrawFlags, uint32_t sampleCount);
    void (*submit_2d_drawlist_ex)(dcDrawList2D*, plRenderEncoder*, float fWidth, float fHeight, uint32_t sampleCount, plShaderHandle* pt2dShader, plShaderHandle* ptSdfShader);
    void (*submit_3d_drawlist_ex)(dcDrawList3D*, plRenderEncoder*, float fWidth, float fHeight, const plMat4* ptMVP, dcDrawFlags, uint32_t sampleCount, plShaderHandle* ptSolidShader, plShaderHandle* ptTexturedShader);

    // misc.
    void (*use_nearest_sampler)(dcDrawLayer2D*);
    void (*use_linear_sampler) (dcDrawLayer2D*);

    // shader overrides (inserts callback into command stream)
    // pass NULL for both shaders to reset to default
    void (*set_shader)   (dcDrawLayer2D*, plShaderHandle* pt2dShader, plShaderHandle* ptSdfShader);
    void (*set_3d_shader)(dcDrawList3D*,  plShaderHandle* ptSolidShader, plShaderHandle* ptTexturedShader);
} dcDrawBackendI;

#endif // DC_DRAW_BACKEND_EXT_H
