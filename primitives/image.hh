#ifndef _IMAGE_HH_
#define _IMAGE_HH_

#include <string>
#include "RenderLib/RenderLib.hh"
#include "geometric.hh"
#include "parent.hh"

class dcImage : public dcGeometric
{
    public:
        dcImage(dcParent *);
        virtual ~dcImage();

        void setTexture(std::string);
        void handleMousePress(double, double);
        void handleMouseRelease(void);
        void draw(void);

        dcParent *PressList;
        dcParent *ReleaseList;

    private:
        tdTexture *textureID;
        bool selected;
};

#endif
