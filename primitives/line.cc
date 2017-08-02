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

    line_start(linewidth, *(color.R), *(color.G), *(color.B), *(color.A));
    for (const auto &mypos : AppData.vertices) gfx_vertex(mypos.x, mypos.y);
    line_end();

    AppData.vertices.clear();
}
