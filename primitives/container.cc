#include <cmath>
#include "opengl_draw.hh"
#include "container.hh"

extern float *getFloatPointer(const char *); // TODO: put in header file

dcContainer::dcContainer(dcParent *myparent) : dcGeometric(myparent)
{
    vwidth = w;
    vheight = h;
}

void dcContainer::setSize(const char *inw, const char *inh)
{
    if (inw)
    {
        float *tmpptr = getFloatPointer(inw);
        if (vwidth == w) vwidth = tmpptr;
        w = tmpptr;
    }
    if (inh)
    {
        float *tmpptr = getFloatPointer(inh);
        if (vheight == h) vheight = tmpptr;
        h = tmpptr;
    }
}

void dcContainer::setVirtualSize(const char *inw, const char *inh)
{
    if (inw) vwidth = getFloatPointer(inw);
    if (inh) vheight = getFloatPointer(inh);
}

void dcContainer::draw(void)
{
    computeGeometry();
    container_start(refx, refy, delx, dely, (*w)/(*vwidth), (*h)/(*vheight), *rotate);
    for (std::list<dcObject *>::iterator myobj = children.begin(); myobj != children.end(); myobj++)
    {
        (*myobj)->draw();
    }
    container_end();
}

void dcContainer::handleMousePress(float inx, float iny)
{
    float ang, originx, originy, tmpx, tmpy, finalx, finaly;

    computeGeometry();
    ang = (*rotate) * 0.01745329252;
    originx = refx - ((delx * cosf(-ang)) + (dely * sinf(-ang)));
    originy = refy - ((dely * cosf(-ang)) - (delx * sinf(-ang)));
    tmpx = (inx - originx) * (*vwidth) / (*w);
    tmpy = (iny - originy) * (*vheight) / (*h);
    finalx = (tmpx * cosf(ang)) + (tmpy * sinf(ang));
    finaly = (tmpy * cosf(ang)) - (tmpx * sinf(ang));

    for (std::list<dcObject *>::iterator myobj = children.begin(); myobj != children.end(); myobj++)
    {
        (*myobj)->handleMousePress(finalx, finaly);
    }
}

float * dcContainer::getContainerWidth(void)
{
    return vwidth;
}

float * dcContainer::getContainerHeight(void)
{
    return vheight;
}
