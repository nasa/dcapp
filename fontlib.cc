#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include "fontlibdefs.hh"

/*************************/
/******** UNICODE ********/
/*************************/
#define UNI_REPLACEMENT_CHAR (UTF32)0x0000FFFD
#define UNI_MAX_LEGAL_UTF32  (UTF32)0x0010FFFF
#define UNI_SUR_HIGH_START   (UTF32)0xD800
#define UNI_SUR_LOW_END      (UTF32)0xDFFF

static const char trailingBytesForUTF8[256] =
{
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

static const UTF32 offsetsFromUTF8[6] = { 0x00000000UL, 0x00003080UL, 0x000E2080UL, 0x03C82080UL, 0xFA082080UL, 0x82082080UL };

/*************************/
/******** UNICODE ********/
/*************************/

static void LoadGlyphInfo(GlyphInfo *, flFont *, UTF32);
static GlyphInfo *GetGlyphInfo(flFont *, UTF32);
static int ConvertUTF8toUTF32(UTF8 *, UTF32 *);
static bool isLegalUTF8(const UTF8 *, int);


flFont *flInitFont(const char *filename, const char *facespec, unsigned int basesize)
{
    static flFont *myfont;
    static FT_Library library = 0;
    int ret, i;
    FT_Face tmpface, newface = 0;
    UTF32 glyph;

    if (!library)
    {
        if (FT_Init_FreeType(&library))
        {
            printf("ERROR: Couldn't initialize FreeType library\n");
            return 0x0;
        }
    }

    myfont = (flFont *)calloc(1, sizeof(flFont));
    if (!myfont)
    {
        printf("ERROR: Unable to allocate memory for font\n");
        return 0x0;
    }

    ret = FT_New_Face(library, filename, 0, &(myfont->face));
    if (ret == FT_Err_Unknown_File_Format)
    {
        printf("ERROR: The font file %s appears to be in an unsupported format\n", filename);
        return 0x0;
    }
    else if (ret)
    {
        printf("ERROR: The font file %s could not be opened or read\n", filename);
        return 0x0;
    }

    if (facespec)
    {
        for (i = 0; i < myfont->face->num_faces && !newface; i++)
        {
            FT_New_Face(library, filename, i, &tmpface);
            if (!strcasecmp(facespec, tmpface->style_name)) newface = tmpface;
            else FT_Done_Face(tmpface);
        }
        if (newface)
        {
            FT_Done_Face(myfont->face);
            myfont->face = newface;
        }
    }

    if (FT_Set_Pixel_Sizes(myfont->face, basesize, basesize))
    {
        printf("ERROR: Unable to set pixel size\n");
    }

    // set font-wide settings here
    myfont->kern_flag = FT_HAS_KERNING(myfont->face);
    myfont->basesize = basesize;
    myfont->descender = (float)(myfont->face->size->metrics.descender>>6);
    myfont->max_advance = 0;
    myfont->max_advance_alnum = 0;
    myfont->max_advance_numeric = 0;
    myfont->extras = 0;

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

    // pre-load commonly-used glyphs
    for (glyph=PRELOAD_START; glyph<=PRELOAD_END; glyph++) LoadGlyphInfo(&(myfont->gdata[glyph]), myfont, glyph);

    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);

    return myfont;
}


