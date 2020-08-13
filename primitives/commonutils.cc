#include <list>
#include <string>
#include <vector>
#include "basicutils/msg.hh"
#include "basicutils/pathinfo.hh"
#include "RenderLib/fontlib.hh"
#include "RenderLib/texturelib.hh"

static std::list<tdTexture *> textures;
static std::list<tdFont *> fonts;

tdTexture *tdLoadTexture(std::string filename)
{
    PathInfo mypath(filename);

    if (!(mypath.isValid()))
    {
        warning_msg("Unable to locate texture file at " << filename);
        return 0x0;
    }

    for (std::list<tdTexture *>::iterator item = textures.begin(); item != textures.end(); item++)
    {
        if (((*item)->getFileName() == mypath.getFullPath())) return *item;
    }

    tdTexture *id = new tdTexture(mypath.getFullPath());

    if (!(id->isValid())) warning_msg("Could not load texture " << filename);

    textures.push_back(id);

    return id;
}

tdFont *tdLoadFont(std::string &filename, std::string &face, unsigned int basesize)
{
    PathInfo mypath(filename);

    if (!(mypath.isValid()))
    {
        warning_msg("Unable to locate font file at " << filename);
        return 0x0;
    }

    for (std::list<tdFont *>::iterator item = fonts.begin(); item != fonts.end(); item++)
    {
        if (((*item)->getFileName() == mypath.getFullPath()) &&
            ((*item)->getBaseSize() == basesize) &&
            ((*item)->getFaceName() == face)) return *item;
    }

    tdFont *id = new tdFont(mypath.getFullPath(), face, basesize);

    if (!(id->isValid())) warning_msg("Could not load font " << filename);

    fonts.push_back(id);

    return id;
}

void addPoint(std::vector<float> &listA, float xA, float yA)
{
    listA.push_back(xA);
    listA.push_back(yA);
}

void addPoint(std::vector<float> &listA, float xA, float yA, float zA)
{
    listA.push_back(xA);
    listA.push_back(yA);
    listA.push_back(zA);
}

void addPoint(std::vector<float> &listA, float xA, float yA, float zA, float uA, float vA)
{
    listA.push_back(xA);
    listA.push_back(yA);
    listA.push_back(zA);
    listA.push_back(uA);
    listA.push_back(vA);
}
