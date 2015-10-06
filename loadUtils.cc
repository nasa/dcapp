#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <list>
#include "dc.hh"
#include "msg.hh"
#include "imgload/imgload.hh"
#include "fontlib.hh"

typedef struct
{
    dcTexture id;
    char *filename;
} textureInfo;

using namespace std;

static list<textureInfo> textures;
static list<flFont *> fonts;
static list<float> floatConstants;
static list<int> integerConstants;
static list<char *> stringConstants;

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
    char *fullpath = getFullPath(filename);

    if (!fullpath)
    {
        error_msg("Unable to locate texture file at " << filename);
        return -1;
    }

    for (list<textureInfo>::iterator item = textures.begin(); item != textures.end(); item++)
    {
        if (!strcmp(item->filename, fullpath))
        {
            free(fullpath);
            return item->id;
        }
    }

    textureInfo newtexture;
    newtexture.id = imgload(fullpath);;
    newtexture.filename = fullpath;
    textures.push_back(newtexture);
    return newtexture.id;
}

flFont *dcLoadFont(const char *filename, const char *face, unsigned int basesize)
{
    char *fullpath = getFullPath(filename);

    if (!fullpath)
    {
        error_msg("Unable to locate font file at " << filename);
        return 0x0;
    }

    for (list<flFont *>::iterator item = fonts.begin(); item != fonts.end(); item++)
    {
        if (!strcmp((*item)->getFileName(), fullpath) && ((*item)->getBaseSize() == basesize))
        {
            if (!face && !(*item)->getFaceName())
            {
                free(fullpath);
                return *item;
            }
            else if (face && (*item)->getFaceName())
            {
                if (!strcmp((*item)->getFaceName(), face))
                {
                    free(fullpath);
                    return *item;
                }
            }
        }
    }

    flFont *id = new flFont(fullpath, face, basesize);
    if (!(id->isValid())) error_msg("Could not load font " << filename);
    fonts.push_back(id);
    return id;
}

float *dcLoadConstant(float fval)
{
    list<float>::iterator fc;
    for (fc = floatConstants.begin(); fc != floatConstants.end(); fc++)
    {
        if (*fc == fval) return &(*fc);
    }
    floatConstants.push_back(fval);
    return &(floatConstants.back());
}

int *dcLoadConstant(int ival)
{
    list<int>::iterator ic;
    for (ic = integerConstants.begin(); ic != integerConstants.end(); ic++)
    {
        if (*ic == ival) return &(*ic);
    }
    integerConstants.push_back(ival);
    return &(integerConstants.back());
}

char *dcLoadConstant(const char *sval)
{
    list<char *>::iterator sc;
    for (sc = stringConstants.begin(); sc != stringConstants.end(); sc++)
    {
        if (!strcmp(*sc, sval)) return *sc;
    }
    stringConstants.push_back(strdup(sval));
    return stringConstants.back();
}
