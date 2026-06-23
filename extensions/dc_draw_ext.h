/*
   dc_draw_ext.h
     * forked from pilotlight (https://github.com/PilotLightTech/pilotlight)
     * a simple 2D & 3D drawing API (mostly for debug drawing)
*/

/*
Index of this file:
// [SECTION] implementation notes
// [SECTION] header mess
// [SECTION] defines
// [SECTION] apis
// [SECTION] includes
// [SECTION] forward declarations & basic types
// [SECTION] public api struct
// [SECTION] enums
// [SECTION] structs
// [SECTION] structs for backends
*/

//-----------------------------------------------------------------------------
// [SECTION] implementation notes
//-----------------------------------------------------------------------------

/*

    Implementation:
        The provided implementation of this extension depends on the following
        APIs being available:

        * plFileI (v1.x)
*/

//-----------------------------------------------------------------------------
// [SECTION] header mess
//-----------------------------------------------------------------------------

#ifndef DC_DRAW_EXT_H
#define DC_DRAW_EXT_H

//-----------------------------------------------------------------------------
// [SECTION] defines
//-----------------------------------------------------------------------------

#define DC_UNICODE_CODEPOINT_INVALID 0xFFFD // invalid Unicode code point (standard value).
#define DC_UNICODE_CODEPOINT_MAX     0xFFFF // maximum Unicode code point supported by this build.
#define DC_DRAW_STENCIL_MAX_DEPTH    8

//-----------------------------------------------------------------------------
// [SECTION] apis
//-----------------------------------------------------------------------------

// dcapp's custom draw API (separate from pilotlight's plDrawI)
#define dcDrawI_version {1, 7, 0}

//-----------------------------------------------------------------------------
// [SECTION] includes
//-----------------------------------------------------------------------------

#include <stdint.h>
#include "pl_math.h"

//-----------------------------------------------------------------------------
// [SECTION] forward declarations & basic types
//-----------------------------------------------------------------------------

// basic types
typedef struct _dcDrawInit    dcDrawInit;    // initialization options (reserved for future use)
typedef struct _dcFontAtlas   dcFontAtlas;   // font atlas data
typedef struct _dcDrawList2D  dcDrawList2D;  // drawlist data for 2D
typedef struct _dcDrawList3D  dcDrawList3D;  // drawlist data for 3D
typedef struct _dcDrawLayer2D dcDrawLayer2D; // opaque type for 2D draw layers
typedef struct _dcDrawCommand   dcDrawCommand;   // opaque type for 2D draw layers
typedef struct _dcDrawCommand3D dcDrawCommand3D; // 3D draw command
typedef struct _dcDrawStencilState dcDrawStencilState;
typedef struct _dcDrawCommandState dcDrawCommandState;

// vertex buffer types
typedef struct _dcDrawVertex          dcDrawVertex;          // vertex type (LAYOUT & PADDING MATTERS)
typedef struct _dcDrawVertex3DSolid   dcDrawVertex3DSolid;   // vertex type (LAYOUT & PADDING MATTERS)
typedef struct _dcDrawVertex3DLine    dcDrawVertex3DLine;    // vertex type (LAYOUT & PADDING MATTERS)
typedef struct _dcDrawVertex3DTextured dcDrawVertex3DTextured; // vertex type (LAYOUT & PADDING MATTERS)
typedef struct _dcDraw3DText          dcDraw3DText;

// primitive options
typedef struct _dcDrawLineOptions  dcDrawLineOptions;  // options for lines
typedef struct _dcDrawSolidOptions dcDrawSolidOptions; // options for solids
typedef struct _dcDrawTextOptions  dcDrawTextOptions;  // options for text
typedef struct _dcDrawFrustumDesc  dcDrawFrustumDesc;  // description for drawing frustums 

