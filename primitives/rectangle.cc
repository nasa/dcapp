#include "geometry.hh"
#include "opengl_draw.hh"
#include "rectangle.hh"

dcRectangle::dcRectangle(float *inx, float *iny, float *inw, float *inh, float *incw, float *inch, unsigned inhal, unsigned inval, float *inrot, float inlw, bool infill, bool inoutline, Kolor *fcolor, Kolor *lcolor)
{
    x = inx;
    y = iny;
    w = inw;
    h = inh;
    containerw = incw; // TODO: these should come from the parent
    containerh = inch; // TODO: these should come from the parent
    halign = inhal;
    valign = inval;
    rotate = inrot;
    linewidth = inlw;
    fill = infill;
    outline = inoutline;
    FillColor.R = fcolor->R;
    FillColor.G = fcolor->G;
    FillColor.B = fcolor->B;
    FillColor.A = fcolor->A;
    LineColor.R = lcolor->R;
    LineColor.G = lcolor->G;
    LineColor.B = lcolor->B;
    LineColor.A = lcolor->A;
}

void dcRectangle::draw(void)
{
    Geometry geo = GetGeometry(x, y, w, h, *containerw, *containerh, halign, valign);
    container_start(geo.refx, geo.refy, geo.delx, geo.dely, 1, 1, *rotate);
    if (fill) rectangle_fill(*(FillColor.R), *(FillColor.G), *(FillColor.B), *(FillColor.A), 0, 0, 0, geo.height, geo.width, geo.height, geo.width, 0);
    if (outline) rectangle_outline(linewidth, *(LineColor.R), *(LineColor.G), *(LineColor.B), *(LineColor.A), 0, 0, 0, geo.height, geo.width, geo.height, geo.width, 0);
    container_end();
}