float flGetFontAdvance(flFont *fontID, flMonoOption mono, char *string)
{
    UTF8 *current = (UTF8 *)string;
    float adv = 0;
    UTF32 out;
    GlyphInfo *ginfo;
    FT_UInt myindex, previndex = 0;
    FT_Vector kern;

    if (!fontID) return 0;

    while (*current)
    {
        current += ConvertUTF8toUTF32(current, &out);

        if (mono == flMonoAll)
        {
            adv += fontID->max_advance;
            if (fontID->kern_flag) previndex = 0;
        }
        else if (mono == flMonoAlphaNumeric && isalnum(out))
        {
            adv += fontID->max_advance_alnum;
            if (fontID->kern_flag) previndex = 0;
        }
        else if (mono == flMonoNumeric && isdigit(out))
        {
            adv += fontID->max_advance_numeric;
            if (fontID->kern_flag) previndex = 0;
        }
        else
        {
            ginfo = GetGlyphInfo(fontID, out);

            if (ginfo) adv += ginfo->advance;

            if (fontID->kern_flag)
            {
                myindex = FT_Get_Char_Index(fontID->face, out);
                if (myindex)
                {
                    FT_Get_Kerning(fontID->face, previndex, myindex, FT_KERNING_DEFAULT, &kern);
                    adv += (float)(kern.x>>6);
                }
                previndex = myindex;
            }
        }
    }

    return adv;
}


float flGetFontDescender(flFont *fontID)
{
    if (fontID) return fontID->descender;
    else return 0;
}


unsigned int flGetFontBaseSize(flFont *fontID)
{
    if (fontID) return fontID->basesize;
    else return 0;
}


void flRenderFont(flFont *fontID, flMonoOption mono, char *string)
{
    if (!fontID) return;

    UTF8 *current = (UTF8 *)string;
    UTF32 out;
    FT_Vector kern;
    FT_UInt myindex, previndex = 0;
    GlyphInfo *ginfo;

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

    while (*current)
    {
        current += ConvertUTF8toUTF32(current, &out);

        ginfo = GetGlyphInfo(fontID, out);
        if (ginfo)
        {
            glTranslatef(ginfo->bitmap_left, ginfo->bitmap_top, 0);
            if (fontID->kern_flag)
            {
                if (mono == flMonoAll || (mono == flMonoAlphaNumeric && isalnum(out)) || (mono == flMonoNumeric && isdigit(out)))
                    previndex = 0;
                else
                {
                    myindex = FT_Get_Char_Index(fontID->face, out);
                    if (myindex)
                    {
                        FT_Get_Kerning(fontID->face, previndex, myindex, FT_KERNING_DEFAULT, &kern);
                        glTranslatef((float)(kern.x>>6), 0, 0);
                    }
                    previndex = myindex;
                }
            }

            glBindTexture(GL_TEXTURE_2D, ginfo->texture);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ginfo->width, ginfo->height, GL_ALPHA, GL_UNSIGNED_BYTE, ginfo->bitmap);

            glBegin(GL_QUADS);
                glTexCoord2f(0, 1);
                glVertex3f(0, 0, 0);
                glTexCoord2f(1, 1);
                glVertex3f(64, 0, 0);
                glTexCoord2f(1, 0);
                glVertex3f(64, 64, 0);
                glTexCoord2f(0, 0);
                glVertex3f(0, 64, 0);
            glEnd();

            if (mono == flMonoAll)
                glTranslatef(fontID->max_advance - ginfo->bitmap_left, -ginfo->bitmap_top, 0);
            else if (mono == flMonoAlphaNumeric && isalnum(out))
                glTranslatef(fontID->max_advance_alnum - ginfo->bitmap_left, -ginfo->bitmap_top, 0);
            else if (mono == flMonoNumeric && isdigit(out))
                glTranslatef(fontID->max_advance_numeric - ginfo->bitmap_left, -ginfo->bitmap_top, 0);
            else
                glTranslatef(ginfo->advance - ginfo->bitmap_left, -ginfo->bitmap_top, 0);
        }
    }

    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}


