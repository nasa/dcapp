#include <cstdlib>
#include <cstring>
#include <climits>
#include <list>
#include "basicutils/msg.hh"
#include "imgload/imgload.hh"
#include "fontlib/fontlib.hh"
#include "dc.hh"

typedef struct
{
    dcTexture id;
    char *filename;
} textureInfo;

static std::list<textureInfo> textures;
static std::list<dcFont> fonts;

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

    for (std::list<textureInfo>::iterator item = textures.begin(); item != textures.end(); item++)
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

dcFont dcLoadFont(const char *filename, const char *face, unsigned int basesize)
{
    char *fullpath = getFullPath(filename);

    if (!fullpath)
    {
        if (filename) error_msg("Unable to locate font file at " << filename);
        return 0x0;
    }

    for (std::list<dcFont>::iterator item = fonts.begin(); item != fonts.end(); item++)
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
