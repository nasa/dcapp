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

    // Slope: deviation of surface normal from the radial (outward) direction.
    // slope = 0   → normal points straight up  → flat terrain
    // slope = 1   → normal is horizontal       → vertical cliff
    vec3 radial = normalize(tShaderIn.tWorldPosition);
    float slope = 1.0 - dot(normal, radial);

    // 3-stop ramp: green (flat) → yellow (~20°) → red (steep ≥ 35°)
    // 20°: slope ≈ 1 - cos(20°) ≈ 0.060
    // 35°: slope ≈ 1 - cos(35°) ≈ 0.181
    vec3 c0 = vec3(0.22, 0.74, 0.22);  // green  (flat)
    vec3 c1 = vec3(0.94, 0.78, 0.12);  // yellow (moderate)
    vec3 c2 = vec3(0.86, 0.14, 0.10);  // red    (steep)

    vec3 color = c0;
    color = mix(color, c1, clamp(slope / 0.060,         0.0, 1.0));
    color = mix(color, c2, clamp((slope - 0.060) / 0.121, 0.0, 1.0));

    // Diffuse lighting
    vec3 w_i = normalize(tDynamicData.tData.tLightDirection);
    float diffuse = max(0.0, dot(normal, w_i));
    float ambient = 0.15;

    outColor.rgb = color * (diffuse + ambient);
    outColor.a   = 1.0;

    if (bool(tDynamicData.tData.tFlags & PL_TERRAIN_SHADER_FLAGS_SHOW_LEVELS))
    {
        outColor += tShaderIn.tColor;
    }

    if (bool(tDynamicData.tData.tFlags & PL_TERRAIN_SHADER_FLAGS_WIREFRAME))
    {
        outColor = tShaderIn.tColor;
    }
}
