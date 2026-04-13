#ifndef PL_SHADER_INTEROP_TERRAIN_H
#define PL_SHADER_INTEROP_TERRAIN_H

//-----------------------------------------------------------------------------
// [SECTION] includes
//-----------------------------------------------------------------------------

#include "pl_shader_interop.h"

//-----------------------------------------------------------------------------
// [SECTION] defines
//-----------------------------------------------------------------------------

#define PL_PLANET_MAX_BINDLESS_TEXTURES 4096

//-----------------------------------------------------------------------------
// [SECTION] enums
//-----------------------------------------------------------------------------

PL_BEGIN_ENUM(plTerrainShaderFlags)
    PL_ENUM_ITEM(PL_TERRAIN_SHADER_FLAGS_NONE,        0)
    PL_ENUM_ITEM(PL_TERRAIN_SHADER_FLAGS_WIREFRAME,   1 << 0)
    PL_ENUM_ITEM(PL_TERRAIN_SHADER_FLAGS_SHOW_LEVELS, 1 << 1)
    PL_ENUM_ITEM(PL_TERRAIN_SHADER_FLAGS_SHOW_CHUNKS, 1 << 3)
    PL_ENUM_ITEM(PL_TERRAIN_SHADER_FLAGS_FLATTEN,     1 << 4)
PL_END_ENUM

//-----------------------------------------------------------------------------
// [SECTION] structs
//-----------------------------------------------------------------------------

PL_BEGIN_STRUCT(plGpuDynPlanetData)

    int iLevel;
    int tFlags;
    uint uTextureIndex;
    int iChunkID;

    vec4 tUVInfo;

    vec3 tLightDirection;
    float fRadius;

    float fHazardMapStrength;
    int _aiUnused0;
    int _aiUnused1;
    int _aiUnused2;

    vec4 tCameraPosHigh;
    vec4 tCameraPosLow;

    mat4 tCameraViewProjection;
    
PL_END_STRUCT(plGpuDynPlanetData)

#endif // PL_SHADER_INTEROP_TERRAIN_H