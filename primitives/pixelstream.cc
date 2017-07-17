#include "geometry.hh"
#include "opengl_draw.hh"
#include "pixelstream.hh"

extern void SetNeedsRedraw(void); // TODO: put in header file

dcPixelStream::dcPixelStream(float *inx, float *iny, float *inw, float *inh, float *incw, float *inch, unsigned inhal, unsigned inval, float *inrot, dcTexture intex)
:
psi(0x0)
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

dcPixelStream::~dcPixelStream()
{
    if (psi) delete psi;
}

void dcPixelStream::updateStreams(unsigned passcount)
{
if (!this->psi) return; // TODO: this can go away if psi is created in constructor
    if (this->psi->frame_count != passcount)
    {
        if (this->psi->psd->reader()) SetNeedsRedraw();
        this->psi->frame_count = passcount;
    }
}

void dcPixelStream::draw(void)
{
if (!this->psi) return; // TODO: this can go away if psi is created in constructor
    size_t nbytes, origbytes, newh, pad, padbytes, offset, offsetbytes;

    Geometry geo = GetGeometry(x, y, w, h, *containerw, *containerh, halign, valign);
    container_start(geo.refx, geo.refy, geo.delx, geo.dely, 1, 1, *rotate);
    newh = (size_t)((float)(psi->psd->width) * (*h) / (*w));
    if (newh > psi->psd->height)
    {
        origbytes = psi->psd->width * psi->psd->height * 3;
        // overpad the pad by 1 so that no unfilled rows show up
        pad = ((newh - psi->psd->height) / 2) + 1;
        padbytes = psi->psd->width * pad * 3;
        nbytes = (size_t)(psi->psd->width * newh * 3);
    }
    draw_image(textureID, geo.width, geo.height);
    container_end();
}
