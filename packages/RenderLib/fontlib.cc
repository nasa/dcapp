#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <strings.h>
#include "basicutils/msg.hh"
#include "RenderLib/RenderLib.hh"
#include "fontlib.hh"

FT_Library tdFont::library = 0;

/*************************/
/******** UNICODE ********/
/*************************/

#define UNI_REPLACEMENT_CHAR (UTF32)0x0000FFFD
#define UNI_MAX_LEGAL_UTF32  (UTF32)0x0010FFFF
#define UNI_SUR_HIGH_START   (UTF32)0xD800
#define UNI_SUR_LOW_END      (UTF32)0xDFFF

const char tdFont::trailingBytesForUTF8[] =
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

const UTF32 tdFont::offsetsFromUTF8[] = { 0x00000000UL, 0x00003080UL, 0x000E2080UL, 0x03C82080UL, 0xFA082080UL, 0x82082080UL };

/*************************/
/******** UNICODE ********/
/*************************/


tdFont::tdFont(const std::string &filespec, const std::string &facespec, unsigned int basespec)
:
face(0x0),
kern_flag(false),
filename(filespec),
facename(facespec),
basesize(basespec),
descender(0),
max_advance(0),
max_advance_alnum(0),
max_advance_numeric(0),
valid(false)
{
    if (!library)
    {
        if (FT_Init_FreeType(&library))
        {
            warning_msg("Couldn't initialize FreeType library");
            return;
        }
    }

    int ret = FT_New_Face(library, this->filename.c_str(), 0, &(this->face));
    if (ret == FT_Err_Unknown_File_Format)
    {
        warning_msg("The font file " << this->filename << " appears to be in an unsupported format");
        return;
    }
    else if (ret)
    {
        warning_msg("The font file " << this->filename << " could not be opened or read");
        return;
    }

    this->valid = true;

    if (!(this->facename.empty()))
    {
        FT_Face tmpface, newface = 0;
        for (int i = 0; i < this->face->num_faces && !newface; i++)
        {
            FT_New_Face(library, this->filename.c_str(), i, &tmpface);
            if (this->facename == tmpface->style_name) newface = tmpface;
            else FT_Done_Face(tmpface);
        }
        if (newface)
        {
            FT_Done_Face(this->face);
            this->face = newface;
        }
    }

    if (FT_Set_Pixel_Sizes(this->face, this->basesize, this->basesize))
    {
        warning_msg("Unable to set pixel size");
    }

    // set font-wide settings here
    this->kern_flag = FT_HAS_KERNING(this->face);
    this->descender = (float)(this->face->size->metrics.descender>>6);

    // pre-load commonly-used glyphs
    for (UTF32 glyph=PRELOAD_START; glyph<=PRELOAD_END; glyph++) this->loadGlyphInfo(&(this->gdata[glyph]), &(this->gdata_outline[glyph]), glyph);
}


tdFont::~tdFont()
{
    FT_Done_Face(this->face);
}


