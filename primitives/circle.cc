#include "opengl_draw.hh"
#include "string_utils.hh"
#include "loadUtils.hh"
#include "varlist.hh"
#include "circle.hh"

dcCircle::dcCircle(dcParent *myparent) : dcGeometric(myparent), linewidth(1), fill(false), outline(false), segments(80)
{
    radius = dcLoadConstant(10.0f);
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
        linewidth = StrToFloat(inval, 1);
        outline = true;
    }
}

void dcCircle::setRadius(const char *inval)
{
    if (inval) radius = getFloatPointer(inval);
}

void dcCircle::setSegments(const char *inval)
{
    if (inval) segments = StrToInt(inval, 80);
}

void dcCircle::draw(void)
{
    computeGeometry();
	if (fill) circle_fill(refx, refy, *radius, segments, *(FillColor.R), *(FillColor.G), *(FillColor.B), *(FillColor.A));
    if (outline) circle_outline(refx, refy, *radius, segments, *(LineColor.R), *(LineColor.G), *(LineColor.B), *(LineColor.A), linewidth);
}
