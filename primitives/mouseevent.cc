#include "geometry.hh"
#include "mouseevent.hh"

// TODO: include file for this
extern void UpdateDisplay(void);

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
        this->PressList->updateData();
        UpdateDisplay(); // TODO: UpdateDisplay should only be called once AFTER all events are processed
        // TODO: add mousebounce stuff here
    }
    else this->selected = false; // is this really necessary?
}

void dcMouseEvent::handleMouseRelease(void)
{
    if (this->selected)
    {
        this->selected = false;
        this->ReleaseList->updateData();
        UpdateDisplay(); // TODO: UpdateDisplay should only be called once AFTER all events are processed
        // TODO: add mousebounce clear here
    }
}
