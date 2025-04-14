#ifndef _PIXELSTREAMDATA_HH_
#define _PIXELSTREAMDATA_HH_

#include <stdint.h>

enum { PixelStreamUnknownProtocol, PixelStreamFileProtocol, PixelStreamMjpegProtocol, PixelStreamTcpProtocol, PixelStreamVsmProtocol, PixelStreamDynamicFileProtocol };

class PixelStreamData
{
    public:
        PixelStreamData();
        virtual ~PixelStreamData();

        bool operator == (const PixelStreamData &);
        bool operator != (const PixelStreamData &);
        virtual int reader(void);
        virtual int writer(void);
        virtual bool writeRequested(void);
        virtual void updateStatus(void);

        unsigned protocol;
        bool connected;
        void *pixels;
        uint32_t width;
        uint32_t height;
};

#endif
