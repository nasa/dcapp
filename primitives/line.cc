#include "opengl_draw.hh"
#include "string_utils.hh"
#include "loadUtils.hh"
#include "line.hh"

dcLine::dcLine(dcParent *myparent) : linewidth(1)
{
    myparent->addChild(this);
    color.R = dcLoadConstant(1.0f);
    color.G = dcLoadConstant(1.0f);
    color.B = dcLoadConstant(1.0f);
    color.A = dcLoadConstant(1.0f);
}

void dcLine::setColor(const char *cspec)
{
    if (cspec) color = StrToColor(cspec, 1, 1, 1, 1);
}

void dcLine::setLineWidth(const char *inval)
{
    if (inval) linewidth = StrToFloat(inval, 1);
}

void dcLine::draw(void)
{
    line_start(linewidth, *(color.R), *(color.G), *(color.B), *(color.A));
    for (std::list<dcObject *>::iterator myobj = children.begin(); myobj != children.end(); myobj++)
    {
        (*myobj)->draw();
    }
    line_end();
}
