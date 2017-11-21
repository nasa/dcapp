#include "nodes.hh"
#include "opengl_draw.hh"
#include "varlist.hh"
#include "vertex.hh"

extern appdata AppData;

dcVertex::dcVertex(dcParent *myparent) : x(0x0), y(0x0)
{
    myparent->addChild(this);
    containerw = getContainerWidth();
    containerh = getContainerHeight();
}

void dcVertex::setPosition(const char *inx, const char *iny)
{
    if (inx) x = getDecimalPointer(inx);
    if (iny) y = getDecimalPointer(iny);
}

void dcVertex::draw(void)
{
    double myx, myy;

    if (x)
    {
        if (*x < 0) myx = *x + *containerw;
        else myx = *x;
    }
    else myx = 0;

    if (y)
    {
        if (*y < 0) myy = *y + *containerh;
        else myy = *y;
    }
    else myy = 0;

    AppData.vertices.push_back(dcPosition(myx, myy));
}
