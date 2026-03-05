#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "pl_shader_interop_planet.h"

#define M_PI 3.1415926535897932384626433832795

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
    float rad = 3.14159265358979323846 / 180.0;
    float deg = 1.0 / rad;

    vec3 sphereNorm = normalize(tShaderIn.tWorldPosition);
    float fLatitude = asin(sphereNorm.y);
    float fLongitude = atan(sphereNorm.x, sphereNorm.z);
    float R   = 1737400.0;


    float lon = fLongitude;
    vec3 N = sphereNorm;
    // float l = length(Ns);
    float lat = fLatitude;

    // vec2 UV = vec2(2);

    // {
    //     // float xresolution = tObjectInfo.tInfo.fXResolution;
    //     // float yresolution = tObjectInfo.tInfo.fYResolution;
    //     float xresolution = 1024;
    //     float yresolution = 1024;
    //     float xmax = 409600.0 * 1.0;
    //     float xmin = -409600.0 * 1.0;
    //     float ymax = 409600.0 * 1.0;
    //     float ymin = -409600.0 * 1.0;
    //     float x0 = 10000.0;
    //     float y0 = 10000.0;

    //     // float r = 2 * R * tan(0.25 * 3.14159265358979323846 - 0.5 * lat); // north hem
    //     float r = 2 * R * tan(0.25 * 3.14159265358979323846 + 0.5 * lat); // south hem
    //     float x = r * sin(lon) + x0;
    //     float y = r * cos(lon) + y0;

    //     float xp = xresolution * (x - xmin) / (xmax - xmin);
    //     float yp = yresolution * (1.0 - ((y - ymin) / (ymax - ymin)));

    //     UV.x = 2.0;

    //     if(xp >= 0.0 && xp < xresolution && yp >= 0.0 && yp < yresolution)
    //     {
    //         UV.x = xp / xresolution;
    //         UV.y = yp / yresolution;
    //     }
    // }

    vec4 tHazardColor = vec4(0.0, 0.0, 0.0, 1.0);
    // if(UV.x < 1.5)
    // {
    //     tHazardColor = texture(sampler2D(at2DTextures[0], tSamplerLinearClamp), UV);
    // }

    vec2 tUVActual = tShaderIn.tUV;
    tUVActual = tUVActual * tDynamicData.tData.tUVInfo.xy;
    tUVActual = tUVActual + tDynamicData.tData.tUVInfo.zw;

    vec3 normal = normalize(tShaderIn.tWorldNormal);
    tHazardColor = texture(sampler2D(at2DTextures[tDynamicData.tData.uTextureIndex], tSamplerLinearClamp), tUVActual) * tDynamicData.tData.fHazardMapStrength;

    vec3 sunlightColor = vec3(1.0, 1.0, 1.0);
    vec3 diffuse = vec3(0.5);
    vec3 ambient = vec3(0);
    
    vec3 w_i = -normalize(tDynamicData.tData.tLightDirection);

    outColor.xyz = diffuse * (max(0.0, dot(normal, w_i)) * sunlightColor + ambient);
    outColor.a = 1.0;

    // if(tDynamicData.tData.uTextureIndex > 0)
    // {
    //     // outColor.rg = tShaderIn.tUV;
        // outColor.rg = tUVActual;
        // outColor.rg = tShaderIn.tUV;
    //     // outColor.rg = tDynamicData.tData.tUVScale;
    //     // outColor.g = 0;
    //     outColor.b = 0;
    // }
    // else
    {
        outColor.rgb += tHazardColor.rgb * 0.3;
    }
    // outColor.rgb = normal;

    if(bool(tDynamicData.tData.tFlags & PL_TERRAIN_SHADER_FLAGS_SHOW_LEVELS))
    {
        outColor += tShaderIn.tColor;
    }

    if(bool(tDynamicData.tData.tFlags & PL_TERRAIN_SHADER_FLAGS_SHOW_CHUNKS))
    {
        outColor += tShaderIn.tColor;
    }

    if(bool(tDynamicData.tData.tFlags & PL_TERRAIN_SHADER_FLAGS_WIREFRAME))
    {
        outColor = tShaderIn.tColor;
    }

    // {

    // fLatitude *= deg;
    // fLongitude *= deg;

    //     if(fLatitude > -60.25 && fLatitude < -59.75)
    //     {
    //         // outColor = vec4(1.0, 1.0, 1.0, 1.0);
    //         outColor.r += 0.3;
    //     }
    //     else if(fLatitude > -70.25 && fLatitude < -69.75)
    //     {
    //         // outColor = vec4(1.0, 1.0, 1.0, 1.0);
    //         outColor.r += 0.3;
    //     }
    //     else if(fLatitude > -45.25 && fLatitude < -44.75)
    //     {
    //         // outColor = vec4(1.0, 1.0, 1.0, 1.0);
    //         outColor.r += 0.3;
    //     }

    //     if(fLongitude > -0.25 && fLongitude < 0.25)
    //     {
    //         // outColor = vec4(1.0, 1.0, 1.0, 1.0);
    //         outColor.r += 0.3;
    //     }
    //     else if(fLongitude > 9.75 && fLongitude < 10.25)
    //     {
    //         // outColor = vec4(1.0, 1.0, 1.0, 1.0);
    //         outColor.r += 0.3;
    //     }
    // }

    
}