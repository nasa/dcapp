#include <cmath>
#include "opengl_draw.hh"
#include "varlist.hh"
#include "container.hh"

dcContainer::dcContainer(dcParent *myparent) : dcGeometric(myparent)
{
    vwidth = w;
    vheight = h;
}

void dcContainer::setSize(const char *inw, const char *inh)
{
    if (inw)
    {
        double *tmpptr = getDecimalPointer(inw);
        if (vwidth == w) vwidth = tmpptr;
        w = tmpptr;
    }
    if (inh)
    {
        double *tmpptr = getDecimalPointer(inh);
        if (vheight == h) vheight = tmpptr;
        h = tmpptr;
    }
}

void dcContainer::setVirtualSize(const char *inw, const char *inh)
{
    if (inw) vwidth = getDecimalPointer(inw);
    if (inh) vheight = getDecimalPointer(inh);
}

void dcContainer::draw(void)
{
    computeGeometry();
    container_start(refx, refy, delx, dely, (*w)/(*vwidth), (*h)/(*vheight), *rotate);
    for (const auto &myobj : children) myobj->draw();
    container_end();
}

void dcContainer::handleMousePress(double inx, double iny)
{
    double ang, originx, originy, tmpx, tmpy, finalx, finaly;

    computeGeometry();
    ang = (*rotate) * 0.01745329252;
    originx = refx - ((delx * cos(-ang)) + (dely * sin(-ang)));
    originy = refy - ((dely * cos(-ang)) - (delx * sin(-ang)));
    tmpx = inx - originx;
    tmpy = iny - originy;
    finalx = ((tmpx * cos(ang)) + (tmpy * sin(ang))) * (*vwidth) / (*w);
    finaly = ((tmpy * cos(ang)) - (tmpx * sin(ang))) * (*vheight) / (*h);

    for (const auto &myobj : children) myobj->handleMousePress(finalx, finaly);
}

void dcContainer::handleMouseMotion(double inx, double iny)
{
    double ang, originx, originy, tmpx, tmpy, finalx, finaly;

    computeGeometry();
    ang = (*rotate) * 0.01745329252;
    originx = refx - ((delx * cos(-ang)) + (dely * sin(-ang)));
    originy = refy - ((dely * cos(-ang)) - (delx * sin(-ang)));
    tmpx = inx - originx;
    tmpy = iny - originy;
    finalx = ((tmpx * cos(ang)) + (tmpy * sin(ang))) * (*vwidth) / (*w);
    finaly = ((tmpy * cos(ang)) - (tmpx * sin(ang))) * (*vheight) / (*h);

    for (const auto &myobj : children) myobj->handleMouseMotion(finalx, finaly);
}

double * dcContainer::getContainerWidth(void)
{
    return vwidth;
}

double * dcContainer::getContainerHeight(void)
{
    return vheight;
}
