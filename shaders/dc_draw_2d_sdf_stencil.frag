// forked from pilotlight (https://github.com/PilotLightTech/pilotlight)
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

    // Discard fragments outside the glyph (distance < 0.5 means outside)
    // This ensures stencil buffer only gets written where text actually is
    if(fDistance < 0.5)
        discard;

    // Output doesn't matter since colorWriteMask=0, but required for valid shader
    fColor = vec4(0.0);
}
