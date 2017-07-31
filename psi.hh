#ifndef _PSI_HH_
#define _PSI_HH_

#include "PixelStream/PixelStream.hh"

class PixelStreamItem
{
    public:
        PixelStreamItem() : psd(0x0), frame_count(0) {};
        ~PixelStreamItem() { if (psd) delete psd; };
        PixelStreamData *psd;
        unsigned frame_count;
};

#endif
