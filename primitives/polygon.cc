#include "opengl_draw.hh"
#include "string_utils.hh"
#include "loadUtils.hh"
#include "polygon.hh"

dcPolygon::dcPolygon(dcParent *myparent) : linewidth(1), fill(false), outline(false)
{
    myparent->addChild(this);
    FillColor.R = dcLoadConstant(0.5f);
    FillColor.G = dcLoadConstant(0.5f);
    FillColor.B = dcLoadConstant(0.5f);
    FillColor.A = dcLoadConstant(0.5f);
    LineColor.R = dcLoadConstant(1.0f);
    LineColor.G = dcLoadConstant(1.0f);
    LineColor.B = dcLoadConstant(1.0f);
    LineColor.A = dcLoadConstant(1.0f);
}

void dcPolygon::setFillColor(const char *cspec)
{
    if (cspec)
    {
        FillColor = StrToColor(cspec, 1, 1, 1, 1);
        fill = true;
    }
}

void dcPolygon::setLineColor(const char *cspec)
{
    if (cspec)
    {
        LineColor = StrToColor(cspec, 1, 1, 1, 1);
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
    if (fill)
    {
        polygon_fill_start(*(FillColor.R), *(FillColor.G), *(FillColor.B), *(FillColor.A));
        for (std::list<dcObject *>::iterator myobj = children.begin(); myobj != children.end(); myobj++)
        {
            (*myobj)->draw();
        }
        polygon_fill_end();
    }
    if (outline)
    {
        polygon_outline_start(linewidth, *(LineColor.R), *(LineColor.G), *(LineColor.B), *(LineColor.A));
        for (std::list<dcObject *>::iterator myobj = children.begin(); myobj != children.end(); myobj++)
        {
            (*myobj)->draw();
        }
        polygon_outline_end();
    }
}
