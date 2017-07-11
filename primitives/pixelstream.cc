#include "pixelstream.hh"

extern void SetNeedsRedraw(void); // TODO: put in header file

dcPixelStream::dcPixelStream()
:
psi(0x0)
{
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