// font types
typedef struct _dcFontRange      dcFontRange;      // a range of characters
typedef struct _dcFont           dcFont;           // a single font with a specific size and config
typedef struct _dcFontConfig     dcFontConfig;     // configuration for loading a single font
typedef struct _dcFontChar       dcFontChar;       // internal type
typedef struct _dcFontGlyph      dcFontGlyph;      // internal type
typedef struct _dcFontCustomRect dcFontCustomRect; // internal type
typedef struct _plRenderEncoder  plRenderEncoder;  // pl_graphics_ext.h

// advanced callbacks (you probably shouldn't be using this, mostly for backends)
typedef void (*dcDrawCallback)(const dcDrawList2D*, const dcDrawCommand*);
#define dcDrawCallbackResetRenderState (dcDrawCallback)(-8)
#define dcDrawCallbackSetShader        (dcDrawCallback)(-9)

// 3D callbacks
typedef void (*dcDrawCallback3D)(const dcDrawList3D*, const dcDrawCommand3D*);
#define dcDrawCallback3DSetShader (dcDrawCallback3D)(-10)

// character types
typedef uint16_t dcUiWChar;

// enums
typedef int dcDrawFlags;     // -> enum _dcDrawFlags     // Flags:
typedef int dcDrawRectFlags; // -> enum _dcDrawRectFlags // Flags:
typedef int dcDrawCommand3DType; // -> enum _dcDrawCommand3DType
typedef int dcDrawCommandFlags; // -> enum _dcDrawCommandFlags
typedef int dcDrawTextFlags; // -> enum _dcDrawTextFlags

// backend texture type
#ifndef plTextureID
    typedef uint32_t plTextureID;
#endif

//-----------------------------------------------------------------------------
// [SECTION] public api struct
//-----------------------------------------------------------------------------

