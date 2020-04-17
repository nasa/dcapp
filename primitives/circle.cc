#include "RenderLib/RenderLib.hh"
#include "string_utils.hh"
#include "constants.hh"
#include "varlist.hh"
#include "circle.hh"

dcCircle::dcCircle(dcParent *myparent) : dcGeometric(myparent), linewidth(1), fill(false), outline(false), segments(80)
{
    radius = getConstantValue(10.0);
    FillColor.set(0.5, 0.5, 0.5);
    LineColor.set(1, 1, 1);
}

void dcCircle::setFillColor(const char *cspec)
{
    if (cspec)
    {
        FillColor.set(cspec);
        fill = true;
    }
}

void dcCircle::setLineColor(const char *cspec)
{
    if (cspec)
    {
        LineColor.set(cspec);
        outline = true;
    }
}

void dcCircle::setLineWidth(const char *inval)
{
    if (inval)
    {
        linewidth = StringToDecimal(inval, 1);
        outline = true;
    }
}

void dcCircle::setRadius(const char *inval)
{
    if (inval) radius = getValueData(inval);
}

void dcCircle::setSegments(const char *inval)
{
    if (inval) segments = StringToInteger(inval, 80);
}

void dcCircle::draw(void)
{
    computeGeometry();
    if (fill) circle_fill(refx, refy, radius->getDecimal(), segments, *(FillColor.R), *(FillColor.G), *(FillColor.B), *(FillColor.A));
    if (outline) circle_outline(refx, refy, radius->getDecimal(), segments, *(LineColor.R), *(LineColor.G), *(LineColor.B), *(LineColor.A), linewidth);
}