float tdFont::getAdvance(const std::string &instring, flMonoOption mono=flMonoNone)
{
    if (!(this->valid)) return 0;

    UTF8 *current = (UTF8 *)instring.c_str();
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


float tdFont::getDescender(void)
{
    return this->descender;
}


bool tdFont::isValid(void)
{
    return this->valid;
}


std::string tdFont::getFileName(void)
{
    return this->filename;
}


std::string tdFont::getFaceName(void)
{
    return this->facename;
}


unsigned int tdFont::getBaseSize(void)
{
    return this->basesize;
}


void tdFont::render(const std::string &instring, bool outline, flMonoOption mono=flMonoNone)
{
    if (!(this->valid)) return;

    UTF8 *current = (UTF8 *)instring.c_str();
    UTF32 out;
    FT_Vector kern;
    FT_UInt myindex, previndex = 0;
    GlyphInfo *ginfo;
    float xkern, xadvance;

    while (*current)
    {
        current += this->convertUTF8toUTF32(current, &out);

        if (!outline)
        	ginfo = this->getGlyphInfo(out);
        else
        	ginfo = this->getGlyphOutlineInfo(out);

        if (ginfo)
        {
            xkern = 0;

            if (mono == flMonoAll)
                xadvance = this->max_advance;
            else if (mono == flMonoAlphaNumeric && isalnum(out))
                xadvance = this->max_advance_alnum;
            else if (mono == flMonoNumeric && isdigit(out))
                xadvance = this->max_advance_numeric;
            else
            {
                xadvance = ginfo->advance;
                if (this->kern_flag)
                {
                    myindex = FT_Get_Char_Index(this->face, out);
                    if (myindex)
                    {
                        FT_Get_Kerning(this->face, previndex, myindex, FT_KERNING_DEFAULT, &kern);
                        xkern = (float)(kern.x>>6);
                    }
                    previndex = myindex;
                }
            }
    		draw_glyph(ginfo->texture, ginfo->bitmap_left, ginfo->bitmap_top, xkern, xadvance);
        }
    }
}


void tdFont::loadGlyphInfo(GlyphInfo *ginfo, GlyphInfo *ginfo_outline, UTF32 index)
{
    unsigned int i, j;
    unsigned char bitmap[64][64];

    FT_Load_Char(this->face, index, FT_LOAD_RENDER);
    FT_GlyphSlot slot = this->face->glyph;

    // pad image to make it a 64x64 for texture rendering
    for (i=0; i<64; i++)
    {
        for (j=0; j<64; j++)
        {
            if (i < (unsigned int)(slot->bitmap.rows) && j < (unsigned int)(slot->bitmap.width))
                bitmap[i][j] = slot->bitmap.buffer[(i * slot->bitmap.width) + j];
            else
                bitmap[i][j] = 0;
        }
    }
    unsigned char bitmap_outline[64][64] = {0};
    static const unsigned char circular_filter[3][3] = {
    	{1, 1, 1},
    	{1, 0, 1},
    	{1, 1, 1}
    };
    unsigned char bitmap_translated[64][64] = {0};
    for (i=0; i<63; i++) {
        for (j=0; j<63; j++) {
        	bitmap_translated[i+1][j+1] = bitmap[i][j];
        }
    }

    int ci, cj, ii, jj;
    for (i=0; i<64; i++) {
        for (j=0; j<64; j++) {
            if (bitmap_translated[i][j]) {
            	for( ci=0; ci<3; ci++ )
            		for ( cj=0; cj<3; cj++ ) {
            			ii = i+ci-1;
            			jj = j+cj-1;
            			if ( ii >= 0 && ii < 64 && jj >= 0 && jj < 64 && circular_filter[ci][cj] && bitmap_translated[ii][jj] < 100 ) {
            				bitmap_outline[ii][jj] = std::min(255, bitmap_translated[i][j] + bitmap_outline[ii][jj]);
            				//bitmap_outline[ii][jj] = std::max(150, (int)bitmap_outline[ii][jj]);
            			}
            		}
            }
        }
    }

    ginfo->index = index;
    ginfo->bitmap_left = (float)(slot->bitmap_left);
    ginfo->bitmap_top = (float)(slot->bitmap_top - 64);
    ginfo->width = slot->bitmap.width;
    ginfo->height = slot->bitmap.rows;
    ginfo->advance = (float)(slot->advance.x>>6);

    // copy to outline glyphInfo
    *ginfo_outline = *ginfo;

    if (ginfo->advance > this->max_advance) this->max_advance = ginfo->advance;
    if (isalnum(index) && ginfo->advance > this->max_advance_alnum) this->max_advance_alnum = ginfo->advance;
    if (isdigit(index) && ginfo->advance > this->max_advance_numeric) this->max_advance_numeric = ginfo->advance;

    create_and_load_glyph(&(ginfo->texture), bitmap_translated);
    create_and_load_glyph(&(ginfo_outline->texture), bitmap_outline);
}


GlyphInfo * tdFont::getGlyphInfo(UTF32 index)
{
    // first, check our pre-load list
    if (index >= PRELOAD_START && index <= PRELOAD_END) return (&(this->gdata[index]));

    // if not in the pre-load list, check the extras list
    if (extras.find(index) != extras.end()) return extras[index];

    // if not in either list, learn it and load it into extras
    GlyphInfo *newglyph = new GlyphInfo;
    GlyphInfo *newglyphOutline = new GlyphInfo;   
    this->loadGlyphInfo(newglyph, newglyphOutline, index);
    extras[index] = newglyph;
    extras_outline[index] = newglyphOutline;
    return newglyph;
}

GlyphInfo * tdFont::getGlyphOutlineInfo(UTF32 index)
{
    // first, check our pre-load list
    if (index >= PRELOAD_START && index <= PRELOAD_END) return (&(this->gdata_outline[index]));

    // if not in the pre-load list, check the extras list
    if (extras_outline.find(index) != extras_outline.end()) return extras_outline[index];

    // if not in either list, learn it and load it into extras
    GlyphInfo *newglyph = new GlyphInfo;
    GlyphInfo *newglyphOutline = new GlyphInfo;   
    this->loadGlyphInfo(newglyph, newglyphOutline, index);
    extras[index] = newglyph;
    extras_outline[index] = newglyphOutline;
    return newglyph;
}


int tdFont::convertUTF8toUTF32(UTF8 *source, UTF32 *dest)
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


bool tdFont::isLegalUTF8(const UTF8 *source, int length)
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
