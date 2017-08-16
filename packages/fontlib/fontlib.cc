#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <strings.h>
#include "fontlib.hh"

FT_Library flFont::library = 0;

/*************************/
/******** UNICODE ********/
/*************************/

#define UNI_REPLACEMENT_CHAR (UTF32)0x0000FFFD
#define UNI_MAX_LEGAL_UTF32  (UTF32)0x0010FFFF
#define UNI_SUR_HIGH_START   (UTF32)0xD800
#define UNI_SUR_LOW_END      (UTF32)0xDFFF

const char flFont::trailingBytesForUTF8[] =
{
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3, 4,4,4,4,5,5,5,5
};

const UTF32 flFont::offsetsFromUTF8[] = { 0x00000000UL, 0x00003080UL, 0x000E2080UL, 0x03C82080UL, 0xFA082080UL, 0x82082080UL };

/*************************/
/******** UNICODE ********/
/*************************/


flFont::flFont(const char *filespec, const char *facespec, unsigned int basesize)
:
face(0x0),
kern_flag(false),
basesize(0),
descender(0),
max_advance(0),
max_advance_alnum(0),
max_advance_numeric(0),
valid(false)
{
    int ret, i;
    FT_Face tmpface, newface = 0;
    UTF32 glyph;

    if (filespec) this->filename = filespec;
    if (facespec) this->facename = facespec;
    this->basesize = basesize;

    if (!library)
    {
        if (FT_Init_FreeType(&library))
        {
            printf("ERROR: Couldn't initialize FreeType library\n");
            return;
        }
    }

    ret = FT_New_Face(library, filespec, 0, &(this->face));
    if (ret == FT_Err_Unknown_File_Format)
    {
        printf("ERROR: The font file %s appears to be in an unsupported format\n", filespec);
        return;
    }
    else if (ret)
    {
        printf("ERROR: The font file %s could not be opened or read\n", filespec);
        return;
    }

    this->valid = true;

    if (facespec)
    {
        for (i = 0; i < this->face->num_faces && !newface; i++)
        {
            FT_New_Face(library, filespec, i, &tmpface);
            if (!strcasecmp(facespec, tmpface->style_name)) newface = tmpface;
            else FT_Done_Face(tmpface);
        }
        if (newface)
        {
            FT_Done_Face(this->face);
            this->face = newface;
        }
    }

    if (FT_Set_Pixel_Sizes(this->face, basesize, basesize))
    {
        printf("ERROR: Unable to set pixel size\n");
    }

    // set font-wide settings here
    this->kern_flag = FT_HAS_KERNING(this->face);
    this->descender = (float)(this->face->size->metrics.descender>>6);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

    // pre-load commonly-used glyphs
    for (glyph=PRELOAD_START; glyph<=PRELOAD_END; glyph++) this->loadGlyphInfo(&(this->gdata[glyph]), glyph);

    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}


flFont::~flFont()
{
    FT_Done_Face(this->face);
}


float flFont::getAdvance(const char *string, flMonoOption mono=flMonoNone)
{
    if (!(this->valid)) return 0;

    UTF8 *current = (UTF8 *)string;
    float adv = 0;
    UTF32 out;
    GlyphInfo *ginfo;
    FT_UInt myindex, previndex = 0;
    FT_Vector kern;

    while (*current)
    {
        current += this->convertUTF8toUTF32(current, &out);

        if (mono == flMonoAll)
        {
            adv += this->max_advance;
            if (this->kern_flag) previndex = 0;
        }
        else if (mono == flMonoAlphaNumeric && isalnum(out))
        {
            adv += this->max_advance_alnum;
            if (this->kern_flag) previndex = 0;
        }
        else if (mono == flMonoNumeric && isdigit(out))
        {
            adv += this->max_advance_numeric;
            if (this->kern_flag) previndex = 0;
        }
        else
        {
            ginfo = this->getGlyphInfo(out);

            if (ginfo) adv += ginfo->advance;

            if (this->kern_flag)
            {
                myindex = FT_Get_Char_Index(this->face, out);
                if (myindex)
                {
                    FT_Get_Kerning(this->face, previndex, myindex, FT_KERNING_DEFAULT, &kern);
                    adv += (float)(kern.x>>6);
                }
                previndex = myindex;
            }
        }
    }

    return adv;
}


