typedef struct
{
    float refx;
    float refy;
    float delx;
    float dely;
    float width;
    float height;
    float left;
    float right;
    float bottom;
    float top;
    float center;
    float middle;
} Geometry;
extern Geometry GetGeometry(float *, float *, float *, float *, float, float, int, int); // TODO: include file for this and above

#include <cmath>
#include "container.hh"

dcContainer::dcContainer(float *invwidth, float *invheight, float *inx, float *iny, float *inw, float *inh, float *incw, float *inch, unsigned inhal, unsigned inval)
{
    vwidth = invwidth;
    vheight = invheight;
    x = inx;
    y = iny;
    w = inw;
    h = inh;
    containerw = incw; // TODO: these should come from the parent
    containerh = inch; // TODO: these should come from the parent
    halign = inhal;
    valign = inval;
}

void dcContainer::handleMousePress(float inx, float iny)
{
printf("CONTAINER mousepress at %f,%f\n", inx, iny);

    Geometry geo;
    float ang, originx, originy, tmpx, tmpy, finalx, finaly;

    geo = GetGeometry(x, y, w, h, *containerw, *containerh, halign, valign);
//    ang = (*rotate)) * 0.01745329252; TODO: add rotate
    originx = geo.refx - ((geo.delx * cosf(-ang)) + (geo.dely * sinf(-ang)));
    originy = geo.refy - ((geo.dely * cosf(-ang)) - (geo.delx * sinf(-ang)));
    tmpx = (inx - originx) * (*vwidth) / (*w);
    tmpy = (iny - originy) * (*vheight) / (*h);
    finalx = (tmpx * cosf(ang)) + (tmpy * sinf(ang));
    finaly = (tmpy * cosf(ang)) - (tmpx * sinf(ang));

    for (std::list<dcObject *>::iterator myobj = children.begin(); myobj != children.end(); myobj++)
    {
        (*myobj)->handleMousePress(finalx, finaly);
    }
}

