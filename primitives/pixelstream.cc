#include "geometry.hh"
#include "opengl_draw.hh"
#include "pixelstream.hh"

extern void SetNeedsRedraw(void); // TODO: put in header file

dcPixelStream::dcPixelStream(float *inx, float *iny, float *inw, float *inh, float *incw, float *inch, unsigned inhal, unsigned inval, float *inrot, dcTexture intex, PixelStreamItem *inpsi)
:
pixels(0x0), memallocation(0)
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
    psi = inpsi;
}

dcPixelStream::~dcPixelStream()
{
    if (psi) delete psi;
}

void dcPixelStream::updateStreams(unsigned passcount)
{
if (!psi) return; // TODO: this can go away if psi is created in constructor
    if (psi->frame_count != passcount)
    {
        if (psi->psd->reader()) SetNeedsRedraw();
        psi->frame_count = passcount;
    }
}

void dcPixelStream::draw(void)
{
if (!psi) return; // TODO: this can go away if psi is created in constructor
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
        if (nbytes > memallocation)
        {
            pixels = realloc(pixels, nbytes);
            memallocation = nbytes;
        }
        bzero(pixels, padbytes);
        bzero((void *)((size_t)pixels + nbytes - padbytes), padbytes);
        bcopy(psi->psd->pixels, (void *)((size_t)pixels + padbytes), origbytes);
    }
    else
    {
        nbytes = (size_t)(psi->psd->width * psi->psd->height * 3);
        if (nbytes > memallocation)
        {
            pixels = realloc(pixels, nbytes);
            memallocation = nbytes;
        }
        bcopy(psi->psd->pixels, pixels, nbytes);
    }

    if (newh < psi->psd->height)
    {
        offset = (psi->psd->height - newh) / 2;
        offsetbytes = psi->psd->width * offset * 3;
        set_texture(textureID, psi->psd->width, newh, (void *)((size_t)pixels + offsetbytes));
    }
    else
        set_texture(textureID, psi->psd->width, newh, pixels);

    draw_image(textureID, geo.width, geo.height);
    container_end();
}
