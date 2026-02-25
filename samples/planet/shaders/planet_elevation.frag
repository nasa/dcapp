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

    // Elevation above lunar mean radius (1737400 m).
    // LOLA south polar range is roughly -8000 m to +5000 m.
    float elevation = length(tShaderIn.tWorldPosition) - 1737400.0;
    float t = clamp((elevation + 8000.0) / 13000.0, 0.0, 1.0);

    // 5-stop color ramp: deep blue → cyan → green → yellow → white
    vec3 c0 = vec3(0.05, 0.05, 0.50);  // deep blue   (lowest)
    vec3 c1 = vec3(0.05, 0.60, 0.80);  // cyan
    vec3 c2 = vec3(0.10, 0.65, 0.10);  // green       (near zero)
    vec3 c3 = vec3(0.85, 0.75, 0.10);  // yellow
    vec3 c4 = vec3(1.00, 1.00, 1.00);  // white       (highest)

    vec3 color = c0;
    color = mix(color, c1, clamp(t / 0.25,        0.0, 1.0));
    color = mix(color, c2, clamp((t - 0.25) / 0.25, 0.0, 1.0));
    color = mix(color, c3, clamp((t - 0.50) / 0.25, 0.0, 1.0));
    color = mix(color, c4, clamp((t - 0.75) / 0.25, 0.0, 1.0));

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
