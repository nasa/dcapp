#include <list>
#include "mouseevent.hh"

extern std::list<dcObject *> mouseheld;
extern int mousebouncemode;

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

void dcMouseEvent::handleMousePress(float inx, float iny)
{
    computeGeometry();
    if ((left < inx) && (inx < right) && (bottom < iny) && (iny < top))
    {
        this->selected = true;
        this->PressList->handleEvent();
        mouseheld.push_back(this->PressList);
        mousebouncemode = 1; // TODO: these two lines (at least) and declarations above should be in a method
    }
    else this->selected = false; // is this really necessary?
}

void dcMouseEvent::handleMouseRelease(void)
{
    if (this->selected)
    {
        this->selected = false;
        this->ReleaseList->handleEvent();
        mouseheld.clear();
    }
}
