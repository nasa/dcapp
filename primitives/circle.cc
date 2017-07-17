#include "geometry.hh"
#include "opengl_draw.hh"
#include "circle.hh"

dcCircle::dcCircle(float *inx, float *iny, float *incw, float *inch, unsigned inhal, unsigned inval, float *inrad, float inlwidth, bool infill, bool inoutline, Kolor *infillcol, Kolor *inlinecol, unsigned insegs)
{
    x = inx;
    y = iny;
    containerw = incw; // TODO: these should come from the parent
    containerh = inch; // TODO: these should come from the parent
    halign = inhal;
    valign = inval;
    radius = inrad;
    linewidth = inlwidth;
    fill = infill;
    outline = inoutline;
    FillColor.R = infillcol->R; // TODO: replace ALL color inits with overrided operator
    FillColor.G = infillcol->G;
    FillColor.B = infillcol->B;
    FillColor.A = infillcol->A;
    LineColor.R = inlinecol->R;
    LineColor.G = inlinecol->G;
    LineColor.B = inlinecol->B;
    LineColor.A = inlinecol->A;
    segments = insegs;
}

void dcCircle::draw(void)
{
float zeroval = 0; // TODO: yuck;
    Geometry geo = GetGeometry(x, y, &zeroval, &zeroval, *containerw, *containerh, halign, valign);
    if (fill) circle_fill(geo.refx, geo.refy, *radius, segments, *(FillColor.R), *(FillColor.G), *(FillColor.B), *(FillColor.A));
    if (outline) circle_outline(geo.refx, geo.refy, *radius, segments, *(LineColor.R), *(LineColor.G), *(LineColor.B), *(LineColor.A), linewidth);
}
