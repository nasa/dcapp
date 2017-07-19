#include "geometry.hh"
#include "mouseevent.hh"

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
}

dcMouseEvent::~dcMouseEvent()
{
    delete PressList;
    delete ReleaseList;
}

void dcMouseEvent::handleMousePress(float inx, float iny)
{
    Geometry geo = GetGeometry(x, y, w, h, *containerw, *containerh, halign, valign);
    if ((geo.left < inx) && (inx < geo.right) && (geo.bottom < iny) && (iny < geo.top))
    {
        this->selected = true;
        this->PressList->handleEvent();
        // TODO: add mousebounce stuff here
    }
    else this->selected = false; // is this really necessary?
}

void dcMouseEvent::handleMouseRelease(void)
{
    if (this->selected)
    {
        this->selected = false;
        this->ReleaseList->handleEvent();
        // TODO: add mousebounce clear here
    }
}
