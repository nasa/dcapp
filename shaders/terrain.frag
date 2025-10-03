#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "terrain.glsl"

//-----------------------------------------------------------------------------
// [SECTION] input & output
//-----------------------------------------------------------------------------

layout(set = 0, binding = 0)  uniform sampler tSampler;
layout(set = 0, binding = 1)  uniform texture2D tHeightMap;
layout(set = 0, binding = 2)  uniform texture2D tNoiseTexture;
layout(set = 0, binding = 3)  uniform texture2D tDiffuseTexture;
layout(set = 0, binding = 4)  uniform sampler tMirrorSampler;

layout(set = 3, binding = 0) uniform PL_DYNAMIC_DATA
{
    plTerrainDynamicData tInfo;
} tObjectInfo;

layout(location = 0) out vec4 outColor;

// output
layout(location = 0) in struct plShaderIn {
    vec3 tPosition;
    float fGridLevel;
} tShaderIn;


void computeTriplanar(in vec3 pos, in vec3 normal, out vec3 texCoordNorthSouth, out vec3 texCoordFlat, out vec3 texCoordEastWest) {
	// Calculate weights for blending
	vec3 weights = vec3(abs(normal.x), max(0.0, abs(normal.y) - 0.55), abs(normal.z));
	weights.x = pow(weights.x, 64);
	weights.y = pow(weights.y, 64);
	weights.z = pow(weights.z, 64);
	weights *= 1.0 / (weights.x + weights.y + weights.z);				
		
	// Perform the uv projection on to each plane: these are the texture coordinates
	texCoordNorthSouth  = vec3(pos.z, sign(normal.x) * 2.0 * pos.x, weights.x);
	texCoordFlat        = vec3(pos.x, pos.z, weights.y);
	texCoordEastWest    = vec3(pos.x, sign(normal.z) * 2.0 * pos.z, weights.z);
}

vec3 readTextures(vec3 texCoord)
{
    // We have to compute gradients outside of the branches, since within
    // a pixel-quad any given branch may be ignored.
    vec2 gradX = dFdx(texCoord.xy);
    vec2 gradY = dFdy(texCoord.xy);

    vec3 value =  textureGrad(sampler2D(tDiffuseTexture, tMirrorSampler), texCoord.xy, gradX, gradY).rgb;

    return value;
}

//-----------------------------------------------------------------------------
// [SECTION] entry
//-----------------------------------------------------------------------------

