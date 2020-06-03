#include <strings.h>
#include "constants.hh"
#include "values.hh"
#include "geometric.hh"

dcGeometric::dcGeometric(dcParent *myparent) : x(0x0), y(0x0), halign(dcLeft), valign(dcBottom), originx(dcLeft), originy(dcBottom)
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
        if (!strcasecmp(inhal, "Left")) halign = dcLeft;
        else if (!strcasecmp(inhal, "Center")) halign = dcCenter;
        else if (!strcasecmp(inhal, "Right")) halign = dcRight;
    }
    if (inval)
    {
        if (!strcasecmp(inval, "Bottom")) valign = dcBottom;
        else if (!strcasecmp(inval, "Middle")) valign = dcMiddle;
        else if (!strcasecmp(inval, "Top")) valign = dcTop;
    }
}

void dcGeometric::setOrigin(const char *inx, const char *iny)
{
    if (inx)
    {
        if (!strcasecmp(inx, "Left")) originx = dcLeft;
        else if (!strcasecmp(inx, "Center")) originx = dcCenter;
        else if (!strcasecmp(inx, "Right")) originx = dcRight;
    }
    if (iny)
    {
        if (!strcasecmp(iny, "Bottom")) originy = dcBottom;
        else if (!strcasecmp(iny, "Middle")) originy = dcMiddle;
        else if (!strcasecmp(iny, "Top")) originy = dcTop;
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
        case dcLeft:
            refx = left;
            delx = 0;
            break;
        case dcCenter:
            refx = center;
            delx = hwidth;
            break;
        case dcRight:
            refx = right;
            delx = width;
            break;
        default:
            break;
    }
    switch (valign)
    {
        case dcBottom:
            refy = bottom;
            dely = 0;
            break;
        case dcMiddle:
            refy = middle;
            dely = hheight;
            break;
        case dcTop:
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
            case dcLeft:
                val = 0;
                break;
            case dcCenter:
                val = (containerW/2);
                break;
            case dcRight:
                val = containerW;
                break;
            default:
                break;
        }
    }
    else
{
//if (x->getDecimal() < 0) printf("WARNING: X in Geometric (%g)\n", x->getDecimal());
if (originx == dcRight) val = containerw->getDecimal() - x->getDecimal();
else val = x->getDecimal();
}

    switch (halign)
    {
        case dcLeft:
            return val;
        case dcCenter:
            return (val - (w/2));
        case dcRight:
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
            case dcBottom:
                val = 0;
                break;
            case dcMiddle:
                val = (containerH/2);
                break;
            case dcTop:
                val = containerH;
                break;
            default:
                break;
        }
    }
    else
{
//if (y->getDecimal() < 0) printf("WARNING: Y in Geometric (%g)\n", y->getDecimal());
if (originy == dcTop) val = containerh->getDecimal() - y->getDecimal();
else val = y->getDecimal();
}

    switch (valign)
    {
        case dcBottom:
            return val;
        case dcMiddle:
            return (val - (h/2));
        case dcTop:
            return (val - h);
        default:
            return 0;
    }
}
