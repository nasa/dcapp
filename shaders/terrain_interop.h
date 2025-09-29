
#include "../pilotlight/shaders/pl_shader_interop.h"

PL_BEGIN_STRUCT(plTerrainDynamicData)

    vec4 tPos;
    vec4 tSunDirection;
    mat4 tCameraViewProjection;

    float  fMetersPerHeightFieldTexel;
    float  fMaxHeight;
    float  fMinHeight;
    float  fScale;
    
    float  fUnitConversion;
    int    iXOffset;
    int    iYOffset;
    int    iTileSize;

    float fGlobalMaxHeight;
    float fGlobalMinHeight;
    float fXUVOffset;
    float fYUVOffset;



PL_END_STRUCT(plTerrainDynamicData)