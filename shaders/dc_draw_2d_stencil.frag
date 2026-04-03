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
    // dash pattern: UV.y >= 1.0 signals a dashed line
    if(In.UV.y >= 1.0)
    {
        int iPattern = int(In.UV.y + 0.5);
        float fPos = fract(In.UV.x);
        int iBit = int(fPos * 8.0) & 7;
        if(((iPattern >> iBit) & 1) == 0)
            discard;

        fColor = vec4(0.0);
    }
    else
    {
        vec4 texColor = texture(sampler2D(tFontAtlas, tFontSampler), In.UV.st);
        float alpha = In.Color.a * texColor.a;

        // Discard transparent fragments so they don't write to stencil buffer
        if(alpha < 0.5)
            discard;

        // Output doesn't matter since colorWriteMask=0, but required for valid shader
        fColor = vec4(0.0);
    }
}
