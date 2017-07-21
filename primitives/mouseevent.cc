#include <list>
#include "geometry.hh"
#include "mouseevent.hh"

extern std::list<dcObject *> mouseheld;
extern int mousebouncemode;

dcMouseEvent::dcMouseEvent(float *inx, float *iny, float *inw, float *inh, float *incw, float *inch, unsigned inhal, unsigned inval)
:
selected(false)
{
    x = inx;
    y = iny;
    w = inw;
    h = inh;
    containerw = incw; // TODO: these should come from the parent
    containerh = inch; // TODO: these should come from the parent
    halign = inhal;
    valign = inval;

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

void dcMouseEvent::completeInitialization(void)
{
    PressList->completeInitialization();
    ReleaseList->completeInitialization();
}

void dcMouseEvent::handleMousePress(float inx, float iny)
{
    Geometry geo = GetGeometry(x, y, w, h, *containerw, *containerh, halign, valign);
    if ((geo.left < inx) && (inx < geo.right) && (geo.bottom < iny) && (iny < geo.top))
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
