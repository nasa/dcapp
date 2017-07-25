#include "opengl_draw.hh"
#include "vertex.hh"

extern float *getFloatPointer(const char *); // TODO: put in header file

dcVertex::dcVertex(dcParent *myparent) : x(0x0), y(0x0)
{
    myparent->addChild(this);
    containerw = getContainerWidth();
    containerh = getContainerHeight();
}

void dcVertex::setPosition(const char *inx, const char *iny)
{
    if (inx) x = getFloatPointer(inx);
    if (iny) y = getFloatPointer(iny);
}

void dcVertex::draw(void)
{
    float myx, myy;

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

    gfx_vertex(myx, myy);
}
