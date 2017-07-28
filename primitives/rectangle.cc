#include "opengl_draw.hh"
#include "string_utils.hh"
#include "loadUtils.hh"
#include "rectangle.hh"

dcRectangle::dcRectangle(dcParent *myparent) : dcGeometric(myparent), linewidth(1), fill(false), outline(false)
{
    FillColor.R = dcLoadConstant(0.5f);
    FillColor.G = dcLoadConstant(0.5f);
    FillColor.B = dcLoadConstant(0.5f);
    FillColor.A = dcLoadConstant(0.5f);
    LineColor.R = dcLoadConstant(1.0f);
    LineColor.G = dcLoadConstant(1.0f);
    LineColor.B = dcLoadConstant(1.0f);
    LineColor.A = dcLoadConstant(1.0f);
}

void dcRectangle::setFillColor(const char *cspec)
{
    if (cspec)
    {
        FillColor = StrToColor(cspec, 1, 1, 1, 1);
        fill = true;
    }
}

void dcRectangle::setLineColor(const char *cspec)
{
    if (cspec)
    {
        LineColor = StrToColor(cspec, 1, 1, 1, 1);
        outline = true;
    }
}

void dcRectangle::setLineWidth(const char *inval)
{
    if (inval)
    {
        linewidth = StrToFloat(inval, 1);
        outline = true;
    }
}

void dcRectangle::draw(void)
{
    computeGeometry();
    container_start(refx, refy, delx, dely, 1, 1, *rotate);
    if (fill) rectangle_fill(*(FillColor.R), *(FillColor.G), *(FillColor.B), *(FillColor.A), 0, 0, 0, height, width, height, width, 0);
    if (outline) rectangle_outline(linewidth, *(LineColor.R), *(LineColor.G), *(LineColor.B), *(LineColor.A), 0, 0, 0, height, width, height, width, 0);
    container_end();
}
