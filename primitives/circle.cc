#include "opengl_draw.hh"
#include "string_utils.hh"
#include "loadUtils.hh"
#include "circle.hh"

extern float *getFloatPointer(const char *); // TODO: put in header file

dcCircle::dcCircle(dcParent *myparent) : dcGeometric(myparent), linewidth(1), fill(false), outline(false), segments(80)
{
    radius = dcLoadConstant(10.0f);
    FillColor.R = dcLoadConstant(0.5f);
    FillColor.G = dcLoadConstant(0.5f);
    FillColor.B = dcLoadConstant(0.5f);
    FillColor.A = dcLoadConstant(0.5f);
    LineColor.R = dcLoadConstant(1.0f);
    LineColor.G = dcLoadConstant(1.0f);
    LineColor.B = dcLoadConstant(1.0f);
    LineColor.A = dcLoadConstant(1.0f);
}

void dcCircle::setFillColor(const char *cspec)
{
    if (cspec)
    {
        FillColor = StrToColor(cspec, 1, 1, 1, 1);
        fill = true;
    }
}

void dcCircle::setLineColor(const char *cspec)
{
    if (cspec)
    {
        LineColor = StrToColor(cspec, 1, 1, 1, 1);
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
