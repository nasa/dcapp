#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "pl_shader_interop_planet.h"

//-----------------------------------------------------------------------------
// [SECTION] input & output
//-----------------------------------------------------------------------------

// input
layout(location = 0) in struct plShaderIn {
    vec4 tColor;
    vec3 tWorldPosition;
    vec3 tWorldNormal;
    vec2 tUV;
} tShaderIn;

// output
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0)  uniform sampler tSamplerLinearClamp;
layout(set = 0, binding = 1)  uniform texture2D at2DTextures[PL_PLANET_MAX_BINDLESS_TEXTURES];

layout(set = 3, binding = 0) uniform PL_DYNAMIC_DATA
{
    plGpuDynPlanetData tData;
} tDynamicData;

//-----------------------------------------------------------------------------
// [SECTION] entry
//-----------------------------------------------------------------------------

void main()
{
    vec2 tUVActual = tShaderIn.tUV;
    tUVActual = tUVActual * tDynamicData.tData.tUVInfo.xy;
    tUVActual = tUVActual + tDynamicData.tData.tUVInfo.zw;

    vec3 normal = normalize(tShaderIn.tWorldNormal);
    vec4 tHazardColor = texture(sampler2D(at2DTextures[tDynamicData.tData.uTextureIndex], tSamplerLinearClamp), tUVActual);

    // Slope angle in degrees from the radial (outward) direction.
    vec3 radial = normalize(tShaderIn.tWorldPosition);
    float cosAngle = dot(normal, radial);
    float slopeDeg = degrees(acos(clamp(cosAngle, 0.0, 1.0)));

    // Discrete 3-band classification with hard transitions:
    //   0 -  5 deg : warm stone gray   (flat terrain)
    //   5 - 10 deg : golden amber      (moderate slopes)
    //  10+    deg  : deep crimson       (steep / hazardous)
    vec3 cFlat    = vec3(0.55, 0.52, 0.48);  // warm stone gray
    vec3 cModerate = vec3(0.85, 0.62, 0.15); // golden amber
    vec3 cSteep   = vec3(0.78, 0.12, 0.10);  // deep crimson

    // Hard step transitions (no blending between bands)
    vec3 color = cFlat;
    color = mix(color, cModerate, step(5.0, slopeDeg));
    color = mix(color, cSteep,    step(10.0, slopeDeg));

    // Diffuse lighting
    vec3 w_i = normalize(tDynamicData.tData.tLightDirection);
    float diffuse = max(0.0, dot(normal, w_i));
    float ambient = 0.15;

    outColor.rgb = color * (diffuse + ambient);
    outColor.a   = 1.0;
    outColor.rgb += tHazardColor.rgb * 0.3;

    if (bool(tDynamicData.tData.tFlags & PL_TERRAIN_SHADER_FLAGS_SHOW_LEVELS))
    {
        outColor += tShaderIn.tColor;
    }

    if (bool(tDynamicData.tData.tFlags & PL_TERRAIN_SHADER_FLAGS_WIREFRAME))
    {
        outColor = tShaderIn.tColor;
    }
}
