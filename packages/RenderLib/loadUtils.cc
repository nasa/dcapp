#include <cstdlib>
#include <cstring>
#include <climits>
#include <list>
#include "basicutils/msg.hh"
#include "basicutils/pathinfo.hh"
#include "fontlib.hh"
#include "texturelib.hh"

static std::list<tdTexture *> textures;
static std::list<tdFont *> fonts;

tdTexture *tdLoadTexture(const char *filename)
{
    if (!filename)
    {
        warning_msg("Invalid filename specified for texture file");
        return 0x0;
    }

    PathInfo *mypath = new PathInfo(filename);

    if (!(mypath->getFullPath()))
    {
        warning_msg("Unable to locate texture file at " << filename);
        delete mypath;
        return 0x0;
    }

    for (std::list<tdTexture *>::iterator item = textures.begin(); item != textures.end(); item++)
    {
        if (((*item)->getFileName() == mypath->getFullPath()))
        {
            delete mypath;
            return *item;
        }
    }

    tdTexture *id = new tdTexture(mypath->getFullPath());
    if (!(id->isValid())) warning_msg("Could not load texture " << filename);
    textures.push_back(id);
    delete mypath;

    return id;
}

tdFont *tdLoadFont(const char *filename, const char *face, unsigned int basesize)
{
    if (!filename) return 0x0;

    PathInfo *mypath = new PathInfo(filename);

    if (!(mypath->getFullPath()))
    {
        warning_msg("Unable to locate font file at " << filename);
        delete mypath;
        return 0x0;
    }

    for (std::list<tdFont *>::iterator item = fonts.begin(); item != fonts.end(); item++)
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

    tdFont *id = new tdFont(mypath->getFullPath(), face, basesize);
    if (!(id->isValid())) warning_msg("Could not load font " << filename);
    fonts.push_back(id);
    delete mypath;

    return id;
}
