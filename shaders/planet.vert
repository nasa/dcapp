#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "pl_shader_interop_planet.h"

/* ------------------------------------------------------------ */
/* Normal decode                                                */
/* ------------------------------------------------------------ */
vec3 Decode(vec2 f)
{
    f = f * 2.0 - 1.0;
    vec3 n = vec3(f.x, f.y, 1.0 - abs(f.x) - abs(f.y));
    float t = max(-n.z, 0.0);
    n.x += n.x >= 0.0 ? -t : t;
    n.y += n.y >= 0.0 ? -t : t;
    return normalize(n);
}

/* ------------------------------------------------------------ */
/* Inputs                                                       */
/* ------------------------------------------------------------ */

#ifdef PL_PLANET_DOUBLE_PRECISON

    layout(location = 0) in vec3 inHighPos;
    layout(location = 1) in vec3 inLowPos;
    layout(location = 2) in vec2 inNormal;
    layout(location = 3) in vec2 inUV;

#else

    layout(location = 0) in vec3 inHighPos;
    layout(location = 1) in vec2 inNormal;
    layout(location = 2) in vec2 inUV;

#endif

/* ------------------------------------------------------------ */
/* Outputs                                                      */
/* ------------------------------------------------------------ */
layout(location = 0) out struct plShaderOut {
    vec4 tColor;
    vec3 tWorldPosition;
    vec3 tWorldNormal;
    vec2 tUV;
} tShaderOut;

/* ------------------------------------------------------------ */
/* Uniforms                                                     */
/* ------------------------------------------------------------ */
layout(set = 3, binding = 0) uniform PL_DYNAMIC_DATA
{
    plGpuDynPlanetData tData;
} tDynamicData;

/* ------------------------------------------------------------ */
/* Main                                                         */
/* ------------------------------------------------------------ */
void main()
{
    /* -------------------------------------------------------- */
    /* Reconstruct world position                               */
    /* -------------------------------------------------------- */

#ifdef PL_PLANET_DOUBLE_PRECISON
    vec3 worldHigh = inHighPos;
    vec3 worldLow  = inLowPos;
#else
    vec3 worldHigh = inHighPos;
    vec3 worldLow  = vec3(0.0);
#endif

    vec3 worldPos = worldHigh + worldLow;

    /* -------------------------------------------------------- */
    /* CORRECT FLATTENING (WORLD SPACE, HI+LO SAFE)             */
    /* -------------------------------------------------------- */

    bool doFlatten = bool(tDynamicData.tData.tFlags & PL_TERRAIN_SHADER_FLAGS_FLATTEN);

    if (doFlatten)
    {
        // Radially project full-precision position onto the sphere
        worldPos = normalize(worldPos) * tDynamicData.tData.fRadius;

        // Re-split after non-linear math
        worldHigh = worldPos;
        worldLow  = worldPos - worldHigh;
    }

    /* -------------------------------------------------------- */
    /* Camera-relative subtraction (unchanged, correct)         */
    /* -------------------------------------------------------- */

    vec3 t1 = worldLow - tDynamicData.tData.tCameraPosLow.xyz;
    vec3 e  = t1 - worldLow;

    vec3 t2 = ((-tDynamicData.tData.tCameraPosLow.xyz - e)
             + (worldLow - (t1 - e)))
             + worldHigh
             - tDynamicData.tData.tCameraPosHigh.xyz;

    vec3 highDifference = t1 + t2;
    vec3 lowDifference  = t2 - (highDifference - t1);

    vec3 tViewPosition = highDifference + lowDifference;

    gl_Position =
        tDynamicData.tData.tCameraViewProjection *
        vec4(tViewPosition, 1.0);

    /* -------------------------------------------------------- */
    /* Outputs                                                  */
    /* -------------------------------------------------------- */

    tShaderOut.tWorldPosition = worldPos;
    tShaderOut.tUV = inUV;
    tShaderOut.tWorldNormal = Decode(inNormal);

    /* -------------------------------------------------------- */
    /* Debug coloring                                           */
    /* -------------------------------------------------------- */

    vec3 atColors[16];
    float fColorStrength = 0.1;

    atColors[0]  = vec3(fColorStrength, 0.0, 0.0);
    atColors[1]  = vec3(0.0, fColorStrength, 0.0);
    atColors[2]  = vec3(0.0, 0.0, fColorStrength);
    atColors[3]  = vec3(fColorStrength, fColorStrength, 0.0);
    atColors[4]  = vec3(fColorStrength, 0.0, fColorStrength);
    atColors[5]  = vec3(0.0, fColorStrength, fColorStrength);
    atColors[6]  = vec3(fColorStrength);
    atColors[7]  = vec3(fColorStrength * 4, fColorStrength, fColorStrength);

    atColors[8]  = vec3(fColorStrength * 3, 0.0, 0.0);
    atColors[9]  = vec3(0.0, fColorStrength * 3, 0.0);
    atColors[10] = vec3(0.0, 0.0, fColorStrength * 3);
    atColors[11] = vec3(fColorStrength * 3, fColorStrength * 3, 0.0);
    atColors[12] = vec3(fColorStrength * 3, 0.0, fColorStrength * 3);
    atColors[13] = vec3(0.0, fColorStrength, fColorStrength * 3);
    atColors[14] = vec3(fColorStrength, fColorStrength * 3, fColorStrength * 3);
    atColors[15] = vec3(fColorStrength * 7, fColorStrength, fColorStrength);

    tShaderOut.tColor.rgb =
        atColors[tDynamicData.tData.iLevel % 16];
    tShaderOut.tColor.a = 1.0;

    if (bool(tDynamicData.tData.tFlags & PL_TERRAIN_SHADER_FLAGS_WIREFRAME))
    {
        tShaderOut.tColor.rgb += vec3(0.3);
    }

    if (bool(tDynamicData.tData.tFlags & PL_TERRAIN_SHADER_FLAGS_SHOW_CHUNKS))
    {
        tShaderOut.tColor.rgb =
            atColors[tDynamicData.tData.iChunkID % 16] + vec3(0.3);
    }
}