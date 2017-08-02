#include "nodes.hh"
#include "opengl_draw.hh"
#include "string_utils.hh"
#include "line.hh"

extern appdata AppData;

dcLine::dcLine(dcParent *myparent) : linewidth(1)
{
    myparent->addChild(this);
    color.set(1, 1, 1);
}

void dcLine::setColor(const char *cspec)
{
    color.set(cspec);
}

void dcLine::setLineWidth(const char *inval)
{
    if (inval) linewidth = StrToFloat(inval, 1);
}

void dcLine::draw(void)
{
    for (const auto &myobj : children) myobj->draw();

	draw_line( AppData.vertices, linewidth, *color.R, *color.G, *color.B, *color.A );

    AppData.vertices.clear();
}
