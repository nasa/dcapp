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

    vec3 normal = normalize(tShaderIn.tWorldNormal);
    vec4 tHazardColor = texture(sampler2D(at2DTextures[tDynamicData.tData.uTextureIndex], tSamplerLinearClamp), tShaderIn.tUV);

    vec3 sunlightColor = vec3(1.0, 1.0, 1.0);
    vec3 diffuse = vec3(0.5);
    vec3 ambient = vec3(0);
    
    vec3 w_i = normalize(tDynamicData.tData.tLightDirection);

    outColor.xyz = diffuse * (max(0.0, dot(normal, w_i)) * sunlightColor + ambient);
    outColor.a = 1.0;
    // outColor.rg = tShaderIn.tUV;
    outColor.rgb += tHazardColor.rgb * 0.3;
    // outColor.rgb = normal;

    if(bool(tDynamicData.tData.tFlags & PL_TERRAIN_SHADER_FLAGS_SHOW_LEVELS))
    {
        outColor += tShaderIn.tColor;
    }

    if(bool(tDynamicData.tData.tFlags & PL_TERRAIN_SHADER_FLAGS_WIREFRAME))
    {
        outColor = tShaderIn.tColor;
    }

}