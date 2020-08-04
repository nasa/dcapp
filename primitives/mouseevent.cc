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
    computeGeometry();
    if ((left < inx) && (inx < right) && (bottom < iny) && (iny < top))
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
