#include "geometry.hh"
#include "opengl_draw.hh"
#include "image.hh"

dcImage::dcImage(float *inx, float *iny, float *inw, float *inh, float *incw, float *inch, unsigned inhal, unsigned inval, float *inrot, dcTexture intex)
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
    textureID = intex;
}

void dcImage::draw(void)
{
    Geometry geo = GetGeometry(x, y, w, h, *containerw, *containerh, halign, valign);
    container_start(geo.refx, geo.refy, geo.delx, geo.dely, 1, 1, *rotate);
    draw_image(textureID, geo.width, geo.height);
    container_end();
}
