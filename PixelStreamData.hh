#ifndef _PIXELSTREAMDATA_HH_
#define _PIXELSTREAMDATA_HH_

#include <stdint.h>

class PixelStreamData
{
    public:
        PixelStreamData();
        virtual ~PixelStreamData();

        virtual int update(void);

        unsigned protocol;
        void *pixels;
        uint32_t width;
        uint32_t height;
};

#endif
