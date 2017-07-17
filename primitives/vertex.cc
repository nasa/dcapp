#include "geometry.hh"
#include "opengl_draw.hh"
#include "vertex.hh"

dcVertex::dcVertex(float *inx, float *iny, float *incw, float *inch)
{
    x = inx;
    y = iny;
    containerw = incw; // TODO: these should come from the parent
    containerh = inch; // TODO: these should come from the parent
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