typedef struct _dcDrawI
{
    // init/cleanup
    void (*initialize)(const dcDrawInit*);
    void (*cleanup)   (void); // usually called by backend "cleanup" func.

    // per frame
    void (*new_frame)(void); // usually called by backend "new_frame" func.

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~fonts~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // font atlas
    dcFontAtlas* (*create_font_atlas)     (void);
    bool         (*prepare_font_atlas)    (dcFontAtlas*); // usually called by backend "build_font_atlas" func.
    void         (*cleanup_font_atlas)    (dcFontAtlas*); // usually called by backend "cleanup_font_atlas" func.
    void         (*set_font_atlas)        (dcFontAtlas*);
    dcFontAtlas* (*get_current_font_atlas)(void);

    dcFont* (*get_first_font)          (dcFontAtlas*);
    dcFont* (*add_default_font)        (dcFontAtlas*);
    dcFont* (*add_font_from_file_ttf)  (dcFontAtlas*, dcFontConfig, const char* file);
    dcFont* (*add_font_from_memory_ttf)(dcFontAtlas*, dcFontConfig, void* data);
    plVec2  (*calculate_text_size)     (const char* text, dcDrawTextOptions);
    plRect  (*calculate_text_bb)       (plVec2 p, const char* text, dcDrawTextOptions);

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~2D~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // drawlists
    dcDrawList2D* (*request_2d_drawlist)(void);
    void          (*return_2d_drawlist) (dcDrawList2D*);
    void          (*prepare_2d_drawlist)(dcDrawList2D*); // usually called by backend "submit_2d_drawlist" func.

    // layers
    dcDrawLayer2D* (*request_2d_layer)(dcDrawList2D*);
    void           (*return_2d_layer) (dcDrawLayer2D*);
    void           (*submit_2d_layer) (dcDrawLayer2D*);

    // drawing (lines)
    void (*add_line)          (dcDrawLayer2D*, plVec2 p0, plVec2 p1, dcDrawLineOptions);
    void (*add_lines)         (dcDrawLayer2D*, plVec2* points, uint32_t count, dcDrawLineOptions);
    void (*add_triangle)      (dcDrawLayer2D*, plVec2 p0, plVec2 p1, plVec2 p2, dcDrawLineOptions);
    void (*add_rect)          (dcDrawLayer2D*, plVec2 pMin, plVec2 pMax, dcDrawLineOptions);
    void (*add_rect_rounded)  (dcDrawLayer2D*, plVec2 pMin, plVec2 pMax, float radius, uint32_t segments, dcDrawRectFlags, dcDrawLineOptions);
    void (*add_quad)          (dcDrawLayer2D*, plVec2 p0, plVec2 p1, plVec2 p2, plVec2 p3, dcDrawLineOptions);
    void (*add_circle)        (dcDrawLayer2D*, plVec2 p, float radius, uint32_t segments, dcDrawLineOptions);
    void (*add_polygon)               (dcDrawLayer2D*, plVec2* points, uint32_t count, dcDrawLineOptions);
    void (*add_polygon_rounded)       (dcDrawLayer2D*, plVec2* points, uint32_t count, float radius, uint32_t segments, dcDrawLineOptions);
    void (*add_bezier_quad)   (dcDrawLayer2D*, plVec2 p0, plVec2 p1, plVec2 p2, uint32_t segments, dcDrawLineOptions);
    void (*add_bezier_cubic)  (dcDrawLayer2D*, plVec2 p0, plVec2 p1, plVec2 p2, plVec2 p3, uint32_t segments, dcDrawLineOptions);

    // drawing (solids)
    void (*add_triangle_filled)      (dcDrawLayer2D*, plVec2 p0, plVec2 p1, plVec2 p2, dcDrawSolidOptions);
    void (*add_triangles_filled)     (dcDrawLayer2D*, plVec2* points, uint32_t count, dcDrawSolidOptions);
    void (*add_rect_filled)          (dcDrawLayer2D*, plVec2 minP, plVec2 maxP, dcDrawSolidOptions);
    void (*add_rect_rounded_filled)  (dcDrawLayer2D*, plVec2 minP, plVec2 maxP, float radius, uint32_t segments, dcDrawRectFlags, dcDrawSolidOptions);
    void (*add_quad_filled)          (dcDrawLayer2D*, plVec2 p0, plVec2 p1, plVec2 p2, plVec2 p3, dcDrawSolidOptions);
    void (*add_circle_filled)        (dcDrawLayer2D*, plVec2 p, float radius, uint32_t segments, dcDrawSolidOptions);
    void (*add_convex_polygon_filled)        (dcDrawLayer2D*, plVec2* points, uint32_t count, dcDrawSolidOptions);
    void (*add_convex_polygon_rounded_filled)(dcDrawLayer2D*, plVec2* points, uint32_t count, float radius, uint32_t segments, dcDrawSolidOptions);
    void (*add_image)                (dcDrawLayer2D*, plTextureID, plVec2 minP, plVec2 maxP);
    void (*add_image_ex)             (dcDrawLayer2D*, plTextureID, plVec2 minP, plVec2 maxP, plVec2 minUV, plVec2 maxUV, uint32_t color);
    void (*add_image_quad)           (dcDrawLayer2D*, plTextureID, plVec2 p0, plVec2 p1, plVec2 p2, plVec2 p3);
    void (*add_image_quad_ex)        (dcDrawLayer2D*, plTextureID, plVec2 p0, plVec2 p1, plVec2 p2, plVec2 p3, plVec2 p0UV, plVec2 p1UV, plVec2 p2UV, plVec2 p3UV, uint32_t color);

    // drawing (text)
    void (*add_text)        (dcDrawLayer2D*, plVec2 p, const char* text, dcDrawTextOptions);
    void (*add_text_clipped)(dcDrawLayer2D*, plVec2 p, const char* text, plVec2 clipMin, plVec2 clipMax, dcDrawTextOptions);

    // clipping
    void          (*push_clip_rect_pt)(dcDrawList2D*, const plRect*, bool bAccumulate);
    void          (*push_clip_rect)   (dcDrawList2D*, plRect, bool bAccumulate);
    void          (*pop_clip_rect)    (dcDrawList2D*);
    const plRect* (*get_clip_rect)    (dcDrawList2D*);

    // advanced (you probably shouldn't be using this, mostly for backends)
    void (*add_2d_callback)(dcDrawLayer2D*, dcDrawCallback, void* userData, uint32_t userDataSize);
    void (*add_3d_callback)(dcDrawList3D*,  dcDrawCallback3D, void* userData, uint32_t userDataSize);
    void (*set_2d_command_state)(dcDrawLayer2D*, dcDrawCommandState);
    void (*set_3d_command_state)(dcDrawList3D*,  dcDrawCommandState);

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~3D~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // drawlists
    dcDrawList3D* (*request_3d_drawlist)(void);
    void          (*return_3d_drawlist)(dcDrawList3D*);

    // text
    void (*add_3d_text)(dcDrawList3D*, plVec3 p, const char* text, dcDrawTextOptions);

    // solid
    void (*add_3d_triangle_filled)    (dcDrawList3D*, plVec3 p0, plVec3 p1, plVec3 p2, dcDrawSolidOptions);
    void (*add_3d_circle_xz_filled)   (dcDrawList3D*, plVec3 center, float radius, uint32_t segments, dcDrawSolidOptions);
    void (*add_3d_band_xz_filled)     (dcDrawList3D*, plVec3 center, float innerRadius, float outerRadius, uint32_t segments, dcDrawSolidOptions);
    void (*add_3d_band_xy_filled)     (dcDrawList3D*, plVec3 center, float innerRadius, float outerRadius, uint32_t segments, dcDrawSolidOptions);
    void (*add_3d_band_yz_filled)     (dcDrawList3D*, plVec3 center, float innerRadius, float outerRadius, uint32_t segments, dcDrawSolidOptions);
    void (*add_3d_centered_box_filled)(dcDrawList3D*, plVec3 center, float width, float height, float depth, dcDrawSolidOptions);
    void (*add_3d_plane_xz_filled)    (dcDrawList3D*, plVec3 center, float width, float height, dcDrawSolidOptions);
    void (*add_3d_plane_xy_filled)    (dcDrawList3D*, plVec3 center, float width, float height, dcDrawSolidOptions);
    void (*add_3d_plane_yz_filled)    (dcDrawList3D*, plVec3 center, float width, float height, dcDrawSolidOptions);
    void (*add_3d_sphere_filled)      (dcDrawList3D*, plSphere, uint32_t latBands, uint32_t longBands, dcDrawSolidOptions);
    void (*add_3d_cylinder_filled)    (dcDrawList3D*, plCylinder, uint32_t segments, dcDrawSolidOptions);
    void (*add_3d_cone_filled)        (dcDrawList3D*, plCone, uint32_t segments, dcDrawSolidOptions);

    // textured
    void (*add_3d_sphere_textured)    (dcDrawList3D*, plTextureID, plSphere, const plMat4* transform, uint32_t latBands, uint32_t longBands, uint32_t color);

    // wireframe
    void (*add_3d_line)        (dcDrawList3D*, plVec3 p0, plVec3 p1, dcDrawLineOptions);
    void (*add_3d_cross)       (dcDrawList3D*, plVec3 p, float length, dcDrawLineOptions);
    void (*add_3d_transform)   (dcDrawList3D*, const plMat4* transform, float length, dcDrawLineOptions);
    void (*add_3d_frustum)     (dcDrawList3D*, const plMat4* transform, dcDrawFrustumDesc, dcDrawLineOptions);
    void (*add_3d_centered_box)(dcDrawList3D*, plVec3 center, float width, float height, float depth, dcDrawLineOptions);
    void (*add_3d_aabb)        (dcDrawList3D*, plVec3 minP, plVec3 maxP, dcDrawLineOptions);
    void (*add_3d_bezier_quad) (dcDrawList3D*, plVec3 p0, plVec3 p1, plVec3 p2, uint32_t segments, dcDrawLineOptions);
    void (*add_3d_bezier_cubic)(dcDrawList3D*, plVec3 p0, plVec3 p1, plVec3 p2, plVec3 tP3, uint32_t segments, dcDrawLineOptions);
    void (*add_3d_circle_xz)   (dcDrawList3D*, plVec3 center, float radius, uint32_t segments, dcDrawLineOptions);
    void (*add_3d_sphere)      (dcDrawList3D*, plSphere, uint32_t latBands, uint32_t longBands, dcDrawLineOptions);
    void (*add_3d_capsule)     (dcDrawList3D*, plCapsule, uint32_t latBands, uint32_t longBands, dcDrawLineOptions);
    void (*add_3d_cylinder)    (dcDrawList3D*, plCylinder, uint32_t segments, dcDrawLineOptions);
    void (*add_3d_cone)        (dcDrawList3D*, plCone, uint32_t segments, dcDrawLineOptions);

} dcDrawI;

