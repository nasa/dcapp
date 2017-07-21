#include "polygon.hh"

dcPolygon::dcPolygon(float inlwidth, bool infill, bool inoutline, Kolor *infillcol, Kolor *inlinecol)
{
    linewidth = inlwidth;
    fill = infill;
    outline = inoutline;
    FillColor.R = infillcol->R;
    FillColor.G = infillcol->G;
    FillColor.B = infillcol->B;
    FillColor.A = infillcol->A;
    LineColor.R = inlinecol->R;
    LineColor.G = inlinecol->G;
    LineColor.B = inlinecol->B;
    LineColor.A = inlinecol->A;
}

void dcPolygon::completeInitialization(void)
{
    for (std::list<dcObject *>::iterator myobj = children.begin(); myobj != children.end(); myobj++)
    {
        (*myobj)->completeInitialization();
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
