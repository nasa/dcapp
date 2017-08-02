#include "nodes.hh"
#include "opengl_draw.hh"
#include "string_utils.hh"
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
        linewidth = StrToFloat(inval, 1);
        outline = true;
    }
}

void dcPolygon::draw(void)
{
    for (const auto &myobj : children) myobj->draw();

    if (fill)
    {
        polygon_fill_start(*(FillColor.R), *(FillColor.G), *(FillColor.B), *(FillColor.A));
        for (const auto &mypos : AppData.vertices) gfx_vertex(mypos.x, mypos.y);
        polygon_fill_end();
    }
    if (outline)
    {
        polygon_outline_start(linewidth, *(LineColor.R), *(LineColor.G), *(LineColor.B), *(LineColor.A));
        for (const auto &mypos : AppData.vertices) gfx_vertex(mypos.x, mypos.y);
        polygon_outline_end();
    }

    AppData.vertices.clear();
}
