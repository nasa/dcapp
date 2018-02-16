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
        linewidth = StringToDecimal(inval, 1);
        outline = true;
    }
}

void dcPolygon::draw(void)
{
    for (const auto &myobj : children) myobj->draw();

    if (fill)
    {
        draw_polygon(AppData.vertices, *(FillColor.R), *(FillColor.G), *(FillColor.B), *(FillColor.A));
	}
    if (outline)
    {
        draw_line(AppData.vertices, linewidth, *(LineColor.R), *(LineColor.G), *(LineColor.B), *(LineColor.A));
    }

    AppData.vertices.clear();
}
