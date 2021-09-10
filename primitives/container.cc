#include <string>
#include <cmath>
#include "RenderLib/RenderLib.hh"
#include "values.hh"
#include "container.hh"

dcContainer::dcContainer(dcParent *myparent) : dcGeometric(myparent)
{
    vwidth = w;
    vheight = h;
}

void dcContainer::setSize(const std::string &inw, const std::string &inh)
{
    if (!inw.empty())
    {
        Value *tmpptr = getValue(inw);
        if (vwidth == w) vwidth = tmpptr;
        w = tmpptr;
    }
    if (!inh.empty())
    {
        Value *tmpptr = getValue(inh);
        if (vheight == h) vheight = tmpptr;
        h = tmpptr;
    }
}

void dcContainer::setVirtualSize(const std::string &inw, const std::string &inh)
{
    if (!inw.empty()) vwidth = getValue(inw);
    if (!inh.empty()) vheight = getValue(inh);
}

void dcContainer::draw(void)
{
    computeGeometry();
    container_start(refx, refy, delx, dely, (w->getDecimal())/(vwidth->getDecimal()), (h->getDecimal())/(vheight->getDecimal()), rotate->getDecimal());
    for (const auto &myobj : children) 
    {
        myobj->processPreCalculations();
        myobj->draw();
        myobj->processPostCalculations();
    }
    container_end();
}

void dcContainer::handleMousePress(double inx, double iny)
{
    double finalx, finaly;

    computeGeometry();
    if (rotate->getDecimal())
    {
        double ang = (rotate->getDecimal()) * 0.01745329252;
        double originx = refx - ((delx * cos(-ang)) + (dely * sin(-ang)));
        double originy = refy - ((dely * cos(-ang)) - (delx * sin(-ang)));
        double tmpx = inx - originx;
        double tmpy = iny - originy;
        finalx = ((tmpx * cos(ang)) + (tmpy * sin(ang))) * (vwidth->getDecimal()) / (w->getDecimal());
        finaly = ((tmpy * cos(ang)) - (tmpx * sin(ang))) * (vheight->getDecimal()) / (h->getDecimal());
    }
    else
    {
        finalx = (inx + delx - refx) * (vwidth->getDecimal()) / (w->getDecimal());
        finaly = (iny + dely - refy) * (vheight->getDecimal()) / (h->getDecimal());
    }

    for (const auto &myobj : children) myobj->handleMousePress(finalx, finaly);
}

void dcContainer::handleMouseMotion(double inx, double iny)
{
    double finalx, finaly;

    computeGeometry();
    if (rotate->getDecimal())
    {
        double ang = (rotate->getDecimal()) * 0.01745329252;
        double originx = refx - ((delx * cos(-ang)) + (dely * sin(-ang)));
        double originy = refy - ((dely * cos(-ang)) - (delx * sin(-ang)));
        double tmpx = inx - originx;
        double tmpy = iny - originy;
        finalx = ((tmpx * cos(ang)) + (tmpy * sin(ang))) * (vwidth->getDecimal()) / (w->getDecimal());
        finaly = ((tmpy * cos(ang)) - (tmpx * sin(ang))) * (vheight->getDecimal()) / (h->getDecimal());
    }
    else
    {
        finalx = (inx + delx - refx) * (vwidth->getDecimal()) / (w->getDecimal());
        finaly = (iny + dely - refy) * (vheight->getDecimal()) / (h->getDecimal());
    }

    for (const auto &myobj : children) myobj->handleMouseMotion(finalx, finaly);
}

Value * dcContainer::getContainerWidth(void) { return vwidth; }
Value * dcContainer::getContainerHeight(void) { return vheight; }
