#include <strings.h>
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
    rotate = dcLoadConstant(0.0f);
}

void dcGeometric::setPosition(const char *inx, const char *iny)
{
    if (inx) x = getDecimalPointer(inx);
    if (iny) y = getDecimalPointer(iny);
}

void dcGeometric::setSize(const char *inw, const char *inh)
{
    if (inw) w = getDecimalPointer(inw);
    if (inh) h = getDecimalPointer(inh);
}

void dcGeometric::setRotation(const char *inr)
{
    if (inr) rotate = getDecimalPointer(inr);
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
    double hwidth, hheight;

    left = GeomX(x, *w, *containerw, halign);
    bottom = GeomY(y, *h, *containerh, valign);
    if (w)
    {
        width = *w;
        hwidth = (0.5 * width);
    }
    else
    {
        width = 0;
        hwidth = 0;
    }
    if (h)
    {
        height = *h;
        hheight = (0.5 * height);
    }
    else
    {
        height = 0;
        hheight = 0;
    }
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

double dcGeometric::GeomX(double *x, double w, double containerW, int halign)
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
    else if (*x < 0) val = *x + containerW;
    else val = *x;

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

double dcGeometric::GeomY(double *y, double h, double containerH, int valign)
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
    else if (*y < 0) val = *y + containerH;
    else val = *y;

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
