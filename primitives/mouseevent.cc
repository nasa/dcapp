typedef struct
{
    float refx;
    float refy;
    float delx;
    float dely;
    float width;
    float height;
    float left;
    float right;
    float bottom;
    float top;
    float center;
    float middle;
} Geometry;
extern Geometry GetGeometry(float *, float *, float *, float *, float, float, int, int); // TODO: include file for this and above

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
printf("mousepress at %f,%f\n", inx, iny);

    Geometry geo = GetGeometry(x, y, w, h, *containerw, *containerh, halign, valign);
    if ((geo.left < inx) && (inx < geo.right) && (geo.bottom < iny) && (iny < geo.top))
    {
printf("ITS A MATCH\n");
#if 0
        this->selected = true;
        this->PressList->updateData();
        UpdateDisplay(); // TODO: UpdateDisplay should only be called once AFTER all events are processed
#endif
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