//-----------------------------------------------------------------------------
// [SECTION] enums
//-----------------------------------------------------------------------------

enum _dcDrawFlags
{
    DC_DRAW_FLAG_NONE            = 0,
    DC_DRAW_FLAG_DEPTH_TEST      = 1 << 0,
    DC_DRAW_FLAG_DEPTH_WRITE     = 1 << 1,
    DC_DRAW_FLAG_CULL_FRONT      = 1 << 2,
    DC_DRAW_FLAG_CULL_BACK       = 1 << 3,
    DC_DRAW_FLAG_FRONT_FACE_CW   = 1 << 4,
    DC_DRAW_FLAG_REVERSE_Z_DEPTH = 1 << 5,
};

enum _dcDrawCommand3DType
{
    DC_DRAW_COMMAND_3D_SOLID,
    DC_DRAW_COMMAND_3D_LINE,
    DC_DRAW_COMMAND_3D_TEXTURED,
};

enum _dcDrawCommandFlags
{
    DC_DRAW_COMMAND_FLAG_NONE        = 0,
    DC_DRAW_COMMAND_FLAG_SDF         = 1 << 0,
    DC_DRAW_COMMAND_FLAG_SDF_BOLD    = 1 << 1,
    DC_DRAW_COMMAND_FLAG_SDF_OUTLINE = 1 << 2,
};

