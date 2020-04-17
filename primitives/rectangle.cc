#include <vector>
#include "RenderLib/RenderLib.hh"
#include "string_utils.hh"
#include "commonutils.hh"
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
        linewidth = StringToDecimal(inval, 1);
        outline = true;
    }
}

void dcRectangle::draw(void)
{
    computeGeometry();
    container_start(refx, refy, delx, dely, 1, 1, rotate->getDecimal());
    std::vector<float> pointsL;
    pointsL.reserve(8);

    addPoint(pointsL, 0, 0);
    addPoint(pointsL, 0, height);
    addPoint(pointsL, width, height);
    addPoint(pointsL, width, 0);

    if (fill)
    {
        draw_quad(pointsL, (*(FillColor.R)), (*(FillColor.G)), (*(FillColor.B)), (*(FillColor.A)));
    }
    if (outline)
    {
        addPoint(pointsL, 0, 0);
        draw_line(pointsL, linewidth, (*LineColor.R), (*LineColor.G), (*LineColor.B), (*LineColor.A));
    }
    container_end();
}
