#ifndef _FONTLIB_HH_
#define _FONTLIB_HH_

#include <string>
#include <map>
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
    unsigned int texture;
} GlyphInfo;

typedef std::map<UTF32, GlyphInfo *> GlyphSet;

class tdFont
{
    public:
        tdFont(const char *, const char *, unsigned int);
        virtual ~tdFont();
        float getAdvance(std::string, flMonoOption);
        float getDescender(void);
        bool isValid(void);
        std::string getFileName(void);
        std::string getFaceName(void);
        unsigned int getBaseSize(void);
        void render(std::string, flMonoOption);
    private:
        static FT_Library library;
        static const char trailingBytesForUTF8[256];
        static const UTF32 offsetsFromUTF8[6];

        FT_Face face;
        FT_Bool kern_flag;
        std::string filename;
        std::string facename;
        FT_UInt basesize;
        float descender;
        float max_advance;
        float max_advance_alnum;
        float max_advance_numeric;
        GlyphInfo gdata[PRELOAD_END+1];
        GlyphSet extras;
        bool valid;

        void loadGlyphInfo(GlyphInfo *, UTF32);
        GlyphInfo *getGlyphInfo(UTF32);
        int convertUTF8toUTF32(UTF8 *, UTF32 *);
        bool isLegalUTF8(const UTF8 *, int);
};

#endif
