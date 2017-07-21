#include <cmath>
#include "geometry.hh"
#include "opengl_draw.hh"
#include "container.hh"

dcContainer::dcContainer(float *invwidth, float *invheight, float *inx, float *iny, float *inw, float *inh, float *incw, float *inch, unsigned inhal, unsigned inval, float *inrot)
{
    vwidth = invwidth;
    vheight = invheight;
    x = inx;
    y = iny;
    w = inw;
    h = inh;
    halign = inhal;
    valign = inval;
    rotate = inrot;
}

void dcContainer::completeInitialization(void)
{
    containerw = getContainerWidth();
    containerh = getContainerHeight();

    for (std::list<dcObject *>::iterator myobj = children.begin(); myobj != children.end(); myobj++)
    {
        (*myobj)->completeInitialization();
    }
}

void dcContainer::draw(void)
{
    Geometry geo = GetGeometry(x, y, w, h, *containerw, *containerh, halign, valign);
    container_start(geo.refx, geo.refy, geo.delx, geo.dely, (*w)/(*vwidth), (*h)/(*vheight), *rotate);
    for (std::list<dcObject *>::iterator myobj = children.begin(); myobj != children.end(); myobj++)
    {
        (*myobj)->draw();
    }
    container_end();
}

void dcContainer::handleMousePress(float inx, float iny)
{
    float ang, originx, originy, tmpx, tmpy, finalx, finaly;

    Geometry geo = GetGeometry(x, y, w, h, *containerw, *containerh, halign, valign);
    ang = (*rotate) * 0.01745329252;
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

float * dcContainer::getContainerWidth(void)
{
    return vwidth;
}

float * dcContainer::getContainerHeight(void)
{
    return vheight;
}
