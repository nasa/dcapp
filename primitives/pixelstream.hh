#ifndef _PIXELSTREAM_HH_
#define _PIXELSTREAM_HH_

#include "PixelStream/PixelStreamData.hh"
#include "object.hh"

// TODO: this structure is duplicated in nodes.hh
class PixelStreamItem
{
    public:
        PixelStreamItem() : psd(0x0), frame_count(0) {};
        ~PixelStreamItem() { if (psd) delete psd; };
        PixelStreamData *psd;
        unsigned frame_count;
};

class dcPixelStream : public dcObject
{
    public:
        dcPixelStream();
        ~dcPixelStream();
        void updateStreams(unsigned);

    private:
        PixelStreamItem *psi;
};

#endif
