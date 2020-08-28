#include <string>
#include <strings.h>
#include "basicutils/stringutils.hh"
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

void dcGeometric::setPosition(const std::string &inx, const std::string &iny)
{
    if (!inx.empty()) x = getValueSSTR(inx);
    if (!iny.empty()) y = getValueSSTR(iny);
}

void dcGeometric::setSize(const std::string &inw, const std::string &inh)
{
    if (!inw.empty()) w = getValueSSTR(inw);
    if (!inh.empty()) h = getValueSSTR(inh);
}

void dcGeometric::setRotation(const std::string &inr)
{
    if (!inr.empty()) rotate = getValueSSTR(inr);
}

void dcGeometric::setAlignment(const std::string &inhal, const std::string &inval)
{
    if (!inhal.empty())
    {
        if (CaseInsensitiveCompare(inhal, "Left")) halign = dcLeft;
        else if (CaseInsensitiveCompare(inhal, "Center")) halign = dcCenter;
        else if (CaseInsensitiveCompare(inhal, "Right")) halign = dcRight;
    }
    if (!inval.empty())
    {
        if (CaseInsensitiveCompare(inval, "Bottom")) valign = dcBottom;
        else if (CaseInsensitiveCompare(inval, "Middle")) valign = dcMiddle;
        else if (CaseInsensitiveCompare(inval, "Top")) valign = dcTop;
    }
}

void dcGeometric::setOrigin(const std::string &inx, const std::string &iny)
{
    if (!inx.empty())
    {
        if (CaseInsensitiveCompare(inx, "Left")) originx = dcLeft;
        else if (CaseInsensitiveCompare(inx, "Center")) originx = dcCenter;
        else if (CaseInsensitiveCompare(inx, "Right")) originx = dcRight;
    }
    if (!iny.empty())
    {
        if (CaseInsensitiveCompare(iny, "Bottom")) originy = dcBottom;
        else if (CaseInsensitiveCompare(iny, "Middle")) originy = dcMiddle;
        else if (CaseInsensitiveCompare(iny, "Top")) originy = dcTop;
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