enum _dcDrawTextFlags
{
    DC_DRAW_TEXT_FLAG_NONE    = 0,
    DC_DRAW_TEXT_FLAG_BOLD    = 1 << 0,
    DC_DRAW_TEXT_FLAG_OUTLINE = 1 << 1,
};

typedef enum _dcDrawStencilMode
{
    DC_DRAW_STENCIL_MODE_NONE,
    DC_DRAW_STENCIL_MODE_CREATE,
    DC_DRAW_STENCIL_MODE_CLEAR,
    DC_DRAW_STENCIL_MODE_DRAW,
} dcDrawStencilMode;

typedef struct _dcDrawStencilState
{
    dcDrawStencilMode tMode;
    uint8_t           uDepth;
} dcDrawStencilState;

typedef struct _dcDrawCommandState
{
    uint32_t           tFlags;
    dcDrawStencilState tStencil;
} dcDrawCommandState;

enum _dcDrawRectFlags
{
    DC_DRAW_RECT_FLAG_NONE                       = 0, // default: DC_DRAW_RECT_FLAG_ROUND_CORNERS_All
    DC_DRAW_RECT_FLAG_ROUND_CORNERS_TOP_LEFT     = 1 << 0,
    DC_DRAW_RECT_FLAG_ROUND_CORNERS_TOP_RIGHT    = 1 << 1,
    DC_DRAW_RECT_FLAG_ROUND_CORNERS_BOTTOM_LEFT  = 1 << 2,
    DC_DRAW_RECT_FLAG_ROUND_CORNERS_BOTTOM_RIGHT = 1 << 4,
    DC_DRAW_RECT_FLAG_ROUND_CORNERS_NONE         = 1 << 5,
    DC_DRAW_RECT_FLAG_ROUND_CORNERS_TOP          = DC_DRAW_RECT_FLAG_ROUND_CORNERS_TOP_LEFT | DC_DRAW_RECT_FLAG_ROUND_CORNERS_TOP_RIGHT,
    DC_DRAW_RECT_FLAG_ROUND_CORNERS_BOTTOM       = DC_DRAW_RECT_FLAG_ROUND_CORNERS_BOTTOM_LEFT | DC_DRAW_RECT_FLAG_ROUND_CORNERS_BOTTOM_RIGHT,
    DC_DRAW_RECT_FLAG_ROUND_CORNERS_LEFT         = DC_DRAW_RECT_FLAG_ROUND_CORNERS_TOP_LEFT | DC_DRAW_RECT_FLAG_ROUND_CORNERS_BOTTOM_LEFT,
    DC_DRAW_RECT_FLAG_ROUND_CORNERS_RIGHT        = DC_DRAW_RECT_FLAG_ROUND_CORNERS_TOP_RIGHT | DC_DRAW_RECT_FLAG_ROUND_CORNERS_BOTTOM_RIGHT,
    DC_DRAW_RECT_FLAG_ROUND_CORNERS_All          = DC_DRAW_RECT_FLAG_ROUND_CORNERS_TOP_LEFT | DC_DRAW_RECT_FLAG_ROUND_CORNERS_TOP_RIGHT | DC_DRAW_RECT_FLAG_ROUND_CORNERS_BOTTOM_LEFT | DC_DRAW_RECT_FLAG_ROUND_CORNERS_BOTTOM_RIGHT,
};

