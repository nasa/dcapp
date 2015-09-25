#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <list>
#include "dc.hh"
#include "msg.hh"
#include "imgload/imgload.hh"
#include "fontlib.hh"

#define DEFAULTBASESIZE 20

typedef struct
{
    dcTexture id;
    char *filename;
} textureInfo;

typedef struct
{
    dcFont id;
    const char *filename;
    const char *face;
    unsigned int basesize;
} fontInfo;

static std::list<textureInfo> textures;
static std::list<fontInfo> fonts;
static std::list<float> floatConstants;
static std::list<int> integerConstants;
static std::list<char *> stringConstants;

static char *getFullPath(const char *fname)
{
    if (!fname) return 0x0;
	char *longname = (char *)calloc(PATH_MAX, sizeof(char));
	if (!realpath(fname, longname))
	{
	    free(longname);
	    return 0x0;
	}
	char *shortname = strdup(longname);
	free(longname);
	return shortname;
}

dcTexture dcLoadTexture(const char *filename)
{
    if (!filename) return -1;

    char *fullpath = getFullPath(filename);

    if (!fullpath)
    {
        error_msg("Unable to locate texture file at `%s'", filename);
        return -1;
    }

    for (std::list<textureInfo>::iterator item = textures.begin(); item != textures.end(); item++)
    {
        if (!strcmp((*item).filename, fullpath))
        {
            free(fullpath);
            return (*item).id;
        }
    }

    textureInfo newtexture;
    newtexture.id = imgload(fullpath);;
    newtexture.filename = fullpath;
    textures.push_back(newtexture);
    return newtexture.id;
}

dcFont dcLoadFont(const char *filename, const char *face, unsigned int basesize)
{
    if (!filename) return 0x0;

    char *fullpath = getFullPath(filename);

    if (!fullpath)
    {
        error_msg("Unable to locate font file at `%s'", filename);
        return 0x0;
    }

    for (std::list<fontInfo>::iterator item = fonts.begin(); item != fonts.end(); item++)
    {
        if (!strcmp((*item).filename, fullpath) && ((*item).basesize == basesize))
        {
            if (!(*item).face && !face)
            {
                free(fullpath);
                return (*item).id;
            }
            else if ((*item).face && face)
            {
                if (!strcmp((*item).face, face))
                {
                    free(fullpath);
                    return (*item).id;
                }
            }
        }
    }

    fontInfo newfont;
    newfont.id = flInitFont(fullpath, face, basesize);
    newfont.filename = fullpath;
    if (face) newfont.face = strdup(face);
    else newfont.face = 0x0;
    newfont.basesize = basesize;
    fonts.push_back(newfont);

    if (!(newfont.id))
    {
        error_msg("Could not load font `%s'", filename);
        return 0x0;
    }

    return newfont.id;
}

dcFont dcLoadFont(const char *filename, const char *face)
{
    return dcLoadFont(filename, face, DEFAULTBASESIZE);
}

dcFont dcLoadFont(const char *filename)
{
    return dcLoadFont(filename, 0x0, DEFAULTBASESIZE);
}

float *dcLoadConstant(float fval)
{
    std::list<float>::iterator fc;
    for (fc = floatConstants.begin(); fc != floatConstants.end(); fc++)
    {
        if (*fc == fval) return &(*fc);
    }
    floatConstants.push_back(fval);
    return &(floatConstants.back());
}

int *dcLoadConstant(int ival)
{
    std::list<int>::iterator ic;
    for (ic = integerConstants.begin(); ic != integerConstants.end(); ic++)
    {
        if (*ic == ival) return &(*ic);
    }
    integerConstants.push_back(ival);
    return &(integerConstants.back());
}

char *dcLoadConstant(const char *sval)
{
    std::list<char *>::iterator sc;
    for (sc = stringConstants.begin(); sc != stringConstants.end(); sc++)
    {
        if (!strcmp(*sc, sval)) return *sc;
    }
    stringConstants.push_back(strdup(sval));
    return stringConstants.back();
}