float flFont::getDescender(void)
{
    return this->descender;
}


bool flFont::isValid(void)
{
    return this->valid;
}


std::string flFont::getFileName(void)
{
    return this->filename;
}


std::string flFont::getFaceName(void)
{
    return this->facename;
}


unsigned int flFont::getBaseSize(void)
{
    return this->basesize;
}


void flFont::render(const char *string, flMonoOption mono=flMonoNone)
{
    if (!(this->valid)) return;

    UTF8 *current = (UTF8 *)string;
    UTF32 out;
    FT_Vector kern;
    FT_UInt myindex, previndex = 0;
    GlyphInfo *ginfo;

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

    while (*current)
    {
        current += this->convertUTF8toUTF32(current, &out);

        ginfo = this->getGlyphInfo(out);
        if (ginfo)
        {
            glTranslatef(ginfo->bitmap_left, ginfo->bitmap_top, 0);
            if (this->kern_flag)
            {
                if (mono == flMonoAll || (mono == flMonoAlphaNumeric && isalnum(out)) || (mono == flMonoNumeric && isdigit(out)))
                    previndex = 0;
                else
                {
                    myindex = FT_Get_Char_Index(this->face, out);
                    if (myindex)
                    {
                        FT_Get_Kerning(this->face, previndex, myindex, FT_KERNING_DEFAULT, &kern);
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
                glTranslatef(this->max_advance - ginfo->bitmap_left, -ginfo->bitmap_top, 0);
            else if (mono == flMonoAlphaNumeric && isalnum(out))
                glTranslatef(this->max_advance_alnum - ginfo->bitmap_left, -ginfo->bitmap_top, 0);
            else if (mono == flMonoNumeric && isdigit(out))
                glTranslatef(this->max_advance_numeric - ginfo->bitmap_left, -ginfo->bitmap_top, 0);
            else
                glTranslatef(ginfo->advance - ginfo->bitmap_left, -ginfo->bitmap_top, 0);
        }
    }

    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}


void flFont::loadGlyphInfo(GlyphInfo *ginfo, UTF32 index)
{
    unsigned int i, j;
    unsigned char bitmap[64][64];

    FT_Load_Char(this->face, index, FT_LOAD_RENDER);
    FT_GlyphSlot slot = this->face->glyph;

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

    if (ginfo->advance > this->max_advance) this->max_advance = ginfo->advance;
    if (isalnum(index) && ginfo->advance > this->max_advance_alnum) this->max_advance_alnum = ginfo->advance;
    if (isdigit(index) && ginfo->advance > this->max_advance_numeric) this->max_advance_numeric = ginfo->advance;
}


GlyphInfo * flFont::getGlyphInfo(UTF32 index)
{
    // first, check our pre-load list
    if (index >= PRELOAD_START && index <= PRELOAD_END) return (&(this->gdata[index]));

    // if not in the pre-load list, check the extras list
    if (extras.find(index) != extras.end()) return extras[index];

    // if not in either list, learn it and load it into extras
    GlyphInfo *newglyph = new GlyphInfo;        
    this->loadGlyphInfo(newglyph, index);
    extras[index] = newglyph;
    return newglyph;
}


int flFont::convertUTF8toUTF32(UTF8 *source, UTF32 *dest)
{
    unsigned short extraBytesToRead = trailingBytesForUTF8[*source];

    if (this->isLegalUTF8(source, extraBytesToRead+1))
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


bool flFont::isLegalUTF8(const UTF8 *source, int length)
{
    UTF8 a;
    const UTF8 *srcptr = source+length;

    switch (length)
    {
        default: return false;
            /* Everything else falls through when "true"... */
        case 4: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;
        case 3: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;
        case 2: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;

            switch (*source)
            {
                /* no fall-through in this inner switch */
                case 0xE0: if (a < 0xA0) return false; break;
                case 0xED: if (a > 0x9F) return false; break;
                case 0xF0: if (a < 0x90) return false; break;
                case 0xF4: if (a > 0x8F) return false; break;
                default:   if (a < 0x80) return false;
            }

        case 1: if (*source >= 0x80 && *source < 0xC2) return false;
    }

    if (*source > 0xF4) return false;

    return true;
}