void main() 
{

    vec2 tTextureSize = textureSize(sampler2D(tHeightMap, tSampler), 0);
    vec2 heightfield_invSize = vec2(1.0 / tTextureSize.x, 1.0 / tTextureSize.y);
    float metersPerHeightFieldTexel = tObjectInfo.tInfo.fMetersPerHeightFieldTexel / tObjectInfo.tInfo.fUnitConversion;
    float fMaxHeight = tObjectInfo.tInfo.fGlobalMaxHeight / tObjectInfo.tInfo.fUnitConversion;
    float fMinHeight = tObjectInfo.tInfo.fGlobalMinHeight / tObjectInfo.tInfo.fUnitConversion;
    float heightfieldTexelsPerMeter = 1.0 / metersPerHeightFieldTexel;

    float baseGridSizeTexels = 128.0;
    float verticalOffset = 0.0;
    float materialTilesPerMeter = 4.0;

    vec2 lowFreqTexCoord = tShaderIn.tPosition.xz * materialTilesPerMeter;
    float lowFreqNoiseValue1 = texture(sampler2D(tNoiseTexture, tMirrorSampler), lowFreqTexCoord * (1.0 / 32.0)).r;
    float lowFreqNoiseValue2 = texture(sampler2D(tNoiseTexture, tMirrorSampler), lowFreqTexCoord * (1.0 / 500.0)).r;

    vec4 heightfield_readMultiplyFirst = vec4(2.0, 2.0, 2.0, fMaxHeight - fMinHeight);
    vec4 heightfield_readAddSecond = vec4(-1.0, -1.0, -1.0, fMinHeight);

    vec2 UV = (tShaderIn.tPosition.xz * heightfieldTexelsPerMeter + 0.5) * heightfield_invSize.xy;

    UV.x += 0.5;
    UV.y += 0.5; // 225 / 4127
    // UV.y += 0.455; // 225 / 4127

    // vec2 UV = (tShaderIn.tPosition.xz * heightfieldTexelsPerMeter + 0.5) * heightfield_invSize.xy;

    vec4 temp0 = textureLod(sampler2D(tHeightMap, tMirrorSampler), UV, 0.0);
    vec4 temp = temp0 * heightfield_readMultiplyFirst + heightfield_readAddSecond;
    
    vec3 shadingPosition = vec3(tShaderIn.tPosition.x, temp.a, tShaderIn.tPosition.z);
    vec3 shadingNormal = normalize(temp.xyz);

    // Weights are stored in the z channel of the texture coordinates
    vec3 texCoordNorthSouth, texCoordFlat, texCoordEastWest;
    computeTriplanar(shadingPosition * materialTilesPerMeter, shadingNormal + vec3(0,(lowFreqNoiseValue1 + lowFreqNoiseValue2 - 0.2) * 0.5,0), texCoordNorthSouth, texCoordFlat, texCoordEastWest);

    vec3 sunlightColor = vec3(1.0, 1.0, 1.0);

    vec3 cameraPos = tObjectInfo.tInfo.tPos.xyz;
    vec3 w_o = normalize(cameraPos - shadingPosition); 
    vec3 w_i = normalize(-tObjectInfo.tInfo.tSunDirection.xyz);
    vec3 w_h = normalize(w_i + w_o);

    // vec3 diffuse = vec3(1.0);
    // vec3 diffuse = vec3(shadingPosition.y / (fMaxHeight - fMinHeight), 0.5, 0.5);
    vec3 diffuse = readTextures(texCoordFlat) * 1.5;
    // if(tShaderIn.fGridLevel < 3.0)
    // {
    //     diffuse = readTextures(texCoordFlat);
    // }
    
    vec3 ambient = vec3(0.01, 0.01, 0.01);

    diffuse *= min(1.2,
                (noise(lowFreqTexCoord * 0.2, 2) * 0.5 + 0.8) *
                (texture(sampler2D(tNoiseTexture, tMirrorSampler), lowFreqTexCoord * (1.0 / 4.5)).r   * 0.9 + 0.6) *
                (lowFreqNoiseValue1 * 0.8 + 0.6) *
                (lowFreqNoiseValue2 * 0.8 + 0.6));

    outColor.xyz = diffuse * (max(0.0, dot(shadingNormal, w_i)) * sunlightColor + ambient) +
            sunlightColor * (0.1 * pow(max(0.0, dot(shadingNormal, w_h)), 6));



    // vec2 tHazardMin = vec2(100, 100);
    // vec2 tHazardMax = vec2(200, 200);
    vec2 tHazardMin = vec2(-22785, 14486);
    vec2 tHazardMax = tHazardMin + vec2(2000, 2000);

    tHazardMin /= tObjectInfo.tInfo.fUnitConversion;
    tHazardMax /= tObjectInfo.tInfo.fUnitConversion;

    // -11397.171875 7246.453125


    if(shadingPosition.x < tHazardMax.x && shadingPosition.x > tHazardMin.x &&
        shadingPosition.z < tHazardMax.y && shadingPosition.z > tHazardMin.y)
    {
        float costheta = dot(shadingNormal, vec3(0.0, 1.0, 0.0));
        if(costheta > 0.99) // 6 degrees
        {
            outColor.r *= 0.5;
            outColor.g *= 2.0;
            outColor.b *= 0.5;
        }
        else if(costheta > 0.97) // 15 degrees
        {
            outColor.r *= 2.0;
            outColor.g *= 2.0;
            outColor.b *= 0.5;
           
        }
        else
        {
            outColor.r *= 2.0;
            outColor.g *= 0.5;
            outColor.b *= 0.5;
        }
    }



    outColor.a = 1.0;




    // outColor = vec4(shadingNormal, 1.0); 
}