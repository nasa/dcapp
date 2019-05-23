#ifndef _IMAGE_HH_
#define _IMAGE_HH_

#include "RenderLib/RenderLib.hh"
#include "geometric.hh"
#include "parent.hh"

class dcImage : public dcGeometric
{
    public:
        dcImage(dcParent *);
        void setTexture(const char *);
        void draw(void);

    private:
        tdTexture *textureID;
};

#endif
