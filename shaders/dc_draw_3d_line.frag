// forked from pilotlight (https://github.com/PilotLightTech/pilotlight)
#version 450 core

//-----------------------------------------------------------------------------
// [SECTION] input
//-----------------------------------------------------------------------------

layout(location = 0) in vec4 Color;
layout(location = 1) noperspective in float DashDistance;
layout(location = 2) flat in uint LineData;

//-----------------------------------------------------------------------------
// [SECTION] output
//-----------------------------------------------------------------------------

layout(location = 0) out vec4 fColor;

void main()
{
    uint uPattern = LineData & 255u;
    if(uPattern != 0u && uPattern != 255u)
    {
        uint uBit = uint(mod(DashDistance, 20.0) / 2.5) & 7u;
        if(((uPattern >> uBit) & 1u) == 0u)
            discard;
    }

    fColor = Color;
}