//-----------------------------------------------------------------------------
// [SECTION] structs
//-----------------------------------------------------------------------------

typedef struct _dcDrawInit
{
    int _iUnused;
} dcDrawInit;

typedef struct _dcDrawFrustumDesc
{
    float fYFov;
    float fAspectRatio;
    float fNearZ;
    float fFarZ;
} dcDrawFrustumDesc;

typedef struct _dcDrawLineOptions
{
    uint32_t uColor;
    float    fThickness;
    uint8_t  uDashPattern;
} dcDrawLineOptions;

typedef struct _dcDrawSolidOptions
{
    uint32_t uColor;
} dcDrawSolidOptions;

typedef struct _dcDrawTextOptions
{
    dcFont*     ptFont;
    float       fSize;      // if zero, will use loaded size
    uint32_t    uColor;
    float       fWrap;      // 0.0f, no wrap
    uint32_t    tFlags;     // dcDrawTextFlags
    const char* pcTextEnd;  // if null terminated, set to NULL
    plMat3      tTransform; // default: identity
} dcDrawTextOptions;

typedef struct _dcFontRange
{
    int         iFirstCodePoint;
    uint32_t    uCharCount;

    // [INTERNAL]
    uint32_t _uConfigIndex;
} dcFontRange;

typedef struct _dcFontConfig
{
    float              fSize;
    const dcFontRange* ptRanges;
    uint32_t           uRangeCount;
    const int*         piIndividualChars;
    uint32_t           uIndividualCharCount;
    dcFont*            ptMergeFont;

    // BITMAP ONLY
    uint32_t uVOverSampling;
    uint32_t uHOverSampling;

    // SDF ONLY (ttf only)
    bool          bSdf;
    int           iSdfPadding;
    unsigned char ucOnEdgeValue;
    
    // [INTERNAL]
    dcFontRange* _sbtRanges;
    dcFontChar* _sbtCharData;
    float       _fSdfPixelDistScale;
    uint32_t    _uCustomRectOffset;
} dcFontConfig;

typedef struct _dcFontGlyph
{
    float x0;
    float y0;
    float u0;
    float v0;
    float x1;
    float y1;
    float u1;
    float v1;
    float fXAdvance;
    float fLeftBearing;  
    int   iSDF : 1;
} dcFontGlyph;

typedef struct _dcFont
{
    float fSize; // loaded size
    
    // [INTERNAL]
    float                   _fLineSpacing;
    dcFontRange*            _sbtRanges;
    uint32_t                _uCodePointCount;
    uint32_t*               _auCodePoints; // glyph index lookup based on codepoint
    dcFontGlyph*            _sbtGlyphs;     // glyphs
    dcFontConfig*           _sbtConfigs;
    struct _dcFontPrepData* _sbtPreps;
    dcFont*                 _ptNextFont;
    dcFontGlyph*            _ptFallbackGlyph;
} dcFont;

