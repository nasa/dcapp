#include <cmath>
#include "mouseevent.hh"

extern void RegisterPressedPrimitive(dcParent *);

dcMouseEvent::dcMouseEvent(dcParent *myparent) : dcGeometric(myparent), selected(false)
{
    PressList = new dcParent;
    ReleaseList = new dcParent;
    PressList->setParent(this);
    ReleaseList->setParent(this);
}

dcMouseEvent::~dcMouseEvent()
{
    delete PressList;
    delete ReleaseList;
}

void dcMouseEvent::handleMousePress(double inx, double iny)
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
        finalx = (tmpx * cos(ang)) + (tmpy * sin(ang));
        finaly = (tmpy * cos(ang)) - (tmpx * sin(ang));
    }
    else
    {
        finalx = inx + delx - refx;
        finaly = iny + dely - refy;
    }

    if ((0 < finalx) && (finalx < width) && (0 < finaly) && (finaly < height))
    {
        this->selected = true;
        this->PressList->handleEvent();
        RegisterPressedPrimitive(this->PressList);
    }
}

void dcMouseEvent::handleMouseRelease(void)
{
    if (this->selected)
    {
        this->selected = false;
        this->ReleaseList->handleEvent();
    }
}
