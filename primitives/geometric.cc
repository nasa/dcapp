#include <strings.h>
#include "valuedata.hh"
#include "constants.hh"
#include "alignment.hh"
#include "varlist.hh"
#include "geometric.hh"

dcGeometric::dcGeometric(dcParent *myparent) : x(0x0), y(0x0), halign(AlignLeft), valign(AlignBottom)
{
    myparent->addChild(this);
    containerw = getContainerWidth();
    containerh = getContainerHeight();
    w = containerw;
    h = containerh;
    rotate = getConstantFromDecimal(0);
}

void dcGeometric::setPosition(const char *inx, const char *iny)
{
    if (inx) x = getValue(inx);
    if (iny) y = getValue(iny);
}

void dcGeometric::setSize(const char *inw, const char *inh)
{
    if (inw) w = getValue(inw);
    if (inh) h = getValue(inh);
}

void dcGeometric::setRotation(const char *inr)
{
    if (inr) rotate = getValue(inr);
}

void dcGeometric::setAlignment(const char *inhal, const char *inval)
{
    if (inhal)
    {
        if (!strcasecmp(inhal, "Left")) halign = AlignLeft;
        else if (!strcasecmp(inhal, "Center")) halign = AlignCenter;
        else if (!strcasecmp(inhal, "Right")) halign = AlignRight;
    }
    if (inval)
    {
        if (!strcasecmp(inval, "Bottom")) valign = AlignBottom;
        else if (!strcasecmp(inval, "Middle")) valign = AlignMiddle;
        else if (!strcasecmp(inval, "Top")) valign = AlignTop;
    }
}

void dcGeometric::computeGeometry(void)
{
    if (w) width = w->getDecimal();
    else width = 0;

    if (h) height = h->getDecimal();
    else height = 0;

    double hwidth = (0.5 * width);
    double hheight = (0.5 * height);

    left = GeomX(x, width, containerw->getDecimal(), halign);
    bottom = GeomY(y, height, containerh->getDecimal(), valign);
    right = left + width;
    top = bottom + height;
    center = left + hwidth;
    middle = bottom + hheight;

    switch (halign)
    {
        case AlignLeft:
            refx = left;
            delx = 0;
            break;
        case AlignCenter:
            refx = center;
            delx = hwidth;
            break;
        case AlignRight:
            refx = right;
            delx = width;
            break;
        default:
            break;
    }
    switch (valign)
    {
        case AlignBottom:
            refy = bottom;
            dely = 0;
            break;
        case AlignMiddle:
            refy = middle;
            dely = hheight;
            break;
        case AlignTop:
            refy = top;
            dely = height;
            break;
        default:
            break;
    }
}

double dcGeometric::GeomX(Value *x, double w, double containerW, int halign)
{
    double val;

    if (!x)
    {
        switch (halign)
        {
            case AlignLeft:
                val = 0;
                break;
            case AlignCenter:
                val = (containerW/2);
                break;
            case AlignRight:
                val = containerW;
                break;
            default:
                break;
        }
    }
// TODO: Remove the next line, but first create new way to achieve this and update all known displays
    else if (x->getDecimal() < 0) val = x->getDecimal() + containerW;
    else val = x->getDecimal();

    switch (halign)
    {
        case AlignLeft:
            return val;
        case AlignCenter:
            return (val - (w/2));
        case AlignRight:
            return (val - w);
        default:
            return 0;
    }
}

double dcGeometric::GeomY(Value *y, double h, double containerH, int valign)
{
    double val;

    if (!y)
    {
        switch (valign)
        {
            case AlignBottom:
                val = 0;
                break;
            case AlignMiddle:
                val = (containerH/2);
                break;
            case AlignTop:
                val = containerH;
                break;
            default:
                break;
        }
    }
// TODO: Remove the next line, but first create new way to achieve this and update all known displays
    else if (y->getDecimal() < 0) val = y->getDecimal() + containerH;
    else val = y->getDecimal();

    switch (valign)
    {
        case AlignBottom:
            return val;
        case AlignMiddle:
            return (val - (h/2));
        case AlignTop:
            return (val - h);
        default:
            return 0;
    }
}
