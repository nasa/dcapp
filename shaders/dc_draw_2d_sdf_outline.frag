// SDF outline shader - single pass
// Renders outline in vertex color, with transparent interior (fill drawn separately on top)
// Based on technique from redblobgames.com/x/2404-distance-field-effects/
#version 450 core
#extension GL_ARB_separate_shader_objects : enable

//-----------------------------------------------------------------------------
// [SECTION] bind group 0
//-----------------------------------------------------------------------------

layout(set = 0, binding = 0)  uniform sampler tFontSampler;

//-----------------------------------------------------------------------------
// [SECTION] bind group 1
//-----------------------------------------------------------------------------

layout(set = 1, binding = 0)  uniform texture2D tFontAtlas;

//-----------------------------------------------------------------------------
// [SECTION] input
//-----------------------------------------------------------------------------

layout(location = 0) in struct { vec4 Color; vec2 UV; } In;

//-----------------------------------------------------------------------------
// [SECTION] output
//-----------------------------------------------------------------------------

layout(location = 0) out vec4 fColor;

void main()
{
    float fDistance = texture(sampler2D(tFontAtlas, tFontSampler), In.UV).a;
    float fSmoothWidth = fwidth(fDistance);

    // outer edge = where outline starts, inner edge = where fill takes over
    float fOuterEdge = 0.25;
    float fInnerEdge = 0.5;

    float fOuterAlpha = smoothstep(fOuterEdge - fSmoothWidth, fOuterEdge + fSmoothWidth, fDistance);
    float fInnerAlpha = smoothstep(fInnerEdge - fSmoothWidth, fInnerEdge + fSmoothWidth, fDistance);

    // only draw in the outline band (between outer and inner edges)
    float fOutlineAlpha = fOuterAlpha - fInnerAlpha;

    if(fOutlineAlpha <= 0.0)
        discard;

    fColor = vec4(In.Color.rgb, In.Color.a * fOutlineAlpha);
}
