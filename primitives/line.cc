#include <string>
#include "app_data.hh"
#include "RenderLib/RenderLib.hh"
#include "basicutils/stringutils.hh"
#include "line.hh"

extern appdata AppData;

dcLine::dcLine(dcParent *myparent) : linewidth(1)
{
    myparent->addChild(this);
    color.set(1, 1, 1);
}

void dcLine::setColor(const std::string &cspec)
{
    if (!cspec.empty()) color.set(cspec);
}

void dcLine::setLineWidth(const std::string &inval)
{
    if (!inval.empty()) linewidth = StringToDecimal(inval, 1);
}

void dcLine::draw(void)
{
    for (const auto &myobj : children) myobj->draw();
    draw_line(AppData.vertices, linewidth, color.R->getDecimal(), color.G->getDecimal(), color.B->getDecimal(), color.A->getDecimal());
    AppData.vertices.clear();
}
