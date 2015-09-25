#ifndef _FONTLIBDEFS_HH_
#define _FONTLIBDEFS_HH_

#include <GL/glu.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#define PRELOAD_START 0x20
#define PRELOAD_END 0x7e

typedef unsigned int UTF32;
typedef unsigned char UTF8;

typedef enum { flMonoNone, flMonoNumeric, flMonoAlphaNumeric, flMonoAll } flMonoOption;

typedef struct
{
    UTF32 index;
    FT_Int width;
    FT_Int height;
    float bitmap_left;
    float bitmap_top;
    float advance;
    GLuint texture;
    unsigned char bitmap[64*64];
} GlyphInfo;

typedef struct glyphnode
{
    GlyphInfo gdata;
    struct glyphnode *next;
} GlyphNode;

typedef struct
{
    FT_Face face;
    FT_Bool kern_flag;
    FT_UInt basesize;
    float descender;
    float max_advance;
    float max_advance_alnum;
    float max_advance_numeric;
    GlyphInfo gdata[PRELOAD_END+1];
    GlyphNode *extras;
} flFont;

#endif