static void LoadGlyphInfo(GlyphInfo *ginfo, flFont *fontID, UTF32 index)
{
    int i, j;
    unsigned char bitmap[64][64];

    FT_Load_Char(fontID->face, index, FT_LOAD_RENDER);
    FT_GlyphSlot slot = fontID->face->glyph;

    glGenTextures(1, &(ginfo->texture));
    glBindTexture(GL_TEXTURE_2D, ginfo->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // pad image to make it a 64x64 for texture rendering
    for (i=0; i<64; i++)
    {
        for (j=0; j<64; j++)
        {
            if (i<slot->bitmap.rows && j<slot->bitmap.width) bitmap[i][j] = slot->bitmap.buffer[(i * slot->bitmap.width) + j];
            else bitmap[i][j] = 0;
        }
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 64, 64, 0, GL_ALPHA, GL_UNSIGNED_BYTE, bitmap);
    bcopy(slot->bitmap.buffer, ginfo->bitmap, slot->bitmap.width * slot->bitmap.rows);

    ginfo->index = index;
    ginfo->bitmap_left = (float)(slot->bitmap_left);
    ginfo->bitmap_top = (float)(slot->bitmap_top - 64);
    ginfo->width = slot->bitmap.width;
    ginfo->height = slot->bitmap.rows;
    ginfo->advance = (float)(slot->advance.x>>6);

    if (ginfo->advance > fontID->max_advance) fontID->max_advance = ginfo->advance;
    if (isalnum(index) && ginfo->advance > fontID->max_advance_alnum) fontID->max_advance_alnum = ginfo->advance;
    if (isdigit(index) && ginfo->advance > fontID->max_advance_numeric) fontID->max_advance_numeric = ginfo->advance;
}


static GlyphInfo *GetGlyphInfo(flFont *fontID, UTF32 index)
{
    // first, check our pre-load list
    if (index >= PRELOAD_START && index <= PRELOAD_END) return (&(fontID->gdata[index]));

    // if not in the pre-load list, check the extras list
    GlyphNode *current;

    for (current = fontID->extras; current; current = current->next)
    {
        if (current->gdata.index == index) return &(current->gdata);
    }

    // if we got here, we didn't find the glyph in extras, so learn it and load it into extras

    // if not in either list, learn it and load it into extras
    GlyphNode **newnode;

    for (newnode = &(fontID->extras); *newnode != 0; newnode = &((*newnode)->next));

    *newnode = (GlyphNode *)calloc(1, sizeof(GlyphNode));
    LoadGlyphInfo(&((*newnode)->gdata), fontID, index);
    (*newnode)->next = 0;

    return &((*newnode)->gdata);
}


static int ConvertUTF8toUTF32(UTF8 *source, UTF32 *dest)
{
    unsigned short extraBytesToRead = trailingBytesForUTF8[*source];

    if (isLegalUTF8(source, extraBytesToRead+1))
    {
        *dest = 0;
        switch (extraBytesToRead)
        {
            case 5: *dest += *source++; *dest <<= 6;
            case 4: *dest += *source++; *dest <<= 6;
            case 3: *dest += *source++; *dest <<= 6;
            case 2: *dest += *source++; *dest <<= 6;
            case 1: *dest += *source++; *dest <<= 6;
            case 0: *dest += *source++;
        }
        *dest -= offsetsFromUTF8[extraBytesToRead];

        if ((*dest > UNI_MAX_LEGAL_UTF32) || (*dest >= UNI_SUR_HIGH_START && *dest <= UNI_SUR_LOW_END)) *dest = UNI_REPLACEMENT_CHAR;
    }
    else *dest = UNI_REPLACEMENT_CHAR;

    return extraBytesToRead+1;
}


static bool isLegalUTF8(const UTF8 *source, int length)
{
    UTF8 a;
    const UTF8 *srcptr = source+length;

    switch (length)
    {
    default:
        return false;
        /* Everything else falls through when "true"... */
    case 4:
        if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;
    case 3:
        if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;
    case 2:
        if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;

        switch (*source)
        {
            /* no fall-through in this inner switch */
            case 0xE0:
                if (a < 0xA0) return false;
                break;
            case 0xED:
                if (a > 0x9F) return false;
                break;
            case 0xF0:
                if (a < 0x90) return false;
                break;
            case 0xF4:
                if (a > 0x8F) return false;
                break;
            default:
                if (a < 0x80) return false;
        }

    case 1:
        if (*source >= 0x80 && *source < 0xC2) return false;
    }

    if (*source > 0xF4) return false;
    return true;
}
