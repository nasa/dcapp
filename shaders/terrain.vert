#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "terrain.glsl"

layout(set = 0, binding = 0)  uniform sampler tSampler;
layout(set = 0, binding = 1)  uniform texture2D tHeightMap;
layout(set = 0, binding = 2)  uniform texture2D tNoiseTexture;
layout(set = 0, binding = 3)  uniform texture2D tDiffuseTexture;
layout(set = 0, binding = 4)  uniform sampler tMirrorSampler;

layout(set = 3, binding = 0) uniform PL_DYNAMIC_DATA
{
    plTerrainDynamicData tInfo;
} tObjectInfo;


layout(location = 0) in vec3 inPos;

layout(location = 0) out struct plShaderOut {
    vec3 tPosition;
    float fGridLevel;
} tShaderIn;



void main()
{
    float fGridLevel = inPos.y;
    
    vec2 tTextureSize = textureSize(sampler2D(tHeightMap, tSampler), 0);
    vec2 heightfield_invSize = vec2(1.0 / tTextureSize.x, 1.0 / tTextureSize.y);
    

    float metersPerHeightFieldTexel = tObjectInfo.tInfo.fMetersPerHeightFieldTexel / tObjectInfo.tInfo.fUnitConversion;
    float fMaxHeight = tObjectInfo.tInfo.fGlobalMaxHeight / tObjectInfo.tInfo.fUnitConversion;
    float fMinHeight = tObjectInfo.tInfo.fGlobalMinHeight / tObjectInfo.tInfo.fUnitConversion;
    float heightfieldTexelsPerMeter = 1.0 / metersPerHeightFieldTexel;
    
    float baseGridSizeTexels = 128.0;
    float invBaseGridSizeTexels = 1.0 / 128.0;
    float verticalOffset = 0.0;
    vec4 heightfield_readMultiplyFirst = vec4(2.0, 2.0, 2.0, fMaxHeight - fMinHeight);
    float heightfield_invReadMultiplyFirst = 1.0 / heightfield_readMultiplyFirst.a;
    vec4 heightfield_readAddSecond = vec4(-1.0, -1.0, -1.0, fMinHeight);

    // Based on the grid, used for snapping the grid
    float mipMetersPerHeightfieldTexel = metersPerHeightFieldTexel * exp2(fGridLevel);

    // Translation of the grid at this vertex
    vec2 objectToWorld = roundToIncrement(tObjectInfo.tInfo.tPos.xz, mipMetersPerHeightfieldTexel);

    vec4 inPosition = vec4(
        tObjectInfo.tInfo.fScale * inPos.x * metersPerHeightFieldTexel + objectToWorld.x,
        0.0,
        tObjectInfo.tInfo.fScale * inPos.z * metersPerHeightFieldTexel + objectToWorld.y,
        1.0);

    // Where is the grid point in the grid's object space? 
    // This will determine the level used for elevation fetch
    vec3 osPosition = inPosition.xyz - tObjectInfo.tInfo.tPos.xyz;

    // Reach size = 1 at the border of the level 0 grid, which has radius baseGridSizeTexels/2,
    // but never go below 1.0 / 4.0, the "-1" level.
    vec2 tMaxComponent = abs(osPosition.xz * 2.0 * invBaseGridSizeTexels * tObjectInfo.tInfo.fUnitConversion);
    // vec2 tMaxComponent = abs(osPosition.xz * 2.0 * invBaseGridSizeTexels);
    float size = max(0.5, max(tMaxComponent.x, tMaxComponent.y));

    // The heightfield texture level must be negatively biased to hit max resolution 
    // before the highest resolution grid, otherwise texture swim will result on
    // nearby surfaces. Increase the magnitude of the negative bias term towards -inf if transitions
    // are too obvious. Decrease it towards zero if tessellation is being wasted on surfaces
    // that are too blocky.
    fGridLevel = max(log2(size) - 0.75, 0.0);
    tShaderIn.fGridLevel = fGridLevel;

    // We want to sample from index (wsPosition.xz * heightfieldTexelsPerMeter + 0.5), but
    // at the appropriate texture resolution and using texture coordinates.
    float lowMIP = floor(fGridLevel);
    float highMIP = lowMIP + 1.0;

    float fractionalLevel = fGridLevel - lowMIP;

    // How many high-level texels to offset to achieve a half-texel offset at this MIP level
    float highMIPHalfTexelOffset = exp2(lowMIP);
    float lowMIPHalfTexelOffset = highMIPHalfTexelOffset * 0.5;

    vec2 lowMIPTexCoord  = (inPosition.xz * heightfieldTexelsPerMeter + lowMIPHalfTexelOffset)  * heightfield_invSize.xy;
    vec2 highMIPTexCoord = (inPosition.xz * heightfieldTexelsPerMeter + highMIPHalfTexelOffset) * heightfield_invSize.xy;

    lowMIPTexCoord.x  += 0.5f;
    lowMIPTexCoord.y  += 0.5f;
    highMIPTexCoord.x += 0.5f;
    highMIPTexCoord.y += 0.5f;

    // vec2 lowMIPTexCoord  = (inPosition.xz * heightfieldTexelsPerMeter + lowMIPHalfTexelOffset)  * heightfield_invSize.xy;
    // vec2 highMIPTexCoord = (inPosition.xz * heightfieldTexelsPerMeter + highMIPHalfTexelOffset) * heightfield_invSize.xy;

    // Manual trilinear interpolation
    float lowMIPValue;
    
    if (lowMIP > 0)
    {
        lowMIPValue = textureLod(sampler2D(tHeightMap, tMirrorSampler), lowMIPTexCoord, lowMIP).w;
    }
    else
    {
        // At the lowest LOD, smooth out sharp corners (which often occur because
        // the input is 8 bit). Providing this branch costs about 0.1 ms but dramatically
        // improves quality.
        const float smoothness = 0.35;
        lowMIPValue  = 
         (textureLod(sampler2D(tHeightMap, tMirrorSampler), vec2( heightfield_invSize.x,  heightfield_invSize.y) * smoothness + lowMIPTexCoord, lowMIP).w +
          textureLod(sampler2D(tHeightMap, tMirrorSampler), vec2( heightfield_invSize.x, -heightfield_invSize.y) * smoothness + lowMIPTexCoord, lowMIP).w +
          textureLod(sampler2D(tHeightMap, tMirrorSampler), vec2(-heightfield_invSize.x, -heightfield_invSize.y) * smoothness + lowMIPTexCoord, lowMIP).w +
          textureLod(sampler2D(tHeightMap, tMirrorSampler), vec2(-heightfield_invSize.x,  heightfield_invSize.y) * smoothness + lowMIPTexCoord, lowMIP).w) * 0.25;

        // Break up very flat surfaces
        // lowMIPValue += (noise(inPosition.xz * 0.5, 3) - 0.5) * 0.75 * heightfield_invReadMultiplyFirst;
    }
    float highMIPValue = textureLod(sampler2D(tHeightMap, tMirrorSampler), highMIPTexCoord, highMIP).w;
    inPosition.y = (mix(lowMIPValue, highMIPValue, fractionalLevel) * heightfield_readMultiplyFirst.w + heightfield_readAddSecond.w) + verticalOffset;

    // float fXMaxCoord = lowMIPTexCoord.x - min(tObjectInfo.tInfo.fXUVOffset, tObjectInfo.tInfo.tExtentsRanges.z);
    float fXMaxCoord = lowMIPTexCoord.x - tObjectInfo.tInfo.fXUVOffset;
    float fXMinCoord = lowMIPTexCoord.x - tObjectInfo.tInfo.fXUVOffset;
    float fYMaxCoord = lowMIPTexCoord.y - tObjectInfo.tInfo.fYUVOffset;
    float fYMinCoord = lowMIPTexCoord.y - tObjectInfo.tInfo.fYUVOffset;

    if (fXMinCoord < 0.0 || fYMinCoord < 0.0 || fXMaxCoord > 1.0 || fYMaxCoord > 1.0)
    {
        // Fold down edges
        inPosition.y = -10000.0;
    }

    // vec2 tWorld = inPosition.xz;
    // vec2 tMin = tObjectInfo.tInfo.tExtents.xy;
    // vec2 tMax = tObjectInfo.tInfo.tExtents.zw;
    // if(tWorld.x < tMin.x || tWorld.y < tMin.y || tWorld.x > tMax.x || tWorld.y > tMax.y)
    // {
    //     // Fold down edges
        // inPosition.y = -10000.0;
    //     // inPosition.y = -100.0 / 100.0;
    // }

    gl_Position = tObjectInfo.tInfo.tCameraViewProjection * inPosition;
    tShaderIn.tPosition = inPosition.xyz;
}