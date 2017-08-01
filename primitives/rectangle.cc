#include "opengl_draw.hh"
#include "string_utils.hh"
#include "rectangle.hh"

dcRectangle::dcRectangle(dcParent *myparent) : dcGeometric(myparent), linewidth(1), fill(false), outline(false)
{
    FillColor.set(0.5, 0.5, 0.5);
    LineColor.set(1, 1, 1);
}

void dcRectangle::setFillColor(const char *cspec)
{
    if (cspec)
    {
        FillColor.set(cspec);
        fill = true;
    }
}

void dcRectangle::setLineColor(const char *cspec)
{
    if (cspec)
    {
        LineColor.set(cspec);
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
