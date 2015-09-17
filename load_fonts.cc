#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "nodes.hh"
#include "opengl_draw.hh"
#include "msg.hh"
#ifdef UseFTGL
#include <FTGL/ftgl.h>
#else
#include "fontlib.hh"
#endif

#define FONTSIZE 20

static struct node *FontList;

static void *FindFont(char *, char *);

void *LoadFont(char *filename, char *face)
{
    void *myfont;
    struct node *data;

    // check if this font has already been loaded
    myfont = FindFont(filename, face);

    // if not, create a new font node and load the font
    if (myfont == 0)
    {
        data = NewNode(0x0, &FontList);
#ifdef UseFTGL
        myfont = ftglCreateTextureFont(filename);
        if (!myfont)
        {
            error_msg("Could not load font `%s'", filename);
            return 0;
        }
        ftglSetFontCharMap(myfont, ft_encoding_unicode);
        ftglSetFontFaceSize(myfont, FONTSIZE, 72); // hard code font size, then use glScale when rendering to size the font
#else
        myfont = flInitFont(filename, face, FONTSIZE);
        if (!myfont)
        {
            error_msg("Could not load font `%s'", filename);
            return 0;
        }
#endif
        data->object.fonts.fontFile = strdup(filename);
        if (face) data->object.fonts.fontFace = strdup(face);
        else data->object.fonts.fontFace = 0;
        data->object.fonts.fontID = myfont;
    }

    return myfont;
}

/*********************************************************************************
 *
 * This function will determine if a texture file has already been loaded.
 *
 *********************************************************************************/
static void *FindFont(char *filename, char *face)
{
    // Traverse the list to find the font file name
    for (struct node *current = FontList; current; current = current->p_next)
    {
        if (!strcmp(current->object.fonts.fontFile, filename))
        {
            if (face && current->object.fonts.fontFace)
            {
                if (!strcasecmp(current->object.fonts.fontFace, face)) return (current->object.fonts.fontID);
            }
            else if (!face && !(current->object.fonts.fontFace)) return (current->object.fonts.fontID);
        }
    }

    // If we made it here, we didn't find the font.
    return 0;
}
