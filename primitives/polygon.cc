#include "nodes.hh"
#include "RenderLib/RenderLib.hh"
#include "basicutils/stringutils.hh"
#include "polygon.hh"

extern appdata AppData;

dcPolygon::dcPolygon(dcParent *myparent) : linewidth(1), fill(false), outline(false)
{
    myparent->addChild(this);
    FillColor.set(0.5, 0.5, 0.5);
    LineColor.set(1, 1, 1);
}

void dcPolygon::setFillColor(const char *cspec)
{
    if (cspec)
    {
        FillColor.set(cspec);
        fill = true;
    }
}

void dcPolygon::setLineColor(const char *cspec)
{
    if (cspec)
    {
        LineColor.set(cspec);
        outline = true;
    }
}

void dcPolygon::setLineWidth(const char *inval)
{
    if (inval)
    {
        linewidth = StringToDecimal(inval, 1);
        outline = true;
    }
}

void dcPolygon::draw(void)
{
    for (const auto &myobj : children) myobj->draw();

    if (fill)
    {
        draw_polygon(AppData.vertices, FillColor.R->getDecimal(), FillColor.G->getDecimal(), FillColor.B->getDecimal(), FillColor.A->getDecimal());
    }
    if (outline)
    {
        draw_line(AppData.vertices, linewidth, LineColor.R->getDecimal(), LineColor.G->getDecimal(), LineColor.B->getDecimal(), LineColor.A->getDecimal());
    }

    AppData.vertices.clear();
}
