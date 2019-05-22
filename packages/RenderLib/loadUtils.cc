#include <cstdlib>
#include <cstring>
#include <climits>
#include <list>
#include "basicutils/msg.hh"
#include "basicutils/pathinfo.hh"
#include "imgload.hh"
#include "fontlib.hh"
#include "RenderLib_internal.hh"

typedef struct
{
    dcTexture id;
    char *filename;
} textureInfo;

static std::list<textureInfo> textures;
static std::list<dcFont> fonts;

dcTexture dcLoadTexture(const char *filename)
{
    if (!filename)
    {
        warning_msg("Invalid filename specified for texture file");
        return -1;
    }

    PathInfo *mypath = new PathInfo(filename);

    if (!(mypath->getFullPath()))
    {
        warning_msg("Unable to locate texture file at " << filename);
        delete mypath;
        return -1;
    }

    for (std::list<textureInfo>::iterator item = textures.begin(); item != textures.end(); item++)
    {
        if (!strcmp(item->filename, mypath->getFullPath()))
        {
            delete mypath;
            return item->id;
        }
    }

    textureInfo newtexture;
    newtexture.filename = strdup(mypath->getFullPath());
    newtexture.id = imgload(newtexture.filename);;
    textures.push_back(newtexture);
    delete mypath;

    return newtexture.id;
}

dcFont dcLoadFont(const char *filename, const char *face, unsigned int basesize)
{
    if (!filename) return 0x0;

    PathInfo *mypath = new PathInfo(filename);

    if (!(mypath->getFullPath()))
    {
        warning_msg("Unable to locate font file at " << filename);
        delete mypath;
        return 0x0;
    }

    for (std::list<dcFont>::iterator item = fonts.begin(); item != fonts.end(); item++)
    {
        if (((*item)->getFileName() == mypath->getFullPath()) && ((*item)->getBaseSize() == basesize))
        {
            if (!face && (*item)->getFaceName().empty())
            {
                delete mypath;
                return *item;
            }
            else if (face && !((*item)->getFaceName().empty()))
            {
                if ((*item)->getFaceName() == face)
                {
                    delete mypath;
                    return *item;
                }
            }
        }
    }

    dcFont id = new flFont(mypath->getFullPath(), face, basesize);
    if (!(id->isValid())) error_msg("Could not load font " << filename);
    fonts.push_back(id);
    delete mypath;

    return id;
}
