#ifndef _IMAGE_HH_
#define _IMAGE_HH_

#include "dc.hh"
#include "geometric.hh"
#include "parent.hh"

class dcImage : public dcGeometric
{
    public:
        dcImage(dcParent *);
        void setTexture(const char *);
        void draw(void);

    private:
        dcTexture textureID;
};

#endif
