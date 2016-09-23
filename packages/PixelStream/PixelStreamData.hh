#ifndef _PIXELSTREAMDATA_HH_
#define _PIXELSTREAMDATA_HH_

#include <stdint.h>

enum { PixelStreamUnknownProtocol, PixelStreamFileProtocol, PixelStreamTcpProtocol };

class PixelStreamData
{
    public:
        PixelStreamData();
        virtual ~PixelStreamData();

        virtual int reader(void);
        virtual int writer(void);

        unsigned protocol;
        void *pixels;
        uint32_t width;
        uint32_t height;
};

#endif