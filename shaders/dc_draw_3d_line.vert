// forked from pilotlight (https://github.com/PilotLightTech/pilotlight)
#version 450 core

//-----------------------------------------------------------------------------
// [SECTION] dynamic bind group
//-----------------------------------------------------------------------------

layout(set = 3, binding = 0) uniform PL_DYNAMIC_DATA
{
    mat4 tMVP;
    vec2 tLogicalDimensions;
} tObjectInfo;

//-----------------------------------------------------------------------------
// [SECTION] input
//-----------------------------------------------------------------------------

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aInfo;
layout(location = 2) in vec3 aPosOther;
layout(location = 3) in uint aColor;
layout(location = 4) in float aDashDistance;
layout(location = 5) in uint aLineData;

//-----------------------------------------------------------------------------
// [SECTION] output
//-----------------------------------------------------------------------------

layout(location = 0) out vec4 Color;
layout(location = 1) noperspective out float DashDistance;
layout(location = 2) flat out uint LineData;

void main()
{
    Color = unpackUnorm4x8(aColor);
    DashDistance = aDashDistance;
    LineData = aLineData;

    vec4 tCurrentProj = tObjectInfo.tMVP * vec4(aPos.xyz, 1.0);
    vec4 tOtherProj   = tObjectInfo.tMVP * vec4(aPosOther.xyz, 1.0);

    float fCurrentW = abs(tCurrentProj.w) > 0.000001 ? tCurrentProj.w : (tCurrentProj.w < 0.0 ? -0.000001 : 0.000001);
    float fOtherW = abs(tOtherProj.w) > 0.000001 ? tOtherProj.w : (tOtherProj.w < 0.0 ? -0.000001 : 0.000001);
    vec2 tCurrentNDC = tCurrentProj.xy / fCurrentW;
    vec2 tOtherNDC = tOtherProj.xy / fOtherW;

    vec2 tDimensions = max(tObjectInfo.tLogicalDimensions, vec2(1.0));
    vec2 tDeltaPixels = (tOtherNDC - tCurrentNDC) * tDimensions * 0.5;
    float fLengthSquared = dot(tDeltaPixels, tDeltaPixels);
    vec2 tDirectionPixels = fLengthSquared > 0.000001
        ? aInfo.z * tDeltaPixels * inversesqrt(fLengthSquared)
        : vec2(0.0);
    vec2 tNormalPixels = vec2(-tDirectionPixels.y, tDirectionPixels.x) * aInfo.y * 0.5;
    vec2 tOffsetNDC = tNormalPixels * 2.0 / tDimensions;

    gl_Position = tCurrentProj;
    gl_Position.xy += tOffsetNDC * tCurrentProj.w * aInfo.x;
}
