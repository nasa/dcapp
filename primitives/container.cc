#include <cmath>
#include "RenderLib/RenderLib.hh"
#include "valuedata.hh"
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
        ValueData *tmpptr = getValueData(inw);
        if (vwidth == w) vwidth = tmpptr;
        w = tmpptr;
    }
    if (inh)
    {
        ValueData *tmpptr = getValueData(inh);
        if (vheight == h) vheight = tmpptr;
        h = tmpptr;
    }
}

void dcContainer::setVirtualSize(const char *inw, const char *inh)
{
    if (inw) vwidth = getValueData(inw);
    if (inh) vheight = getValueData(inh);
}

void dcContainer::draw(void)
{
    computeGeometry();
    container_start(refx, refy, delx, dely, (w->getDecimal())/(vwidth->getDecimal()), (h->getDecimal())/(vheight->getDecimal()), rotate->getDecimal());
    for (const auto &myobj : children) myobj->draw();
    container_end();
}

void dcContainer::handleMousePress(double inx, double iny)
{
    double ang, originx, originy, tmpx, tmpy, finalx, finaly;

    computeGeometry();
    ang = (rotate->getDecimal()) * 0.01745329252;
    originx = refx - ((delx * cos(-ang)) + (dely * sin(-ang)));
    originy = refy - ((dely * cos(-ang)) - (delx * sin(-ang)));
    tmpx = inx - originx;
    tmpy = iny - originy;
    finalx = ((tmpx * cos(ang)) + (tmpy * sin(ang))) * (vwidth->getDecimal()) / (w->getDecimal());
    finaly = ((tmpy * cos(ang)) - (tmpx * sin(ang))) * (vheight->getDecimal()) / (h->getDecimal());

    for (const auto &myobj : children) myobj->handleMousePress(finalx, finaly);
}

void dcContainer::handleMouseMotion(double inx, double iny)
{
    double ang, originx, originy, tmpx, tmpy, finalx, finaly;

    computeGeometry();
    ang = (rotate->getDecimal()) * 0.01745329252;
    originx = refx - ((delx * cos(-ang)) + (dely * sin(-ang)));
    originy = refy - ((dely * cos(-ang)) - (delx * sin(-ang)));
    tmpx = inx - originx;
    tmpy = iny - originy;
    finalx = ((tmpx * cos(ang)) + (tmpy * sin(ang))) * (vwidth->getDecimal()) / (w->getDecimal());
    finaly = ((tmpy * cos(ang)) - (tmpx * sin(ang))) * (vheight->getDecimal()) / (h->getDecimal());

    for (const auto &myobj : children) myobj->handleMouseMotion(finalx, finaly);
}

ValueData * dcContainer::getContainerWidth(void) { return vwidth; }
ValueData * dcContainer::getContainerHeight(void) { return vheight; }
