#include <cstdlib>
#include <cstring>
#include <climits>
#include <string>
#include <list>
#include <strings.h>
#include "basicutils/msg.hh"
#include "imgload/imgload.hh"
#include "fontlib/fontlib.hh"
#include "dc.hh"
#include "TaraDraw/TaraDraw.hh"

#ifdef IOS_BUILD
#include <CoreFoundation/CoreFoundation.h>
#endif

typedef struct
{
    dcTexture textureID;    // changed for easier searching
    char *filename;
} textureInfo;

using namespace std;

static list<textureInfo> textures;
static list<dcFont> fonts;
static list<float> floatConstants;
static list<int> integerConstants;
static list<char *> stringConstants;

char *getFullPath(const char *fname)
{
#ifndef IOS_BUILD
    if (!fname) return 0x0;
	char *longname = (char *) calloc(PATH_MAX, sizeof(char));
	if (!realpath(fname, longname))
	{
	    free(longname);
	    return 0x0;
	}
	char *shortname = strdup(longname);
	free(longname);
	return shortname;
#else
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    
    // Look for a resource in the main bundle by name and type.
    CFURLRef urlL = CFBundleCopyResourceURL( mainBundle,
                                         CFStringCreateWithCString(NULL, fname, kCFStringEncodingUTF8),
                                         nullptr,
                                         nullptr );
    
    if( urlL )
        return strdup( CFStringGetCStringPtr( CFURLCopyPath( urlL ), kCFStringEncodingUTF8 ) );
    
    return nullptr;
#endif
}

dcTexture dcLoadTexture(const char *filename)
{
    char *fullpath = getFullPath(filename);
    
    if (!fullpath)
    {
        error_msg("Unable to locate texture file at " << filename);
        return -1;
    }
    
#if 0
    for( const auto &pTexL : textures )
    {
        if( !strcmp( pTexL.filename, fullpath ) )
        {
            free( fullpath );
            return pTexL.id;
        }
    }
#else
    for (list<textureInfo>::iterator item = textures.begin(); item != textures.end(); item++)
    {
        if (!strcmp(item->filename, fullpath))
        {
            free(fullpath);
            return item->textureID;
        }
    }
#endif
    textureInfo newtexture;
    newtexture.textureID = imgload(fullpath);
//    newtexture.textureID = tdLoadImageAsTexture( fullpath );
    newtexture.filename = fullpath;
    textures.push_back(newtexture);
    return newtexture.textureID;
}

dcFont dcLoadFont(const char *filename, const char *face, unsigned int basesize)
{
    char *fullpath = getFullPath(filename);

    if (!fullpath)
    {
        error_msg("Unable to locate font file at " << filename);
        return 0x0;
    }

    for (list<dcFont>::iterator item = fonts.begin(); item != fonts.end(); item++)
    {
        if (((*item)->getFileName() == fullpath) && ((*item)->getBaseSize() == basesize))
        {
            if (!face && (*item)->getFaceName().empty())
            {
                free(fullpath);
                return *item;
            }
            else if (face && !((*item)->getFaceName().empty()))
            {
                if ((*item)->getFaceName() == face)
                {
                    free(fullpath);
                    return *item;
                }
            }
        }
    }

    dcFont id = new flFont(fullpath, face, basesize);
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
