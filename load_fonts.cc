#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <list>
#include "msg.hh"
#ifdef UseFTGL
#include <FTGL/ftgl.h>
#else
#include "fontlib.hh"
#endif

#define FONTSIZE 20

class dcFont
{
    public:
        dcFont();
        virtual ~dcFont();

        void create(const char *, const char *);
        char *getFileName(void);
        char *getFace(void);
        void *getID(void);

    private:
        char *filename;
        char *face;
        void *id;
};

static std::list<dcFont *> myfonts;

void *LoadFont(const char *filename, const char *face)
{
    std::list<dcFont *>::iterator dcf;
    for (dcf = myfonts.begin(); dcf != myfonts.end(); dcf++)
    {
        if (!strcmp((*dcf)->getFileName(), filename))
        {
            if (face && (*dcf)->getFace())
            {
                if (!strcasecmp((*dcf)->getFace(), face)) return (*dcf)->getID();
            }
            else if (!face && !((*dcf)->getFace())) return (*dcf)->getID();
        }
    }

    dcFont *newfont = new dcFont;
    newfont->create(filename, face);
    myfonts.push_back(newfont);
    return newfont->getID();
}

dcFont::dcFont()
:
filename(0x0),
face(0x0),
id(0x0)
{
}

dcFont::~dcFont()
{
    if (this->filename) free(this->filename);
    if (this->face) free(this->face);
}

void dcFont::create(const char *infile, const char *inface)
{
    if (!infile) return;
    this->filename = strdup(infile);
    if (inface) this->face = strdup(inface);

#ifdef UseFTGL
        this->id = ftglCreateTextureFont(this->filename);
        if (!(this->id))
        {
            error_msg("Could not load font `%s'", this->filename);
            return;
        }
        ftglSetFontCharMap(this->id, ft_encoding_unicode);
        ftglSetFontFaceSize(this->id, FONTSIZE, 72); // hard code font size, then use glScale when rendering to size the font
#else
        this->id = flInitFont(this->filename, this->face, FONTSIZE);
        if (!(this->id))
        {
            error_msg("Could not load font `%s'", this->filename);
            return;
        }
#endif
}

char * dcFont::getFileName(void)
{
    return this->filename;
}

char * dcFont::getFace(void)
{
    return this->face;
}

void * dcFont::getID(void)
{
    return this->id;
}