//-----------------------------------------------------------------------------
// [SECTION] structs for backends
//-----------------------------------------------------------------------------

typedef struct _dcDrawVertex
{
    float    afPos[2];
    float    afUv[2];
    uint32_t uColor;
} dcDrawVertex;

typedef struct _dcDrawVertex3DSolid
{
    float    afPos[3];
    uint32_t uColor;
} dcDrawVertex3DSolid;

typedef struct _dcDrawVertex3DLine
{
    float    afPos[3];
    float    fDirection;
    float    fThickness;
    float    fMultiply;
    float    afPosOther[3];
    uint32_t uColor;
} dcDrawVertex3DLine;

typedef struct _dcDrawVertex3DTextured
{
    float    afPos[3];
    float    afUv[2];
    uint32_t uColor;
} dcDrawVertex3DTextured;

typedef struct _dcDraw3DText
{
    dcFont*  ptFont;
    float    fSize;
    plVec3   tP;
    uint32_t uColor;
    char     acText[PL_MAX_NAME_LENGTH];
    float    fWrap;
} dcDraw3DText;

typedef struct _dcDrawCommand3D
{
    dcDrawCommand3DType eType;
    uint32_t            uVertexOffset;
    uint32_t            uIndexOffset;
    uint32_t            uElementCount;
    plTextureID         tTextureId;      // textured commands only
    dcDrawCommandState  tState;
    dcDrawCallback3D    tUserCallback;
    void*               pUserCallbackData;
    uint32_t            uUserCallbackDataSize;
} dcDrawCommand3D;

typedef struct _dcDrawList3D
{
    // solid
    dcDrawVertex3DSolid* sbtSolidVertexBuffer;
    uint32_t*            sbtSolidIndexBuffer;

    // lines
    dcDrawVertex3DLine*  sbtLineVertexBuffer;
    uint32_t*            sbtLineIndexBuffer;

    // textured
    dcDrawVertex3DTextured* sbtTexturedVertexBuffer;
    uint32_t*               sbtTexturedIndexBuffer;
    plTextureID             tTexturedTexture;

    // commands
    dcDrawCommand3D* sbtDrawCommands3D;

    // text
    dcDraw3DText*  sbtTextEntries;
    dcDrawList2D*  pt2dDrawlist;
    dcDrawLayer2D* ptLayer;

    // [INTERNAL]
    int                iLastCommand3D;
    dcDrawCommandState tCommandState;
} dcDrawList3D;

typedef struct _dcDrawCommand
{
    uint32_t       uVertexOffset;
    uint32_t       uIndexOffset;
    uint32_t       uElementCount;
    plTextureID    tTextureId;
    plRect         tClip;
    dcDrawCommandState tState;
    dcDrawCallback tUserCallback;
    void*          pUserCallbackData;
    uint32_t       uUserCallbackDataSize;
} dcDrawCommand;

typedef struct _dcDrawList2D
{
    dcDrawVertex*  sbtVertexBuffer;
    uint32_t*      sbuIndexBuffer;
    uint32_t       uIndexBufferByteSize;
    dcDrawCommand* sbtDrawCommands;
    
    // [INTERNAL]
    dcDrawLayer2D** _sbtSubmittedLayers;
    dcDrawLayer2D** _sbtLayerCache;
    dcDrawLayer2D** _sbtLayersCreated;
    plRect*         _sbtClipStack;
} dcDrawList2D;

typedef struct _dcFontAtlas
{

    plVec2         tAtlasSize;
    plTextureID    tTexture;
    unsigned char* pucPixelsAsRGBA32;
    void*          ptUserData;

    // [INTERNAL]
    dcFont*           _ptFontListHead;
    dcFontCustomRect* _sbtCustomRects;
    unsigned char*    _pucPixelsAsAlpha8;
    plVec2            _tWhiteUv;
    int               _iGlyphPadding;
    size_t            _szPixelDataSize;
    dcFontCustomRect* _ptWhiteRect;
    float             _fTotalArea;
} dcFontAtlas;

#endif // DC_DRAW_EXT_H
